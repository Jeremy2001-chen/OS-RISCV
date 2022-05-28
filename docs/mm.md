# 内存管理

## 启动

对于每个 CPU 启动时，他们处在 M 级别。在将控制权交给操作系统时，转换为 S 级别。

在 `Init.c` 中我们会对内核页表进行初始化，并且开启页表映射。

我们会在 `Init.c` 中调用 `memoryInit()` 函数，在该函数中我们做：

- 初始化物理页控制块。
- 构造内核页表。
- 开启三级页表映射。

物理地址分配关系：在物理地址空间中，位于 0x8000_0000 下方的一些是专门用于与外设交互的内存。 OpenSBI 与 U-Boot 位于 0x8000_0000 处，即内存的开始位置。内核则位于 0x8020_0000 处，在启动时 U-Boot 会将内核加载到 0x8020_0000 这个物理地址处。

在 `kernelEnd` 到 `PHYSICAL_MEMORY_TOP` 之间的这一部分物理内存，则被用来分配给用户程序，或者申请作为页表等其它用处。

```c
PHYSICAL_MEMORY_TOP-> +----------------------------+----
                      |                            |     
                      |    Free Physical memory    |    
                      |                            |                         
   kernelEnd   -----> +----------------------------+----
                      |                            |                         
                      |          Kernel            |    
                      |                            |                           
 0x8020 0000   -----> +----------------------------+----
                      |                            |                           
                      |         Open SBI           |    
                      |                            |                 
 0x8000 0000  ----->  +----------------------------+----
                      |                            |                           
                      |                            |                           
                      |                            |                           
                      +----------------------------+---
                      |                            |                           
                      |           MMIO             |                
                      |                            |                           
                      +----------------------------+----
                      |                            |                           
                      |                            |                           
                      |                            |                           
                      +----------------------------+                           

```

### 物理页控制块

```c
// 物理页控制块
typedef struct PhysicalPage {
    PageListEntry link; //链表指针
    u32 ref; //物理页的引用计数，表示有多少虚拟页映射到了该物理页
    int hartId;
} PhysicalPage;
PhysicalPage pages[PHYSICAL_PAGE_NUM];
```

### 初始化物理页控制块

```c
    u32 n = PA2PPN(kernelEnd); //计算内核与OpenSBI占用了多少内存。
    for (i = 0; i < n; i++) {
        pages[i].ref = 1;	//将内核与OpenSBI占用的内存标记为“已占用”
    }

    n = PA2PPN(PHYSICAL_MEMORY_TOP);//计算内存一共被分为几页
    LIST_INIT(&freePages);//初始化物理页控制块
    for (; i < n; i++) {
        pages[i].ref = 0;
        LIST_INSERT_HEAD(&freePages, &pages[i], link);//将物理页控制块插入“空闲链表”
    }
```

### 维护空闲链表

空闲页链表我们采用单链表的形式，并且提供 `LIST_REMOVE(...)`、`LIST_INSERT_HEAD(...)`、`LIST_INSERT_TAIL(...)` 来实现从链表中删除、从链表头插入、从链表尾插入。

### 内核态地址空间

我们设置内核页表，使得内核可以访问物理地址和各种 MMIO 外设。对于 RISC-V 的开发板，RAM（physical memory）开始在 0x8000_0000 处，并且对于 unmatched 开发板拥有 16G 的内存。同时开发板上还有很多 MMIO 接口，这些外设的物理地址在 0x8000_0000 以下。内核可以通过读写这些特殊的物理内存来与外设交互。

我们的内核访问内存和外设时用“直接映射”，将 RAM 和外设的虚拟地址设为和它的虚拟地址相同。例如，无论启不启用也表，访问 0x8000_0000 时都会访问到物理内存的最开始位置。“直接映射”简化了内核访问内存和外设的代码。

同时，我们有一些没采用“直接映射”的虚拟地址。

- 对于 `TRAMPOLINE` 页，每个页表的 `TRAMPOLINE_BASE` 全都映射到同一个物理地址。对于 `TRAMPOLINE` 页的具体作用，我们将在“系统调用与异常中断”部分详细介绍。
- 内核栈空间。每个进程拥有自己的内核栈空间，我们将每个进程的内核栈设置为 `KERNEL_PROCESS_SP_TOP - (u64)(p - processes) * 10 * PAGE_SIZE`。其中`p-processes` 代表当前进程控制块的编号。我们在两个内核栈之间设置了守卫页，该页的PTE_V为0。如果内核栈溢出了，访问到了守卫页，内核将会造成 page-fault异常。

我们在 `virtualMemory()` 中构造了内核态页表，其中

- 将 0x8000_0000 ~ PHYSICAL_MEMORY_TOP的地址映射到真正的物理地址，这使得在内核态下也可以访问内存的物理地址和虚拟地址相同。
- SPI 、UART 的虚拟地址和物理地址相等。
- 将用于错误处理的页 TRAMPOLINE ，从虚拟地址 TRAMPOLINE_BASE 映射到物理地址的trampoline。

```c
    //映射SPI
    pageInsert(kernelPageDirectory, SPI_CTRL_ADDR, SPI_CTRL_ADDR, PTE_READ | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY);
    //映射UART
    pageInsert(kernelPageDirectory, UART_CTRL_ADDR, UART_CTRL_ADDR, PTE_READ | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY);
    extern char textEnd[];
    va = pa = (u64)kernelStart;
    //映射内存
    for (u64 i = 0; va + i < (u64)textEnd; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_EXECUTE | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY);
    }
    va = pa = (u64)textEnd;
    //映射内存
    for (u64 i = 0; va + i < PHYSICAL_MEMORY_TOP; i += PAGE_SIZE) {
        pageInsert(kernelPageDirectory, va + i, pa + i, PTE_READ | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY);
    }
    extern char trampoline[];
    //映射trampoline
    pageInsert(kernelPageDirectory, TRAMPOLINE_BASE, (u64)trampoline, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE | PTE_ACCESSED | PTE_DIRTY);
    pageInsert(kernelPageDirectory, TRAMPOLINE_BASE + PAGE_SIZE, (u64)trampoline + PAGE_SIZE, 
        PTE_READ | PTE_WRITE | PTE_EXECUTE | PTE_ACCESSED | PTE_DIRTY);
```

### 开启三级页表

```c
void startPage() {
    // 将当前页表设置为内核页表
    w_satp(MAKE_SATP(kernelPageDirectory));
    // 刷新缓存（TLB）
    sfence_vma();
}
```

`memoryInit(...)` 函数调用 `startPage()` 函数来启用内核页表。它将内核页表的基地址写入 `w_satp` 寄存器，并且刷新缓存（TLB）。

在这一步操作之后，我们的内核将真正使用我们的内核页表，因为物理地址部分我们使用了一一映射（也即物理地址与虚拟地址相同），所以 CPU 的下一条指令将被映射到正确的物理内存位置。

每个 RISC-V CPU 将会把页表缓存进自己的 TLB（Translation Look-aside Buffer）之中，当 OS 切换页表时，也必须用 `sfence_vma` 来刷新TLB。否则在刷新TLB之前，某些虚拟地址将指向其它进程的物理内存，每当执行 `w_satp(...)` 后或者在trampoline中切换页表后都需要调用`sfence_vma` 来刷新 TLB。

## 进程地址空间

每一个进程拥有一个独立的页表，当操作系统切换进程时，同时会切换进程的页表。每个进程的虚拟地址空间从 0x0 开始，并且最大是 $2^64$。

当一个进程向操作系统申请更多的内存空间时，OS 将调用 `LIST_REMOVE(...)` 从空闲页链表中取出队首元素，并且调用 `pageInsert(...)` 将虚拟地址映射到该页面对应的物理地址，并且给予权限位`PTE_READ ， PTE_EXECUTE ， PTE_WRITE ， PTE_ACCESSED ， PTE_DIRTY`。

首先，每个进程的页表将逻辑地址转化为不同的物理地址，所以每个进程拥有它们自己的地址空间。第二，每个进程在页表的作用下看起来像是拥有从 0x0 开始的连续的逻辑地址空间，尽管它所占用的物理地址是可能不连续的。第三，我们将所有进程的 `TRAMPOLINE_BASE` 都映射到同一个物理页，所以所有进程都能访问到 trampoline 页面

## 插入页表

插入页表通过 `pageInsert(pgdir, va, pa, perm)` 函数实现。

该函数首先调用 `pageWalk(...)` 函数，若该函数返回的也表项 pte 是一个合法的页表项 (PTE_V = 1) ，则调用 `pageRemove(...)` 将 pte 指向的页移除掉。

再次调用 `pageWalk(...)`，找到虚拟地址 va 对应的页表项 pte ，并且将 pte 设置为 pa 对应的物理页号，并给予相应的权限。

将 pa 对应的物理页的引用计数 `ref` 加 1。

最后调用 `sfence_vma` 刷新缓存（TLB）。

## 创建页表

对于每一个页表，我们用一个 64 位unsigned类型的指针 `u64*` 指向页表的基地址。对于每一个指向页表的指针 `u64* pgdir` ，它可能是指向内核页表或某一个用户页表。

核心函数 `int pageWalk(u64 *pgdir, u64 va, bool create, u64 **pte)`，这个函数将在给定的页表 `pgdir` ，寻找虚拟地址 `va` 对应的页表项。

该函数的具体实现，模仿 RISC-V 硬件根据虚拟地址查找页表项的过程。该函数每次从高向低考虑虚拟地址中的 9 bits，根据这 9 bits 得到下一级也表页对应的也表项，并且找到下一级页表页对应的物理地址。如果在查找的过程中发现页表项不合法，根据 `create` 参数决定是否申请页表页，如果 `create==1`，则会自动申请二级页表和三级页表。最后将 `*pte` 指向对应的页表项。

`copyout(...)` 函数负责将内核态的数据拷贝到用户态的某个虚拟地址处。

`coptin(...)` 函数负责将用户态某个虚拟地址的数据拷贝到内核态。

这两个函数主要用于在系统调用传参时，将用户态的字符串等拷贝到内核态。

