# Stage2 bugs

## mmap 与 brk 分配的内存的位置,

mmap 与 brk 所分配的内存不应该交融在一起。

## waitpid 可以根据 flags 允许不 block ，此时返回值 -1

## accept 可以设置为不 block

## select + read 实现读入

## /proc/self/exe 指向当前文件

## 保护了浮点数上下文

## gdb调试方法

## timespec timeval 单位不一样

## 页表权限管理更加完善，权限错误时发送 SEGV,完善 mprotect
    WRITE 默认给READ， EXE 默认有READ

## 内核栈需要开大2个页面

## 向 ld 传递环境变量
    "LD_LIBRARY_PATH=/"

## Kernal 版本太低 libc 会报错

## 用户态在 / 处调用mkdir -p 。

## 如何通过 SIG_KILL 杀掉内核态管道。

## 开 O2 后出现问题需要给函数注明 noreturn

## brk 返回值一定是新的 brk 地址

## clear_children_tid 在 exec 时一定要清零

## /dev/zero 和 /dev/null 可以直接读写，不需要经过磁盘

# glibc和g++的O_CREATE 宏不一样

# 心得

## reloc_cluster 特别慢

## 文件系统 inode 设计