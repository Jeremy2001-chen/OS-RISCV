#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

int userMain(int argc, char** argv) {
    int i;

    if (argc < 2) {
        write(2, "Usage: rm files...\n", 20);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        if (unlink(argv[i]) < 0) {
            printf("rm: %s failed to delete\n", argv[i]);
            break;
        }
    }

    return 0;
}