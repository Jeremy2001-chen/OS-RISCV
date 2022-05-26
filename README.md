# Too Low Too Simple Kernel

## 基本情况

RISC-V 内核，目前已经在 Sifive Unmatched 开发板上通过基础系统调用的测试。

## 环境与配置

* 交叉编译器：riscv64-unknown-elf-gcc 11.1.0
* QEMU 模拟器：qemu-riscv64 5.0.0
* OpenSBI：`https://github.com/riscv-software-src/opensbi/commit/3383d6a4d1461bb029b21fa53417382e34ae4906`

## 项目文档

* [大赛第一阶段文档]()
* [内存管理](docs/mm.md)
* [进程管理]()
* [驱动](docs/driver.md)
* [文件系统]()
* [用户程序和 Shell](docs/shell.md)
* [测试程序](docs/test.md)