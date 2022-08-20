# Too Low Too Simple Kernel

## 基本情况

RISC-V 内核，目前已经在 Sifive Unmatched 开发板上通过基础系统调用的测试。

在决赛第一阶段，我们的内核增加了信号、动态链接、信号量、内核级线程的支持，并通过了 Musl-Libc 上的测试。

在决赛第二阶段，我们完善了我们的内核，提高了内核的性能，并且移植了 redis 和 musl-gcc。

## 环境与配置

* 交叉编译器：riscv64-unknown-elf-gcc 11.1.0
* QEMU 模拟器：qemu-riscv64 5.0.0
* OpenSBI：[2022.04.11 3383d6a](https://github.com/riscv-software-src/opensbi/commit/3383d6a4d1461bb029b21fa53417382e34ae4906)

## 仓库架构

```c
OS-RISCV
    |- bootloader (QEMU 启动所需文件)
    |- docs (比赛文档)
    |- include (内核头文件源码)
    |- kernel (内核源码)
    |    |- boot (内核入口)
    |    |- driver (串口驱动及 SD 卡驱动)
    |    |- fs (文件系统相关代码)
    |    |- lock (锁相关代码)
    |    |- memory (内存管理相关)
    |    |- system (系统特性相关，包括信号量、信号、资源)
    |    |- trap (异常及加载)
    |    |- utility (辅助文件)
    |    |- yield (进程管理及线程管理)
    |- linkscript (链接脚本)
    |- root (磁盘文件格式)
    |    |- bin (可执行文件)
    |    |- etc (配置文件相关)
    |    |- home (测试相关)
    |    |- lib (动态库及静态库)
    |    |- usr (用户程序)
    |- user (用户文件及自行编写的 Shell)
    |- utility (辅助文件)
```

## 文档

### 总体情况

* [总体情况](docs/global.md)

### 基本功能

* [内存管理](docs/memory.md)
* [进程管理和线程管理](docs/process.md)
* [异常和系统调用](docs/trap.md)

### 外设

* [SD 卡驱动](docs/sd.md)
* [UART 驱动](docs/uart.md)

### 文件系统

* [文件系统介绍](docs/fat-design.md)
* [文件系统实现](docs/fat-impl.md)
* [文件系统相关系统调用](docs/fat-syscall.md)

### 多核

* [多核启动](docs/multicore.md)
* [睡眠锁](docs/sleeplock.md)
* [异步IO](docs/asynIO.md)

### 用户程序

* [用户程序和 Shell](docs/shell.md)

### 测试程序

* [测试程序](docs/test.md)

### 决赛第一阶段

* [动态链接](docs/dynamic.md)
* [信号量](docs/futex.md)
* [信号](docs/signal.md)

### 决赛第二阶段

* [懒加载](docs/lazy_load.md)
* [Socket](docs/socket.md)
* [Redis](docs/redis.md)
* [Musl-gcc](docs/gcc.md)

### 经验分享

* [经验分享](docs/experience.md)

## 编译内核和镜像

* 编译内核：`make` 指令会生成内核镜像
* 生成磁盘镜像：`make fat` 会将用户态打包生成用户态 FAT32 磁盘格式的镜像 `./fs.img` ，在 `./user/mnt` 目录下会保存所有的用户程序，可以拷贝到 SD 卡上进行测试
* 清空内核和镜像：`make clean`
* 查看磁盘镜像：`make mount` 将磁盘镜像 `fs.img` 挂载到 `/mnt` 上
* 解除查看：`make umount` 取消挂载
* 在 QEMU 上测试内核：`make run`，此命令在使用前需要先生成**磁盘镜像**