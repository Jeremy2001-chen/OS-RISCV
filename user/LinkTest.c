#include <Syscall.h>
#include <Printf.h>

int userMain(int argc, char **argv) {
    printf("start link test....\n");
    char buf[300];
    int fd;
    if (!cwd(buf, 100)) {
        printf("1 get current work directory error!\n");
        exit(1);
    }

    if (mkdir("test", 0777) < 0) {
        printf("2 make dir error\n");
        exit(1);
    }

    if (chdir("./test") < 0) {
        printf("3 change dir error\n");
        exit(1);
    }

    if ((fd = open("testfile", O_CREATE)) < 0) {
        printf("4 create testfile\n");
        exit(1);
    }

    if (close(fd) < 0) {
        printf("5 close file\n");
        exit(1);
    }

    if (chdir("..") < 0) {
        printf("6 change dir error\n");
        exit(1);
    }

    if (link("test", "ret") < 0) {
        printf("7 link error\n");
        exit(1);        
    }

    if (chdir("/ret") < 0) {
        printf("8 change dir error\n");
        exit(1);
    }

    if ((fd = open("testfile", O_RDWR) < 0)) {
        printf("9 open dir error\n");
        exit(1);
    }

    if (close(fd) < 0) {
        printf("10 close file\n");
        exit(1);
    }

    printf("finish link test....\n");
    return 0;
}
