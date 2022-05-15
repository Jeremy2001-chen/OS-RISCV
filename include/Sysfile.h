#ifndef _SYSFILE_H_
#define _SYSFILE_H_

//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <Driver.h>
#include <Process.h>
#include <Sysarg.h>
#include <fat.h>
#include <file.h>
#include <string.h>

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
int argfd(int n, int* pfd, struct file** pf);
// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
int fdalloc(struct file* f);

void syscallDup(void);
void syscallDupAndSet(void);
void syscallRead(void);
void syscallWrite(void);
void syscallClose(void);
void syscallGetFileState(void);
void syscallOpenAt(void);
void syscallMakeDirAt(void);
void syscallChangeDir(void);
void syscallGetWorkDir(void);
void syscallPipe(void);
void syscallDevice(void);
void syscallReadDir(void);
void syscallMount(void);
void syscallUmount(void);

u64 sys_remove(void);
// Must hold too many locks at a time! It's possible to raise a deadlock.
// Because this op takes some steps, we can't promise
u64 sys_rename(void);

#define AT_FDCWD -100

#endif