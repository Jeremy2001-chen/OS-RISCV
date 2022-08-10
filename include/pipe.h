#ifndef __PIPE_H
#define __PIPE_H

#include <Spinlock.h>
#include <file.h>
#include <Type.h>

#define PIPESIZE 65536
#define MAX_PIPE 128

struct pipe {
    struct Spinlock lock;
    char data[PIPESIZE];
    u64 nread;     // number of bytes read
    u64 nwrite;    // number of bytes written
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
};

int pipeNew(struct File** f0, struct File** f1);
void pipeClose(struct pipe* pi, int writable);
int pipeWrite(struct pipe* pi, bool isUser, u64 addr, int n);
int pipeRead(struct pipe* pi, bool isUser, u64 addr, int n);
#endif