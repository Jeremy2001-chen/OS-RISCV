#include <Elf.h>
#include <Page.h>
#include <Process.h>
#include <Type.h>
#include <fat.h>
#include <file.h>
#include <string.h>
#include <Sysarg.h>
#include <debug.h>
#include <Trap.h>
#define MAXARG 32  // max exec arguments

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int loadseg(u64* pagetable,
                   u64 va,
                   struct dirent* de,
                   uint offset,
                   uint sz) {
    uint i, n;
    u64 pa;
    int cow;
    MSG_PRINT("");
    for (i = 0; i < sz; i += PGSIZE) {
        pa = vir2phy(pagetable, va + i, &cow);
        if (pa == NULL)
            panic("loadseg: address should exist");
        if (cow) {
            cowHandler(pagetable, va + i);
        }
        if (sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        if (eread(de, 0, (u64)pa, offset + i, n) != n)
            return -1;
    }

    return 0;
}

static int prepSeg(u64* pagetable, uint va, uint filesz) {
    assert(va % PGSIZE == 0);
    for (int i = va; i < va + filesz; i += PGSIZE) {
        PhysicalPage* p;
        if (pageAlloc(&p) < 0)
            return -1;
        pageInsert(pagetable, i, page2pa(p),
                   PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
    }
    return 0;
}

int exec(char* path, char** argv) {
    MSG_PRINT("in exec");
    STR_PRINT(path);
    // char **s = argv;

    // while(*s)
    //     printf("%s ", *s++);
    // printf("\n");

    /*char *s, *last*/;
    int i, off;
    u64 argc,  sp, ustack[MAXARG], stackbase;
    Ehdr elf;
    struct dirent* de;
    Phdr ph;
    u64 *pagetable = 0;
    Process* p = myproc();

    u64* oldpagetable = p->pgdir;

    PhysicalPage *page;
    int r = allocPgdir(&page);
    if (r < 0) {
        panic("setup page alloc error\n");
        return r;
    }

    p->heapBottom = USER_HEAP_BOTTOM;// TODO,these code have writen twice
    pagetable = (u64*)page2pa(page);
    extern char trampoline[];
    pageInsert(pagetable, TRAMPOLINE_BASE, (u64)trampoline, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE);
    pageInsert(pagetable, TRAMPOLINE_BASE + PAGE_SIZE, ((u64)trampoline) + PAGE_SIZE, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE);    
    
    MSG_PRINT("setup");

    if ((de = ename(path)) == 0) {
        MSG_PRINT("find file error\n");
        return -1;
    }
    elock(de);

    MSG_PRINT("lock file success");
    // Check ELF header
    if (eread(de, 0, (u64)&elf, 0, sizeof(elf)) != sizeof(elf)){
        MSG_PRINT("read header error\n");
        goto bad;
    }
    if (!is_elf_format((u8*) &elf)){
        MSG_PRINT("not elf format\n");
        goto bad;
    }


    MSG_PRINT("begin map");
    // Load program into memory.
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
        if (eread(de, 0, (u64)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
        if (ph.type != PT_LOAD)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;
        if (ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if (prepSeg(pagetable, ph.vaddr, ph.memsz))
            goto bad;
        if ((ph.vaddr % PGSIZE) != 0)
            goto bad;
        if (loadseg(pagetable, ph.vaddr, de, ph.offset, ph.filesz) < 0)
            goto bad;
    }
    eunlock(de);
    eput(de);
    de = 0;

    p = myproc();
    sp = USER_STACK_TOP;
    stackbase = sp - PGSIZE;
    if (pageAlloc(&page)){
        MSG_PRINT("allock stack error\n");
        goto bad;
    }
    pageInsert(pagetable, stackbase, page2pa(page),
               PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);

    MSG_PRINT("begin push args\n");
    // Push argument strings, prepare rest of stack in ustack.
    for (argc = 0; argv[argc]; argc++) {
        if (argc >= MAXARG)
            goto bad;
        sp -= strlen(argv[argc]) + 1;
        sp -= sp % 16;  // riscv sp must be 16-byte aligned
        if (sp < stackbase)
            goto bad;
        if (copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
        ustack[argc] = sp;
    }
    ustack[argc] = 0;

    // push the array of argv[] pointers.
    sp -= (argc + 1) * sizeof(u64);
    sp -= sp % 16;
    if (sp < stackbase)
        goto bad;
    if (copyout(pagetable, sp, (char*)ustack, (argc + 1) * sizeof(u64)) < 0)
        goto bad;

    MSG_PRINT("end push args\n");
    // arguments to user main(argc, argv)
    // argc is returned via the system call return
    // value, which goes in a0.
    getHartTrapFrame()->a1 = sp;

    // Save program name for debugging.
    /*
    for (last = s = path; *s; s++)
        if (*s == '/')
            last = s + 1;
    safestrcpy(p->name, last, sizeof(p->name));
    */

    HEX_PRINT(getHartTrapFrame()->epc);
    HEX_PRINT(getHartTrapFrame()->sp);
    MSG_PRINT("out exec");
    // Commit to the user image.
    p->pgdir = pagetable;
    getHartTrapFrame()->epc = elf.entry;  // initial program counter = main
    getHartTrapFrame()->sp = sp;          // initial stack pointer

    //free old pagetable
    pgdirFree(oldpagetable);
	asm volatile("fence.i");
    MSG_PRINT("out exec");
    return argc;  // this ends up in a0, the first argument to main(argc, argv)

bad:
    if (pagetable)
        pgdirFree((u64*)pagetable);
    if (de) {
        if(holdingsleep(&de->lock))
            eunlock(de);
        eput(de);
    }
    return -1;
}

#define MAXPATH      128   // maximum file path name
u64 sys_exec(void) {
    char path[MAXPATH], *argv[MAXARG];
    int i;
    u64 uargv, uarg;

    if (argstr(0, path, MAXPATH) < 0 || argaddr(1, &uargv) < 0) {
        return -1;
    }
    memset(argv, 0, sizeof(argv));
    for (i = 0;; i++) {
        if (i >= NELEM(argv)) {
            goto bad;
        }
        if (fetchaddr(uargv + sizeof(u64) * i, (u64*)&uarg) < 0) {
            goto bad;
        }
        if (uarg == 0) {
            argv[i] = 0;
            break;
        }
        PhysicalPage *page;
        if(pageAlloc(&page))
            goto bad;
        argv[i] = (char *)page2pa(page);
        if (argv[i] == 0)
            goto bad;
        if (fetchstr(uarg, argv[i], PGSIZE) < 0)
            goto bad;
    }

    int ret = exec(path, argv);

    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        pageFree(pa2page((u64)argv[i]));

    return ret;

bad:
    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        pageFree(pa2page((u64)argv[i]));
    return -1;
}
