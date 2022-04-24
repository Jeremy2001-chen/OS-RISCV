#include "LibMain.h"
#include "./include/SyscallLib.h"
#include "./include/Syscall.h"

void exit(void) {
    processDestory(0);
}

void libMain() {
    userMain();
    exit();
}