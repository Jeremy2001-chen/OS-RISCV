# 进程调度

## 进程控制块

```c
typedef struct Process {
    Trapframe trapframe;  // 每个进程对应的 Trapframe，保存中断或异常时的寄存器信息
    struct ProcessTime processTime;  //记录进程在用户态或内核态运行的事件
    CpuTimes cpuTime;
    LIST_ENTRY(Process) link;          // 空闲进程控制块链表指针
    u64 awakeTime;                     //进程被唤醒的时间
    u64* pgdir;                        //进程页表
    u32 id;                            //进程ID
    u32 parentId;                      //父进程ID
    LIST_ENTRY(Process) scheduleLink;  //调度队列指针
    u32 priority;                      //优先级
    enum ProcessState state;           //进程的状态
    struct Spinlock lock;              // 访问进程控制块的互斥锁
    struct dirent* cwd;                // Current directory
    struct file* ofile[NOFILE];        //打开文件列表
    u64 chan;                          // wait Object
    u64 currentKernelSp;               //进程的内核栈指针
    int reason;  //进程让出控制权的理由（在用户态让出还是内核态让出）
    u64 retValue;    //进程的返回值
    u64 heapBottom;  //进程的堆顶部，用于sbrk系统调用
} Process;
```

获取当前运行的进程的代码如下。每个 cpu 正在运行的进程存储在 `currentProcess[hartId]` 数组内，需要注意的是，我们在调用 `myproc()` 获取进程时，必须要关中断。否则，在获取完 hartId 后发生中断，则以 hardId 为数组下标取出的进程是错误的。

```c
Process* myproc() {
    interruptPush(); //关中断
    int hartId = r_hartid();
//<----------interrupt-------------->
    if (currentProcess[hartId] == NULL)
        panic("get current process error");
    Process *ret = currentProcess[hartId];
    interruptPop(); //开中断
    return ret;
}
```

## 进程ID

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

## 进程状态

```c
enum ProcessState {
    UNUSED,   // 控制块未使用
    SLEEPING, // 睡眠（阻塞）
    RUNNABLE, // 就绪
    RUNNING,  // 正在运行
    ZOMBIE    // 僵尸进程
};
```

`UNUSED` ：进程控制块空闲，可以被分配给某一个进程。

`SLEEPING` ：进程等待某资源或调用 sleep 函数。

`RUNNABLE` ：进程就绪，可以被运行。

`RUNNING` ：进程正在被运行。

`ZOMBIE` ：该进程已经执行完毕，但是返回值没有被父进程获取。



## 启动初始化

OS 在 `processInit()`  函数里对进程控制块进行初始化。

```c
void processInit() {
    printf("Process init start...\n");
    
    //初始化链表头节点
    LIST_INIT(&freeProcesses);
    LIST_INIT(&scheduleList[0]);
    LIST_INIT(&scheduleList[1]);
    int i;
    extern u64 kernelPageDirectory[];
    for (i = PROCESS_TOTAL_NUMBER - 1; i >= 0; i--) {
        processes[i].state = UNUSED;  //进程控制块标记为空闲
        processes[i].trapframe.kernelSatp = MAKE_SATP(kernelPageDirectory);  //设置内核页表
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);  //将进程块插入空闲进程块链表
    }
    w_sscratch((u64)getHartTrapFrame());  //设置cpu的Trapframe的位置
    printf("Process init finish!\n");
}

```

## 进程初始化

OS 在 `setup(...)` 函数中对某个进程进行初始化

- 给进程申请页目录
- 设置 `p->heapBottom` ，该变量用于 `sbrk` 系统调用
- 设置 `p->cwd` 工作目录为 `&rootFileSystem.root`
- 申请进程内核态下的栈空间，并将它插入到内核页表，具体的地址信息参见**内存管理**部分
- 将 `trampoline`、`Trapframe` 所在的页面插入进程页表，当进程进入 Trap 时，内核会运行 `trampoline` 中的代码，并且将寄存器信息保存在 `Trapframe` 页面中。关于这两个页面的详细信息参见 **Trap** 部分。

## 进程申请

在 `processAlloc(...)` 函数中我们实现了进程申请的功能。

- 获取空闲进程块链表的锁。
- 判断是否有空闲进程块

- `p = LIST_FIRST(&freeProcesses);`  取出队首的进程控制块
- 释放链表的锁。
- 调用 `setup()` 进行初始化
- 生成进程 ID
- 设置父进程
- 修改进程的 Trapframe 中的用户态栈指针和内核态栈指针，这将在进程返回用户态时用到。

## 创建进程

函数 `processCreatePriority()`根据传入的 `binary` 创建新的进程，其中 `binary` 为进程的二进制文件。

- 申请进程控制块
- 设置进程优先级
- 调用 `loadElf(...)` 将 `binary` 中的段填入刚申请的进程。
- 设置起始执行的位置 `p->trapframe.epc = entryPoint;` 
- 将进程插入调度队列

## 运行进程

`processRun(Process* p)` 运行  `p` 进程。

- 如果之前该 cpu 正在运行其它进程，则将 `TRAMPOLINE` 中保存的寄存器信息拷贝到刚才运行的进程控制块中
- `p->state = RUNNING;`
- 接下来判断让出 cpu 使用权的原因，若 `p->reason == 1` 表示该进程主动让出（因为 pipe 阻塞）。
  - 将当前进程的进程控制块的  Trapframe 拷贝到 trampoline 中的 Trapframe
  - 调用 `sleepRec()`，该函数从进程的内核态栈中读读取上下文并恢复执行
- 若 `p->reason == 0` ，则将
  - 将当前进程的进程控制块的  Trapframe 拷贝到 trampoline 中的 Trapframe
  - 设置内核栈
  - 调用 `userTrapReturn()` 函数恢复上下文。该函数的详细信息参见 **Trap** 部分。

## 进程销毁

首先调用 `processFree()`，该函数做以下操作：

- 释放进程所持有的资源，包括页表、数据页、打开的文件
- 唤醒因为 `wait()` 函数而等待的父进程
- 将进程状态设置为 `ZOMBIE`

将 cpu 的当前进程设置为 `NULL`。

重新设置内核栈。

调度下一个进程。



## yield 让出使用权

`yield()` 函数主要执行以下步骤。

- 将当前进程的进程状态改为 `RUNNABLE`。
- 将当前进程插入当前调度队列的末尾。
- 不断交替访问两个调度队列，若发现一个 `RUNNABLE` 的进程，则运行该进程。