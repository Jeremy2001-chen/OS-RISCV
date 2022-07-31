# 进程管理和线程管理

## 基本情况

在初赛阶段，我们的内核以进程为单位进行调度，实现方式较为简单。在决赛第一阶段，为了符合 POSIX 规范，我们实现了内核级线程：所有的调度单位均以**线程**为单位，进程则为资源的一种管理单位。

## 进程

### 进程控制块

进程控制块将保留部分资源，剩余资源将由线程管理。

```c
typedef struct Process {
    struct ProcessTime processTime; // 记录进程上一次运行的时间，用于计算 CPU 时间
    CpuTimes cpuTime; // 记录进程在用户态或内核态运行的 CPU 时间
    LIST_ENTRY(Process) link; // 空闲进程控制块链表指针
    u64 *pgdir; // 进程页表
    u32 processId; // 本进程编号
    u32 parentId; // 父进程编号
    u32 priority; // 进程优先级
    enum ProcessState state; //进程状态
    struct Spinlock lock; // 访问进程控制块的互斥锁
    struct File *ofile[NOFILE]; // 打开文件列表
    u32 retValue; // 进程返回值
    u64 heapBottom; // 进程的堆顶部地址
    struct dirent *cwd; // 进程当前工作目录
    int threadCount; // 进程包含的线程数量
    struct ResourceLimit fileDescription; // 文件描述符数量最大限制
} Process;
```

获取进程控制块的方式是调用 `myProcess` 函数，该函数会根据维护的 currentThread 数组和所在核编号 hartId 获取当前正在执行的线程，并以此获得对应的线程。

### 进程编号

进程的 ID 由两部分组成，第一部分为一个独一无二的编号 nextId，第二部分为**进程控制块**的编号。同时为了防止多核时发生竞争，在生成进程 Id 时要获取 `processIdLock`。

```c
u32 generateProcessId(Process *p) {
    acquireLock(&processIdLock);
    static u32 nextId = 0;
    u32 processId = (++nextId << (1 + LOG_PROCESS_NUM)) | (u32)(p - processes);
    releaseLock(&processIdLock);
    return processId;
}
```

### 进程状态

进程总共包含三种状态，如下所示：

- `UNUSED` ：进程控制块空闲，可以被分配给某一个进程。

- `RUNNING` ：该进程控制块正在被使用。

- `ZOMBIE` ：该进程已经执行完毕，但是返回值没有被父进程获取。

### 进程返回值

进程的返回值由最后一个线程的返回值决定，当一个进程的最后一个线程执行完毕后，会将该线程的返回值设置为本进程的返回值。

### 启动初始化

内核在 `processInit()` 函数里对所有的进程控制块进行初始化。

```c
void processInit() {
    printf("Process init start...\n");
    
    // 初始化相关互斥锁
    initLock(&freeProcessesLock, "freeProcess");
    initLock(&processIdLock, "processId");
    initLock(&waitLock, "waitProcess");

    LIST_INIT(&freeProcesses); // 初始化空闲进程列表
    
    // 将空闲进程控制块插入列表中
    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);
    }

    threadInit(); // 线程初始化
    w_sscratch((u64)getHartTrapFrame()); //设置 CPU 的 Trapframe 的位置
    printf("Process init finish!\n");
}
```

### 进程初始化

在申请一个新的进程时，需要申请进程控制块，因此需要进行使用 `processSetup` 函数进行进程初始化。

- 申请页目录
- 设置堆地址
- 设置当前工作目录为 `&rootFileSystem.root`
- 设置文件描述符数量限制为最大值
- 将 `trampoline`、`Trapframe` 所在的页面插入进程页表，当进程进入 Trap 时，内核会运行 `trampoline` 中的代码，并且将寄存器信息保存在 `Trapframe` 页面中。关于这两个页面的详细信息参见 **Trap** 部分。
- 将 `signalTrampoline` 所在页面插入进程页表，当线程处理完信号后，会跳转到该页面通过 SYS_signalReturn 系统调用结束信号处理。
- 将 `signalHandler` 所在页面插入到内核页表中

### 进程申请

申请新进程通过 `processAlloc` 函数进行。

- 从空闲进程列表中取出第一个进程控制块
- 进行进程初始化
- 设置进程编号、状态和父进程编号

### 进程销毁

当一个进程的所有线程全部结束之后，该进程将会调用 `processFree` 函数自行销毁：

- 释放进程所持有的资源，包括页表、数据页、打开的文件
- 将进程状态设置为 `ZOMBIE`
- 唤醒因为 `wait` 函数而等待的父进程

## 线程

### 线程控制块

```c
typedef struct Thread {
    Trapframe trapframe; // 每个线程对应的 Trapframe，保存中断或异常时的寄存器信息
    LIST_ENTRY(Thread) link; // 空闲线程控制块链表指针 
    u64 awakeTime; // 线程下一次可以被调度的时间
    u32 id; // 线程编号
    LIST_ENTRY(Thread) scheduleLink; // 线程调度队列链表 
    enum ProcessState state; // 线程状态
    struct Spinlock lock; // 访问线程控制块互斥锁
    u64 chan; // 等待资源
    u64 currentKernelSp; // 内核栈指针
    int reason; // 调度原因
    u32 retValue; // 线程返回值
    SignalSet blocked; // 线程信号掩码
    SignalSet pending; // 线程信号状态
    SignalSet processing; // 线程处理信号状态
    u64 setChildTid; // 设置 tid 地址标识
    u64 clearChildTid; // 清空 tid 地址标识
    struct Process* process; // 线程对应进程控制块指针
    u64 robustHeadPointer; // robust 锁地址
	struct SignalContextList waitingSignal; // 需要处理的信号链表
} Thread;
```

获取线程控制块的方式是调用 `myThread` 函数，该函数会根据维护的 currentThread 数组和所在核编号 hartId 获取当前正在执行的线程。

```c
Thread* myThread() {
    interruptPush();
    int hartId = r_hartid();
    if (currentThread[hartId] == NULL) {
        // printf("[Kernel]No thread run in the hart %d\n", hartId);
        interruptPop();
        return NULL;
    }
    Thread* ret = currentThread[hartId];
    interruptPop();
    return ret;
}
```

### 线程编号

进程的 ID 由两部分组成，第一部分为一个独一无二的编号 nextId，第二部分为**线程控制块**的编号。同时为了防止多核时发生竞争，在生成进程 Id 时要获取 `threadIdLock`。

```c
u32 generateThreadId(Thread* th) {
    acquireLock(&threadIdLock);
    static u32 nextId = 0;
    u32 threadId = (++nextId << (1 + LOG_PROCESS_NUM)) | (u32)(th - threads);
    releaseLock(&threadIdLock);
    return threadId;
}
```

### 线程状态

线程总共包含四种状态，如下所示：

- `UNUSED` ：线程控制块空闲，可以被分配给某一个线程。

- `SLEEPING` ：线程等待某资源或调用 sleep 函数。

- `RUNNABLE` ：线程就绪，可以被运行。

- `RUNNING` ：该线程正在运行。

### 线程返回值

线程的返回值为系统调用 `SYS_exit` 传入的 `ErrorCode` 决定。

### 启动初始化

内核在 `threadInit()` 函数里对所有的线程控制块进行初始化。

```c
extern u64 kernelPageDirectory[];
void threadInit() {
    // 线程相关锁初始化
    initLock(&freeThreadListLock, "freeThread"); 
    initLock(&scheduleListLock, "scheduleList");
    initLock(&threadIdLock, "threadId");

    // 空闲链表和调度列表初始化
    LIST_INIT(&freeThreades);
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);

    int i;
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        threads[i].state = UNUSED; // 设置线程没有使用
        threads[i].trapframe.kernelSatp = MAKE_SATP(kernelPageDirectory); // 设置内核页表
        LIST_INSERT_HEAD(&freeThreades, &threads[i], link); // 插入线程空闲链表
    }
}
```

### 线程初始化

在申请一个新的线程时，需要申请线程控制块，因此需要进行使用 `threadSetup` 函数进行线程初始化。

- 相关资源初始化
- 初始化信号链表
- 内核栈中插入对应栈空间用于内核处理

### 线程申请

线程的申请包含两种方式，一种称为主线程申请，调用函数为 `mainThreadAlloc`，而另一种方式为普通线程申请 `threadAlloc`。

主线程申请用于创建新的进程时，通常是用户态使用 `Fork` 函数时会触发。在 `clone` 函数时将分发到 `processFork` 分支处理进程的克隆。由于我们调度的最小单元为线程，因此在创建一个进程的同时我们需要创建一个用于调度和执行的主线程。在创建主线程时，会通过 `processAlloc` 申请一个新的进程，并将这个线程与进程进行绑定。`processFork` 时会使用写时复制技术拷贝整个地址空间，通过将页表项中写权限去除，并在页表项中设置 `COW` 权限位实现。当某一个进程尝试修改地址空间中的数据时，将触发权限异常，此时进入内核态将分发到写时赋值处理函数 `cowHandler` 进行处理，在该函数中将重新映射一个数据相同的页面在虚拟地址中，并设置正确的读写权限。

普通线程申请用于基于当前进程创建一个新的线程，通常是用户态使用 `pthread_create` 函数时会触发。在 `clone` 函数时将分发到 `threadFork` 分支处理线程的克隆。在创建普通线程时，只需要调用线程初始化函数，并将新申请的线程绑定到当前进程中。`threadFork` 并不会拷贝地址空间，因为一个进程的所有线程是共同使用一个页目录的。该函数会设置新的栈空间页面（这是 libc 通过 `SYS_mmap` 系统调用确定地址），线程存储数据 `tls` 地址和用户态线程链表地址 `clearChildTid`。

### 线程调度

线程的调度通过 `yield` 函数进行线程调度和选择。

- 如果当前线程非空且主动放弃资源则将上下文拷贝到线程控制块的 `Trapframe` 中
- 将当前线程插入调度队列尾部
- 从线程调度列表中取出首部，如果线程状态为 `RUNNABLE` 且唤醒时间小于当前时间，则结束调度，否则重复这一过程
- 清除当前线程所对应的 `futex` 列表
- 运行线程

### 线程运行

`threadRun(Thread* thread)` 运行 `thread` 线程。

- 如果之前该 CPU 正在运行其它进程，则将核心 `trampoline` 中保存的寄存器信息拷贝到刚才运行的线程控制块中的 `Trapframe` 中
- 设置进程状态为 `RUNNING`
- 接下来判断让出 CPU 使用权的原因，若为 `KERNEL_GIVE_UP` 表示该进程在内核主动让出（因为 pipe/socket 阻塞）。
  - 将当前进程的线程控制块的 `Trapframe` 拷贝到 `trampoline` 中的 `Trapframe`
  - 调用 `sleepRec`，该函数从进程的内核态栈中读读取上下文并恢复执行
- 若为回到用户态执行，则将
  - 将当前进程的进程控制块的 `Trapframe` 拷贝到 `trampoline` 中的 `Trapframe`
  - 设置内核栈地址
  - 调用 `userTrapReturn` 函数恢复上下文。该函数的详细信息参见 **Trap** 部分。

### 线程销毁

首先调用 `threadFree`，该函数做以下操作：

- 释放线程所持有的资源，比如所含有的信号链表
- 若 `clearChildTid` 被设置则需要将该地址所含有的值清空为 0（这是因为 libc 在 `pthread_exit` 时会抓锁清除自己，该地址维护的是线程链表锁的拥有者）
- 将进程所包含的线程数减少 1，若该线程为最后一个线程则设置返回值并调用 `processFree`
- 将当前线程插入到空闲链表中

## 总结

在大赛第一阶段我们实现了线程级调度，对于线程和进程的理解也更加深刻了。