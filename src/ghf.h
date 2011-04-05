// GPL License - see http://opensource.org/licenses/gpl-license.php
// Copyright 2006 *nixCoders team - don't forget to credits us

#ifndef GHF_H_
#define GHF_H_

#include <gelf.h>

#ifdef __cplusplus
extern "C" {
#endif

// asm.c
int disassemble_x86(void *addr);
void *detourFunction(void *orig, void *det);
void *undetourFunction(void *orig, void *tramp);
int nopInstruction(unsigned int address);

// elf.c
Elf *initElf(char *filename);
Elf_Scn *getSection(Elf *elf, char *sectionName);
GElf_Shdr *getSectionHeader(Elf *elf, char *sectionName);
GElf_Sym *getSymbol(char *filename, char *symbol);
int pltHook(char *symbol, void *hook);
int isStripped(char *filename);

// utils.c
unsigned long crc32FromFile(const char* filename);

#ifdef __cplusplus
}
#endif

// vfunction.c
int getVTblIndex(const char *fmt, ...);

#define ADDRTYPE unsigned long
#define VTBL(classptr) (*(ADDRTYPE *)classptr)
#define PVFN_(classptr, offset) (VTBL(classptr) + offset)
#define VFN_(classptr, offset) *(ADDRTYPE *)PVFN_(classptr, offset)
#define PVFN(classptr, offset) PVFN_(classptr, (offset * sizeof(void *)))
#define VFN(classptr, offset) VFN_(classptr, (offset * sizeof(void *)))

#define DEFVFUNC( funcname , returntype , proto ) \
	typedef returntype (* funcname##Func ) proto ; \
	funcname##Func funcname = NULL; 
#define HOOKVFUNC( classptr , index , funcname , newfunc ) \
	mprotect(GET_PAGE(*(unsigned int **)classptr), 4096, PROT_READ | PROT_WRITE | PROT_EXEC); \
	funcname = ( funcname##Func )VFN( classptr , getVTblIndex("%p", index) ); \
	*(ADDRTYPE*)PVFN( classptr , getVTblIndex("%p", index) ) = (unsigned int)newfunc ; \
	mprotect(GET_PAGE(*(unsigned int **)classptr), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

#endif /*GHF_H_*/
