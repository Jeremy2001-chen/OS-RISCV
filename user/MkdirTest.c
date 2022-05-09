#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

char *__argv[]={0};

int userMain(int argc, char** argv) {
    int rt, fd;

    rt = mkdir("test_mkdir", 0666);
    printf("mkdir ret: %d\n", rt);
    assert(rt != -1);
    // syscallExec("/ls.b",__argv);
    fd = open("/test_mkdir", O_RDONLY /*| O_DIRECTORY*/);
    if (fd > 0) {
        printf("  mkdir success.\n");
        close(fd);
    } else{
        printf("fd=%d\n",fd);
        printf("  mkdir error.\n");
    }
    return 0;
}