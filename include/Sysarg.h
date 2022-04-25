#ifndef __SYSARG_H
#define __SYSARG_H

#include "Type.h"
int argint(int, int*);
int argstr(int, char*, int);
int argaddr(int, u64*);
int fetchstr(u64, char*, int);
int fetchaddr(u64, u64*);
void syscall();
int copyinstr(u64* pagetable, char* dst, u64 srcva, u64 max);

#endif
