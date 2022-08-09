# Redis

## 链接

```
http://download.redis.io/releases/redis-3.2.9.tar.gz
```



## 编译

```shell
cd ~/riscv64--musl--bleeding-edge-2020.08-1/riscv64-buildroot-linux-musl/sysroot/usr/include/linux
touch version.h
```



```shell
make CC="/home/zzy/riscv64--musl--bleeding-edge-2020.08-1/bin/riscv64-buildroot-linux-musl-gcc -U__linux__ -static" MALLOC=libc persist-settings 

make CC="/home/zzy/riscv64--musl--bleeding-edge-2020.08-1/bin/riscv64-buildroot-linux-musl-gcc -U__linux__ -static" MALLOC=libc

cp src/{redis-cli,redis-server} ../OS-RISCV/libc-test/
```



## 运行

```shell
redis-server &
redis-cli
```

## 输入

```
set a 114514
get a
```

