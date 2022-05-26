# 测试程序

## 基本情况

为了验证系统调用正常工作，除了通过了初赛的测试样例，我们还编写了一部分程序进行测试。

## 测试程序

测试程序包括驱动测试、进程调度测试、管道测试、基础文件系统测试、虚拟文件系统测试、Fork 测试、进程编号测试程序。

同时我们编写了可以通过大赛测试点的程序。

### 驱动测试

这部分测试是在编写 SD 卡驱动时顺带一块完成的。

测试的方法如下面的代码所示：

```c
    for (int i = 0; i < 1024; i++) {
        binary[i] = i & 7;
        sdWrite(binary, j, 2);
        for (int i = 0; i < 1024; i++) {
            binary[i] = 0;
        }
        sdRead(binary, j, 2);
        for (int i = 0; i < 1024; i++) {
            if (binary[i] != (i & 7)) {
                panic("gg: %d ", j);
                break;
            }
        }
    }
```

方法比较简单，首先往磁盘块内写数据，之后再从中拿出数据。

如果得到的结果并不一样，会触发第27行的 panic，表明驱动存在问题。

这一部分测试程序放在了 `Sd.c` 文件内，并没有放在用户态进程，因为考虑到我们的驱动是放在内核态的，并不向用户暴露这个接口。

### 进程调度测试

这部分是测试时钟中断和系统调用是否正常响应是完成的。

测试代码放在了两个文件 `ProcessA.c` 和 `ProcessB.c`中，代码如下所示（上面为 A 文件，底下为 B 文件）：

```c
int userMain(int argc, char **argv) {
    for (int i = 1; i <= 10000000; ++ i) {
        syscallPutchar('a');
    }
    printf("process finish\n");
    return 0;
}
```

```c
int userMain(int argc, char **argv) {
    for (int i = 1; i <= 10000000; ++ i) {
        syscallPutchar('b');
    }
    printf("process finish\n");
    return 0;
}
```

将两个进程的优先级设置为一样。在开启时钟中断后会出现两个程序交替输出的 a 和 b 的情况，且连续的 a 和 b 数量是接近的，证明中断响应正确且系统调用流程基本走通。

该测试程序在**多核**下是可以正确执行的。

### 管道测试

这部分是在完成管道时编写的测试程序。

测试代码如下所示：

```c
int pipe2(char* s) {
    int fds[2], pid;// xstatus;
    int seq, i, n, cc, total, ret;
    enum { N = 5, SZ = 1033 };

    if (pipe(fds) != 0) {
        printf("%s: Pipe alloc failed\n", s);
        return 1;
    }
    pid = fork();
    seq = 0;
    if (pid == 0) {
        close(fds[0]);
        for (n = 0; n < N; n++) {
            for (i = 0; i < SZ; i++)
                buf[i] = seq++;
            int r;
            if ((r=write(fds[1], buf, SZ)) != SZ) {
                printf("%s: pipe1 oops 1\n", s);
                return 1;
            }
        }
        printf("child finish\n");
        return 0;
    } else if (pid > 0) {
        close(fds[1]);
        total = 0;
        cc = 1;
        while ((n = read(fds[0], buf, cc)) > 0) {
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
```

流程如下所示：

* 用户态通过系统调用 `syscallPipe` 申请管道
* 系统调用 `fork` 创建父子进程
* 子进程关闭读端，父进程关闭写端
* 子进程向管道中写数据，每次写的大写最多不超过1033
* 父进程开始从管道中读取数据，读取大小从1开始不断翻倍
* 子进程写完数据后直接返回，父进程在读完所有数据后也将返回

该测试程序放在了用户态下 `PipeTest.c` 文件中，很好的测试了管道的正确性。

### 基础文件系统测试

该部分是在完成文件系统时测试文件系统基础功能时编写的。

测试代码如下所示：

```c
int userMain(int argc, char **argv) {
    int fd = open("/testfile", O_RDONLY);
    printf("fd1=%d\n", fd);
    char buf[10]={0};
    int n = read(fd, buf, sizeof(buf));
    printf("[reading file %d bytes]: %s\n", n, buf);

    int fd2 = open("/createfile", O_WRONLY|O_CREATE);
    printf("fd2=%d\n", fd2);
    char to_write[]="testcreatefile";
    n = write(fd2, "testcreatefile", sizeof(to_write));
    printf("[writing file %d bytes]\n", n);

    close(fd2);

    fd2 = open("/createfile", O_RDONLY);
    char to_read[sizeof(to_write)+1];
    n =  read(fd2, to_read, sizeof(to_read));
    printf("[reading file %d bytes]: %s\n", n, to_read);
    for(;;);
}
```

流程如下所示：

* 打开路径 `/testfile` 下文件
* 从文件中读取数据向，输出读到的内容
* 在路径 `/createfile` 下创建文件
* 向该文件写数据，并关闭该文件
* 重新打开 `/createfile` ，输出读取到的数据

该测试程序放在了用户态下 `SysfileTest.c` 文件中，测试了文件系统的基本功能。

### 虚拟文件系统测试

该部分是在完成虚拟文件系统时编写的测试程序。

在测试前，需要将一个 Fat32 格式的磁盘镜像文件拷贝到 SD 卡上，该磁盘镜像中包含一个名为 `1.txt` 的文件，文件内容为"Mount Test Successful!"

测试代码如下所示：

```c
char *__argv[]={0};

int userMain(int argc, char** argv) {
    int rt, fd;

    rt = mount("fs.img", "/mnt", "vfat", 0, 0);
    assert(rt != -1);
    char buf[20]={0};
    fd = open("/mnt/1.txt", O_RDONLY);
    read(fd, buf, sizeof(buf));
    printf("%s\n", buf);
    return 0;
}
```

流程如下所示：

* 将磁盘镜像 `fs.img` 挂载到 SD 卡中 `/mnt` 目录下
* 打开该目录下的 `1.txt` 文件
* 输出文件内容，若为"Mount Test Successful!"证明虚拟文件系统成功实现

该测试程序放在了用户态下 `MountTest.c` 文件中，测试了虚拟文件系统的功能。

### Fork 测试

该部分是在完成 `Fork` 部分时编写的测试程序。

测试代码如下所示：

```c
int userMain(int argc, char **argv) {
    printf("start fork test....\n");
    int a = 0;
    int id = 0;
    if ((id = fork()) == 0) {
        if ((id = fork()) == 0) {
            a += 3;
            if ((id = fork()) == 0) {
                a += 4;
                for (;;) {
                    printf("   this is child3 :a:%d\n", a);
                }
            }
            for (;;) {
                printf("  this is child2 :a:%d\n", a);
            }
        }
        a += 2;
        for (;;) {
            printf(" this is child :a:%d\n", a);
        }
    }
    a++;
    for (;;) {
        printf("this is father: a:%d\n", a);
    }
}
```

该测试程序放在用户态下 `ForkTest.c` 文件中，测试了 fork 的写时复制是否正确。

### 进程编号测试程序

该部分是在补充系统调用时编写的测试程序，同时也作为 Fork 的强测。

测试代码如下所示：

```c
int userMain(int argc, char **argv) {
    int pid = getpid();
    printf("[ProcessTest] current pid is %lx\n", pid);
    for (int i = 1; i <= 10; i ++ ) {
        int r = fork();
        if (r > 0) {
            printf("[ProcessTest] Process %lx fork a child process %lx\n", getpid(), r);
        }
        printf("[ProcessTest] this pid is %lx, parent pid is %lx\n", getpid(), getppid());
    }
    return 0;
}
```

该测试程序放在用户态下 `ProcessIdTest.c` 文件中，测试了进程编号系统调用和 Fork 结合是否正确。

该程序在**多核**下也能正常工作，证明 Fork 基本没有并发问题。

## 大赛测试点程序

为了通过初赛阶段系统调用测试点，我们编写了下面的测试程序：

```c
char *argv[]={"ls", "arg1", "arg2", 0};
char *syscallList[] = {"getpid", "getppid", "dup", "exit", "yield", "pipe", "times", "gettimeofday", "sleep", "dup2", "getcwd", "open", "read", "write", "close", "execve", "chdir", "waitpid", "brk", "wait", "fork", "mkdir_", "openat", "fstat", "mmap", "munmap", "clone", "mount", "umount", "unlink", "getdents", "uname", "sh"};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    for (int i = 0; i < sizeof(syscallList) / sizeof(char*); i++) {
        int pid = fork();
        if (pid == 0) {
            exec(syscallList[i], argv);
        } else {
            wait(0);
        }
    }
    
    exit(0);
    uPanic("reach here\n");
}
```

首先初始化标准输入、标准输出和标准异常。

通过 Fork 方式来跑起所有的进程，子进程负责跑 SD 卡上进程，而父进程等待子进程结束后继续执行后面的子进程。

该测试程序放在用户态下 `SyscallTest.c` 文件中，通过了所有初赛测试点。