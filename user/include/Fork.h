#ifndef _USER_FORK_H_
#define _USER_FORK_H_

#include <Syscall.h>

inline long fork() {
    return syscallFork();
}

#endif