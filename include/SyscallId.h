#ifndef _SYSCALL_ID_H_
#define _SYSCALL_ID_H_

#define SYSCALL_PUTCHAR 4
#define SYSCALL_GET_PROCESS_ID 1
#define SYSCALL_PROCESS_DESTORY 3
#define SYSCALL_FORK 220
#define SYSCALL_PUT_STRING 5
#define SYSCALL_OPEN 6
#define SYSCALL_READ 7
#define SYSCALL_CLOSE 9
#define SYSCALL_READDIR 10
#define SYSCALL_FSTAT 11
//---------pipe-----------
#define SYSCALL_PIPE 12
#define SYSCALL_SBRK 13 // TODO , this id is wrong
#define SYSCALL_CHDIR 14// TODO , this id is wrong
#define SYSCALL_CWD 15 // TODO , this id is wrong

#define SYSCALL_DEV 20
#define SYSCALL_DUP 23

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#define SYSCALL_SCHED_YIELD 124

#define SYSCALL_GET_PID 172
#define SYSCALL_GET_PARENT_PID 173

#define SYSCALL_EXEC 221
#define SYSCALL_WAIT 260
#endif