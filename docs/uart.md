# 串口驱动

## 基本情况

我们将 OpenSBI 上的 Uart 驱动从 M 级移植到了 S 级，减少陷入 M 级导致的时延。

Sifive unmatched 开发板上串口采用 UART 模式进行传输，通过查阅手册可以发现该开发板上 UART 包括 UART0 和 UART1。

我们采用 UART0 进行数据传输。

| 物理地址    | 寄存器名称   | 内容描述   |
| ---------- | ---------- | ---------|
| 0x10010000 | txdata     | 传输寄存器 |
| 0x10010004 | rxdata     | 接收寄存器 |

## 输出字符

输出字符如下所示：

```c
inline void putchar(char ch)
{
    int* uartRegTXFIFO = (int*)(uartBaseAddr + UART_REG_TXFIFO);
	while (readl(uartRegTXFIFO) & UART_TXFIFO_FULL);
    writel(ch, uartRegTXFIFO);
}
```

在传输数据时，以字节为单位进行数据传输。当该寄存器最高位为1时表明 FIFO 队列已满，需要等待队列清空。只有 FIFO 队列未满时才能向该寄存器写数据。

这里 real 和 writel 并不是简单的向 MMIO 对应的内存中写数据，如下所示：

```c
#define __io_br()	do {} while (0)
#define __io_ar()	__asm__ __volatile__ ("fence i,r" : : : "memory");
#define __io_bw()	__asm__ __volatile__ ("fence w,o" : : : "memory");
#define __io_aw()	do {} while (0)

#define readl(c)	({ u32 __v; __io_br(); __v = __raw_readl(c); __io_ar(); __v; })
#define writel(v,c)	({ __io_bw(); __raw_writel((v),(c)); __io_aw(); })
```

读和写均需要一条 `fence` 指令保证 FIFO 队列及时清空，以防止阻塞读入输出。

## 读取字符

读取字符如下所示：

```c
inline int getchar(void)
{
    int* uartRegRXFIFO = (int*)(uartBaseAddr + UART_REG_RXFIFO);
	u32 ret = readl(uartRegRXFIFO);
    while (ret & UART_RXFIFO_EMPTY) {
        ret = readl(uartRegRXFIFO);
    }
    if ((ret & UART_RXFIFO_DATA) == '\r')
        return '\n';
    return ret & UART_RXFIFO_DATA;
}
```

读取数据同样以字节为单位进行数据传输。当该寄存器最高位为1表明 FIFO 队列为空，此时需要轮询直到等待队列不为空，获取字符。

由于 `windows` 上的回车为 `\r\n` ，因此当识别到 `\r` 时证明读取到了换行，需要转成 `\n` 。