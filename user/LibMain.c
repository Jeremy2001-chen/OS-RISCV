#include "LibMain.h"
#include "./include/SyscallLib.h"
#include "./include/Syscall.h"
#include "./include/Printf.h"

void libMain() {
    userMain();
    exit(0);
}