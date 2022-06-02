# 多核启动

## 基本情况

在 OpenSBI 中，多个核心将同时开始启动，最快的核心将以热启动的方式对整个系统进行配置，并来到操作系统的入口 0x80200000 ，该核心称为主核。

而对于其他三个副核，他们将停留在冷启动的 `sbi_hsm_hart_wait` 函数中：

```c
	while (atomic_read(&hdata->state) != SBI_HSM_STATE_START_PENDING) {
		wfi();
	};
```

直到主核向它们发送 IPI 中断并改变核的状态，副核才会继续执行并来到操作系统的入口 0x80200000 。

## 唤醒其他核心

openSBI 已经封装了到 M 级的系统调用用于改变核的状态并产生中断。

下面的代码是改变编号为 hartId 核心的状态。

```c

inline void setMode(int hartId) {
	register u64 a7 asm ("a7") = 0x48534D;
	register u64 a0 asm ("a0") = hartId;
	register u64 a1 asm ("a1") = 0x80200000;
	register u64 a2 asm ("a2") = 19260817; // priv
	register u64 a6 asm ("a6") = 0; // funcid
	asm volatile ("ecall" : "+r" (a0) : "r"(a0), "r"(a1), "r"(a2), "r"(a6), "r" (a7) : "memory");
}

```

其中 a2 寄存器的值对于这个系统调用没有影响。

之后主核发送 IPI 来唤醒其他的副核。

```c
static inline void sbi_send_ipi(const unsigned long* hart_mask) {
    SBI_CALL_1(SBI_SEND_IPI, hart_mask);
}
```

## 确保互斥

在主核执行到操作系统的代码段之后，首先对中断、异常、驱动、文件系统等模块进行设置，在设置完毕后通过改变状态和发送 IPI 来唤醒副核，最后通过同步原语确保其他核心在主核完成设置后才能继续执行。

之后副核做自己页表的映射和 MMU 的启动，就可以开始进程调度了。

```c

volatile int mainCount = 1000;

void main(u64 hartId) {
    initHartId(hartId);

    if (mainCount == 1000) {

        printf("Hello, risc-v!\nCurrent hartId: %ld \n\n", hartId);
        
        //some setting

        for (int i = 1; i < 5; ++ i) {
            if (i != hartId) {
                unsigned long mask = 1 << i;
                setMode(i);
                sbi_send_ipi(&mask);
            }
        }

        trapInit();

        __sync_synchronize();     

        initFinish = 1;

    } else {
        while (initFinish == 0);
        __sync_synchronize();

        printf("Hello, risc-v!\nCurrent hartId: %ld \n\n", hartId);

        startPage();
        trapInit();

    }

    yield();
}
```