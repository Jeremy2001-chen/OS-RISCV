# Too Low Too Simple Kernel

## 基本情况

RISC-V 内核，目前已经在 Sifive Unmatched 开发板上通过基础系统调用的测试。

## 环境与配置

* 交叉编译器：riscv64-unknown-elf-gcc 11.1.0
* QEMU 模拟器：qemu-riscv64 5.0.0
* OpenSBI：[2022.04.11 3383d6a](https://github.com/riscv-software-src/opensbi/commit/3383d6a4d1461bb029b21fa53417382e34ae4906)

## 文档

### 基本功能

* [内存管理](docs/mm.md)
* [进程管理](docs/schedule.md)
* [异常和系统调用](docs/trap.md)

### 外设

* [驱动](docs/driver.md)

### 文件系统

* [FAT32 介绍](docs/fat-design.md)
* [文件系统实现](docs/fat-impl.md)
* [文件系统相关系统调用](docs/fat-syscall.md)

### 多核

* [多核启动](docs/multicore.md)
* [睡眠锁](docs/sleeplock.md)

### 用户程序

* [用户程序和 Shell](docs/shell.md)

### 测试程序

* [测试程序](docs/test.md)

## 编译内核和镜像

* 编译内核：`make` 指令会生成内核镜像
* 生成磁盘镜像：`make fat` 会将用户态打包生成用户态 FAT32 磁盘格式的镜像 `./fs.img` ，在 `./user/mnt` 目录下会保存所有的用户程序，可以拷贝到 SD 卡上进行测试
* 清空内核和镜像：`make clean`
* 查看磁盘镜像：`make mount` 将磁盘镜像 `fs.img` 挂载到 `/mnt` 上
* 解除查看：`make umount` 取消挂载
* 在 QEMU 上测试内核：`make run`，此命令在使用前需要先生成**磁盘镜像**
