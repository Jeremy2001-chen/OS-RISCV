#include <Type.h>
#include <Driver.h>
#include <Uart.h>
#include <Spinlock.h>
#include <file.h>
#include <Process.h>
#include <Thread.h>
#include <IO.h>

static u64 uartBaseAddr = 0x10010000;

struct AsynInput asynInputBuffer;

struct Spinlock consoleLock;

void consoleInterrupt(int c) {
    // todo

}

static inline u32 __raw_readl(const volatile void *addr)
{
	u32 val;

	asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
	return val;
}

static inline void __raw_writel(u32 val, volatile void *addr)
{
	asm volatile("sw %0, 0(%1)" : : "r"(val), "r"(addr));
}

#define __io_br()	do {} while (0)
#define __io_ar()	__asm__ __volatile__ ("fence i,r" : : : "memory");
#define __io_bw()	__asm__ __volatile__ ("fence w,o" : : : "memory");
#define __io_aw()	do {} while (0)

#define readl(c)	({ u32 __v; __io_br(); __v = __raw_readl(c); __io_ar(); __v; })
#define writel(v,c)	({ __io_bw(); __raw_writel((v),(c)); __io_aw(); })

inline void putchar(char ch)
{
    int* uartRegTXFIFO = (int*)(uartBaseAddr + UART_REG_TXFIFO);
	while (readl(uartRegTXFIFO) & UART_TXFIFO_FULL);
    writel(ch, uartRegTXFIFO);
}

inline u8 asynGetChar() {
    u8 ret = -1;
    acquireLock(&asynInputBuffer.lock);
    if (asynInputBuffer.head != asynInputBuffer.tail) {
        ret = asynInputBuffer.buffer[asynInputBuffer.head & (IO_BUFFER - 1)];
        asynInputBuffer.head++;
    }
    releaseLock(&asynInputBuffer.lock);
    return ret;
}

inline int getchar(void)
{
    int* uartRegRXFIFO = (int*)(uartBaseAddr + UART_REG_RXFIFO);
	u32 ret = readl(uartRegRXFIFO);
    if (ret & UART_RXFIFO_EMPTY) {
        return -1;
    }
    // if ((ret & UART_RXFIFO_DATA) == '\r')
    //     return '\n';
    return ret & UART_RXFIFO_DATA;
}

bool hasChar() {
    return asynInputBuffer.head != asynInputBuffer.tail;
}

int consoleWrite(int user_src, u64 src, u64 start, u64 n) {
    int i;
    for (i = 0; i < n; i++) {
        char c;
        if(either_copyin(&c, user_src, src+i, 1) == -1)
            break;
        if (c == '\n')
            putchar('\r');
        putchar(c);
    }
    return i;
}

//TODO, 未考虑多进程
//没有回显
#define GET_BUF_LEN 64
char buf[GET_BUF_LEN];
int consoleRead(int isUser, u64 dst, u64 start, u64 n) {
    int i;
    for (i = 0; i < n; i++) {
        char c = asynGetChar();
        if (c == (char)-1 && i == 0) {
            getHartTrapFrame()->epc -= 4;
            yield();
        }
        if (c == (char)-1)
            return i;
        if (c == '\n')
            putchar('\r');
        if (either_copyout(isUser, dst + i, &c, 1) == -1)
            break;
    }
    return i;
}

void consoleInit() {
    initLock(&consoleLock, "console");

    devsw[DEV_CONSOLE].read = consoleRead;
    devsw[DEV_CONSOLE].write = consoleWrite;
}

void asynInputInit() {
    initLock(&asynInputBuffer.lock, "asynInput");
    asynInputBuffer.head = asynInputBuffer.tail = 0;
}

void asynInput() {
    while(true) {
        acquireLock(&asynInputBuffer.lock);
        if (asynInputBuffer.tail - asynInputBuffer.head == IO_BUFFER) {
            releaseLock(&asynInputBuffer.lock);
            continue;
        }
        u8 c = getchar();
        if (c != (char)-1) {
            asynInputBuffer.buffer[asynInputBuffer.tail & (IO_BUFFER - 1)] = c;
            asynInputBuffer.tail++;
        }
        releaseLock(&asynInputBuffer.lock);
    }
}
