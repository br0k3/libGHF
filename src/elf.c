#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 

#include "ghf.h"

// Don't forget to call	elf_end(Elf *);
Elf *initElf(char *filename) {
	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "ghf: error: Outdated libelf\n");
		return NULL;
	}
	
	if (!filename)
		filename = "/proc/self/exe";

	int fd = open(filename, O_RDONLY);
	Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
	close(fd);

	if (!elf) {
		fprintf(stderr, "ghf: error: Invalid ELF\n");
		return NULL;
	}

	return elf;
}

Elf_Scn *getSection(Elf *elf, char *sectionName) {
	GElf_Ehdr ehdr;
	gelf_getehdr(elf, &ehdr);
	
	// Loop through sections
	Elf_Scn *scn = NULL;
	while ((scn = elf_nextscn(elf, scn)) != NULL) {
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);

		if (!strcmp(elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name), sectionName))
			break;
	}

	return scn;
}

GElf_Shdr *getSectionHeader(Elf *elf, char *sectionName) {
	GElf_Ehdr ehdr;
	gelf_getehdr(elf, &ehdr);
	
	static GElf_Shdr shdr;
	memset(&shdr, 0, sizeof(GElf_Shdr));

	Elf_Scn *scn = getSection(elf, sectionName);
	if (!scn)
		return NULL;

	gelf_getshdr(scn, &shdr);

	return &shdr;
}

// Get symbol from an ELF. If filename is NULL, the current process is use.
// Return NULL if the symbol is not found
GElf_Sym *getSymbol(char *filename, char *symbol) {
	Elf *elf = initElf(filename);

	if (!elf)
		return NULL;

	static GElf_Sym sym;

	// Sections where to search for symbol
	char *sections[] = {
		".symtab",
		".dynsym"
	};

	int sCount = 0;
	for (; sCount < (sizeof(sections) / sizeof(char *)); sCount++) {
		Elf_Scn *scn = getSection(elf, sections[sCount]);
		if (!scn)
			continue;
	
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);
		
		Elf_Data *data = elf_getdata(scn, NULL);
		if (!data)
			continue;
		
		// Search the symbol
		int count = 0;
		for (; count < (shdr.sh_size / shdr.sh_entsize); count++) {
			gelf_getsym(data, count, &sym);
	
			// Symbol found
			if (!strcmp(elf_strptr(elf, shdr.sh_link, sym.st_name), symbol)) {
				elf_end(elf);
				return &sym;
			}
		}
	}

	// Symbol not found
	elf_end(elf);
	return NULL;
}

int pltHook(char *symbol, void *hook) {
	Elf *elf = initElf(NULL);
	if (!elf)
		return 0;

	// Get symbol hash

	int symbolHash = -1;

	Elf_Scn *scn = getSection(elf, ".dynsym");
	if (!scn) {
		fprintf(stderr, "ghf: pltHook: .dynsym section not found\n");
		return 0;
	}

	GElf_Shdr shdr;
	gelf_getshdr(scn, &shdr);
	
	Elf_Data *data = elf_getdata(scn, NULL);
	if (!data)
		return 0;
	
	// Search the symbol
	int count = 0;
	for (; count < (shdr.sh_size / shdr.sh_entsize); count++) {
		GElf_Sym sym;
		gelf_getsym(data, count, &sym);
		if (!strcmp(elf_strptr(elf, shdr.sh_link, sym.st_name), symbol)) {
			symbolHash = count;
			break;
		}
	}
	
	// Sanity check
	if (symbolHash == -1)
		return 0;

	// Search symbol hash in "Relocation Procedure Linkage Table" section

	scn = getSection(elf, ".rel.plt");
	if (!scn) {
		fprintf(stderr, "ghf: pltHook: .rel.plt section not found\n");
		return 0;
	}

	gelf_getshdr(scn, &shdr);
	
	data = elf_getdata(scn, NULL);
	if (!data)
		return 0;

	// Search the symbol
	count = 0;
	for (; count < (shdr.sh_size / shdr.sh_entsize); count++) {
		GElf_Rel rel;
		gelf_getrel(data, count, &rel);

		// If symbol found
		if (ELF64_R_SYM(rel.r_info) == symbolHash) {
			*(void **)(unsigned int)rel.r_offset = hook;
			elf_end(elf);
			return 1;
		}
	}

	// Symbol not found	
	elf_end(elf);
	return 0;
}

// Return value:
//  1 if stripped
//  0 if not stripped
// -1 if error
int isStripped(char *filename) {
	Elf *elf = initElf(filename);
	if (!elf)
		return -1;

	Elf_Scn *scn1 = getSection(elf, ".symtab");
	Elf_Scn *scn2 = getSection(elf, ".dynsym");

	elf_end(elf);
	
	return ((scn1 == NULL) && (scn2 == NULL));
}

