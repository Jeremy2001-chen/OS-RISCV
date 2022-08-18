# 测试程序

## 基本情况

为了验证系统调用正常工作，除了通过了初赛的测试样例，我们还编写了一部分程序进行测试。

在决赛第一阶段，我们编写了测试程序通过所有 Musl-Libc Test 的测试点。

在决赛第二阶段，我们编写了测试程序通过所有 Busybox、Lua 和 Lmbench 的测试点，并且在内核上支持了使用 musl-gcc 编译的 redis 和 musl-gcc 本身，并为此编写了对应的测试程序。

## 测试程序

测试程序包括驱动测试、进程调度测试、管道测试、基础文件系统测试、虚拟文件系统测试、Fork 测试、进程编号测试程序、软链接测试程序。

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

如果得到的结果并不一样，会触发 panic 表明驱动存在问题。

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

该程序在**多核**下也能正常工作，证明 Fork 没有并发问题。

### 链接测试程序

本次大赛的测试点中并没有软链接的系统调用测试点，因此我们编写了相应测试程序。

```c
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
```

测试流程如下所示：

* 首先在当前工作目录（根目录）创建文件夹 `test`
* 切换到 `test` 目录内创建文件 `testfile`
* 回到根目录，创建软链接 `test` 链接到 `ret`
* 进入 `ret` 目录，尝试打开 `testfile` 文件

如果能正常打开说明链接正常实现。

该测试程序放在用户态下 `LinkTest.c` 文件中，验证了链接的正确实现。

## 初赛阶段测试程序

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

该测试程序放在目录 `user/SyscallTest.c`，通过了所有初赛测试点。

## 决赛第一阶段测试程序

为了通过决赛第一阶段 Musl-Libc 测试点，我们编写了下面的测试程序：

```c
#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *staticArgv[] = {"./runtest.exe", "-w", "entry-static.exe", "", 0};
char *staticList[] = {
    "pthread_cancel",
    "pthread_cond_smasher",
    "pthread_condattr_setclock", 
    "pthread_cancel_points", "pthread_tsd", "pthread_robust_detach",
    "pthread_cancel_sem_wait", "pthread_exit_cancel",
    "pthread_cond", "pthread_once_deadlock", "pthread_rwlock_ebusy",
    "putenv_doublefree", "search_hsearch", "basename", "clocale_mbfuncs", "clock_gettime",
    "crypt", "dirname", "env", "fdopen", "fnmatch",
    "fscanf", "fwscanf", "iconv_open", "inet_pton", 
    "mbc", "memstream", "qsort", "random", "argv",
    "search_insque", "search_lsearch", "search_tsearch",
    "setjmp", "snprintf", "socket", "sscanf", "sscanf_long",
    "stat", "strftime", "string", "string_memcpy", "string_memmem",
    "string_memset", "string_strchr", "string_strcspn", "string_strstr",
    "strptime", "strtod", "strtod_simple", "strtof", "strtol", "strtold",
    "swprintf", "tgmath", "time", "tls_align", "udiv", "ungetc", "utime",
    "wcsstr", "wcstol", "pleval", "daemon_failure", "dn_expand_empty",
    "dn_expand_ptr_0", "fflush_exit", "fgets_eof", "fgetwc_buffering",
    "fpclassify_invalid_ld80", "ftello_unflushed_append", "getpwnam_r_crash",
    "getpwnam_r_errno", "iconv_roundtrips", "inet_ntop_v4mapped",
    "inet_pton_empty_last_field", "iswspace_null", "lrand48_signextend",
    "lseek_large", "malloc_0", "mbsrtowcs_overflow", "memmem_oob_read",
    "memmem_oob", "mkdtemp_failure", "mkstemp_failure", "printf_1e9_oob",
    "printf_fmt_g_round", "printf_fmt_g_zeros", "printf_fmt_n", 
    "regex_backref_0", "regex_bracket_icase", "regex_ere_backref", 
    "regex_escaped_high_byte", "regex_negated_range", "regexec_nosub", 
    "rewind_clear_error", "rlimit_open_files", "scanf_bytes_consumed", 
    "scanf_match_literal_eof", "scanf_nullbyte_char", "setvbuf_unget", "sigprocmask_internal",
    "sscanf_eof", "statvfs", "strverscmp", "syscall_sign_extend", "uselocale_0",
    "wcsncpy_read_overflow", "wcsstr_false_negative"};

char *dynamicArgv[] = {"./runtest.exe", "-w", "entry-dynamic.exe", "", 0};
char *dynamicList[] = {
    "pthread_cancel",
    "pthread_cond_smasher",
    "pthread_condattr_setclock",
    "pthread_cancel_points", "pthread_tsd", "pthread_robust_detach",
    "pthread_cond", "pthread_exit_cancel",
    "pthread_once_deadlock", "pthread_rwlock_ebusy", "sem_init",
    "tls_init", "tls_local_exec", "tls_get_new_dtv",
    "putenv_doublefree",
    "argv", "basename", "clocale_mbfuncs", "clock_gettime", "crypt",
    "dirname", "dlopen", "env", "fdopen", "fnmatch", "fscanf",
    "fwscanf", "iconv_open", "inet_pton", "mbc", "memstream",
    "qsort", "random", "search_hsearch", "search_insque", "search_lsearch",
    "search_tsearch", "setjmp", "snprintf", "socket",
    "sscanf", "sscanf_long", "stat", "strftime", "string", "string_memcpy",
    "string_memmem", "string_memset", "string_strchr", "string_strcspn",
    "string_strstr", "strptime", "strtod", "strtod_simple", "strtof",
    "strtol", "strtold", "swprintf", "tgmath", "time", "udiv", 
    "ungetc", "utime", "wcsstr", "wcstol", "daemon_failure", "dn_expand_empty",
    "dn_expand_ptr_0", "fflush_exit", "fgets_eof", "fgetwc_buffering", "fpclassify_invalid_ld80",
    "ftello_unflushed_append", "getpwnam_r_crash", "getpwnam_r_errno", "iconv_roundtrips",
    "inet_ntop_v4mapped", "inet_pton_empty_last_field", "iswspace_null", "lrand48_signextend",
    "lseek_large", "malloc_0", "mbsrtowcs_overflow", "memmem_oob_read", "memmem_oob",
    "mkdtemp_failure", "mkstemp_failure", "printf_1e9_oob", "printf_fmt_g_round",
    "printf_fmt_g_zeros", "printf_fmt_n", "regex_backref_0", "regex_bracket_icase",
    "regex_ere_backref", "regex_escaped_high_byte", "regex_negated_range",
    "regexec_nosub", "rewind_clear_error", "rlimit_open_files", "scanf_bytes_consumed",
    "scanf_match_literal_eof", "scanf_nullbyte_char", "setvbuf_unget", "sigprocmask_internal",
    "sscanf_eof", "statvfs", "strverscmp", "syscall_sign_extend", "uselocale_0",
    "wcsncpy_read_overflow", "wcsstr_false_negative"};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    for (int i = 0; i < sizeof(staticList) / sizeof(char*); i++) {
        int pid = fork();
        if (pid == 0) {
            staticArgv[3] = staticList[i];
            exec("./runtest.exe", staticArgv);
        } else {
            wait(0);
        }
    }
    
    for (int i = 0; i < sizeof(dynamicList) / sizeof(char*); i++) {
        int pid = fork();
        if (pid == 0) {
            dynamicArgv[3] = dynamicList[i];
            exec("./runtest.exe", dynamicArgv);
        } else {
            wait(0);
        }
    }
    exit(0);
}
```

该程序首先测试所有静态链接的测试点，之后再继续测试动态链接测试点。

该测试文件放在 `user/MuslLibcTest.c`，通过了决赛第一阶段的所有测试点。

## 决赛第二阶段测试程序

在决赛第二阶段由于包含了 Busybox，因此我们直接使用 Busybox 的 Shell 解释器去执行对应的三个脚本，得以通过所有的测试点，测试程序如下所示：

```c
#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argvBusybox[] = {"./busybox", "sh", "busybox_testcode.sh", 0};
char *argvLua[] = {"./busybox", "sh", "lua_testcode.sh", 0};
char *argvLmbanch[] = {"./busybox", "sh", "lmbench_testcode.sh", 0};
char *shell[] = {"./busybox", "sh", 0};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    int pid = fork();
    if (pid == 0) {
        exec("./busybox", argvBusybox);
    } else {
        wait(0);
    }

    pid = fork();
    if (pid == 0) {
        exec("./busybox", argvLua);
    } else {
        wait(0);
    }
    
    pid = fork();
    if (pid == 0) {
        exec("./busybox", argvLmbanch);
    } else {
        wait(0);
    }

    exit(0);
}
```

由于 Busybox 解释器本身与 Bash 语法并不相同，因此有一些输出会有些不一样，但是评测结果并不影响。

该测试文件放在 `user/BusyBoxTest.c`，通过了决赛第二阶段的所有测试点。

## Redis 测试程序

我们将 Redis 服务端程序 `redis-serve`，Redis 客户端程序 `redis-cli` 和 Redis 测试集程序 `redis-benchmark` 放到了生成镜像的 `/root` 目录下，在生成镜像时将全部放至根目录下。

首先启动 BusyBox 的 `Shell`：

```c
#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char *argvBusybox[] = {"./busybox", "sh", "busybox_testcode.sh", 0};
char *argvLua[] = {"./busybox", "sh", "lua_testcode.sh", 0};
char *argvLmbanch[] = {"./busybox", "sh", "lmbench_testcode.sh", 0};
char *shell[] = {"./busybox", "sh", 0};

void userMain() {
    dev(1, O_RDWR); //stdin
    dup(0); //stdout
    dup(0); //stderr

    int pid = fork();
    if (pid == 0) {
        exec("./busybox", shell);
    } else {
        wait(0);
    }

    exit(0);
}
```

该测试程序依然使用了 `user/BusyBoxTest.c` 中的代码。

在 `Shell` 中首先启动 Redis 服务端进程：

```shell
/ # redis-server redis.conf &
```

该命令在**后台**执行 `redis-server` 进程，`redis.conf` 是预设好的 Redis 配置文件。

之后等待 Redis 服务进程初始化结束，启动 `redis-cli` 进程进行客户端交互：

```shell
/ # redis-cli
```

执行上述命令后会进入 Redis 的 Shell，之后执行最简单的 Set 和 Get 操作即可测试 Redis 是否正确加载和实现：

```shell
> 127.0.0.1: set x 1111
OK!
> 127.0.0.1: get x
"1111"
> 127.0.0.1: delete x
OK!
> 127.0.0.1: get x
(nil)
```

Redis 的 benchmark 测试程序执行下面的指令即可执行：

/todo

## GCC 测试程序

我们将编译的 GCC 放置在 `/usr/musl-gcc/bin` 下，编写了两个简单的测试程序进行编译的测试，分别测试静态编译和动态编译：

首先是 `a_plus_b` 测试：

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int a, b;
    FILE* file = fopen("a_plus_b.in", "r");
    if (file == NULL) {
        printf("No such file!\n");
        exit(0);
    }
    fscanf(file, "%d%d", &a, &b);
    printf("a+b=%d\n", a + b);
    fclose(file);
    return 0;
}
```

该测试程序将从文件 `a_plus_b.in` 中读入两个数，并将两个数之和输出到屏幕上。测试文件内容为：

```c
3 4
```

在控制台上编译该程序并进行静态链接：

```c
/ # /usr/musl-gcc/bin/gcc a_plus_b.c -static
/ # ./a.out
7
/ #
```

接下来是参数传递测试 `argv.c`:

```c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int a, b;
    FILE* file = fopen("argc.out", "w");
    if (file == NULL) {
        printf("No such file!\n");
        exit(0);
    }
    fprintf(file, "argc=%d\n", argc);
    for (int i = 0; i < argc; i++) {
        fprintf(file, "argv[%d]: %s\n", i, argv[i]);
    }
    fclose(file);
    return 0;
}
```

该测试程序将读入可执行文件的所有参数，并将这些参数输出到文件中：

在控制台上编译该程序并进行动态链接：

```c
/ # /usr/musl-gcc/bin/gcc argv.c
/ # ./a.out 1 2 3
argc=4
argv[0]: ./a.out
argv[1]: 1
argv[2]: 2
argv[3]: 3
/ #
```
