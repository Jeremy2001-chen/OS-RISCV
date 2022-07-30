#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

void userMain() {
    // dev(1, O_RDWR); //stdin
    // dup(0); //stdout
    // dup(0); //stderr

    char s[] = "hello world";
    printf("s position: %lx %c\n", s, s[1]);
    int pid = fork(), wstatus;
    if (pid == 0) {
        printf("child: %s\n", s);
        exit(3);
    }
    printf("parent: %s\n", s);
    int r = waitpid(pid , (u64)&wstatus, 0);
    printf("s position: %lx %c\n", s, s[2]);
    printf("need: %s\n", s);
    printf("test : %x %x\n", r, wstatus);
}
