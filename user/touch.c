#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

int userMain(int argc, char** argv) {
    int i;

    if (argc < 2) {
        write(2, "Usage: touch files...\n", 23);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        int fd;
        if ((fd = open(argv[i], O_RDWR | O_CREATE)) < 0) {
            printf("touch: %s failed to create\n", argv[i]);
            break;
        }
        close(fd);
    }
    return 0;
}