# RISCV64之旅

## RISC-V

### 寄存器列表

| 寄存器编号   | 编程接口名 | 补充说明                        |
|---------|-------|-----------------------------|
| x0      | zero  | -----                       |
| x1      | ra    | -----                       |
| x2      | sp    | -----                       |
| x3      | gp    | -----                       |
| x4      | tp    | Thread pointer              |
| x5-7    | t0-t2 | -----                       |
| x8      | s0/fp | -----                       |
| x9      | s1    | -----                       |
| x10-x11 | a0-1  | Parameter(2) & Return Value |
| x12-x17 | a2-7  | Parameter(8)                |
| x18-x27 | s2-11 | -----                       |
| x28-x31 | t3-6  | -----                       |

### 和MIPS指令差异

无符号: addu -> addw

存储指令: ld(64) sw(32) sd(64)

### 内嵌汇编

这块在之前移植开发时基本没有使用，因此需要学习。

#### 基本内联汇编

asm("statements")

插入多条汇编: asm("li x17, 81\n\t"
                "ecall")

volatile: 阻止编译器进行优化

#### 扩展内联汇编

asm volatile(

"statements"（汇编语句模板）:

output_regs（输出部分）:

input_regs（输入部分）:

clobbered_regs（破坏描述部分）

)

#### 在C语言中输出某个寄存器

```c
int x;
asm volatile(
    "sd s5, %0"
    :"=m"(x)
    :
    : "memory")
printf("x=%d\n", x)
```

将寄存器s5中的值输出到变量x中

### 机器特权态

最高级: 机器模式(M)

次高级: 监管者模式(S)

最低级: 用户模式(U)










