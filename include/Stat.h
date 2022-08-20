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

// struct stat {

//     __mode_t st_mode;		/* File mode.  */
// #ifndef __USE_FILE_OFFSET64
//     __ino_t st_ino;		/* File serial number.  */
// #else
//     __ino64_t st_ino;		/* File serial number.	*/
// #endif
//     __dev_t st_dev;		/* Device containing the file.  */
//     __nlink_t st_nlink;		/* Link count.  */

//     __uid_t st_uid;		/* User ID of the file's owner.  */
//     __gid_t st_gid;		/* Group ID of the file's group.  */
// #ifndef __USE_FILE_OFFSET64
//     __off_t st_size;		/* Size of file, in bytes.  */
// #else
//     __off64_t st_size;		/* Size of file, in bytes.  */
// #endif

//     __time_t st_atime;		/* Time of last access.  */
//     __time_t st_mtime;		/* Time of last modification.  */
//     __time_t st_ctime;		/* Time of last status change.  */

//     /* This should be defined if there is a `st_blksize' member.  */
// #undef	_STATBUF_ST_BLKSIZE
//   };
#endif