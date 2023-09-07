/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ELF definitions
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _ELF_H_
#define _ELF_H_

#include <hal/hal.h>


typedef u16 Elf32_Half;
typedef u32 Elf32_Word;
typedef u32 Elf32_Addr;
typedef u32 Elf32_Off;
typedef s32 Elf32_Sword;


typedef u16 Elf64_Half;
typedef u32 Elf64_Word;
typedef u64 Elf64_Addr;
typedef u64 Elf64_Off;
typedef s64 Elf64_Sword;
typedef u64 Elf64_Xword;


#define EI_NIDENT 16

#define STT_LOPROC 13
#define STT_HIPROC 15

#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff
#define DT_NEEDED 1
#define DT_STRTAB 5

#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_R_SYM(i)   ((i) >> 8)
#define ELF32_R_TYPE(i)  ((unsigned char)(i))


/* File header */

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_hsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr;


typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;


/* Section header with fields' descriptions */

/* Segment types: Elf32_Shdr.sh_type  */
enum {
	SHT_NULL = 0x0,           /* Section header table entry unused */
	SHT_PROGBITS = 0x1,       /* Program data */
	SHT_SYMTAB = 0x2,         /* Symbol table */
	SHT_STRTAB = 0x3,         /* String table */
	SHT_RELA = 0x4,           /* Relocation entries with addends */
	SHT_HASH = 0x5,           /* Symbol hash table */
	SHT_DYNAMIC = 0x6,        /* Dynamic linking information */
	SHT_NOTE = 0x7,           /* Notes */
	SHT_NOBITS = 0x8,         /* Program space with no data (bss) */
	SHT_REL = 0x9,            /* Relocation entries, no addends */
	SHT_SHLIB = 0x0A,         /* Reserved */
	SHT_DYNSYM = 0x0B,        /* Dynamic linker symbol table */
	SHT_INIT_ARRAY = 0x0E,    /* Array of constructors */
	SHT_FINI_ARRAY = 0x0F,    /* Array of destructors */
	SHT_PREINIT_ARRAY = 0x10, /* Array of pre-constructors */
	SHT_GROUP = 0x11,         /* Section group */
	SHT_SYMTAB_SHNDX = 0x12,  /* Extended section indices */
	SHT_NUM = 0x13,           /* Number of defined types. */
	SHT_LOOS = 0x60000000,    /* Start OS-specific. */
	SHT_LOPROC = 0x70000000,  /* Values in this inclusive range are reserved for processor-specific semantics */
	SHT_HIPROC = 0x7fffffff,  /* Values in this inclusive range are reserved for processor-specific semantics */
	SHT_LOUSER = 0x80000000,  /* This value specifies the lower bound of the range of indexes reserved for application programs.*/
	SHT_HIUSER = 0xffffffff,
};


/* Segment flags: Elf32_Shdr.sh_flags */
enum {
	SHF_WRITE = 0x1,              /* Writable */
	SHF_ALLOC = 0x2,              /* Occupies memory during execution */
	SHF_EXECINSTR = 0x4,          /* Executable */
	SHF_MERGE = 0x10,             /* Might be merged */
	SHF_STRINGS = 0x20,           /* Contains null-terminated strings */
	SHF_INFO_LINK = 0x40,         /* 'sh_info' contains SHT index */
	SHF_LINK_ORDER = 0x80,        /* Preserve order after combining */
	SHF_OS_NONCONFORMING = 0x100, /* Non-standard OS specific handling required */
	SHF_GROUP = 0x200,            /* Section is member of a group */
	SHF_TLS = 0x400,              /* Section hold thread-local data */
	SHF_MASKOS = 0x0ff00000,      /* OS-specific */
	SHF_MASKPROC = 0xf0000000,    /* Processor-specific */
	SHF_ORDERED = 0x4000000,      /* Special ordering requirement (Solaris) */
	SHF_EXCLUDE = 0x8000000,      /* Section is excluded unless referenced or allocated (Solaris) */
};


/* Section header */
typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;


typedef struct {
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
} Elf64_Shdr;


/* Program header fields' descriptions */

/* Segment types: Elf32_Phdr.p_type */
enum {
	PHT_NULL = 0x0,
	PHT_LOAD = 0x1,
	PHT_DYNAMIC = 0x2,
	PHT_INTERP = 0x3,
	PHT_NOTE = 0x4,
	PHT_SHLIB = 0x5,
	PHT_PHDR = 0x6,
	PHT_LOPROC = 0x70000000,
	PHT_HIPROC = 0x7fffffff
};


/* Segment flags: Elf32_Phdr.p_flags */
enum {
	PHF_X = 0x1,               /* Execute */
	PHF_W = 0x2,               /* Write */
	PHF_R = 0x4,               /* Read */
	PHF_MASKPROC = 0xf0000000, /* Unspecified */
};


typedef struct {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;


typedef struct {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr p_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
} Elf64_Phdr;


#endif
