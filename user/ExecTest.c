#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argv[]={"ls.b", "arg1", "arg2", 0};

void userMain() {
    printf("[exec test]\n");
    syscallExec("/ls.b", argv);
    printf("exec error\n");
    return;//should not reach here;
}
