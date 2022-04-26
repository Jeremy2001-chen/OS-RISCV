#ifndef __ULIB_H
#define __ULIB_H

#include "../../include/Type.h"
#include "../../include/debug.h"
#include <Syscall.h>
#include <userfile.h>

int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);


#endif