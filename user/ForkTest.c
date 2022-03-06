#include <Syscall.h>
#include <Fork.h>
#include <Printf.h>

void userMain(void) {
    uPrintf("start fork test....\n");
    int a = 0;
    int id = 0;
    if ((id = fork()) == 0) {
        if ((id = fork()) == 0) {
            a += 3;
            if ((id = fork()) == 0) {
                a += 4;
                for (;;) {
                    uPrintf("   this is child3 :a:%d\n", a);
                }
            }
            for (;;) {
                uPrintf("  this is child2 :a:%d\n", a);
            }
        }
        a += 2;
        for (;;) {
            uPrintf(" this is child :a:%d\n", a);
        }
    }
    a++;
    for (;;) {
        uPrintf("this is father: a:%d\n", a);
    }
}
