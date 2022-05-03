#include <Syscall.h>
#include <SyscallLib.h>
#include <Printf.h>
#include <userfile.h>
#include <../include/debug.h>

char buf[2000];
int pipe1(char* s) {
    int fds[2], pid;// xstatus;
    int seq, i, n, cc, total, ret;
    enum { N = 5, SZ = 1033 };

    if (syscallPipe(fds) != 0) {
        printf("%s: syscallPipe() failed\n", s);
        return 1;
    }
    pid = fork();
    seq = 0;
    if (pid == 0) {
        syscallClose(fds[0]);
        for (n = 0; n < N; n++) {
            for (i = 0; i < SZ; i++)
                buf[i] = seq++;
            int r;
            if ((r=syscallWrite(fds[1], buf, SZ)) != SZ) {
                printf("%s: pipe1 oops 1\n", s);
                return 1;
            }
        }
        printf("CHILD FINISH\n");
        return 0;
    } else if (pid > 0) {
        // printf("hello\n");
        syscallClose(fds[1]);
        total = 0;
        cc = 1;
        while ((n = syscallRead(fds[0], buf, cc)) > 0) {
            // printf("%x\n", n);

            for (i = 0; i < n; i++) {
                if ((buf[i] & 0xff) != (seq++ & 0xff)) {
                    printf("%s: pipe1 oops 2 %d\n", s, n);
                    return 1;
                }
            }
            total += n;
            cc = cc * 2;
            if (cc > sizeof(buf))
                cc = sizeof(buf);
            // printf("%x\n", n);
        }
        if (total != N * SZ) {
            printf("%s: pipe1 oops 3 total %d\n", total);
            return 1;
        }
        wait((u64)&ret);
        printf("ret value: %d\n", ret);
        return 0;
    } else {
        printf("%s: fork() failed\n", s);
        return 1;
    }
    return 0;
}

int pipe2(char* s) {
    for (int k = 0; k < 9; k++) {
        printf("[Turn]%lx\n", k);
        int fds[2], pid;
        int seq, i, n, cc, total;
        enum { N = 5, SZ = 1033 };

        if (syscallPipe(fds) != 0) {
            printf("%s: syscallPipe() failed\n", s);
            return 1;
        }
        pid = fork();
        seq = 0;
        if (pid == 0) {
            syscallClose(fds[0]);
            for (n = 0; n < N; n++) {
                for (i = 0; i < SZ; i++)
                    buf[i] = seq++;
                int r;
                if ((r=syscallWrite(fds[1], buf, SZ)) != SZ) {
                    printf("%s: pipe1 oops 1, total %d\n", s, r);
                    return 1;
                }
            }
            //syscallClose(fds[1]);
        } else if (pid > 0) {
            syscallClose(fds[1]);
            total = 0;
            cc = 1;
            // printf("Now: %x\n", (seq & 0xff));
            while ((n = syscallRead(fds[0], buf, cc)) > 0) {
                for (i = 0; i < n; i++) {
                    if ((buf[i] & 0xff) != (seq++ & 0xff)) {
                        printf("%s: pipe1 oops 2 %d\n", s, n);
                        return 1;
                    }
                }
                total += n;
                cc = cc * 2;
                if (cc > sizeof(buf))
                    cc = sizeof(buf);
            }
            if (total != N * SZ) {
                printf("%s: pipe1 oops 3 total %d\n", s, total);
                return 1;
            }
            //syscallClose(fds[0]);
            int ret;
            wait((u64)&ret);
            syscallClose(fds[0]);
            syscallClose(fds[1]);
            // wait(&xstatus);
        } else {
            printf("%s: fork() failed\n", s);
            return 1;
        }
    }
    return 0;
}

char test[2000];
int pipe3(char* s) {
    enum { N = 5, SZ = 1033 };

    for (int i = 1; i <= 2; i ++)
        fork();
    int fds[2], pid;

    if (syscallPipe(fds) != 0) {
        printf("%s: syscallPipe() failed\n", s);
        return 1;
    }
    pid = fork();
    printf("%x\n", pid);

    if (pid == 0) {
        syscallClose(fds[0]);
        // printf("hello\n");
        int r = syscallWrite(fds[1], test, 100);
        printf("[RET VALUE]%d\n", r);
    } else {
        syscallClose(fds[1]);
        // printf("reach?\n");
        wait(0);
    }
    return 0;
}

void userMain(){
    pipe2("[Test Pipe]");
}