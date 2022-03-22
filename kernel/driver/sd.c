#include <Platform.h>
#include <Spi.h>
#include <Uart.h>
#include <Driver.h>
#include <MemoryConfig.h>

//#include "common.h"


#define MAX_CORES 8

#define TL_CLK 0x8000000
#ifndef TL_CLK
#error Must define TL_CLK
#endif

#define F_CLK TL_CLK

static volatile u32 * const spi = (void *)(SPI_CTRL_ADDR);

static inline u8 spi_xfer(u8 d)
{
	i32 r;
	int cnt = 0;
	REG32(spi, SPI_REG_TXFIFO) = d;
	do {
		cnt++;
		r = REG32(spi, SPI_REG_RXFIFO);
	} while (r < 0);
	//printf("aaa  %d\n", cnt);
	return r;
}

static inline u8 sd_dummy(void)
{
	return spi_xfer(0xFF);
}

static u8 sd_cmd(u8 cmd, u32 arg, u8 crc)
{
	unsigned long n;
	u8 r;

	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_HOLD;
	sd_dummy();
	spi_xfer(cmd);
	spi_xfer(arg >> 24);
	spi_xfer(arg >> 16);
	spi_xfer(arg >> 8);
	spi_xfer(arg);
	spi_xfer(crc);

	n = 1000;
	do {
		r = sd_dummy();
		if (!(r & 0x80)) {
			printf("sd:cmd: %x\r\n", r);
			goto done;
		}
	} while (--n > 0);
	printf("sd_cmd: timeout\n");
done:
	return r;
}

static inline void sd_cmd_end(void)
{
	sd_dummy();
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_AUTO;
}


static void sd_poweron(void)
{
	long i;
	REG32(spi, SPI_REG_SCKDIV) = 3;
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_OFF;
	for (i = 10; i > 0; i--) {
		sd_dummy();
	}
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_AUTO;
}

static int sd_cmd0(void)
{
	int rc;
	printf("CMD0");
	rc = (sd_cmd(0x40, 0, 0x95) != 0x01);
	sd_cmd_end();
	printf("\n\n%d\n", rc);
	return rc;
}

static int sd_cmd8(void)
{
	int rc;
	printf("CMD8");
	rc = (sd_cmd(0x48, 0x000001AA, 0x87) != 0x01);
	sd_dummy(); /* command version; reserved */
	sd_dummy(); /* reserved */
	rc |= ((sd_dummy() & 0xF) != 0x1); /* voltage */
	rc |= (sd_dummy() != 0xAA); /* check pattern */
	sd_cmd_end();
	return rc;
}

static void sd_cmd55(void)
{
	sd_cmd(0x77, 0, 0x65);
	sd_cmd_end();
}

static int sd_acmd41(void)
{
	u8 r;
	printf("ACMD41");
	do {
		sd_cmd55();
		r = sd_cmd(0x69, 0x40000000, 0x77); /* HCS = 1 */
	} while (r == 0x01);
	return (r != 0x00);
}

static int sd_cmd58(void)
{
	return 0;// TODO
	int rc;
	printf("CMD58");
	rc = (sd_cmd(0x7A, 0, 0xFD) != 0x00);
	rc |= ((sd_dummy() & 0x80) != 0x80); /* Power up status */
	sd_dummy();
	sd_dummy();
	sd_dummy();
	sd_cmd_end();
	return rc;
}

static int sd_cmd16(void)
{
	int rc;
	printf("CMD16");
	rc = (sd_cmd(0x50, 0x200, 0x15) != 0x00);
	sd_cmd_end();
	return rc;
}

static u16 crc16_round(u16 crc, u8 data) {
	crc = (u8)(crc >> 8) | (crc << 8);
	crc ^= data;
	crc ^= (u8)(crc >> 4) & 0xf;
	crc ^= crc << 12;
	crc ^= (crc & 0xff) << 5;
	return crc;
}

#define SPIN_SHIFT	6
#define SPIN_UPDATE(i)	(!((i) & ((1 << SPIN_SHIFT)-1)))
#define SPIN_INDEX(i)	(((i) >> SPIN_SHIFT) & 0x3)

//static const char spinner[] = { '-', '/', '|', '\\' };

int sdRead(u8 *buf, u64 startSector, u32 sectorNumber) {
	volatile u8 *p = (void *)buf;
	int rc = 0;

	printf("CMD18");
	printf("LOADING  ");

	if (sd_cmd(0x52, startSector * 512, 0xE1) != 0x00) {
		sd_cmd_end();
		return 1;
	}
	do {
		u16 crc, crc_exp;
		long n;

		crc = 0;
		n = 512;
		while (sd_dummy() != 0xFE);
		do {
			u8 x = sd_dummy();
			//printf("%d ", x);
			*p++ = x;
			crc = crc16_round(crc, x);
		} while (--n > 0);

		crc_exp = ((u16)sd_dummy() << 8);
		crc_exp |= sd_dummy();

		if (crc != crc_exp) {
			printf("\b- CRC mismatch ");
			rc = 1;
			break;
		}

		if (SPIN_UPDATE(sectorNumber)) {
			//printf("\b");
			//printf(spinner[SPIN_INDEX(i)]);
		}
	} while (--sectorNumber > 0);
	sd_cmd_end();

	sd_cmd(0x4C, 0, 0x01);
	sd_cmd_end();

	return rc;
}

int sdWrite(u8 *buf, u64 startSector, u32 sectorNumber) {
	//REG32(spi, SPI_REG_SCKDIV) = (F_CLK / 16666666UL);
	if (sd_cmd(25 | 0x40, startSector * 512, 0) != 0) {
		sd_cmd_end();
		return 1;
	}
	sd_dummy();

	u8 *p = buf;
	while (sectorNumber--) {
		spi_xfer(0xFC);
		int n = 512;
		do {
			spi_xfer(*p++);
		} while (--n > 0);
		sd_dummy();
		sd_dummy();
		sd_dummy();
		sd_dummy();
	}

	int timeout = 0xfff;
	while (--timeout) {
		int x = sd_dummy();
		if (5 == (x & 0x1f)) {
			break;
		}
	}
	if (timeout == 0) {
		//panic("");
	}

	sd_cmd_end();
	sd_cmd(0x4C, 0, 0x01);
	sd_cmd_end();
	return 0;
}

int sdInit(void) {
	REG32(uart, UART_REG_TXCTRL) = UART_TXEN;

	sd_poweron();
	printf("INIT\n");
	if (sd_cmd0() ||
	    sd_cmd8() ||
	    sd_acmd41() ||
	    sd_cmd58() ||
	    sd_cmd16()) {
		printf("ERROR");
		return 1;
	}

	REG32(spi, SPI_REG_SCKDIV) = (F_CLK / 16666666UL);
	printf("BOOT");

	__asm__ __volatile__ ("fence.i" : : : "memory");
	return 0;
}
