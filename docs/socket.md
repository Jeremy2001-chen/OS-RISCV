# Socket

## introduction

我们的 OS 实现了 socket 的相关接口与系统调用，包括 socket, read, write, bind, listen, accept, connect.

我们的 socket 只支持 localhost 之间的消息通信，暂时还没实现与网络相关的协议，也即暂时不支持 TCP 这一层的协议。

## createSocket

socket 的申请采用类似位图的方法，申请出来的 socket 会和一个 File 绑定在一起，与 File 使用相同的 read、write 接口

内核总共会维护 SOCKET_COUNT 个 socket 结构体来为进程分配。

## read && write

类似于管道的实现，OS 为每个 socket 开辟了一个 PAGE_SIZE 大小的页面作为循环队列，每个 socket 有两个指针 head 和 tail，分别指向循环队列的队首和队尾。

socket 分为阻塞读写与非阻塞读写
- 非阻塞读写，需要先通过 select 等接口查询该 socket 就绪后，再进行读写；或者在用户态论询该 socket
- 阻塞读写，若 read 时管道为空或 write 时管道为满，则阻塞进程（通过 trapframe->epc-4），直到该 socket 可以读或者可以写。

## bind

将创建的socket绑定到指定的IP地址和端口上。

## listen

该函数将套接字转换为被动式套接字，表示此套接字可以开始接受其它 socket 的连接。此时若有客户端调用 connect 连接该 socket，那么客户端的 socket 将被加入服务端 socket 的等待队列中。

## accept

该函数一般由服务器调用，将从请求队列返回下一个建立成功的连接。如果已完成连接队列为空，若为阻塞调用，则将该程序挂起，否则直接返回 -EAGAIN。成功时返回套接字描述符。
若成功从请求队列里取出了一个连接，则重新申请一个 socket 并将该 socket 的两端设为服务器和客户端，唤醒那个正在等待的客户端，并将新 socket 和一个新的 fd 绑定并返回。

## connect(sockfd, const SocketAddr *addr)

connect 通常由客户端调用，用来与服务器建立 socket 连接。

connect 需要生成一个本地的端口作为这个 socket 的新端口，并找到处于 listen 状态并且端口号相同和 addr 相同的另一个 socket。若远程 socket 的等待队列没满，则将本地 socket 加入远程 socket 的等待队列里。
之后本进程调用 sleep 挂起，等待远程建立连接。