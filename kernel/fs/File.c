//
// Support functions for system calls that involve file descriptors.
//
#include "fat.h"
#include <file.h>
#include <Process.h>
#include <Page.h>
#include <string.h>
#include <Spinlock.h>
#include <Defs.h>
#include <pipe.h>
#include <Debug.h>
#include <Socket.h>
#include <Mmap.h>

struct devsw devsw[NDEV];
struct {
    struct Spinlock lock;
    struct File file[NFILE];
} ftable;

void fileinit(void) {
    initLock(&ftable.lock, "ftable");
    struct File* f;
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        memset(f, 0, sizeof(struct File));
    }
}

// Allocate a file structure.
struct File* filealloc(void) {
    struct File* f;

    acquireLock(&ftable.lock);
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) {
            f->ref = 1;
            releaseLock(&ftable.lock);
            return f;
        }
    }
    releaseLock(&ftable.lock);
    return NULL;
}

// Increment ref count for file f.
struct File* filedup(struct File* f) {
    acquireLock(&ftable.lock);
    if (f->ref < 1)
        panic("filedup");
    f->ref++;
    releaseLock(&ftable.lock);
    return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct File* f) {
    struct File ff;

    // printf("[FILE CLOSE]%x %x\n", f, f->ref);
    acquireLock(&ftable.lock);
    if (f->ref < 1)
        panic("fileclose");
    if (--f->ref > 0) {
        releaseLock(&ftable.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    releaseLock(&ftable.lock);

    // printf("FILECLOSE %x\n", ff.type);
    if (ff.type == FD_PIPE) {
        pipeclose(ff.pipe, ff.writable);
    } else if (ff.type == FD_ENTRY) {
        eput(ff.ep);
    } else if (ff.type == FD_DEVICE) {
    } else if (ff.type == FD_SOCKET) {
        socketFree(ff.socket);
    }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int filestat(struct File* f, u64 addr) {
    struct Process *p = myProcess();
    struct stat st;

    if (f->type == FD_ENTRY) {
        elock(f->ep);
        estat(f->ep, &st);
        eunlock(f->ep);
        if(copyout(p->pgdir, addr, (char *)&st, sizeof(st)) < 0)
            return -1;
        return 0;
    }
    return -1;
}

// Read from file f.
// addr is a user virtual address.
int fileread(struct File* f, bool isUser, u64 addr, int n) {
    int r = 0;

    if (f->readable == 0)
        return -1;

    switch (f->type) {
        case FD_PIPE:
            if (!isUser) {
                panic("");
            }
            r = piperead(f->pipe, addr, n);
            break;
        case FD_DEVICE:
            if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
                return -1;
            r = devsw[f->major].read(isUser, addr, 0, n);
            break;
        case FD_ENTRY:
            elock(f->ep);
            if ((r = eread(f->ep, isUser, addr, f->off, n)) > 0)
                f->off += r;
            eunlock(f->ep);
            break;
        default:
            panic("fileread");
    }

    return r;
}

// Write to file f.
// addr is a user virtual address.
int filewrite(struct File* f, bool isUser, u64 addr, int n) {
    int ret = 0;

    if (f->writable == 0)
        return -1;

    if (f->type == FD_PIPE) {
        if (!isUser) {
            panic("");
        }
        ret = pipewrite(f->pipe, addr, n);
        assert(ret != 0);
    } else if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
            return -1;
        ret = devsw[f->major].write(isUser, addr, 0, n);
    } else if (f->type == FD_ENTRY) {
        elock(f->ep);
        if (ewrite(f->ep, isUser, addr, f->off, n) == n) {
            ret = n;
            f->off += n;
        } else {
            ret = -1;
        }
        eunlock(f->ep);
    } else {
        panic("filewrite");
    }

    return ret;
}

// Read from dir f.
// addr is a user virtual address.
int dirnext(struct File* f, u64 addr) {
    struct Process* p = myProcess();

    if (f->readable == 0 || !(f->ep->attribute & ATTR_DIRECTORY))
        return -1;

    struct dirent de;
    struct stat st;
    int count = 0;
    int ret;
    elock(f->ep);
    while ((ret = enext(f->ep, &de, f->off, &count)) ==
           0) {  // skip empty entry
        f->off += count * 32;
    }
    eunlock(f->ep);
    if (ret == -1)
        return 0;

    f->off += count * 32;
    estat(&de, &st);
    if (copyout(p->pgdir, addr, (char*)&st, sizeof(st)) < 0)
        return -1;

    return 1;
}

u64 do_mmap(struct File* fd, u64 start, u64 len, int perm, int flags, u64 off) {
    bool alloc = (start == 0);
    if (alloc) {
        myProcess()->heapBottom = UP_ALIGN(myProcess()->heapBottom, PAGE_SIZE);
        start = myProcess()->heapBottom;
        myProcess()->heapBottom = UP_ALIGN(myProcess()->heapBottom + len, PAGE_SIZE);
    }
    u64 addr = start, end = start + len;
    start = DOWN_ALIGN(start, 12);
    while (start < end) {
        u64* pte;
        u64 pa = pageLookup(myProcess()->pgdir, start, &pte);
        if (pa > 0 && (*pte & PTE_COW)) {
            cowHandler(myProcess()->pgdir, start);
        }
        PhysicalPage* page;
        if (pageAlloc(&page) < 0) {
            return -1;
        }
        pageInsert(myProcess()->pgdir, start, page2pa(page), perm | PTE_USER | PTE_READ | PTE_WRITE | PTE_EXECUTE);
        start += PGSIZE;
    }

    if (flags & MAP_ANONYMOUS) {
        return addr;
    }
    /* if fd == NULL, we think this is a anonymous map */
    if (fd != NULL) {
        fd->off = off;
        if (fileread(fd, true, addr, len)) {
            return addr;
        } else {
            return -1;
        }
    } else {
        return addr;
    }
}