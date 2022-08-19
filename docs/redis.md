# Redis

## 下载链接

```
http://download.redis.io/releases/redis-3.2.9.tar.gz
```

## 编译

```shell
make CC="/home/zzy/riscv64--musl--bleeding-edge-2020.08-1/bin/riscv64-buildroot-linux-musl-gcc -U__linux__ -static" MALLOC=libc persist-settings # 编译依赖库

make CC="/home/zzy/riscv64--musl--bleeding-edge-2020.08-1/bin/riscv64-buildroot-linux-musl-gcc -U__linux__ -static" MALLOC=libc # 编译 redis

cp src/{redis-cli,redis-server} redis.conf ../OS-RISCV/libc-test/
```
Note: 
- 如果 ./dep/文件夹有修改（或编译到其它指令集），一定要重新 make 一遍 persist-settings，否则 persist-settings 不会更新。一般情况下只需要执行第二个 make 就可以。
- `-U__linux__` 表示取消 `__linux__` 这个宏定义，否则会启动一些 linux 的特性，比如 "version.h"和 epoll。


## 运行

```shell
redis-server redis.conf &
redis-cli
redis-benchmark -n 1000 -c 1
```

## 系统调用

只展示一些核心的与 socket 有关的系统调用。

server 的核心系统调用
```
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 3   # 创建一个 socket
setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0 # 若处在TIME_WAIT阶段可以重用该socket
bind(3, {sa_family=AF_INET, sin_port=htons(6379), sin_addr=inet_addr("127.0.0.1")}, 16) = 0     # 设置地址以及监听的端口号
listen(3, 511)                          = 0     # 设置为监听状态
fcntl(3, F_SETFL, O_RDWR|O_NONBLOCK)    = 0     # 设置为非阻塞
select(4, [3], [], NULL, {tv_sec=0, tv_usec=100000}) = 0 (Timeout)      # 一直 select 直到 3 号文件描述符（那个 socket）有连接请求到来
accept(3, {sa_family=AF_INET, sin_port=htons(40968), sin_addr=inet_addr("127.0.0.1")}, [128->16]) = 4 # 接受等待队列的地一个请求，并返回一个新的 socket fd，也就是 4 号 fd。
fcntl(4, F_SETFL, O_RDWR|O_NONBLOCK)    = 0 # 设置为非阻塞
setsockopt(4, SOL_TCP, TCP_NODELAY, [1], 4) = 0
setsockopt(4, SOL_SOCKET, SO_KEEPALIVE, [1], 4) = 0
accept(3, 0x7ffcca942830, [128])        = -1 EAGAIN (资源暂时不可用)    # 再次以非阻塞的方式询问是否有其它的连接请求
select(5, [3 4], [], NULL, {tv_sec=0, tv_usec=15000}) = 1 (in [4], left {tv_sec=0, tv_usec=14995}) # 同时监听 3 号 socket 有无连接请求，以及 4 号 socket 有无数据传来。
read(4, "*1\r\n$7\r\nCOMMAND\r\n", 16384) = 17 # 从 4 号 socket 读取数据
write(4, "*174\r\n*6\r\n$6\r\nzscore\r\n:3\r\n", 26) = 26 # 向 4 号 socket 写入数据

```

client 核心系统调用

```
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 3 # 客户端创建一个 socket
fcntl(3, F_SETFL, O_RDWR|O_NONBLOCK)    = 0 # 将 socket 设置为非阻塞
connect(3, {sa_family=AF_INET, sin_port=htons(6379), sin_addr=inet_addr("127.0.0.1")}, 16) = -1 EINPROGRESS (操作现在正在进行) # 连接服务器
poll([{fd=3, events=POLLOUT}], 1, -1)   = 1 ([{fd=3, revents=POLLOUT}]) # 直到 socket 链接服务器完成。
write(3, "*1\r\n$7\r\nCOMMAND\r\n", 17) = 17 # 写数据
read(3, "*174\r\n*6\r\n$6\r\ndbsize\r\n:1\r\n", 16384) = 26 # 读数据
```

# 测试结果

```
/ # redis-server redis.conf &
/ # 6146:M 01 Jan 09:33:12.757 * Increased maximum number of open files to 10032 (it was originally set to 1024).
                _._                                                  
           _.-``__ ''-._                                             
      _.-``    `.  `_.  ''-._           Redis 3.2.9 (00000000/0) 64 bit
  .-`` .-```.  ```\/    _.,_ ''-._                                   
 (    '      ,       .-`  | `,    )     Running in standalone mode
 |`-._`-...-` __...-.``-._|'` _.-'|     Port: 6379
 |    `-._   `._    /     _.-'    |     PID: 6146
  `-._    `-._  `-./  _.-'    _.-'                                   
 |`-._`-._    `-.__.-'    _.-'_.-'|                                  
 |    `-._`-._        _.-'_.-'    |           http://redis.io        
  `-._    `-._`-.__.-'_.-'    _.-'                                   
 |`-._`-._    `-.__.-'    _.-'_.-'|                                  
 |    `-._`-._        _.-'_.-'    |                                  
  `-._    `-._`-.__.-'_.-'    _.-'                                   
      `-._    `-.__.-'    _.-'                                       
          `-._        _.-'                                           
              `-.__.-'                                               

6146:M 01 Jan 09:33:13.376 # Server started, Redis version 3.2.9
6146:M 01 Jan 09:33:13.421 * The server is now ready to accept connections on port 6379
/ # redis-benchmark -n 1000 -c 2 -q
PING_INLINE: 323.21 requests per second
PING_BULK: 268.24 requests per second
SET: 174.76 requests per second
GET: 306.09 requests per second
INCR: 322.16 requests per second
LPUSH: 175.19 requests per second
RPUSH: 295.60 requests per second
LPOP: 293.00 requests per second
RPOP: 193.12 requests per second
SADD: 179.05 requests per second
SPOP: 179.44 requests per second
LPUSH (needed to benchmark LRANGE): 177.87 requests per second
LRANGE_100 (first 100 elements): 158.18 requests per second
LRANGE_300 (first 300 elements): 237.30 requests per second
LRANGE_500 (first 450 elements): 145.03 requests per second
LRANGE_600 (first 600 elements): 131.46 requests per second
MSET (10 keys): 310.75 requests per second
```