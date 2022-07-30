#include <LibMain.h>
#include "./include/SyscallLib.h"
#include "./include/Syscall.h"
#include "./include/Printf.h"

void libMain(int argc, char **argv, char **envs) {
    int ret = userMain(argc, argv);
    exit(ret);
}