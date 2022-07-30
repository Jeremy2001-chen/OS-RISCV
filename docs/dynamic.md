# 动态链接程序的加载

我们的OS可以同时支持运行静态链接的程序和动态链接的程序。

## 寻找动态链接器

我们会创建一个软链接 `/lib/ld-musl-riscv64-sf.so.1 -> /libc.so`，其中 `libc.so` 是所有动态链接的程序所依赖的动态链接器。

我们在 `exec` 函数中加载程序，首先需要根据该程序的所有 Segment 中是否含有 `INTERP` 段来确定该程序是静态链接还是动态链接。对于含有 `INTERP` 段的程序，`INTERP` 段的内容就是动态链接器的路径 ，我们就可以根据这个路径找到动态链接器对应的 `.so` 文件。

```
libc-test$ readelf -l entry-dynamic.exe

Elf 文件类型为 EXEC (可执行文件)
Entry point 0x2d834
There are 8 program headers, starting at offset 64

程序头：
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000010040 0x0000000000010040
                 0x00000000000001c0 0x00000000000001c0  R      0x8
  INTERP         0x0000000000000200 0x0000000000010200 0x0000000000010200
                 0x000000000000001d 0x000000000000001d  R      0x1
      [Requesting program interpreter: /lib/ld-musl-riscv64-sf.so.1]
  LOAD           0x0000000000000000 0x0000000000010000 0x0000000000010000
                 0x000000000004782c 0x000000000004782c  R E    0x1000
  LOAD           0x0000000000048000 0x0000000000059000 0x0000000000059000
                 0x0000000000004d90 0x000000000080e300  RW     0x1000
  DYNAMIC        0x000000000004a020 0x000000000005b020 0x000000000005b020
                 0x00000000000001f0 0x00000000000001f0  RW     0x8
  TLS            0x0000000000048000 0x0000000000059000 0x0000000000059000
                 0x0000000000001041 0x0000000000003041  R      0x1000
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000048000 0x0000000000059000 0x0000000000059000
                 0x0000000000003000 0x0000000000003000  R      0x1
```



## 加载动态链接器

本OS目前只能将动态链接器作为程序所依赖的共享对象加载，不能独立加载动态链接器，及不能支持诸如 `libc.so entry-dynamic.exe argv` 这种格式的命令。

计算 ldso 所占用的虚拟内存的大小 `total_size = 最高的虚拟地址 - 最低的虚拟地址`

加载时首先通过 `mmap` 申请 `total_size` 大小的匿名空间，并且 `mmap` 的返回值即为动态链接器应该加载到的基地址 `interp_load_addr`。

然后再将所有 `PT_LOAD` 的段通过 `mmap` 将虚拟地址映射到文件的内容，但是映射时虚拟地址需要加上刚才得到的 `interp_load_addr`。

```
| <-------------------------total_size-----------------------------> |
| <------Seg1------>     <-----Seg2------>   <---------Seg3--------> |
```



## 初始化进程栈（参数、环境变量、辅助数组）

接下来需要初始化进程的栈，栈的结构如下。自顶向下依次为参数个数、参数指针、环境变量指针、辅助数组、参数数据区、环境变量数据区。如下图所示

```
argc
argv[1]
argv[2]
NULL
envp[1]
envp[2]
NULL
AT_HWCAP ELF_HWCAP
AT_PAGESZ ELF_EXEC_PAGESIZE
... ...
NULL NULL
"argv1"
"argv2"
"env1=a"
"LD_LIBRARY_PATH=/"
```

其中辅助数组的信息如下所示。

| 项目      | 含义                                                         | 取值                                                         |
| --------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| AT_HWCAP  | RISC-V CPU的扩展功能信息                                     | 原则上应该从CPU中读取特定的寄存器，这里默认不启用任何CPU扩展，所以该位设为0 |
| AT_PAGESZ | 页面大小                                                     | 与虚拟内存的页面大小一致，设为PAGE_SIZE (4096)               |
| AT_PHDR   | 段表(Segment Table)在虚拟内存中的地址                        | 在加载**原**程序的时候获得，它位于**原**程序的某个段中。我们需要遍历所有的段，看一下哪个段在文件中的范围覆盖了段表，并计算段表在虚拟内存中的地址。 |
| AT_PHENT  | 每个段头的大小                                               | 32位与64位不同，我们采用64位的操作系统，所以是64位程序的程序头的大小 `sizeof(Phdr)` |
| AT_PHNUM  | 段的数量                                                     | 由程序头获得                                                 |
| AT_BASE   | 动态链接器被加载到的基地址                                   | 在加载时由操作系统的 `mmap` 决定，将 `mmap` 返回的地址传给动态链接器 |
| AT_ENTRY  | 原程序的入口                                                 | 由程序头获得                                                 |
| AT_SECURE | 是否启用安全模式。非安全模式下动态链接器会访问环境变量作为查找.so文件的路径，在安全模式下不会访问环境变量。 | 我们使用非安全模式启动该程序，并设置 `LD_LIBRARY_PATH` 这个环境变量为根目录 `LD_LIBRARY_PATH='/'` |
| AT_RANDOM | 16字节随机数的地址，作为libc随机数的种子                     | 将 `16byte` 的随机数存在栈上                                 |
| AT_EXECFN | 传递给动态连接器该程序的名称，用户态地址                     | `ustack[1]` ，即为 `argv[0]` 指向的位置，也就是程序的名称    |

```
NEW_AUX_ENT(AT_HWCAP, ELF_HWCAP); //CPU的extension信息，这里默认不添加任何CPU扩展，设为0
NEW_AUX_ENT(AT_PAGESZ, ELF_EXEC_PAGESIZE); //PAGE_SIZE
NEW_AUX_ENT(AT_PHDR, phdr_addr);// Phdr * phdr_addr; 指向用户态。
NEW_AUX_ENT(AT_PHENT, sizeof(Phdr)); //每个 Phdr 的大小
NEW_AUX_ENT(AT_PHNUM, elf.phnum); //phdr的数量
NEW_AUX_ENT(AT_BASE, interp_load_addr);
NEW_AUX_ENT(AT_ENTRY, elf.entry);//源程序的入口
NEW_AUX_ENT(AT_SECURE, 0);//我们使用非安全模式启动该程序，在该模式下会启用LD_LIBRARY_PATH这个环境变量。
NEW_AUX_ENT(AT_RANDOM, u_rand_bytes);//16byte随机数的地址。
NEW_AUX_ENT(AT_EXECFN, ustack[1]/*用户态地址*/); /* 传递给动态连接器该程序的名称 */
NEW_AUX_ENT(0, 0);
```

## 设置返回地址

若程序是静态链接而成，则 `elf_entry` 就是原程序的入口地址。

若程序是动态链接而成，则 `elf_entry` 是 `ldso 的入口地址 + interp_load_addr`，也就是说，程序的控制权首先会被交给动态链接器，动态链接器完成初始化后，才会将控制权交给原程序。

将 `epc` 设为 `elf_entry`，在从异常中返回时就可以跳转到静态库的入口或动态链接器的入口。
