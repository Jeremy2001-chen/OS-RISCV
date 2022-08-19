# Musl-gcc

## 基本情况

为了使我们的内核功能更加强大，我们尝试让内核支持 `Musl-gcc`。

## 可执行文件

我们首先尝试从 `riscv-musl` 仓库下直接编译生成 `Musl-gcc` 的编译器，但是不论是用本地交叉编译器编译，还是使用大赛提供的镜像文件中的 RISC-V 编译器编译得到的 `Musl-gcc` 均形如下面的内容：

```shell
#!/bin/sh
exec "${REALGCC:-riscv64-unknown-linux-gnu-gcc}" "$@" -specs "/home/xxx/riscv-musl/usr/local/musl/lib/musl-gcc.specs"
```

这本质是一个脚本，依赖于本地编译的 `gcc` ，通过导入配置文件 `specs` 实现 `Musl-gcc` 编译器。这在我们的内核中是无法使用的，因为在磁盘镜像上并不存在原生的 `gcc`，因此我们需要去找到真正的可执行文件去进行加载执行。

最终在 [Musl-GCC](https://more.musl.cc/x86_64-linux-muslx32/) 上找到了 `riscv-musl` 编译器的可执行文件，经过在大赛提供的镜像文件中进行测试我们发现 11.2.1 版本的编译器中链接器 `ld` 有错误，无法直接使用。而 10.2.1 版本的编译器可以正常使用，因此我们决定移植这一版本的 `Musl-gcc`。

## 编译准备

`gcc` 是一个驱动式程序，它调用其他程序来依次进行编译，汇编和链接。因此需要将对应的如 `as`、`collect2`、`ld` 等程序放在 `gcc` 能找到的位置，我们的文件按照下面的顺序组织的文件夹顺序：

```shell
--- usr
    |--- bin(GCC 相关可执行文件目录)
    |     |--- gcc
    |     |--- ld
    |     ...
    |--- lib(GCC 编译加载时的动态库和静态库)
    |     |--- libc.a
    |     |--- libgcc.a
    |     ...
    |--- libexec/gcc/riscv64-linux-musl/10.2.1（GCC 编译时需要的加载的程序）
    |     |--- as
    |     |--- cc1
    |     ...
    |--- usr/include（GCC 编译时所需头文件）
          |--- aio.h
          |--- alloca.h
          |--- ansidecl.h
          ...
```

按照上面的组织结构，我们的内核可以成功执行起 `Musl-gcc`。

## 内核调整

* 添加 `SYS_madvise` 系统调用
* 修复 `SYS_lseek` 系统调用，允许当前文件偏移大小超过文件大小。
* 修复 `SYS_mmap` 匿名内存空间映射问题，当映射到原有位置时需要清空数据。当映射大小不为页对齐时注意不能破坏后面的数据。
* 支持 PIE(position-independent executable) 文件的加载。

## 编译器测试

为了验证我们的编译器能正常工作，这里我们选择编写了两个比较有代表性的程序，分别通过静态链接和动态链接验证程序被正确编译和执行。

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

最后是汇编文件和 C 文件混合编译链接，测试程序如下所示:

```asm
    .text
    .globl main
main:
    jal logMain
    li a7, 93
    ecall
```

```c
#include <stdio.h>

int logMain() {
    printf("    ____________________          ___                            \n");
    printf("   |________    ________|        /  /                            \n");
    printf("            /  /                /  /                             \n");
    printf("           /  /                /  /                              \n");
    printf("          /  /                /  /                               \n");
    printf("         /  /___   ____      /  /        ______                __ \n");
    printf("        /  /    \\ /    \\    /  /        /    \\ \\      __      / /\n");
    printf("       /  /  __  |  __  |  /  /        |  __  \\ \\    /  \\    / /\n");
    printf("      /  /| (__) | (__) | /  /         | (__) |\\ \\  / /\\ \\  / /\n");
    printf("     /  / |      |      |/  /__________|      | \\ \\/ /  \\ \\/ /\n");
    printf("    /__/   \\____/ \\____/ \\______________\\____/   \\__/    \\__/\n");
    return 0;
}
```

经过编译可以得到输出队标的程序。