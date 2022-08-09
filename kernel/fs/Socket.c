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
    printf("[%s] family %x type  %x protocal %x\n", __func__, family, type, protocal);
    // assert(family == 2);
    File *f = filealloc();
    Socket *s;
    if (socketAlloc(&s) < 0) {
        panic("");
    }
    s->addr.family = family;
    s->pending_h = s->pending_t = 0;
    f->socket = s;
    f->type = FD_SOCKET;
    f->readable = f->writable = true;
    assert(f != NULL);
    return fdalloc(f);
}

int bindSocket(int fd, SocketAddr *sa) {
    printf("[%s]  addr %lx port %lx \n",__func__, sa->addr, sa->port);
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

int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest) {
    buf[len] = 0;
    printf("[%s] %s\n", __func__, buf);
    for (int i = 0; i < SOCKET_COUNT; i++) {
        if (sockets[i].used && (/*sockets[i].addr.addr == dest->addr ||*/ sockets[i].addr.addr == 0)) {
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

int receiveFrom(Socket *s, u64 buf, u32 len, int flags, u64 srcAddr) {
    char *src = (char*)(getSocketBufferBase(s) + (s->head & (PAGE_SIZE - 1)));
    printf("[%s] %s\n",__func__, src);
    u32 num = MIN(len, (s->tail - s->head));
    int len1 = MIN(num, PAGE_SIZE - (s->head & (PAGE_SIZE - 1)));
    copyout(myProcess()->pgdir, buf, src, len1);
    if (len1 < num) {
        copyout(myProcess()->pgdir, buf + len1, (char*)(getSocketBufferBase(s)), num - len1);
    }
    s->head += num;
    return num;
}

int socket_read(Socket* sock, bool isUser, u64 addr, int n) {
    assert(isUser == 1);
    return receiveFrom(sock, addr, n, 0, 0);
}
int socket_write(Socket* sock, bool isUser, u64 addr, int n) {
    static char buf[128];
    either_copyin(buf, isUser, addr, n);
    return sendTo(0, buf, n, 0, &sock->target_addr);
}

static Socket* remote_find_socket(const SocketAddr* addr) {
    for (int i = 0; i < SOCKET_COUNT; ++i) {
        if (/*sockets[i].addr.family == addr->family &&*/
            sockets[i].addr.port == addr->port) {
            return &sockets[i];
        }
    }
    return NULL;
}

int accept(int sockfd, SocketAddr* addr) {
    printf("[%s]] addr %lx port %x\n", __func__, addr->addr, addr->port);
    File* f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);
    Socket* local_sock = f->socket;
    *addr = local_sock->pending_queue[local_sock->pending_h++];

    Socket* new_sock;
    socketAlloc(&new_sock);
    new_sock->addr = local_sock->addr;
    new_sock->target_addr = *addr;

    File* new_f = filealloc();
    assert(new_f != NULL);
    new_f->socket = local_sock;
    new_f->type = FD_SOCKET;
    new_f->readable = new_f->writable = true;

/* ----------- process Remote Host --------- */

    Socket * peer_sock = remote_find_socket(addr);
    peer_sock -> target_addr  = local_sock->addr;
    
/* ----------- process Remote Host --------- */

    return fdalloc(new_f);
}


static SocketAddr gen_local_socket_addr() {
    static int local_addr = (127 << 24) + 1;
    static int local_port = 10000;
    SocketAddr addr;
    addr.addr = local_addr;
    addr.port = local_port++;
    return addr;
}

/**
 * @brief The connect() system call connects the socket referred to by the
 * file descriptor sockfd to the address specified by addr. 
 * 
 * @param sockfd 
 * @param addr 
 * @return int      If the connection or binding succeeds, zero is returned.  
 * On error, -1 is returned, and errno is set to indicate the error.
 */
int connect(int sockfd, const SocketAddr* addr) {
    printf("[%s] fd %d addr %lx port %x\n", __func__, sockfd, addr->addr,
           addr->port);
    File* f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);

    Socket* local_sock = f->socket;
    local_sock->addr = gen_local_socket_addr();
    local_sock->target_addr = *addr;
 
/* ----------- process Remote Host --------- */

    // Socket* target_socket = remote_find_socket(addr);
    // if (target_socket == NULL) {
    //     printf("remote socket don't exists!");
    //     return -1;
    // }
    // target_socket->pending_queue[target_socket->pending_t++] = *local_sock->addr;

/* ----------- process Remote Host --------- */


    /* 这里应该通过 TCP
     * 与远程主机建立连接，但是这里默认与本机建立了连接，直接返回 */
    return 0;
}