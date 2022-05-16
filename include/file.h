#ifndef __FILE_H
#define __FILE_H

#include "Type.h"


#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_APPEND  0x004
// #define O_CREATE  0x200
#define O_CREATE  0x40
#define O_TRUNC   0x400

#define O_DIRECTORY 0x0200000

#define NDEV 4
#define NFILE 64 //Number of fd that all process can open
typedef struct file {
    enum { FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE } type;
    int ref;  // reference count
    char readable;
    char writable;
    struct pipe* pipe;  // FD_PIPE
    struct dirent* ep;
    uint off;     // FD_ENTRY
    short major;  // FD_DEVICE
} file;

#define major(dev) ((dev) >> 16 & 0xFFFF)
#define minor(dev) ((dev)&0xFFFF)
#define mkdev(m, n) ((uint)((m) << 16 | (n)))

// map major device number to device functions.
struct devsw {
    int (*read)(int isUser, u64 dst, u64 start, u64 len);
    int (*write)(int isUser, u64 src, u64 start, u64 len);
};

extern struct devsw devsw[];

#define DEV_SD 0
#define DEV_CONSOLE 1

struct file* filealloc(void);
void fileclose(struct file*);
struct file* filedup(struct file*);
void fileinit(void);
int fileread(struct file*, u64, int n);
int filestat(struct file*, u64 addr);
int filewrite(struct file*, u64, int n);
int dirnext(struct file* f, u64 addr);

#endif