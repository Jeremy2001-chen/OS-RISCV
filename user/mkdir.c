#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

int userMain(int argc, char** argv) {
    int i;

    if (argc < 2) {
        write(2, "Usage: mkdir files...\n", 23);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0777) < 0) {
            printf("mkdir: %s failed to create\n", argv[i]);
            break;
        }
    }
    return 0;
}