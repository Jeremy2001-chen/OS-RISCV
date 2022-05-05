#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argv[]={"ls.b", "arg1", "arg2", 0};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr
    int pid = fork();
    if (pid == 0) {
        pid = fork();
        if (pid == 0)
            syscallExec("/getppid", argv);
        else
            syscallExec("/dup", argv);
    } else {
        syscallExec("/getpid", argv);
        exit(0);
    }
    // write(1, argv[0], strlen(argv[0]));
    // printf("[exec test]\n");
    // syscallExec("/ls.b", argv);
    // printf("exec error\n");
    return;//should not reach here;
}
