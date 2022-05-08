//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <fat.h>
#include <Process.h>
#include <sysfile.h>
#include <Driver.h>
#include <string.h>
#include <Sysarg.h>
#include <file.h>
#include <debug.h>
#include <string.h>
#include <Page.h>
#include <pipe.h>

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
int argfd(int n, int* pfd, struct file** pf) {
    int fd;
    struct file* f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
int fdalloc(struct file* f) {
    int fd;
    struct Process* p = myproc();

    for (fd = 0; fd < NOFILE; fd++) {
        if (p->ofile[fd] == 0) {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

u64 sys_dup(void) {
    struct file* f;
    int fd;

    if (argfd(0, 0, &f) < 0)
        return -1;
    if ((fd = fdalloc(f)) < 0)
        return -1;
    filedup(f);
    return fd;
}

int sysDupAndSet(void) {
    struct file* f;
    int fdnew;

    if (argfd(0, 0, &f) < 0)
        return -1;
    if (argint(1, &fdnew) < 0)
        return -1;
    if (myproc()->ofile[fdnew] != NULL)
        return -1;
    myproc()->ofile[fdnew] = f;
    filedup(f);
    return fdnew;
}

u64 sys_read(void) {
    struct file* f;
    int n;
    u64 p;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    return fileread(f, p, n);
}

u64 sys_write(void) {
    struct file* f;
    int n;
    u64 p;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;

    return filewrite(f, p, n);
}

u64 sys_close(void) {
    int fd;
    struct file* f;

    if (argfd(0, &fd, &f) < 0)
        return -1;
    myproc()->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

u64 sys_fstat(void) {
    struct file* f;
    u64 st;  // user pointer to struct stat

    if (argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
        return -1;
    return filestat(f, st);
}


u64 sys_open(void) {
    char path[FAT32_MAX_PATH];
    int fd, omode;
    struct file* f;
    struct dirent* ep;

    if (argstr(0, path, FAT32_MAX_PATH) < 0 || argint(1, &omode) < 0)
        return -1;
    

    if (omode & O_CREATE) {
        ep = create(path, T_FILE, omode);
        if (ep == NULL) {
            return -1;
        }
    } else {
        if ((ep = ename(path)) == NULL) {
            return -1;
        }
        elock(ep);
        if ((ep->attribute & ATTR_DIRECTORY) && omode != O_RDONLY) {
            eunlock(ep);
            eput(ep);
            return -1;
        }
    }

    if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
        if (f) {
            fileclose(f);
        }
        eunlock(ep);
        eput(ep);
        return -1;
    }

    if (!(ep->attribute & ATTR_DIRECTORY) && (omode & O_TRUNC)) {
        etrunc(ep);
    }

    f->type = FD_ENTRY;
    f->off = (omode & O_APPEND) ? ep->file_size : 0;
    f->ep = ep;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    eunlock(ep);

    DEC_PRINT(fd);
    return fd;
}

u64 sys_mkdir(void) {
    char path[FAT32_MAX_PATH];
    struct dirent* ep;

    if (argstr(0, path, FAT32_MAX_PATH) < 0 ||
        (ep = create(path, T_DIR, 0)) == 0) {
        return -1;
    }
    eunlock(ep);
    eput(ep);
    return 0;
}

int sys_cwd(void) {
    u64 addr;
    int n;
    if (argaddr(0,  &addr) < 0 || argint(1, &n) < 0)
        return -1;
    return copyout(myproc()->pgdir, addr, myproc()->cwd->filename, n);
}

u64 sys_chdir(void) {
    char path[FAT32_MAX_PATH];
    struct dirent* ep;
    struct Process* p = myproc();

    if (argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = ename(path)) == NULL) {
        return -1;
    }
    elock(ep);
    if (!(ep->attribute & ATTR_DIRECTORY)) {
        eunlock(ep);
        eput(ep);
        return -1;
    }
    eunlock(ep);
    eput(p->cwd);
    p->cwd = ep;
    return 0;
}

u64 sys_pipe(void) {
    u64 fdarray;  // user pointer to array of two integers
    struct file *rf, *wf;
    int fd0, fd1;
    struct Process* p = myproc();

    if (argaddr(0, &fdarray) < 0)
        return -1;
    if (pipealloc(&rf, &wf) < 0)
        return -1;
    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
        if (fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    if (copyout(p->pgdir, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
        copyout(p->pgdir, fdarray + sizeof(fd0), (char*)&fd1, sizeof(fd1)) <
            0) {
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    return 0;
}

u64 sys_dev(void) {
    int fd, omode;
    int major;
    struct file* f;

    if (argint(0, &major) || argint(1, &omode) < 0) {
        panic("get parse error\n");
        return -1;
    }

    printf("test major %x\n", major);
    if (omode & O_CREATE) {
        panic("dev file on FAT");
    }

    if (major < 0 || major >= NDEV)
        return -1;

    if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        return -1;
    }

    f->type = FD_DEVICE;
    f->off = 0;
    f->ep = 0;
    f->major = major;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    return fd;
}

// To support ls command
u64 sys_readdir(void) {
    struct file* f;
    u64 p;

    if (argfd(0, 0, &f) < 0 || argaddr(1, &p) < 0)
        return -1;
    return dirnext(f, p);
}

// get absolute cwd string
/*
u64 sys_getcwd(void) {
    u64 addr;
    if (argaddr(0, &addr) < 0)
        return -1;

    struct dirent* de = myproc()->cwd;
    char path[FAT32_MAX_PATH];
    char* s;
    int len;

    if (de->parent == NULL) {
        s = "/";
    } else {
        s = path + FAT32_MAX_PATH - 1;
        *s = '\0';
        while (de->parent) {
            len = strlen(de->filename);
            s -= len;
            if (s <= path)  // can't reach root "/"
                return -1;
            strncpy(s, de->filename, len);
            *--s = '/';
            de = de->parent;
        }
    }

    // if (copyout(myproc()->pgdir, addr, s, strlen(s) + 1) < 0)
    if (copyout2(addr, s, strlen(s) + 1) < 0)
        return -1;

    return 0;
}
*/

// Is the directory dp empty except for "." and ".." ?
static int isdirempty(struct dirent* dp) {
    struct dirent ep;
    int count;
    int ret;
    ep.valid = 0;
    ret = enext(dp, &ep, 2 * 32, &count);  // skip the "." and ".."
    return ret == -1;
}

u64 sys_remove(void) {
    char path[FAT32_MAX_PATH];
    struct dirent* ep;
    int len;
    if ((len = argstr(0, path, FAT32_MAX_PATH)) <= 0)
        return -1;

    char* s = path + len - 1;
    while (s >= path && *s == '/') {
        s--;
    }
    if (s >= path && *s == '.' && (s == path || *--s == '/')) {
        return -1;
    }

    if ((ep = ename(path)) == NULL) {
        return -1;
    }
    elock(ep);
    if ((ep->attribute & ATTR_DIRECTORY) && !isdirempty(ep)) {
        eunlock(ep);
        eput(ep);
        return -1;
    }
    elock(ep->parent);  // Will this lead to deadlock?
    eremove(ep);
    eunlock(ep->parent);
    eunlock(ep);
    eput(ep);

    return 0;
}

// Must hold too many locks at a time! It's possible to raise a deadlock.
// Because this op takes some steps, we can't promise
u64 sys_rename(void) {
    char old[FAT32_MAX_PATH], new[FAT32_MAX_PATH];
    if (argstr(0, old, FAT32_MAX_PATH) < 0 ||
        argstr(1, new, FAT32_MAX_PATH) < 0) {
        return -1;
    }

    struct dirent *src = NULL, *dst = NULL, *pdst = NULL;
    int srclock = 0;
    char* name;
    if ((src = ename(old)) == NULL || (pdst = enameparent(new, old)) == NULL ||
        (name = formatname(old)) == NULL) {
        goto fail;  // src doesn't exist || dst parent doesn't exist || illegal
                    // new name
    }
    for (struct dirent* ep = pdst; ep != NULL; ep = ep->parent) {
        if (ep ==
            src) {  // In what universe can we move a directory into its child?
            goto fail;
        }
    }

    uint off;
    elock(src);  // must hold child's lock before acquiring parent's, because we
                 // do so in other similar cases
    srclock = 1;
    elock(pdst);
    dst = dirlookup(pdst, name, &off);
    if (dst != NULL) {
        eunlock(pdst);
        if (src == dst) {
            goto fail;
        } else if (src->attribute & dst->attribute & ATTR_DIRECTORY) {
            elock(dst);
            if (!isdirempty(dst)) {  // it's ok to overwrite an empty dir
                eunlock(dst);
                goto fail;
            }
            elock(pdst);
        } else {  // src is not a dir || dst exists and is not an dir
            goto fail;
        }
    }

    if (dst) {
        eremove(dst);
        eunlock(dst);
    }
    memmove(src->filename, name, FAT32_MAX_FILENAME);
    emake(pdst, src, off);
    if (src->parent != pdst) {
        eunlock(pdst);
        elock(src->parent);
    }
    eremove(src);
    eunlock(src->parent);
    struct dirent* psrc = src->parent;  // src must not be root, or it won't
                                        // pass the for-loop test
    src->parent = edup(pdst);
    src->off = off;
    src->valid = 1;
    eunlock(src);

    eput(psrc);
    if (dst) {
        eput(dst);
    }
    eput(pdst);
    eput(src);

    return 0;

fail:
    if (srclock)
        eunlock(src);
    if (dst)
        eput(dst);
    if (pdst)
        eput(pdst);
    if (src)
        eput(src);
    return -1;
}