#include <Type.h>
#include <Driver.h>
#include <Uart.h>

static u64 uartBaseAddr = 0x10010000; 
void consoleInit() {
    // todo
}

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
