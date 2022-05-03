#include <Syscall.h>
#include <SyscallLib.h>
#include <Printf.h>
#include <userfile.h>
#include <../include/debug.h>

char buf[2000];
int pipe1(char* s) {
    int fds[2], pid;// xstatus;
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
                printf("%s: pipe1 oops 1\n", s);
                return 1;
            }
        }
    } else if (pid > 0) {
        syscallClose(fds[1]);
        total = 0;
        cc = 1;
        while ((n = syscallRead(fds[0], buf, cc)) > 0) {
            for (i = 0; i < n; i++) {
                if ((buf[i] & 0xff) != (seq++ & 0xff)) {
                    printf("%s: pipe1 oops 2\n", s);
                    return 1;
                }
            }
            total += n;
            cc = cc * 2;
            if (cc > sizeof(buf))
                cc = sizeof(buf);
        }
        if (total != N * SZ) {
            printf("%s: pipe1 oops 3 total %d\n", total);
            return 1;
        }
        syscallClose(fds[0]);
        // wait(&xstatus);
        return 1;
    } else {
        printf("%s: fork() failed\n", s);
        return 1;
    }
    return 0;
}

int pipe2(char* s) {
    // for (int k = 0; k < 5; k++) {
    //     printf("[Turn]%lx\n", k);
    //     int fds[2];
    //     printf("%x\n", fds);
    //     // int seq, i, n, cc, total;
    //     enum { N = 5, SZ = 1033 };

    //     if (syscallPipe(fds) != 0) {
    //         printf("%s: syscallPipe() failed\n", s);
    //         return 1;
    //     }
    //     fork();
        // pid = fork();
        // seq = 0;
        // if (pid == 0) {
        //     syscallClose(fds[0]);
        //     // for (n = 0; n < N; n++) {
        //     //     for (i = 0; i < SZ; i++)
        //     //         buf[i] = seq++;
        //     //     int r;
        //     //     if ((r=syscallWrite(fds[1], buf, SZ)) != SZ) {
        //     //         printf("%s: pipe1 oops 1, total %d\n", s, r);
        //     //         return 1;
        //     //     }
        //     // }
        //     syscallClose(fds[1]);
        // } else if (pid > 0) {
        //     syscallClose(fds[1]);
        //     // total = 0;
        //     // cc = 1;
        //     // while ((n = syscallRead(fds[0], buf, cc)) > 0) {
        //     //     for (i = 0; i < n; i++) {
        //     //         if ((buf[i] & 0xff) != (seq++ & 0xff)) {
        //     //             printf("%s: pipe1 oops 2\n", s);
        //     //             return 1;
        //     //         }
        //     //     }
        //     //     total += n;
        //     //     cc = cc * 2;
        //     //     if (cc > sizeof(buf))
        //     //         cc = sizeof(buf);
        //     // }
        //     // if (total != N * SZ) {
        //     //     printf("%s: pipe1 oops 3 total %d\n", s, total);
        //     //     return 1;
        //     // }
        //     // syscallClose(fds[0]);
        //     // wait(&xstatus);
        // } else {
        //     printf("%s: fork() failed\n", s);
        //     return 1;
        // }
    // }
    return 0;
}

void userMain(){
    pipe1("[Test Pipe]");
}