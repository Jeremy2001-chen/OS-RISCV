#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argv[]={"ls.b", "arg1", "arg2", 0};
char *syscallList[] = {"/getpid", "/getppid", "/dup", "/exit", "/yield", "/pipe", "/read"};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    for (int i = 0; i < 7; i++) {
        int pid = fork();
        if (pid == 0) {
            syscallExec(syscallList[i], argv);
        } else {
            wait(0);
        }
    }
    
    exit(0);
    uPanic("reach here\n");
}
