//
// Support functions for system calls that involve file descriptors.
//
#include "fat.h"
#include "file.h"
#include <Process.h>
#include <Page.h>
#include <string.h>
#include <Spinlock.h>
#include <defs.h>
#include <pipe.h>
#include <debug.h>

struct devsw devsw[NDEV];
struct {
    struct Spinlock lock;
    struct file file[NFILE];
} ftable;

void fileinit(void) {
    initLock(&ftable.lock, "ftable");
    struct file* f;
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        memset(f, 0, sizeof(struct file));
    }
#ifdef ZZY_DEBUG
    printf("fileinit\n");
#endif
}

// Allocate a file structure.
struct file* filealloc(void) {
    struct file* f;

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
struct file* filedup(struct file* f) {
    acquireLock(&ftable.lock);
    if (f->ref < 1)
        panic("filedup");
    f->ref++;
    releaseLock(&ftable.lock);
    return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct file* f) {
    struct file ff;

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
    }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int filestat(struct file* f, u64 addr) {
    struct Process *p = myproc();
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
int fileread(struct file* f, u64 addr, int n) {
    int r = 0;

    if (f->readable == 0)
        return -1;

    switch (f->type) {
        case FD_PIPE:
            r = piperead(f->pipe, addr, n);
            break;
        case FD_DEVICE:
            if (f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
                return -1;
            r = devsw[f->major].read(1, addr, n);
            break;
        case FD_ENTRY:
            elock(f->ep);
            if ((r = eread(f->ep, 1, addr, f->off, n)) > 0)
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
int filewrite(struct file* f, u64 addr, int n) {
    int ret = 0;

    if (f->writable == 0)
        return -1;

    if (f->type == FD_PIPE) {
        ret = pipewrite(f->pipe, addr, n);
        assert(ret != 0);
    } else if (f->type == FD_DEVICE) {
        if (f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
            return -1;
        ret = devsw[f->major].write(1, addr, n);
    } else if (f->type == FD_ENTRY) {
        elock(f->ep);
        if (ewrite(f->ep, 1, addr, f->off, n) == n) {
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
int dirnext(struct file* f, u64 addr) {
    struct Process* p = myproc();

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