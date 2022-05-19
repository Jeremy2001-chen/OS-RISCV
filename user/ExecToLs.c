#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *__argv[]={"ls.b", "/", "/", 0};

int userMain(int argc, char **argv) {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr
    printf("exec to ls.b\n");
    syscallExec("/sh.b", __argv);
    printf("exec error\n");
    return -1;
}
