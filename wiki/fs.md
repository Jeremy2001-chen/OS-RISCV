# 文件系统

How to run:

first:

```shell
make fat
```

这一步将制作一个fat32格式磁盘镜像，位于./fs.img。

这一步将造成256M的硬盘写入量。

我们建议只有在必须的时候才执行`make fat`操作，因为磁盘的写入会比较慢。

second:

```shell
make run
```

即可运行qemu模拟器