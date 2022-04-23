#include "LibMain.h"
#include "Syscall.h"

void exit(void) {
    syscallProcessDestory(0);
}

void libMain() {
    userMain();
    exit();
}