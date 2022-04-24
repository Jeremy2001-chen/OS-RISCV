#include <Syscall.h>
#include <Printf.h>

void userMain() {
    for (int i = 1; i <= 10000000; ++ i) {
        printf("This is process A\n");
        //syscallPutchar('a');
    }
    printf("process finish\n");
}