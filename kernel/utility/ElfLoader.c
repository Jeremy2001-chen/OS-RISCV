#include <Elf.h>
#include <Error.h>


int loadElf(u8 *binary, int size, u64 *entry, void *userData,
    int (*map)(u64 va, u32 segmentSize, u8 *bin, u32 binSize, void *userData)) {
    
    Ehdr *ehdr = (Ehdr*) binary;
    Phdr *phdr = 0;

    u8 *ph_table = 0;
    u16 entry_cnt;
    u16 entry_size;
    int r;

    if (size < 4 || !is_elf_format(binary)) {
        return -NOT_ELF_FILE;
    }

    ph_table = binary + ehdr->phoff;
    entry_cnt = ehdr->phnum;
    entry_size = ehdr->phentsize;

    while (entry_cnt--) {
        phdr = (Phdr*)ph_table;
        if (phdr->type == PT_LOAD) {
            r = map(phdr->vaddr, phdr->memsz, binary + phdr->offset, phdr->filesz, userData);
            if (r < 0) {
                return r;
            }
        }
        ph_table += entry_size;
    }
    
    *entry = ehdr->entry;
    return 0;
}