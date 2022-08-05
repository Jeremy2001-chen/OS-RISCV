//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <fat.h>
#include <Process.h>
#include <Sysfile.h>
#include <Driver.h>
#include <string.h>
#include <Sysarg.h>
#include <file.h>
#include <Debug.h>
#include <string.h>
#include <Page.h>
#include <pipe.h>
#include <FileSystem.h>
#include <Iovec.h>
#include <Thread.h>
#include <Fcntl.h>
#include <Error.h>

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
int argfd(int n, int* pfd, struct File** pf) {
    int fd;
    struct File* f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        if (pfd)
            *pfd = -1;
        if (pf)
            *pf = NULL;
        return -1;
    }
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
int fdalloc(struct File* f) {
    int fd;
    struct Process* p = myProcess();
    
    for (fd = 0; fd < p->fileDescription.hard; fd++) {
        if (p->ofile[fd] == 0) {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

void syscallDup(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0;
    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if ((fd = fdalloc(f)) < 0) {
        tf->a0 = -EMFILE;
        return;
    }

    filedup(f);
    tf->a0 = fd;

   // printf("%s %d %d\n", __FILE__, __LINE__, fd);
}

void syscallDupAndSet(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f, *f2;
    int fd = tf->a0, fdnew = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if (fdnew < 0 || fdnew >= NOFILE) {
        tf->a0 = -1;
        return;
    }
    if ((f2 = myProcess()->ofile[fdnew]) != NULL) {
        fileclose(f2);
    }

    myProcess()->ofile[fdnew] = f;
    filedup(f);
    tf->a0 = fdnew;
}

void syscall_fcntl(void){
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0/*, cmd = tf->a1, flag = tf->a2*/;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    switch (tf->a1)
    {
    case FCNTL_GETFD:
        tf->a0 = 1;
        return;
    case FCNTL_SETFD:
        tf->a0 = 0;
        return;
    case FCNTL_GET_FILE_STATUS:
        tf->a0 = 04000;
        return;
    case FCNTL_DUPFD_CLOEXEC:
        fd = fdalloc(f);
        filedup(f);
        tf->a0 = fd;
        return;
    default:
        panic("%d\n", tf->a1);
        break;
    }

    // printf("syscall_fcntl fd:%x cmd:%x flag:%x\n", fd, cmd, flag);
    tf->a0 = 0;
    return;
}
void syscallRead(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        printf("%s %d\n", __FILE__, __LINE__);
        printf("fd: %d %lx\n", fd, f);
        tf->a0 = -1;
        return;
    }

    if (len < 0) {
        printf("%s %d\n", __FILE__, __LINE__);
        tf->a0 = -1;
        return;
    }

    tf->a0 = fileread(f, true, uva, len);
}

void syscallWrite(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if (len < 0) {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filewrite(f, true, uva, len);
}

void syscallWriteVector() {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        goto bad;
    }

    int cnt = tf->a2;
    if (cnt < 0 || cnt >= IOVMAX) {
        goto bad;
    }

    struct Iovec vec[IOVMAX];
    struct Process* p = myProcess();

    if (copyin(p->pgdir, (char*)vec, tf->a1, cnt * sizeof(struct Iovec)) != 0) {
        goto bad;
    }

    u64 len = 0;
    for (int i = 0; i < cnt; i++) {
        len += filewrite(f, true, (u64)vec[i].iovBase, vec[i].iovLen);
    }
    tf->a0 = len;
    return;

bad:
    tf->a0 = -1;
}

void syscallReadVector() {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        goto bad;
    }

    int cnt = tf->a2;
    if (cnt < 0 || cnt >= IOVMAX) {
        goto bad;
    }

    struct Iovec vec[IOVMAX];
    struct Process* p = myProcess();

    if (copyin(p->pgdir, (char*)vec, tf->a1, cnt * sizeof(struct Iovec)) != 0) {
        goto bad;
    }

    u64 len = 0;
    for (int i = 0; i < cnt; i++) {
        len += fileread(f, true, (u64)vec[i].iovBase, vec[i].iovLen);
    }
    tf->a0 = len;
    return;

bad:
    tf->a0 = -1;
}

void syscallClose(void) {
    Trapframe* tf = getHartTrapFrame();
    int fd = tf->a0;
    struct File* f;

    // if (fd == 0) {
    //     panic("");
    // }

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }
    
    myProcess()->ofile[fd] = 0;
    fileclose(f);
    tf->a0 = 0;
}

void syscallGetFileState(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0;
    u64 uva = tf->a1; 

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filestat(f, uva);
}

void syscallGetFileStateAt(void) {
    Trapframe* tf = getHartTrapFrame();
    int dirfd = tf->a0/*, flags = tf->a2*/;
    u64 uva = tf->a2; 
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }
    // printf("Path: %s\n", path);
    struct dirent* entryPoint = ename(dirfd, path);
    if (entryPoint == NULL) {
        tf->a0 = -1;
        return;
    }

    struct stat st;
    elock(entryPoint);
    estat(entryPoint, &st);
    eunlock(entryPoint);
    if (copyout(myProcess()->pgdir, uva, (char*)&st, sizeof(struct stat)) < 0) {
        tf->a0 = -1;
        return;
    }
    tf->a0 = 0;
}

extern struct entry_cache* ecache;
void syscallGetDirent() {
    struct File* f;
    int fd, n;
    u64 addr;

    if (argfd(0, &fd, &f) < 0 || argaddr(1, &addr) < 0 || argint(2, &n) < 0) {
        getHartTrapFrame()->a0 = -2;
        return;
    }
    struct Process* p = myProcess();
    static char buf[512];
    struct linux_dirent64* dir64 = (struct linux_dirent64*)buf;

    if (f->type == FD_ENTRY) {
        int count = 0;
        int ret;
        struct dirent de;
        int nread=0;
        elock(f->ep);
        for (;;) {
            de.valid = 0;
            while ((ret = enext(f->ep, &de, f->off, &count)) ==
                   0) {  // skip empty entry
                f->off += count * 32;
                de.valid = 0;
            }
            if (ret == -1)
                break;
            f->off += count * 32;

            int len = strlen(de.filename);
            int prefix = ((u64)dir64->d_name - (u64)dir64);

            if (n < prefix + len + 1) {
                eunlock(f->ep);
                getHartTrapFrame()->a0 = nread;
                return;
            }            

            dir64->d_ino = 0;
            dir64->d_off = 0;  // This maybe wrong;
            dir64->d_reclen = len + prefix + 1;
            dir64->d_type = (de.attribute & ATTR_DIRECTORY) ? DT_DIR : DT_REG;
            if (copyout(p->pgdir, addr, (char*)dir64, prefix) != 0) {
                getHartTrapFrame()->a0 = -4;
                return;
            }
            if (copyout(p->pgdir, addr + prefix, de.filename, len+1) != 0) {
                getHartTrapFrame()->a0 = -114;
                return;
            }
            addr += prefix + len + 1;
            nread += prefix + len + 1;
            n -= prefix + len + 1;
        }
        eunlock(f->ep);

        // elock(f->ep);
        // dir64->d_ino = f->ep - ecache->entries;
        // dir64->d_off = f->ep->off;
        // dir64->d_reclen = 0;  // What's this?
        // dir64->d_type = f->ep->attribute;
        // int len = strlen(f->ep->filename);
        // int prefix = ((u64)dir64->d_name - (u64)dir64);
        // if (len + prefix + 1 > n) {
        //     getHartTrapFrame()->a0 = -1;
        //     eunlock(f->ep);
        //     return; 
        // }
        // if (copyout(p->pgdir, addr, (char*)dir64, prefix) != 0) {
        //     getHartTrapFrame()->a0 = -4;
        //     eunlock(f->ep);
        //     return;
        // }
        // if (copyout(p->pgdir, addr + prefix, f->ep->filename, len) != 0) {
        //     getHartTrapFrame()->a0 = -114;
        //     eunlock(f->ep);
        //     return;
        // }
        getHartTrapFrame()->a0 = nread;
        return;
    }
    getHartTrapFrame()->a0 = -5;
    return;
}

//todo: support the mode
//todo: change the directory? whether we should add the ref(eput)
void syscallOpenAt(void) {
    Trapframe* tf = getHartTrapFrame();
    int startFd = tf->a0, flags = tf->a2, mode = tf->a3;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }

    struct dirent* entryPoint;
    printf("open path: %s\n", path);
    printf("startFd: %d, path: %s, flags: %x, mode: %x\n", startFd, path, flags, mode);
    if (flags & O_CREATE) {
        entryPoint = create(startFd, path, T_FILE, mode);
        if (entryPoint == NULL) {
            tf->a0 = -1;
            goto bad;
        }
    } else {
        if ((entryPoint = ename(startFd, path)) == NULL) {
            tf->a0 = -1;
            goto bad;
        }
        elock(entryPoint);
        if (!(entryPoint->attribute & ATTR_DIRECTORY) && (flags & O_DIRECTORY)) {
            eunlock(entryPoint);
            eput(entryPoint);
            tf->a0 = -1;
            goto bad;
        }
        if ((entryPoint->attribute & ATTR_DIRECTORY) && (flags & 0xFFF) != O_RDONLY) { //todo
            eunlock(entryPoint);
            eput(entryPoint);
            tf->a0 = -1;
            goto bad;
        }
    }
    struct File* file;
    int fd;
    if ((file = filealloc()) == NULL || (fd = fdalloc(file)) < 0) {
        if (file) {
            fileclose(file);
        }
        eunlock(entryPoint);
        eput(entryPoint);
        tf->a0 = -24;
        goto bad;
    }

    if (!(entryPoint->attribute & ATTR_DIRECTORY) && (flags & O_TRUNC)) {
        etrunc(entryPoint);
    }

    file->type = FD_ENTRY;
    file->off = (flags & O_APPEND) ? entryPoint->file_size : 0;
    file->ep = entryPoint;
    file->readable = !(flags & O_WRONLY);
    file->writable = (flags & O_WRONLY) || (flags & O_RDWR);

    eunlock(entryPoint);


    tf->a0 = fd;
    // printf("open at: %d\n", fd);
bad:
    return;
    // printf("open at: %d\n", tf->a0);
}

//todo: support the mode
//todo: change the directory? whether we should add the ref(eput)
void syscallMakeDirAt(void) {
    Trapframe* tf = getHartTrapFrame();
    int dirFd = tf->a0, mode = tf->a2;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }

    struct dirent* entryPoint;

    if ((entryPoint = create(dirFd, path, T_DIR, mode)) == 0) {
        goto bad;
    }

    eunlock(entryPoint);
    eput(entryPoint);
    tf->a0 = 0;
    return;

bad:
    tf->a0 = -1;
}

void syscallChangeDir(void) {
    Trapframe* tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    struct dirent* ep;

    struct Process* process = myProcess();
    
    if (fetchstr(tf->a0, path, FAT32_MAX_PATH) < 0 || (ep = ename(AT_FDCWD, path)) == NULL) {
        tf->a0 = -1;
        return;
    }

    elock(ep);
    if (!(ep->attribute & ATTR_DIRECTORY)) {
        eunlock(ep);
        eput(ep);
        tf->a0 = -1;
        return;
    }    

    eunlock(ep);
    eput(process->cwd);
    process->cwd = ep;
    tf->a0 = 0;
}

//todo: support alloc addr
void syscallGetWorkDir(void) {
    Trapframe* tf = getHartTrapFrame();
    u64 uva = tf->a0;
    int n = tf->a1;

    if (uva == 0) {
        panic("Alloc addr not implement for cwd\n");
    }

    int len = getAbsolutePath(myProcess()->cwd, 1, uva, n);

    if (len < 0) {
        tf->a0 = -1;
        return;
    }
    
    tf->a0 = uva;
}

void syscallPipe(void) {
    Trapframe* tf = getHartTrapFrame();
    u64 fdarray = tf->a0;  // user pointer to array of two integers
    struct File *rf, *wf;
    int fd0, fd1;
    struct Process* p = myProcess();

    if (pipealloc(&rf, &wf) < 0) {
        goto bad;
    }

    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
        if (fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        goto bad;
    }
    
    if (copyout(p->pgdir, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
        copyout(p->pgdir, fdarray + sizeof(fd0), (char*)&fd1, sizeof(fd1)) <
            0) {
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        goto bad;
    }
    
    tf->a0 = 0;
    return;

bad:
    tf->a0 = -1;
}

void syscallDevice(void) {
    Trapframe* tf = getHartTrapFrame();
    int fd, omode = tf->a1;
    int major = tf->a0;
    struct File* f;

    if (omode & O_CREATE) {
        panic("dev file on FAT");
    }

    if (major < 0 || major >= NDEV) {
        goto bad;
    }

    if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        goto bad;
    }

    f->type = FD_DEVICE;
    f->off = 0;
    f->ep = 0;
    f->major = major;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    tf->a0 = fd;
    return;

bad:
    tf->a0 = -1;
}

void syscallReadDir(void) {
    Trapframe* tf = getHartTrapFrame();
    struct File* f;
    int fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myProcess()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }
    
    tf->a0 = dirnext(f, uva);
}

// get absolute cwd string
/*
u64 sys_getcwd(void) {
    u64 addr;
    if (argaddr(0, &addr) < 0)
        return -1;

    struct dirent* de = myProcess()->cwd;
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

    // if (copyout(myProcess()->pgdir, addr, s, strlen(s) + 1) < 0)
    if (copyout2(addr, s, strlen(s) + 1) < 0)
        return -1;

    return 0;
}
*/

// Is the directory dp empty except for "." and ".." ?
static int isDirEmpty(struct dirent* dp) {
    struct dirent ep;
    int count;
    ep.valid = 0;
    return enext(dp, &ep, 2 * 32, &count) == -1; // skip the "." and ".."
}

/*
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
    if ((ep->attribute & ATTR_DIRECTORY) && !isDirEmpty(ep)) {
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
} */

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
    if ((src = ename(AT_FDCWD, old)) == NULL || (pdst = enameparent(AT_FDCWD, new, old)) == NULL ||
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
            if (!isDirEmpty(dst)) {  // it's ok to overwrite an empty dir
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

void syscallMount() {
    Trapframe *tf = getHartTrapFrame();
    u64 imagePathUva = tf->a0, mountPathUva = tf->a1, typeUva = tf->a2, dataUva = tf->a4;
    int flag = tf->a3;
    char imagePath[FAT32_MAX_FILENAME], mountPath[FAT32_MAX_FILENAME], type[10], data[10];
    if (fetchstr(typeUva, type, 10) < 0 || strncmp(type, "vfat", 4)) {
        tf->a0 = -1;
        return;
    }
    struct dirent *ep, *dp;
    if (fetchstr(imagePathUva, imagePath, FAT32_MAX_PATH) < 0 || (ep = ename(AT_FDCWD, imagePath)) == NULL) {
        tf->a0 = -1;
        return;
    }
    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (dp = ename(AT_FDCWD, mountPath)) == NULL) {
        tf->a0 = -1;
        return;
    }
    if (dataUva && fetchstr(dataUva, data, 10) < 0) {
        tf->a0 = -1;
        return;
    }
    assert(flag == 0);
    FileSystem *fs;
    if (fsAlloc(&fs) < 0) {
        tf->a0 = -1;
        return;
    }

    struct File *file = filealloc();
    file->off = 0;
    file->readable = true;
    file->writable = true;
    if (ep->head) {
        file->type = ep->head->image->type;
        file->ep = ep->head->image->ep;
    } else {
        file->type = FD_ENTRY;
        file->ep = ep;
    }
    fs->name[0] = 'm';
    fs->name[1] = 0;
    fs->image = file;
    fs->read = mountBlockRead;
    fatInit(fs);
    fs->next = dp->head;
    dp->head = fs;
    tf->a0 = 0;
}

void syscallUmount() {
    Trapframe *tf = getHartTrapFrame();
    u64 mountPathUva = tf->a0;
    int flag = tf->a1;
    char mountPath[FAT32_MAX_FILENAME];
    struct dirent *ep;

    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (ep = ename(AT_FDCWD, mountPath)) == NULL) {
        tf->a0 = -1;
        return;
    }

    assert(flag == 0);

    if (ep->head == NULL) {
        tf->a0 = -1;
        return;
    }

    extern DirentCache direntCache;
    acquireLock(&direntCache.lock);
    // bool canUmount = true;
    for(int i = 0; i < ENTRY_CACHE_NUM; i++) {
        struct dirent* entry = &direntCache.entries[i];
        if (entry->fileSystem == ep->head) {
            eput(entry);
        }
    }

    releaseLock(&direntCache.lock);

    ep->head->valid = 0;
    ep->head = ep->head->next;

    tf->a0 = 0;
}

int do_linkat(int oldDirFd, char* oldPath, int newDirFd, char* newPath) {
    struct dirent *entryPoint, *targetPoint = NULL;

    if ((entryPoint = ename(oldDirFd, oldPath)) == NULL) {
        goto bad;
    }

    if ((targetPoint = create(newDirFd, newPath, T_FILE, O_RDWR)) == NULL) {
        goto bad;
    }

    char buf[FAT32_MAX_PATH];
    if (getAbsolutePath(entryPoint, 0, (u64)buf, FAT32_MAX_PATH) < 0) {
        goto bad;
    }

    int len = strlen(buf);
    if (ewrite(targetPoint, 0, (u64)buf, 0, len + 1) != len + 1) {
        goto bad;
    }

    targetPoint->_nt_res = DT_LNK;
    eupdate(targetPoint);
    return 0;
bad:
    if (targetPoint) {
        eremove(targetPoint);
    }
    return -1;
}

void syscallLinkAt() {
    Trapframe* tf = getHartTrapFrame();
    int oldDirFd = tf->a0, newDirFd = tf->a2, flags = tf->a4;

    assert(flags == 0);
    char oldPath[FAT32_MAX_PATH], newPath[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, oldPath, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }
    if (fetchstr(tf->a3, newPath, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }

    tf->a0 = do_linkat(oldDirFd, oldPath, newDirFd, newPath);
}

void syscallUnlinkAt() {
    Trapframe *tf = getHartTrapFrame();
    int dirFd = tf->a0, flags = tf->a2;
    
    assert(flags == 0);
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }
    struct dirent* entryPoint;

    if((entryPoint = ename(dirFd, path)) == NULL) {
        goto bad;
    }

    entryPoint->_nt_res = 0;
    eremove(entryPoint);

    tf->a0 = 0;
    return;
bad:
    tf->a0 = -1;
}

void syscallLSeek() {
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0, mode = tf->a2;
    u64 offset = tf->a1;
    if (fd < 0 || fd >= NOFILE) {
        goto bad;
    }
    struct File* file = myProcess()->ofile[fd];
    if (file == 0) {
        goto bad;
    }
    u64 off = offset;
    switch (mode) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            off += file->off;
            break;
        case SEEK_END:
            off += file->ep->file_size;
            break;
        default:
            goto bad;
    }
    if (file->type != FD_ENTRY) {
        file->off = off;
    } else {
        file->off = (off >= file->ep->file_size ? file->ep->file_size : off);
    }
    tf->a0 = off;
    return;
bad:
    tf->a0 = -1;
}

void syscallPRead() {
    Trapframe *tf = getHartTrapFrame();
    int fd = tf->a0;
    struct File* file = myProcess()->ofile[fd];
    if (file == 0) {
        goto bad;
    }
    u32 off = file->off;
    tf->a0 = eread(file->ep, true, tf->a1, tf->a3, tf->a2);
    file->off = off;
    return;
bad:
    tf->a0 = -1;
}

void syscallUtimensat() {
    Trapframe *tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    int dirFd = tf->a0;
    struct dirent *de;
    if (tf->a1) {
        if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
            tf->a0 = -1;
            return;
        }
        if ((dirFd < 0 && dirFd != AT_FDCWD) || dirFd >= NOFILE) {
            tf->a0 = -EBADF;
            return;
        }
        if((de = ename(dirFd, path)) == NULL) {
            tf->a0 = -ENOTDIR;
            return;
        }
    } else {
        File *f;
        if (dirFd < 0 || dirFd >= NOFILE || (f = myProcess()->ofile[dirFd]) == NULL) {
            tf->a0 = -EBADF;
            return;
        }
        de = f->ep;
    }
    TimeSpec ts[2];
    copyin(myProcess()->pgdir, (char*)ts, tf->a2, sizeof(ts));
    eSetTime(de, ts);
    tf->a0 = 0;
}

void syscallSendFile() {
    Trapframe *tf = getHartTrapFrame();
    int outFd = tf->a0, inFd = tf->a1;
    if (outFd < 0 || outFd >= NOFILE) {
        goto bad;
    }
    if (inFd < 0 || inFd >= NOFILE) {
        goto bad;
    }
    struct File *outFile = myProcess()->ofile[outFd];
    struct File *inFile = myProcess()->ofile[inFd];
    if (outFile == NULL || inFile == NULL) {
        goto bad;
    }
    u32 offset;
    if (tf->a2) {
        copyin(myProcess()->pgdir, (char*) &offset, tf->a2, sizeof(u32));
        inFile->off = offset;
    }
    u8 buf[512];
    u32 count = tf->a3, size = 0;
    while (count > 0) {
        int len = MIN(count, 512);
        int r = fileread(inFile, false, (u64)buf, len);
        if (r > 0)
            r = filewrite(outFile, false, (u64)buf, r);
        size += r;
        if (r != len) {
            break;
        }
        count -= len; 
    }
    if (tf->a2) {
        copyout(myProcess()->pgdir, tf->a2, (char*) &inFile->off, sizeof(u32));
    }
    tf->a0 = size;
    return;
bad:
    tf->a0 = -1;
    return;
}

void syscallAccess() {
    Trapframe *tf = getHartTrapFrame();
    int dirfd = tf->a0;
    char path[FAT32_MAX_PATH];
    if (fetchstr(tf->a1, path, FAT32_MAX_PATH) < 0) {
        tf->a0 = -1;
        return;
    }
    struct dirent* entryPoint = ename(dirfd, path);
    if (entryPoint == NULL) {
        tf->a0 = -1;
        return;
    }
    tf->a0 = 0;
}

extern FileSystem rootFileSystem;
int getAbsolutePath(struct dirent* d, int isUser, u64 buf, int maxLen) {
    char path[FAT32_MAX_PATH];
    
    if (d->parent == NULL) {
        return either_copyout(isUser, buf, "/", 2);
    }
    char *s = path + FAT32_MAX_PATH - 1;
    *s = '\0';
    while (d->parent) {
        int len = strlen(d->filename);
        s -= len;
        if (s <= path || s - path <= FAT32_MAX_PATH - maxLen)  // can't reach root "/"
            return -1;
        strncpy(s, d->filename, len);
        *--s = '/';
        d = d->parent;
    }
    return either_copyout(isUser, buf, (void*)s, strlen(s) + 1);
}

