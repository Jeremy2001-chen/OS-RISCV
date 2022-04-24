#include <Syscall.h>
#include <Printf.h>

void userMain() {
    //int pid = getpid();
    int pid = getpid();
    printf("current pid is %lx\n", pid);
    for (int i = 1; i <= 3; i ++ ) {
        int r = fork();
        if (r > 0) {
            printf("Process %lx fork a child process %lx\n", getpid(), r);
        }
        printf("this pid is %lx, parent pid is %lx\n", getpid(), getppid());
    }
}