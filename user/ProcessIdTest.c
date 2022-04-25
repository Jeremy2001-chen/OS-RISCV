#include <Syscall.h>
#include <Printf.h>

void userMain() {
    //int pid = getpid();
    int pid = getpid();
    printf("[ProcessTest] current pid is %lx\n", pid);
    for (int i = 1; i <= 10; i ++ ) {
        int r = fork();
        if (r > 0) {
            printf("GG\n");
            //printf("[ProcessTest] Process %lx fork a child process %lx\n", getpid(), r);
        }

        printf("KK\n");
        printf("%d\n", i);

        //printf("[ProcessTest] this pid is %lx, parent pid is %lx\n", getpid(), getppid());
    }
    printf("hi\n");
}