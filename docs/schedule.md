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
    FileSystem* fileSystem;            //文件系统
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

其中 `ZOMBIE` 表示该进程已经执行完毕，但是返回值没有被父进程获取。

## 初始化

OS在 `processInit()`  函数里对进程控制块进行初始化。

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
        processes[i].state = UNUSED;  //标记为空闲
        processes[i].trapframe.kernelSatp = MAKE_SATP(kernelPageDirectory);  //设置内核页表
        LIST_INSERT_HEAD(&freeProcesses, &processes[i], link);  //将进程块插入空闲进程块链表
    }
    w_sscratch((u64)getHartTrapFrame());  //设置Trapframe的位置
    printf("Process init finish!\n");
}

```

