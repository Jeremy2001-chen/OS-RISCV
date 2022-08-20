#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argvBusybox[] = {"./busybox", "sh", "busybox_testcode.sh", 0};
char *argvLua[] = {"./busybox", "sh", "lua_testcode.sh", 0};
char *argvLmbanch[] = {"./busybox", "sh", "lmbench_testcode.sh", 0};
char *shell[] = {"./home/busybox", "sh", 0};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    int pid = fork();
    // if (pid == 0) {
    //     exec("./busybox", argvBusybox);
    // } else {
    //     wait(0);
    // }

    // pid = fork();
    // if (pid == 0) {
    //     exec("./busybox", argvLua);
    // } else {
    //     wait(0);
    // }
    
    // pid = fork();
    // if (pid == 0) {
    //     exec("./bin/busybox", shell);
    // } else {
    //     wait(0);
    // }

    // pid = fork();
    if (pid == 0) {
        exec("./bin/busybox", shell);
    } else {
        wait(0);
    }

    exit(0);
}
