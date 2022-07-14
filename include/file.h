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

typedef struct Socket Socket;
typedef struct File {
    enum { FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE, FD_SOCKET } type;
    int ref;  // reference count
    char readable;
    char writable;
    struct pipe* pipe;  // FD_PIPE
    struct dirent* ep;
    Socket *socket;
    uint off;     // FD_ENTRY
    short major;  // FD_DEVICE
} File;

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

struct File* filealloc(void);
void fileclose(struct File*);
struct File* filedup(struct File*);
void fileinit(void);
int fileread(struct File*, u64, int n);
int filestat(struct File*, u64 addr);
int filewrite(struct File*, u64, int n);
int dirnext(struct File* f, u64 addr);

/* these are defined by POSIX and also present in glibc's dirent.h */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

#endif