#ifndef __FILE_H
#define __FILE_H

#include "Type.h"

#define O_RDONLY        0x000
#define O_WRONLY        0x001
#define O_RDWR          0x002
#define O_CREATE_GLIBC  0100
#define O_CREATE_GPP    0x200
#define O_APPEND        02000
#define O_TRUNC         01000

#define O_DIRECTORY 0x0200000

#define NDEV 4
#define NFILE 512 //Number of fd that all process can open

typedef struct Socket Socket;
typedef struct Dirent Dirent;
typedef struct File {
    enum { FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE, FD_SOCKET } type;
    int ref;  // reference count
    char readable;
    char writable;
    struct pipe* pipe;  // FD_PIPE
    Dirent* ep;
    Socket *socket;
    uint off;     // FD_ENTRY
    short major;  // FD_DEVICE
    Dirent* curChild;   // current child for getDirent
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
int fileread(struct File*, bool, u64, int n);
int filestat(struct File*, u64 addr);
int filewrite(struct File*, bool, u64, int n);

/* File types.  */
#define	DIR_TYPE	0040000	/* Directory.  */
#define	CHR_TYPE	0020000	/* Character device.  */
#define	BLK_TYPE	0060000	/* Block device.  */
#define	REG_TYPE	0100000	/* Regular file.  */
#define	FIFO_TYPE	0010000	/* FIFO.  */
#define	LNK_TYPE	0120000	/* Symbolic link.  */
#define	SOCK_TYPE	0140000	/* Socket.  */

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

u64 do_mmap(struct File* fd, u64 start, u64 len, int perm, int type, u64 off);

#endif