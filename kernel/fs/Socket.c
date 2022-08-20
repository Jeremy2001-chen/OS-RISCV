#include <Socket.h>
#include <Driver.h>
#include <Process.h>
#include <Sysfile.h>
#include <File.h>
#include <Page.h>
#include <String.h>
#include <Hart.h>
#include <Thread.h>

Socket sockets[SOCKET_COUNT];

inline u64 getSocketBufferBase(Socket *s) {
    return SOCKET_BUFFER_BASE + (((u64)(s - sockets)) << PAGE_SHIFT);
}

int socketAlloc(Socket **s) {
    for (int i = 0; i < SOCKET_COUNT; i++) {
        if (!sockets[i].used) {
            sockets[i].used = true;
            sockets[i].head = sockets[i].tail = 0;
            sockets[i].listening = 0;
            sockets[i].pending_h = sockets[i].pending_t = 0;
            initLock(&sockets[i].lock, "socket lock");
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

/**
 * @brief 寻找远程主机（其实就是本机）上与 local_sock 建立连接的 socket
 */
 Socket* remote_find_peer_socket(const Socket* local_sock) {
    for (int i = 0; i < SOCKET_COUNT; ++i) {
        if (sockets[i].used &&
            sockets[i].addr.family == local_sock->target_addr.family &&
            sockets[i].addr.port == local_sock->target_addr.port &&
            sockets[i].target_addr.port == local_sock->addr.port) {
            return &sockets[i];
        }
    }
    return NULL;
}

void socketFree(Socket *s) {
    extern u64 kernelPageDirectory[];
    pageRemove(kernelPageDirectory, getSocketBufferBase(s));
    s->used = false;
}

int createSocket(int family, int type, int protocal) {
    // printf("[%s] family %x type  %x protocal %x\n", __func__, family, type, protocal);
    // if (family != 2)
    //     return -1;  // ANET_ERR
    // assert(family == 2);
    File *f = filealloc();
    Socket *s;
    if (socketAlloc(&s) < 0) {
        panic("");
    }
    s->addr.family = family;
    s->pending_h = s->pending_t = 0;
    s->listening = 0;
    
    f->socket = s;
    f->type = FD_SOCKET;
    f->readable = f->writable = true;
    assert(f != NULL);
    return fdalloc(f);
}

int bindSocket(int fd, SocketAddr *sa) {
    // printf("[%s]  addr 0x%lx port 0x%lx \n",__func__, sa->addr, sa->port);
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    Socket *s = f->socket;
    assert(s->addr.family == sa->family);
    // assert(sa->addr == 0 || (sa->addr >> 24) == 127);
    s->addr.addr = sa->addr;
    s->addr.port = sa->port;
    return 0;
}

int listen(int sockfd) {
    File* f = myProcess()->ofile[sockfd];
    if (f == NULL)
        panic("");
    assert(f->type == FD_SOCKET);
    Socket* sock = f->socket;
    if (!sock)
        assert(0);
    sock->listening = 1;
    return 0;
}

int getSocketName(int fd, u64 va) {
    File *f = myProcess()->ofile[fd];
    assert(f->type == FD_SOCKET);
    copyout(myProcess()->pgdir, va, (char*)&f->socket->addr, sizeof(SocketAddr));
    return 0;
}

// 有修改，这个接口暂时无法通过 libc-test
int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest) {
    buf[len] = 0;

    int i = remote_find_peer_socket(sock) - sockets;
    // printf("[%s] sockid %d addr 0x%x port 0x%x data %s\n",   __func__, i , dest->addr, dest->port, buf);
    char *dst = (char*)(getSocketBufferBase(&sockets[i]) + (sockets[i].tail & (PAGE_SIZE - 1)));
    u32 num = MIN(PAGE_SIZE - (sockets[i].tail - sockets[i].head), len);
    if (num == 0) {
        getHartTrapFrame()->epc -= 4;
        yield();
    }
    int len1 = MIN(num, PAGE_SIZE - (sockets[i].tail & (PAGE_SIZE - 1)));
    strncpy(dst, buf, len1);
    if (len1 < num) {
        strncpy((char*)(getSocketBufferBase(&sockets[i])), buf + len1, num - len1);
    }
    sockets[i].tail += num;
    return num;
}

int receiveFrom(Socket* s, u64 buf, u32 len, int flags, u64 srcAddr) {
    char* src = (char*)(getSocketBufferBase(s) + (s->head & (PAGE_SIZE - 1)));
    // printf("[%s] sockid %d data %s\n", __func__, s - sockets, src);
    u32 num = MIN(len, (s->tail - s->head));
    if (num == 0) {
        getHartTrapFrame()->epc -= 4;
        yield();
    }
    int len1 = MIN(num, PAGE_SIZE - (s->head & (PAGE_SIZE - 1)));
    copyout(myProcess()->pgdir, buf, src, len1);
    if (len1 < num) {
        copyout(myProcess()->pgdir, buf + len1, (char*)(getSocketBufferBase(s)),
                num - len1);
    }
    s->head += num;
    // printf("[%s] sockid %d num %d\n", __func__, s - sockets, num);
    return num;
}

int socket_read(Socket* sock, bool isUser, u64 addr, int n) {
    assert(isUser == 1);
    return receiveFrom(sock, addr, n, 0, 0);
}
int socket_write(Socket* sock, bool isUser, u64 addr, int n) {
    static char buf[PAGE_SIZE * 10];
    either_copyin(buf, isUser, addr, n);
    return sendTo(sock, buf, n, 0, &sock->target_addr);
}

/**
 * @brief 寻找远程主机（其实就是本机）上的一个 Socket，该 Socket 已经被 bind() 绑定了一个 addr，
 * 并且该 addr 和参数中的 addr 相同。
 * 
 * @param addr      target socket addr on remote host.
 * @return Socket*  Pointer to Socket who has been listening a addr which equals to @param addr
 *                  NULL if there are no such socket
 */
static Socket* remote_find_listening_socket(const SocketAddr* addr) {
    for (int i = 0; i < SOCKET_COUNT; ++i) {
        if (sockets[i].used && sockets[i].addr.family == addr->family &&
            sockets[i].addr.port == addr->port && sockets[i].listening) {
            return &sockets[i];
        }
    }
    return NULL;
}


/**
 * @brief The accept() system call is used with connection-based socket
 * types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first
 * connection request on the queue of pending connections for the
 * listening socket, sockfd, creates a new connected socket, and
 * returns a new file descriptor referring to that socket.  The
 * newly created socket is not in the listening state.  The original
 * socket sockfd is unaffected by this call.
 * 
 * @param sockfd The argument sockfd is a socket that has been created with
 * socket, bound to a local address with bind, and is
 * listening for connections after a listen. 
 * @param addr The argument addr is a pointer to a sockaddr structure.  This
 * structure is filled in with the address of the peer socket, as
 * known to the communications layer. 
 * @return int 
 */
int accept(int sockfd, SocketAddr* addr) {
    // printf("[%s] fd %d\n", __func__, sockfd);
    File* f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);
    Socket* local_sock = f->socket;
    if (local_sock->pending_h == local_sock->pending_t) {
        // acquireLock(&local_sock->lock);
        // printf("[%s] sleeping sid %d\n",__func__, local_sock-sockets);
        // sleep(local_sock, &local_sock->lock);
        // printf("[%s] waked up \n",__func__);
        // releaseLock(&local_sock->lock);
        return -11; //EAGAIN /* Try again */
    }

    *addr =
        local_sock->pending_queue[(local_sock->pending_h++) % PENDING_COUNT];

    Socket* new_sock;
    socketAlloc(&new_sock);
    new_sock->addr = local_sock->addr;
    new_sock->target_addr = *addr;

    File* new_f = filealloc();
    assert(new_f != NULL);
    new_f->socket = new_sock;
    new_f->type = FD_SOCKET;
    new_f->readable = new_f->writable = true;

/* ----------- process on Remote Host --------- */

    Socket * peer_sock = remote_find_peer_socket(new_sock);
    wakeup(peer_sock);
    // printf("[%s] wake up client\n", __func__);

/* ----------- process on Remote Host --------- */

    return fdalloc(new_f);
}

/**
 * @brief 生成一个本地的socket Address，及本地 ip 地址 + 新分配的 port.
 */
static SocketAddr gen_local_socket_addr() {
    static int local_addr = (127 << 24) + 1;
    static int local_port = 10000;
    SocketAddr addr;
    addr.family = 2;
    addr.addr = local_addr;
    addr.port = local_port++;
    return addr;
}


/**
 * @brief The connect() system call connects the socket referred to by the
 * file descriptor sockfd to the address specified by addr. 
 * 
 * @param sockfd    local socket fd.
 * @param addr      target socket addr on remote host.
 * @return int      0 if success; -1 if remote socket does not exists!.
 */
int connect(int sockfd, const SocketAddr* addr) {
    // printf("[%s] fd %d addr %lx port %x\n", __func__, sockfd, addr->addr,
        //    addr->port);
    File* f = myProcess()->ofile[sockfd];
    assert(f->type == FD_SOCKET);

    Socket* local_sock = f->socket;
    local_sock->addr = gen_local_socket_addr();
    local_sock->target_addr = *addr;
 
/* ----------- process on Remote Host --------- */
    Socket* target_socket = remote_find_listening_socket(addr);
    if (target_socket == NULL) {
        printf("remote socket don't exists!");
        return -1;
    }
    if (target_socket->pending_t - target_socket->pending_h == PENDING_COUNT) {
        return -1;  // Connect Count Reach Limit.
    }
    target_socket->pending_queue[(target_socket->pending_t++) % PENDING_COUNT] =
        local_sock->addr;
    // printf("[%s] wakeup server sid %d\n", __func__, target_socket-sockets);
    // wakeup(target_socket);

    // Wait for server to accept the connection request.
    acquireLock(&local_sock->lock);
    // printf("[%s] sleeping \n",__func__);
    sleep(local_sock, &local_sock->lock);
    // printf("[%s] waked up \n",__func__);
    releaseLock(&local_sock->lock);

/* ----------- process on Remote Host --------- */

    return 0;
}