#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    int pid = fork(), wstatus;
    if (pid == 0) {
        exit(3);
    }

    int r = waitpid(pid , (u64)&wstatus, 0);
    printf("test : %x %x\n", r, wstatus);
}
