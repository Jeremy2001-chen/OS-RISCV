#ifndef __STAT_H
#define __STAT_H

#include "Type.h"

#define T_DIR 1     // Directory
#define T_FILE 2    // File
#define T_DEVICE 3  // Device
#define T_LINK 4
#define T_CHAR 5

#define STAT_MAX_NAME 32

// struct stat {
//     char name[STAT_MAX_NAME + 1];
//     int dev;      // File system's disk device
//     short type;   // Type of file
//     u64 size;  // Size of file in bytes
// };

struct stat {
    u64 st_dev;
    u64 st_ino;
    u32 st_mode;
    u32 st_nlink;
    u32 st_uid;
    u32 st_gid;
    u64 st_rdev;
    unsigned long __pad;
    long int st_size;
    u32 st_blksize;
    int __pad2;
    u64 st_blocks;
    long st_atime_sec;
    long st_atime_nsec;
    long st_mtime_sec;
    long st_mtime_nsec;
    long st_ctime_sec;
    long st_ctime_nsec;
    unsigned __unused[2];
    // char name[STAT_MAX_NAME + 1]; //[ADD] file name
    // short type;   // Type of file
};

#endif