# 信号量

## 基本情况

在 Musl-Libc 中，用户态实现了一种高效的同步互斥机制 futex(Fast Userspace Mutexes)。该机制会尝试在用户态进行自旋操作，如果成功写入则相当于获取了临界区的锁，不需要陷入内核态。否则在自旋次数超过一定限制后会通过 `SYS_futex` 系统调用陷入内核态，由内核来阻塞这个线程，并等待其他线程唤醒这个线程。这种模式相比于传统的内核态系统互斥提升了效率，并且在一定程度上还减小了内核的压力。

## 系统调用

我们的内核支持了 `SYS_futex` 系统调用，该函数原型如下所示：

```c
int futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);
```

根据 `op` 类型不同，`futex` 函数的各个参数的含义也有所不同，如下表所示：

|      op       |      含义                             | 实现
| ------------- | ------------------------------------- | ------------------------------------------------------ |
| FUTEX_WAIT    | 表示信号量 P 操作                      | 从 uaddr 取值，如果不为 val 则表示非原子操作返回-1。若相等则表示该线程会等待 uaddr 的资源，当 timeout 不为空时表示线程等待的最大时间 |
| FUTEX_WAKE    | 表示信号量 V 操作                      | 唤醒最多 val 个因为 uaddr 阻塞的线程 |
| FUTEX_REQUEUE | 表示从等待一个信号量转为等待另一个信号量 | 唤醒最多 val 个因为 uaddr 阻塞的线程，其余因为 uaddr 阻塞的线程转为阻塞在 uaddr2 上 |

## 数据结构

```c
typedef struct FutexQueue
{
    u64 addr;
    Thread* thread;
    u8 valid;
} FutexQueue;
```

* addr: 阻塞的地址
* thread: 阻塞的线程
* valid: 该结构体是否有效

使用下面的数组维护的内核态互斥信息：
```c
FutexQueue futexQueue[FUTEX_COUNT];
```

## 相关函数

### futexWait

```c
void futexWait(u64 addr, Thread* thread, TimeSpec* ts);
```

该函数将 `thread` 线程阻塞在 addr 地址。当超时计时器 `ts` 不为空时，并不会将 `thread` 线程状态改为 `SLEEPING`，而是将该线程的`awakeTime` 设置为 ts->second * 1000000 + ts->microSecond，即下次可以再进行调度的时间。当超时计时器 `ts` 为空时，则直接将线程状态改为 `SLEEPING`。

当线程因为 `futexWait` 超时之后结束重新调度时，实际上内核态结构体的这一项依然是有效，因此我们每次调度之后需要清空内核态中的所有包含当前 `thread` 的 `futexQueue` 项。

### futexWake

```c
void futexWake(u64 addr, int n);
```

该函数将唤醒不超过 `n` 个阻塞在 addr 地址的线程。

### futexRequeue

```c
void futexRequeue(u64 addr, int n, u64 newAddr);
```

该函数将唤醒不超过 `n` 个阻塞在 addr 地址的线程，并将其余卡在 addr 地址的线程转为卡在 newAddr 上。

## 总结

* 实现了一个简单的 futex 模型，考虑的情况没有那么全面

