#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>
#define MAXARG 32 

/**
 * len:    include the 0 in the end.
 * return: the number of bytes that read successfully (0 in the end is not included)
 */
int readline(int fd, char *buf, int len)
{
    char *p = buf;
    while (read(fd, p, 1) != 0 && p < buf + len) {
        if (*p == '\n') {
            if (p == buf) {     // ignore empty line
                continue;
            }
            *p = '\0';
            break;
        }
        p++;
    }
    if (p == buf) {
        return 0;
    }
    return p - buf;
}

int userMain(int argc, char **argv)
{
    if (argc < 2) {
        write(2, "Usage: xargs COMMAND [INITIAL-ARGS]...\n", 40);
        exit(-1);
    }
    char *argvs[MAXARG];
    char buf[128];
    int i;
    for (i = 1; i < argc; i++) {
        argvs[i - 1] = argv[i];         // argvs[0] = COMMAND
    }
    i--;
    if (readline(0, buf, 128) == 0) {   // if there is no input
        argvs[i] = 0;
        if (fork() == 0) {
            exec(argv[1], argvs);
            printf("xargs: exec %s fail\n", argv[1]);
            exit(0);
        }
        wait(0);
    } else {
        argvs[i] = buf;
        argvs[i + 1] = 0;
        do {
            if (fork() == 0) {
                exec(argv[1], argvs);
                printf("xargs: exec %s fail\n", argv[1]);
                exit(0);
            }
            wait(0);
        } while (readline(0, buf, 128) != 0);
    }
    return 0;
}