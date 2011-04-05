// GPL License - see http://opensource.org/licenses/gpl-license.php
// Copyright 2006 *nixCoders team - don't forget to credit us

#include <dis-asm.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

typedef u_int32_t ptr_size_t;	// for x86
//typedef u_int64_t ptr_size_t;	// for ia64 need test

#define ASM_JUMP 0xe9
#define ASM_CALL 0xe8
#define ASM_NOP 0x90
#define ASM_MOV_VALUE_TO_EBX 0xbb
// mov    (%esp),%ebx;	ret
#define ASM_GET_PC_THUNK "\x8b\x1c\x24\xc3" 
#define ASM_JUMP_SIZE 5
#define ASM_CALL_SIZE 5

// Helper macro
#define GET_PAGE(addr) ((void *)(((ptr_size_t)addr) & ~((ptr_size_t)(getpagesize() - 1))))
#define unprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC))
#define reprotect(addr) (mprotect(GET_PAGE(addr), getpagesize(), PROT_READ | PROT_EXEC))

// Disassembler callbacks
int my_buffer_read_memory(bfd_vma from, bfd_byte *to, unsigned int length, struct disassemble_info *info) {
	memcpy((void *)to, (void *)(ptr_size_t)from, length);
	return 0;
}

void my_perror_memory(int status, bfd_vma memaddr, struct disassemble_info *info) {
	info->fprintf_func(info->stream, "ghf: Unknown error %d\n", status);
	exit(1);
}

// For disable disassembler output
fprintf_ftype my_fprintf(FILE *stream, const char *format, ...) {
	return 0;
}

// Return the length of the instruction
int disassemble_x86(void *addr) {
	// Initialize info for disassembler
	disassemble_info info;
	init_disassemble_info(&info, stdout, (fprintf_ftype)my_fprintf);
	info.mach = bfd_mach_i386_i386;
	info.read_memory_func = my_buffer_read_memory;
	info.memory_error_func = my_perror_memory;

	// Disassemble instruction
	return print_insn_i386((bfd_vma)(ptr_size_t)addr, &info);
}

void *detourFunction(void *orig, void *det) {
	// Sanity check
	if (!orig || !det) {
		printf("detourFunction: error: bad args, orig: %p tramp: %p\n", orig, det);
		raise(SIGSEGV);
	}
	// Get instructions length
	int iLen = 0;
	while (iLen < ASM_JUMP_SIZE)
		iLen += disassemble_x86(orig + iLen);

	// Backup instructions before it's override by the jump
	void *tramp = malloc(iLen + ASM_JUMP_SIZE);
	// Sanity check
	if (!tramp) {
		perror("detourFunction: error: malloc() tramp failed\n");
		raise(SIGSEGV);
	}
	int iCount = 0;
	while (iCount < iLen) {
		int len = disassemble_x86(orig + iCount);

		// Sanity check
		if (len < 1) {
			printf("error: can't disassemble at %p\n", orig);
			raise(SIGKILL);
		}

		// Search for a call in the backuped instructions and correct his value (because call are relative)
		if ((len == ASM_CALL_SIZE) && (*(unsigned char *)(orig + iCount) == ASM_CALL)) {
			// Check if the call is "__i686.get_pc_thunk.bx", if yes replace it by mov $offset, %ebx
			if (!memcmp((void *)((orig + iCount + 5) + *(uint *)(orig + iCount + 1)), ASM_GET_PC_THUNK, sizeof(ASM_GET_PC_THUNK) - 1)) {
				*(unsigned char *)(tramp + iCount) = ASM_MOV_VALUE_TO_EBX;
				*(void **)(tramp + iCount + 1) = (void *)(orig + iCount + 5);
			} else {
				*(unsigned char *)(tramp + iCount) = ASM_CALL;
				*(void **)(tramp + iCount + 1) = *(void **)(orig + iCount + 1) - ((tramp + iCount + 1) - (orig + iCount + 1));
			}
		// If not a call, simply copy the instruction
		} else {
			memcpy(tramp + iCount, orig + iCount, len);
		}
		iCount += len;
	}
	
	// Write a jump to the original function after the backuped intructions
	*(unsigned char *)(tramp + iLen) = ASM_JUMP;
	*(void **)(tramp + iLen + 1) = (void *)((((uint)orig) + iLen) - (uint)(tramp + iLen + ASM_JUMP_SIZE));

	// Write a jump to the detour at the original function
	unprotect(orig);
	*(unsigned char *)orig = ASM_JUMP;
	*(void **)((uint)orig + 1) = (void *)(((uint)det) - (((uint)orig) + ASM_JUMP_SIZE));
	reprotect(orig);

	return tramp;
}

void undetourFunction(void *orig, void *tramp) {
	int iLen = 0;
	while (iLen < ASM_JUMP_SIZE)
		iLen += disassemble_x86(tramp + iLen);

	unprotect(orig);
	memcpy(orig, tramp, iLen);
	reprotect(orig);

	free(tramp);
	tramp = NULL;
}


int nopInstruction(void *addr) {
	unsigned int size = disassemble_x86(addr);

	unprotect(addr);
	int count = 0;
	for (; count < size; count++)
		*(unsigned char *)(addr + count) = 0x90;
	reprotect(addr);

	return size;
}
