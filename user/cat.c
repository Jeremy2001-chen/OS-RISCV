#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

char buf[512];

void cat(int fd) {
    int n;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            write(2, "cat: write error\n", 18);
            exit(1);
        }
    }
    if (n < 0) {
        write(2, "cat: read error\n", 17);
        exit(1);
    }
}

int userMain(int argc, char** argv) {
    int fd, i;

    if (argc <= 1) {
        cat(0);
        exit(0);
    }

    for (i = 1; i < argc; i++) {
        if ((fd = open(argv[i], 0)) < 0) {
            printf("cat: cannot open %s\n", argv[i]);
            exit(1);
        }
        cat(fd);
        close(fd);
    }
    return 0;
}