# 异常与系统调用

在我们的OS的设计中，有三种事件会导致CPU停止执行普通指令并且强制跳转到一段特殊的代码来处理这个事件。

- 系统调用（system call），当程序执行ecall指令来让内核为它做一些事情。
- 异常（exception），当用户态程序或内核态程序做了一些非法的事情（除以0或访问非法地址）。
- 中断（interrupt），设备会发出信号来表示这个设备需要被注意。例如DMA操作中磁盘完成了读写操作。

异常发生时，控制权将转移到内核中，内核保存寄存器和其它状态以便于可以恢复之前的代码。之后内核执行适当的处理程序。之后内核恢复之前保存的状态并且从异常返回，原始代码从中断处继续开始执行。

我们的OS将所有的Trap在内核中处理，而不会分发到用户态。

下文中将system call、exception、interrupt统称为异常。

## 异常处理机制

内核通过读取一些控制寄存器来判断哪一种Trap发生了。下面是最重要的几个寄存器。

- stvec：内核将自己的异常处理函数的地址保存在这个寄存器里。
- sepc：当异常发生时，RISC-V将程序计数器（PC）的值保存在这个寄存器里。
- scause：RISC-V 将异常的原因保存在这个寄存器里。
- sstatus：该寄存器里的SIE位控制着中断（interrupt）是否被启用，如果SIE位被设置位0，RISC-V将忽略所有的设备中断知道SIE位被置为1。该寄存器里的SPP位表示这个异常是来在于用户态还是内核态。

这些与异常相关的寄存器都是S级别的寄存器，并且不能在U级被读写。在多核心芯片上的每个CPU都有自己的一套控制寄存器。

- 如果当前异常是一个设备中断（interrupt），并且sstatus::SIE位的值为0，则忽略该中断，什么事情也不做。
- 将sstatus::SIE位设置为0，关闭中断。
- 将pc赋值给sepc
- 将当前的特权级（U级或者S级）赋值给sstatus::SPP
- 将异常的原因赋值给scause
- 将特权级设置为S级
- 将stvec赋值给pc
- 从新的pc值处开始执行。

## 初始化

在 `main` 函数中，我们调用 `trapInit` 函数来进行异常初始化。

```c
void trapInit() {
    w_stvec((u64)kernelVector); //将stvec设为kernelVector
    w_sstatus(r_sstatus() | SSTATUS_SIE | SSTATUS_SPIE); //开启全局中断
    w_sip(0); //清除所有的中断等待位
    w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE); //开启外部、软件、时钟中断
}
```

## 退出异常

当我们的调度器选择了一个进程并准备调度它，或者从异常返回时，会调用 `userTrapReturn` 从内核态（S级）转换为用户态（U级）。`userTrapReturn` 函数的主要流程有以下几步：

- `w_stvec(TRAMPOLINE_BASE + ((u64)userVector - (u64)trampoline));` 其中 `userVector` 是trampoline页中的一个函数，我们通过该公式计算出 `userVector` 在用户页表中的位置，并将这个函数设置为 trap handler。

- `trapframe->kernelSp = getProcessTopSp(myproc());` 设置该进程的内核态栈指针，这个内核栈将在该进程发生Trap时使用。

- `trapframe->trapHandler = (u64)userTrap;` 设置主异常处理函数，所有的异常（syscall，interrupt，exception）都将经过这个函数。

- ```c
      sstatus &= ~SSTATUS_SPP; //将异常前的特权级设置为U级，所以在Trap返回时会返回到U级
      sstatus |= SSTATUS_SPIE; //切换到U级时开启中断
  ```

- 调用 trampoline 中的汇编函数 `userReturn` 。

在 `userReturn` 函数中，我们进行退出异常的具体操作：

- `csrw satp, a1` 将 `satp` 设为用户进程页表。并且调用 `sfence.vma` 刷新缓存
- 设置 `sscratch` 寄存器，该寄存器标识 `Trapframe` 的地址
- 恢复 `sepc` `ra` `sp` 等寄存器。
- 调用 `sert` 从S级返回到U级

## 进入异常

在退出异常时，将 `stvec` 设置为了 `userVector`， 当用户进程发生异常时，将会跳转到 `userVector` 函数，下面是 `userVector` 函数的处理流程：

- 从 `sscratch` 中获得 `Trapframe` 的地址
- 将 `ra` `sp` 等寄存器存入 `Trapframe` 里
- 将 `sp` 设置为内核栈指针
- 将 `satp` 设置为内核页表
- 跳转到主异常处理函数 `userTrap`

`userTrap` 的处理流程如下：

- `w_stvec((u64) kernelVector);` 将异常向量设置为 `kernelVector` ，`kernelVector` 的作用与 `userVector` 类似，都起到了保存现场的作用，但是最后跳转到的处理函数 `kernelTrap`，该函数用于处理在内核态发生的异常。

- 如果这是一个设备中断，则在 `trapDevice` 中处理相应的设备。然后返回用户态。

  ```c
  if (scause & SCAUSE_INTERRUPT) {
          trapDevice();
          yield();
  }
  ```

- 若不是硬件中断，则判断是哪一种syscall或exception。其中包括ecall、读页错误、写页错误。

  ```c
  switch (scause & SCAUSE_EXCEPTION_CODE){
         case SCAUSE_ENVIRONMENT_CALL: //ecall系统调用
              trapframe->epc += 4;
              syscallVector[trapframe->a7]();
              break;
          case SCAUSE_LOAD_PAGE_FAULT: //read page fault
          case SCAUSE_STORE_PAGE_FAULT: //write page fault
              pa = pageLookup(currentProcess[hartId]->pgdir, r_stval(), &pte);
              if (pa == 0) {
                  pageout(currentProcess[hartId]->pgdir, r_stval());
              } else if (*pte & PTE_COW) {
                  cowHandler(currentProcess[hartId]->pgdir, r_stval());
              } else {
                  panic("unknown");
              }
              break;
  ```

- 最后调用 `userTrapReturn();` 返回用户程序。

## 内核态Trap

内核态 Trap 的处理函数为 `kernelTrap`。在正常情况下能进入内核态 Trap的方式只有外部中断，否则内核直接 panic。

对于外部中断的情况，我们调用 `trapDevice` 处理外部中断，然后该函数返回。