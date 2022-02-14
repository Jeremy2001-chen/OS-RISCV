#ifndef _ELF_H_
#define _ELF_H_

#include <Type.h>

#define MAG_SIZE 4
#define ELF_MAGIC0  0x7f
#define ELF_MAGIC1  0x45
#define ELF_MAGIC2  0x4c
#define ELF_MAGIC3  0x46

#define PT_NULL		0x00000000
#define PT_LOAD		0x00000001
#define PT_DYNAMIC	0x00000002
#define PT_INTERP	0x00000003
#define PT_NOTE		0x00000004
#define PT_SHLIB	0x00000005
#define PT_PHDR		0x00000006
#define PT_LOOS		0x60000000
#define PT_HIOS		0x6fffffff
#define PT_LOPROC	0x70000000
#define PT_HIRPOC	0x7fffffff

#define PF_ALL	0x7
#define PF_X	0x1
#define PF_W	0x2
#define PF_R	0x4

#define ET_EXEC 2

typedef struct {
	u8 magic[MAG_SIZE];
	u8 type;
	u8 data;
	u8 version;
	u8 osabi;
	u8 abiversion;
	u8 pad[7];
} Indent;

typedef struct {
	Indent indent;
	u16 type;
	u16 machine;
	u32 version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
} Ehdr;

typedef struct {
	u32 type;
	u32 flags;
	u64 offset;
	u64 vaddr;
	u64 paddr;
	u64 filesz;
	u64 memsz;
	u64 align;
} Phdr;

int loadElf(u8 *binary, int size, u64 *entry, void *userData, 
    int (*map)(u64, u32, u8*, u32, void*));

#endif