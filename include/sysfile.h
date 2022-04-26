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

u64 sys_dup(void);

u64 sys_read(void);

u64 sys_write(void);

u64 sys_close(void);

u64 sys_fstat(void);
u64 sys_open(void);
u64 sys_mkdir(void);
u64 sys_chdir(void);

u64 sys_pipe(void);

// To support ls command
u64 sys_readdir(void);
u64 sys_remove(void);
// Must hold too many locks at a time! It's possible to raise a deadlock.
// Because this op takes some steps, we can't promise
u64 sys_rename(void);