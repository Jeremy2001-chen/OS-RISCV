#include <Elf.h>
#include <Error.h>
#include <Page.h>
#include <Process.h>
#include <Type.h>
#include <fat.h>
#include <file.h>
#include <string.h>
#include <Sysarg.h>
#include <Debug.h>
#include <Trap.h>
#include <Sysfile.h>
#include <uapi/linux/auxvec.h>
#include <Mmap.h>

#define MAXARG 32  // max exec arguments

// AT_VECTOR_SIZE: size of auxiliary table.
#define AT_VECTOR_SIZE (2 * (2 + 20 + 1))
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

static int loadSegment(u64* pagetable, u64 va, u64 segmentSize, struct dirent* de, u32 fileOffset, u32 binSize) {
    u64 offset = va - DOWN_ALIGN(va, PAGE_SIZE);
    PhysicalPage* page = NULL;
    u64* entry;
    u64 i;
    int r = 0;    
    if (offset > 0) {
        page = pa2page(pageLookup(pagetable, va, &entry));
        if (page == NULL) {
            if (pageAlloc(&page) < 0) {
                printf("load segment error when we need to alloc a page!\n");
            }
            pageInsert(pagetable, va, page2pa(page), PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(binSize, PAGE_SIZE - offset);
        if (eread(de, 0, page2pa(page) + offset, fileOffset, r) != r) {
            panic("load segment error when eread on offset\n");
        }
    }

    for (i = r; i < binSize; i += r) {
        if (pageAlloc(&page) != 0) {
            panic("load segment error when we need to alloc a page 1!\n");
        }
        pageInsert(pagetable, va + i, page2pa(page), PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, binSize - i);
        if (eread(de, 0, page2pa(page), fileOffset + i, r) != r) {
            panic("load segment error when eread on offset 1\n");
        }
    }

    offset = va + i - DOWN_ALIGN(va + i, PAGE_SIZE);
    if (offset > 0) {
        page = pa2page(pageLookup(pagetable, va + i, &entry));
        if (page == NULL) {
            if (pageAlloc(&page) != 0) {
                panic("load segment error when we need to alloc a page 2!\n");
            }
            pageInsert(pagetable, va + i, page2pa(page), PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        }
        r = MIN(segmentSize - i, PAGE_SIZE - offset);
        bzero((void*)page2pa(page) + offset, r);
    }

    for (i += r; i < segmentSize; i += r) {
        if (pageAlloc(&page) != 0) {
            panic("load segment error when we need to alloc a page 3!\n");
        }
        pageInsert(pagetable, va + i, page2pa(page), PTE_EXECUTE | PTE_READ | PTE_WRITE | PTE_USER);
        r = MIN(PAGE_SIZE, segmentSize - i);
        bzero((void*)page2pa(page), r);
    }
    return 0;
}

/* for compat with linux interface */
void* kmalloc(int size, int policy) {
    if (size > PAGE_SIZE){
        panic("Do not support kmalloc a mem which size > PAGE_SIZE");
    }
	PhysicalPage* p;
    pageAlloc(&p);
    return (void *)page2pa(p);
}
void kfree(void* mem) {
    pageFree(pa2page((u64)mem));
}
static u64 total_mapping_size(const Phdr* phdr, int nr) {
    u64 min_addr = -1;
    u64 max_addr = 0;
    bool pt_load = false;
    int i;

    for (i = 0; i < nr; i++) {
        if (phdr[i].type == PT_LOAD) {
            min_addr = min(min_addr, DOWN_ALIGN(phdr[i].vaddr, PGSIZE));//最低虚拟地址
            max_addr = max(max_addr, phdr[i].vaddr + phdr[i].memsz);//最高虚拟地址
            pt_load = true;
        }
    }
    return pt_load ? (max_addr - min_addr) : 0;
}

static u64 elf_map(struct File* filep,
                   u64 addr,
                   const Phdr* eppnt,
                   int prot,
                   int type,
                   u64 total_size) {
    u64 map_addr;
    u64 size = eppnt->filesz + PAGE_OFFSET(eppnt->vaddr, PAGE_SIZE);
    u64 off = eppnt->offset - PAGE_OFFSET(eppnt->vaddr, PAGE_SIZE);
    addr = DOWN_ALIGN(addr, PAGE_SIZE);
    // size = UP_ALIGN(size, PGSIZE);

    /* mmap() will return -EINVAL if given a zero size, but a
     * segment with zero filesize is perfectly valid */
    if (!size)
        return addr;

    /*
     * total_size is the size of the ELF (interpreter) image.
     * The _first_ mmap needs to know the full size, otherwise
     * randomization might put this image into an overlapping
     * position with the ELF binary image. (since size < total_size)
     * So we first map the 'big' image - and unmap the remainder at
     * the end. (which unmap is needed for ELF images with holes.)
     */

    if (total_size) {
        total_size = UP_ALIGN(total_size, PAGE_SIZE);
        map_addr = do_mmap(NULL, addr, total_size, prot, type, off);
        addr = map_addr;
    }

    map_addr = do_mmap(filep, addr, size, prot, type, off);

    if (map_addr == -1)
        panic("mmap interpreter fail!");

    return (map_addr);
}
static inline int make_prot(u32 p_flags) {
    return PTE_READ | PTE_WRITE | PTE_EXECUTE | PTE_USER | PTE_ACCESSED;
/*
    int prot = 0;

    if (p_flags & PF_R)
        prot |= PTE_READ;
    if (p_flags & PF_W)
        prot |= PTE_WRITE;
    if (p_flags & PF_X)
        prot |= PTE_EXECUTE;
    return prot|PTE_USER|PTE_ACCESSED;
*/
}
//加载动态链接器
u64 load_elf_interp(u64* pagetable,
                    Ehdr* interp_elf_ex,
                    struct dirent* interpreter,
                    u64 no_base,
                    Phdr* interp_elf_phdata) {
	Phdr  *eppnt;
	u64 load_addr = 0;
	int load_addr_set = 0;
	u64 last_bss = 0, elf_bss = 0;
	u64 error = ~0UL;
	u64 total_size;
	int i;


	total_size = total_mapping_size(interp_elf_phdata,
					interp_elf_ex->phnum);
	if (!total_size) {
		error = -EINVAL;
		goto out;
	}
/*

| <-----------------total_size------------------> |
| <------Seg1------>     <---------Seg2-------->  |
首先申请total_size（这里是不固定的，需要OS来分配）。直接调用 do_mmap();
然后再把每个段填进去（这里的地址是确定的）

*/
	eppnt = interp_elf_phdata;
	for (i = 0; i < interp_elf_ex->phnum; i++, eppnt++) {
		if (eppnt->type == PT_LOAD) {
			int elf_type = MAP_PRIVATE;
			int elf_prot = make_prot(eppnt->flags);
			u64 vaddr = 0;
			u64 k, map_addr;

			vaddr = eppnt->vaddr;

            /* 第一次会走第二个分支，第二次会走第一个分支*/
			if (interp_elf_ex->type == ET_EXEC || load_addr_set)
				elf_type |= MAP_FIXED;
			else if (no_base && interp_elf_ex->type == ET_DYN)
				load_addr = -vaddr;
            
            struct File interp_file;
            interp_file.ep = interpreter;
            interp_file.type = FD_ENTRY;
            interp_file.readable = 1;
			map_addr = elf_map(&interp_file, load_addr + vaddr,
					eppnt, elf_prot, elf_type, total_size);
			total_size = 0;
			error = map_addr;
			if (map_addr == -1)
                panic("elf_map failed");

			if (!load_addr_set &&
			    interp_elf_ex->type == ET_DYN) {
				load_addr = map_addr - PAGE_OFFSET(vaddr, PAGE_SIZE);
				load_addr_set = 1;
			}


			/*
			 * Find the end of the file mapping for this phdr, and
			 * keep track of the largest address we see for this.
			 */
			k = load_addr + eppnt->vaddr + eppnt->filesz;
			if (k > elf_bss)
				elf_bss = k;

			/*
			 * Do the same thing for the memory mapping - between
			 * elf_bss and last_bss is the bss section.
			 */
			k = load_addr + eppnt->vaddr + eppnt->memsz;
			if (k > last_bss) {
				last_bss = k;
			}
		}
	}

    /* 我们并不需要扩展 bss 段， 访问bss时操作系统会pageout */
	/*
	 * Now fill out the bss section: first pad the last page from
	 * the file up to the page boundary, and zero it from elf_bss
	 * up to the end of the page.
	 */
    /*
	if (padzero(elf_bss)) {
		error = -EFAULT;
		goto out;
	}*/
	/*
	 * Next, align both the file and mem bss up to the page size,
	 * since this is where elf_bss was just zeroed up to, and where
	 * last_bss will end after the vm_brk_flags() below.
	 */
    /*
	elf_bss = ELF_PAGEALIGN(elf_bss);
	last_bss = ELF_PAGEALIGN(last_bss);
    */
	/* Finally, if there is still more bss to allocate, do it. */
    /*
	if (last_bss > elf_bss) {
		error = vm_brk_flags(elf_bss, last_bss - elf_bss,
				bss_prot & PROT_EXEC ? VM_EXEC : 0);
		if (error)
			goto out;
	}*/

	error = load_addr;
out:
	return error;
}

int exec(char* path, char** argv);
/* sizeof(linux_binprm->buf) */
#define BINPRM_BUF_SIZE 128
int load_script(char* bprmbuf, char* path, char** argv) {
    char *cp, *i_name, *i_arg;
    if ((cp = strchr(bprmbuf, '\n')) == NULL)
        cp = bprmbuf + strlen(bprmbuf);
    *cp = '\0';
    while (cp > bprmbuf) {
        cp--;
        if ((*cp == ' ') || (*cp == '\t'))
            *cp = '\0';
        else
            break;
    }
    for (cp = bprmbuf + 2; (*cp == ' ') || (*cp == '\t'); cp++)
        ;
    if (*cp == '\0')
        return -ENOEXEC; /* No interpreter name found */
    i_name = cp;
    i_arg = NULL;
    for (; *cp && (*cp != ' ') && (*cp != '\t'); cp++)
        /* nothing */;
    while ((*cp == ' ') || (*cp == '\t'))
        *cp++ = '\0';
    if (*cp)
        i_arg = cp;

    char* script_argv[MAXARG] = {0};
    int script_argc = 0;
    script_argv[script_argc++] = i_name;
    if (i_arg) {
        script_argv[script_argc++] = i_arg;
    }
    while (*argv) {
        script_argv[script_argc++] = *(argv++);
    }
    return exec(i_name, script_argv);
}

int exec(char* path, char** argv) {
    // char **s = argv;

    // while(*s)
    //     printf("%s ", *s++);
    // printf("\n");

    /*char *s, *last*/;
    int i, off;
    u64 argc,  sp, ustack[MAXARG + AT_VECTOR_SIZE], stackbase;
    Ehdr elf;
    struct dirent* de;
    Phdr ph;
    u64 *pagetable = 0, *old_pagetable = 0;
    Process* p = myProcess();
    u64* oldpagetable = p->pgdir;
    u64 phdr_addr = 0; // virtual address in user space, point to the program header. We will pass 'phdr_addr' to ld.so

    if ((de = ename(AT_FDCWD, path, true)) == 0) {
        MSG_PRINT("find file error\n");
        return -1;
    }
    elock(de);

/* ========== check executable format (script or elf) =========== */

    char bprmbuf[BINPRM_BUF_SIZE];
    memset(bprmbuf, 0, sizeof(bprmbuf));
    eread(de, 0, (u64)bprmbuf, 0, BINPRM_BUF_SIZE - 1);
    if (bprmbuf[0] == '#' && bprmbuf[1] == '!') {
        eunlock(de);
        eput(de);
        return load_script(bprmbuf, path, argv);
    }

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
    extern char signalTrampoline[];
    pageInsert(pagetable, SIGNAL_TRAMPOLINE_BASE, (u64)signalTrampoline,
        PTE_USER | PTE_EXECUTE | PTE_READ | PTE_WRITE);

    r = pageAlloc(&page);
    if (r < 0) {
        panic("setup stack alloc error\n");
        return r;
    }
    pageInsert(pagetable, USER_STACK_TOP - PGSIZE, page2pa(page), PTE_USER | PTE_READ | PTE_WRITE | PTE_EXECUTE); // We must alloc the stack

    old_pagetable = p->pgdir;
    p->pgdir = pagetable;

    MSG_PRINT("setup");


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
        if (eread(de, 0, (u64)&ph, off, sizeof(ph)) != sizeof(ph)) {
            goto bad;
        }
        if (ph.type != PT_LOAD)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;
        if (loadSegment(pagetable, ph.vaddr, ph.memsz, de, ph.offset, ph.filesz) < 0)
            goto bad;
		/*
		 * Figure out which segment in the file contains the Program
		 * Header table, and map to the associated memory address.
		 */
        if (ph.offset <= elf.phoff && elf.phoff < ph.offset + ph.filesz) {
            phdr_addr = elf.phoff - ph.offset + ph.vaddr;
        }
    }

/* ============= Dynamic Link, find Interpreter Path and load Interpreter =============== */
    int retval = 0;
    struct dirent* interpreter = NULL;
    Ehdr* interp_elf_ex;
    u64 elf_entry;
    u64 interp_load_addr = 0;
    u64 load_bias =0;  // load_bias only work when object is ET_DYN, such as ./ld.so
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
        if (eread(de, 0, (u64)&ph, off, sizeof(ph)) != sizeof(ph)) {
            goto bad;
        }
        if (ph.type == PT_GNU_PROPERTY) {
            panic("do not support PT_GNU_PROPERTY Segment");
        }

        if (ph.type != PT_INTERP)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;

        if (ph.filesz > 4096 || ph.filesz < 2)
            panic("interpreter path too long !");

        char* elf_interpreter = kmalloc(ph.filesz, 0);
        if (!elf_interpreter)
            panic("Alloc page for elf_interpreter error!");

        retval = eread(de, 0, (u64)elf_interpreter, ph.offset, ph.filesz);
        if (retval < 0)
            panic("read interp path error");

        /* make sure path is NULL terminated */
        if (elf_interpreter[ph.filesz - 1] != '\0')
            panic("interpreter path is not NULL terminated");

        interpreter = ename(AT_FDCWD, elf_interpreter, true);

        // kfree(elf_interpreter);
        if (interpreter == NULL)
            panic("open interpreter error!");

        interp_elf_ex = kmalloc(sizeof(*interp_elf_ex), 0);
        if (!interp_elf_ex) {
            panic("Alloc page for interpreter Elf_Header error!");
        }

        /* Get the interp header */
        retval = eread(interpreter, 0, (u64)interp_elf_ex, 0,
                       sizeof(*interp_elf_ex));
        if (retval < 0)
            panic("read interpreter file error");
    }
    if (interpreter) {
        u64 interp_Phdrs_size = sizeof(Phdr) * interp_elf_ex->phnum;
        Phdr* interp_elf_phdata = kmalloc(interp_Phdrs_size, 0);
        eread(interpreter, 0, (u64)interp_elf_phdata, interp_elf_ex->phoff,
              interp_Phdrs_size);
        elf_entry = load_elf_interp(pagetable, interp_elf_ex, interpreter, load_bias,
                                    interp_elf_phdata);
        interp_load_addr = elf_entry;
        elf_entry += interp_elf_ex->entry;
        // kfree(interp_elf_ex);
        // kfree(interp_elf_phdata);
    } else {
        elf_entry = elf.entry;
    }

#ifdef ZZY_DEBUG
    printf("end of load interpreter\n");
#endif
/* ============ End of find and load interpreter ============== */

    p = myProcess();
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
        ustack[argc + 1] = sp;
    }
    ustack[0] = argc;
    ustack[argc + 1] = 0;

    char *envVariable[] = {"LD_LIBRARY_PATH=/", "PATH=/", /*"LOOP_O=100", "TIMING_O=100", "ENOUGH=100"*/};
    int envCount = sizeof(envVariable) / sizeof(char*);
    for (i = 0; i < envCount; i++) {
        sp -= strlen(envVariable[i]) + 1;
        sp -= sp % 16;  // riscv sp must be 16-byte aligned
        if (sp < stackbase)
            goto bad;
        if (copyout(pagetable, sp, envVariable[i], strlen(envVariable[i]) + 1) < 0)
            goto bad;
        ustack[argc + 2 + i] = sp;
    }
    ustack[argc + 2 + envCount] = 0;

/* ============== Dynamic Link, put args passed to ld.so ================ */

/*
    argc
    argv[1]
    argv[2]
    NULL
    envp[1]
    envp[2]
    NULL
    AT_HWCAP ELF_HWCAP
    AT_PAGESZ ELF_EXEC_PAGESIZE
    NULL NULL
    "argv1"
    "argv2"
    "va=a"
    "vb=b"
*/

	/*
	 * Generate 16 random bytes for userspace PRNG seeding.
	 */
    static u8 k_rand_bytes[] = {0, 1, 2,  3,  4,  5,  6,  7,
                                8, 9, 10, 11, 12, 13, 14, 15};
    sp -= 32;
    sp -= sp % 16;
    if (sp < stackbase)
        goto bad;
    if (copyout(pagetable, sp, (char *)k_rand_bytes, sizeof(k_rand_bytes)) < 0)
        goto bad;
    u64 u_rand_bytes = sp;

    u64* elf_info = ustack + (argc + envCount + 3) ;
#define NEW_AUX_ENT(id, val) \
	do { \
		*elf_info++ = id; \
		*elf_info++ = val; \
	} while (0)

    #define ELF_HWCAP 0  /* we assume this risc-v cpu have no extensions */
	#define ELF_EXEC_PAGESIZE PAGE_SIZE

    /* these function always return 0 */
	#define from_kuid_munged(x, y) (0)
	#define from_kgid_munged(x,y) (0)

    u64 secureexec = 0; // the default value is 1, 但是我不清楚哪些情况会把它变成0
	NEW_AUX_ENT(AT_HWCAP, ELF_HWCAP); //CPU的extension信息
	NEW_AUX_ENT(AT_PAGESZ, ELF_EXEC_PAGESIZE); //PAGE_SIZE
	NEW_AUX_ENT(AT_PHDR, phdr_addr);// Phdr * phdr_addr; 指向用户态。
	NEW_AUX_ENT(AT_PHENT, sizeof(Phdr)); //每个 Phdr 的大小
	NEW_AUX_ENT(AT_PHNUM, elf.phnum); //phdr的数量
	NEW_AUX_ENT(AT_BASE, interp_load_addr);
	NEW_AUX_ENT(AT_ENTRY, elf.entry);//源程序的入口
	NEW_AUX_ENT(AT_UID, from_kuid_munged(cred->user_ns, cred->uid));// 0
	NEW_AUX_ENT(AT_EUID, from_kuid_munged(cred->user_ns, cred->euid));// 0
	NEW_AUX_ENT(AT_GID, from_kgid_munged(cred->user_ns, cred->gid));// 0
	NEW_AUX_ENT(AT_EGID, from_kgid_munged(cred->user_ns, cred->egid));// 0
	NEW_AUX_ENT(AT_SECURE, secureexec);//安全，默认1。该模式下不会启用LD_LIBRARY_PATH等
	NEW_AUX_ENT(AT_RANDOM, u_rand_bytes);//16byte随机数的地址。
#ifdef ELF_HWCAP2
	NEW_AUX_ENT(AT_HWCAP2, ELF_HWCAP2);
#endif
	NEW_AUX_ENT(AT_EXECFN, ustack[1]/*用户态地址*/); /* 传递给动态连接器该程序的名称 */
	/* And advance past the AT_NULL entry.  */
    NEW_AUX_ENT(0, 0);
    //auxiliary 辅助数组
#undef NEW_AUX_ENT
/* ============= End put args for ld.so =============== */

    u64 copy_size = (elf_info - ustack) * sizeof(u64);
#ifdef ZZY_DEBUG
    printf("copy size = %x\n", copy_size);
#endif

    // push the array of argv[] pointers, envp[] pointers, auxv[] array.
    sp -= copy_size; /* now elf_info is the stack top */
    sp -= sp % 16;
    if (sp < stackbase)
        goto bad;
    if (copyout(pagetable, sp, (char*)ustack, copy_size) < 0)
        goto bad;

#ifdef ZZY_DEBUG
    for (u64* i = (u64 *)sp; (u64)i < USER_STACK_TOP; ++i) {
        u64 ret;
        copyin(pagetable, (char*)&ret, (u64)(i), sizeof(u64));
        printf("*%lx = %lx\n", (u64)i, ret);
    }
#endif
    // arguments to user main(argc, argv)
    // argc is returned via the system call return
    // value, which goes in a0.
    getHartTrapFrame()->a1 = sp;
    // printf("sp:  %lx\n", sp);

    // Save program name for debugging.
    /*
    for (last = s = path; *s; s++)
        if (*s == '/')
            last = s + 1;
    safestrcpy(p->name, last, sizeof(p->name));
    */
   
    // Commit to the user image.


    getHartTrapFrame()->epc = elf_entry;  // initial program counter = main
    getHartTrapFrame()->sp = sp;          // initial stack pointer
    
    
    char buf[FAT32_MAX_PATH];
    r = getAbsolutePath(de, false, (u64)buf, sizeof(buf));
    if (r) {
        panic("");
    }
    eunlock(de);
    eput(de);
    de = ename(AT_FDCWD, "/proc/self/exe", false);
    if (!de) {
        panic("");
    }
    r = ewrite(de, false, (u64)buf, 0, strlen(buf) + 1);
    if (r < 0) {
        panic("");
    }

    myThread()->clearChildTid = 0;
    //free old pagetable
    pgdirFree(oldpagetable);
    asm volatile("fence.i");
    // printf("[exec]exec finish %s\n", path);
    return 0;  // this ends up in a0, the first argument to main(argc, argv)

bad:
    p->pgdir = old_pagetable;
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
