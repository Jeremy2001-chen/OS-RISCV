# Socket

## introduction

我们的 OS 实现了 socket 的相关接口，包括 read, write, listen, accept, connect.

我们的 socket 只支持 localhost 之间的消息通信，不支持 TCP 这一层的协议。

## createSocket

socket 的申请采用位图实现

## read && write

为每个 socket 开了一个 PAGE_SIZE 大小的循环队列

## bind

## listen



## accept



## connect