#include <Syscall.h>
#include <Printf.h>

int userMain(int argc, char **argv) {
    for (int i = 1; i <= 10000000; ++ i) {
        printf("This is process B\n");
        //syscallPutchar('b');
    }   
    printf("process finish\n");
    return 0;
}