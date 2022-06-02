#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

int userMain(int argc, char** argv) {
    int i;

    for (i = 1; i < argc; i++) {
        write(1, argv[i], strlen(argv[i]));
        if (i + 1 < argc) {
            write(1, " ", 1);
        } else {
            write(1, "\n", 1);
        }
    }
    return 0;    
}