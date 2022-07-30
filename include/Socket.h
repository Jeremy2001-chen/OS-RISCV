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
    char zero[8];
} SocketAddr;

typedef struct Socket {
    bool used;
    Process *process;
    SocketAddr addr;
    u64 head;
    u64 tail; // tail is equal or greater than head
} Socket;

int createSocket(int domain, int type, int protocal);
int bindSocket(int fd, SocketAddr *sa);
int getSocketName(int fd, u64 va);
int sendTo(int fd, char *buf, u32 len, int flags, SocketAddr *dest);
int receiveFrom(int fd, u64 buf, u32 len, int flags, u64 srcAddr);
void socketFree(Socket *s);

#endif