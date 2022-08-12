#ifndef _SOCKET_H_
#define _SOCKET_H_
#include "Type.h"
#include <Process.h>

#define SOCKET_COUNT 128

typedef struct Process Process;
typedef struct {
    u16 family;
    u16 port;
    u32 addr;
    char zero[24];
} SocketAddr;

typedef struct Socket {
    bool used;
    Process *process;
    SocketAddr addr; /* local addr */
    SocketAddr target_addr; /* remote addr */
    u64 head;
    u64 tail; // tail is equal or greater than head
    SocketAddr pending_queue[10];
    int pending_h, pending_t;
} Socket;

int createSocket(int domain, int type, int protocal);
int bindSocket(int fd, SocketAddr *sa);
int getSocketName(int fd, u64 va);
int sendTo(Socket *sock, char *buf, u32 len, int flags, SocketAddr *dest);
int receiveFrom(Socket *s, u64 buf, u32 len, int flags, u64 srcAddr);
void socketFree(Socket *s);
int accept(int sockfd, SocketAddr* addr);
int connect(int sockfd, const SocketAddr* addr);
int socket_read(Socket* sock, bool isUser, u64 addr, int n);
int socket_write(Socket* sock, bool isUser, u64 addr, int n);
#endif