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

void syscallDup(void) {
    Trapframe* tf = getHartTrapFrame();
    struct file* f;
    int fd = tf->a0;
    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if ((fd = fdalloc(f)) < 0) {
        tf->a0 = -1;
        return;
    }

    filedup(f);
    tf->a0 = fd;
}

void syscallDupAndSet(void) {
    Trapframe* tf = getHartTrapFrame();
    struct file* f;
    int fd = tf->a0, fdnew = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if (fdnew < 0 || fdnew >= NOFILE || myproc()->ofile[fdnew] != NULL) {
        tf->a0 = -1;
        return;
    }

    myproc()->ofile[fdnew] = f;
    filedup(f);
    tf->a0 = fdnew;
}

void syscallRead(void) {
    Trapframe* tf = getHartTrapFrame();
    struct file* f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if (len < 0) {
        tf->a0 = -1;
        return;
    }
    
    tf->a0 = fileread(f, uva, len);
}

void syscallWrite(void) {
    Trapframe* tf = getHartTrapFrame();
    struct file* f;
    int len = tf->a2, fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    if (len < 0) {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filewrite(f, uva, len);
}

void syscallClose(void) {
    Trapframe* tf = getHartTrapFrame();
    int fd = tf->a0;
    struct file* f;

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }
    
    myproc()->ofile[fd] = 0;
    fileclose(f);
    tf->a0 = 0;
}

void syscallGetFileState(void) {
    Trapframe* tf = getHartTrapFrame();
    struct file* f;
    int fd = tf->a0;
    u64 uva = tf->a1; 

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
        tf->a0 = -1;
        return;
    }

    tf->a0 = filestat(f, uva);
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
    bool absolutePath = (path[0] == '/');
    struct dirent* entryPoint, *startEntry;

    if (!absolutePath && startFd != AT_FDCWD) {
        startEntry = myproc()->cwd;
        myproc()->cwd = myproc()->ofile[startFd]->ep;
    }

    if (flags & O_CREATE) {
        entryPoint = create(path, T_FILE, mode);
        if (entryPoint == NULL) {
            goto bad;
        }
    } else {
        if ((entryPoint = ename(path)) == NULL) {
            goto bad;
        }
        elock(entryPoint);
        if (!(entryPoint->attribute & ATTR_DIRECTORY) && (flags & O_DIRECTORY)) {
            eunlock(entryPoint);
            eput(entryPoint);
            goto bad;
        }
        if ((entryPoint->attribute & ATTR_DIRECTORY) && (flags & 0xFFF) != O_RDONLY) { //todo
            eunlock(entryPoint);
            eput(entryPoint);
            goto bad;
        }
    }

    struct file* file;
    int fd;
    if ((file = filealloc()) == NULL || (fd = fdalloc(file)) < 0) {
        if (file) {
            fileclose(file);
        }
        eunlock(entryPoint);
        eput(entryPoint);
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
    if (!absolutePath && startFd != AT_FDCWD) {
        myproc()->cwd = startEntry;
    }

    tf->a0 = fd;
    return;
bad:  
    if (!absolutePath && startFd != AT_FDCWD) {
        myproc()->cwd = startEntry;
    }
    tf->a0 = -1;
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

    bool absolutePath = (path[0] == '/');
    struct dirent* entryPoint, *startEntry;

    if (!absolutePath && dirFd != AT_FDCWD) {
        startEntry = myproc()->cwd;
        myproc()->cwd = myproc()->ofile[dirFd]->ep;    
    }

    if ((entryPoint = create(path, T_DIR, mode)) == 0) {
        goto bad;
    }

    eunlock(entryPoint);
    eput(entryPoint);
    tf->a0 = 0;
    return;

bad:  
    if (!absolutePath && dirFd != AT_FDCWD) {
        myproc()->cwd = startEntry;
    }
    tf->a0 = -1;
}

void syscallChangeDir(void) {
    Trapframe* tf = getHartTrapFrame();
    char path[FAT32_MAX_PATH];
    struct dirent* ep;

    struct Process* p = myproc();
    
    if (fetchstr(tf->a0, path, FAT32_MAX_PATH) < 0 || (ep = ename(path)) == NULL) {
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
    eput(p->cwd);
    p->cwd = ep;
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

    if (copyout(myproc()->pgdir, uva, myproc()->cwd->filename, n) < 0) {
        tf->a0 = -1;
        return;
    } 
    
    tf->a0 = uva;
}

void syscallPipe(void) {
    Trapframe* tf = getHartTrapFrame();
    u64 fdarray = tf->a0;  // user pointer to array of two integers
    struct file *rf, *wf;
    int fd0, fd1;
    struct Process* p = myproc();

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
    struct file* f;

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
    struct file* f;
    int fd = tf->a0;
    u64 uva = tf->a1;

    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL) {
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
    if (fetchstr(imagePathUva, imagePath, FAT32_MAX_PATH) < 0 || (ep = ename(imagePath)) == NULL) {
        tf->a0 = -1;
        return;
    }
    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (dp = ename(mountPath)) == NULL) {
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

    struct file *file = filealloc();
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

    if (fetchstr(mountPathUva, mountPath, FAT32_MAX_PATH) < 0 || (ep = ename(mountPath)) == NULL) {
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