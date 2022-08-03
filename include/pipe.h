#ifndef __PIPE_H
#define __PIPE_H

#include <Spinlock.h>
#include <file.h>

#define PIPESIZE 512
struct pipe {
    struct Spinlock lock;
    char data[PIPESIZE];
    uint nread;     // number of bytes read
    uint nwrite;    // number of bytes written
    int readopen;   // read fd is still open
    int writeopen;  // write fd is still open
};

int pipealloc(struct File** f0, struct File** f1);
void pipeclose(struct pipe* pi, int writable);
int pipewrite(struct pipe* pi, bool isUser, u64 addr, int n);
int piperead(struct pipe* pi, bool isUser, u64 addr, int n);
#endif