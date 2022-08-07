#include <Socket.h>
#include <Driver.h>
#include <Process.h>
#include <Sysfile.h>
#include <file.h>
#include <Page.h>
#include <string.h>
Socket sockets[SOCKET_COUNT];

inline u64 getSocketBufferBase(Socket *s) {
    return SOCKET_BUFFER_BASE + (((u64)(s - sockets)) << PAGE_SHIFT);
}

int socketAlloc(Socket **s) {
    for (int i = 0; i < SOCKET_COUNT; i++) {
        if (!sockets[i].used) {
            sockets[i].used = true;
            sockets[i].head = sockets[i].tail = 0;
            PhysicalPage *page;
            int r;
            if ((r = pageAlloc(&page)) < 0) {
                return r;
            }
            extern u64 kernelPageDirectory[];
            pageInsert(kernelPageDirectory, getSocketBufferBase(&sockets[i]), page2pa(page), PTE_READ | PTE_WRITE);
            sockets[i].process = myProcess();
            *s = &sockets[i];
            return 0;
        }
    }
    return -1;
}

void socketFree(Socket *s) {
    extern u64 kernelPageDirectory[];
    pageRemove(kernelPageDirectory, getSocketBufferBase(s));
    s->used = false;
}

int createSocket(int family, int type, int protocal) {
    printf("family %x type  %x protocal %x\n", family, type, protocal);
    // assert(family == 2);
    File *f = filealloc();
    Socket *s;
    if (socketAlloc(&s) < 0) {
        panic("");
    }
    s->addr.family = family;
    f->socket = s;
    f->type = FD_SOCKET;
    assert(f != NULL);
    return fdalloc(f);
}

int bindSocket(int fd, SocketAddr *sa) {
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    Socket *s = f->socket;
    assert(s->addr.family == sa->family);
    assert(sa->addr == 0 || (sa->addr >> 24) == 127);
    s->addr.addr = sa->addr;
    s->addr.port = sa->port;
    return 0;
}

int getSocketName(int fd, u64 va) {
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    copyout(myProcess()->pgdir, va, (char*)&f->socket->addr, sizeof(SocketAddr));
    return 0;
}

int sendTo(int fd, char *buf, u32 len, int flags, SocketAddr *dest) {
    for (int i = 0; i < SOCKET_COUNT; i++) {
        if (sockets[i].used && (sockets[i].addr.addr == dest->addr || sockets[i].addr.addr == 0)) {
            char *dst = (char*)(getSocketBufferBase(&sockets[i]) + (sockets[i].tail & (PAGE_SIZE - 1)));
            u32 num = MIN(PAGE_SIZE - (sockets[i].tail - sockets[i].head), len);
            int len1 = MIN(num, PAGE_SIZE - (sockets[i].tail & (PAGE_SIZE - 1)));
            strncpy(dst, buf, len1);
            if (len1 < num) {
                strncpy((char*)(getSocketBufferBase(&sockets[i])), buf + len1, num - len1);
            }
            sockets[i].tail += num;
            return num;
        }
    }
    return -1;
}

int receiveFrom(int fd, u64 buf, u32 len, int flags, u64 srcAddr) {
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    Socket *s = f->socket;
    char *src = (char*)(getSocketBufferBase(s) + (s->head & (PAGE_SIZE - 1)));
    u32 num = MIN(len, (s->tail - s->head));
    int len1 = MIN(num, PAGE_SIZE - (s->head & (PAGE_SIZE - 1)));
    copyout(myProcess()->pgdir, buf, src, len1);
    if (len1 < num) {
        copyout(myProcess()->pgdir, buf + len1, (char*)(getSocketBufferBase(s)), num - len1);
    }
    s->head += num;
    return num;
}
