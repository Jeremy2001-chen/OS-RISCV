# SD卡驱动

## 基本情况

Sifive unmatched 开发板的 SD 驱动使用标准 SPI 总线协议，我们基于标准协议内容编写出可以使用的驱动。

通过查阅芯片设计图，可以观察出与 SD 卡连接的 SPI 总线为 QSPI 2，地址空间如下图所示：

| 物理地址   | 寄存器名称 | 内容描述           |
| ---------- | ---------- | ------------------ |
| 0x10050000 | sckdiv     | 串口频率除数寄存器 |
| 0x10050010 | csid       | 片选寄存器         |
| 0x10050014 | csdef      | 片选状态寄存器     |
| 0x10050018 | csmode     | 片选模式寄存器     |
| 0x10050040 | fmt        | 数据格式寄存器     |
| 0x10050048 | txdata     | 传输数据寄存器     |
| 0x1005004C | rxdata     | 接受数据寄存器     |

SPI 总线寄存器的存取通过 MMIO(Memory Map Input Output) 方式，将对应的物理地址插入到页表中来完成对应寄存器的访问。



## 数据传输

在 SPI 模式下数据交互包含主端和从端，CPU 作为驱动的主端发送命令，而 SD 卡作为驱动的从端接收命令并给予反馈。

主从之间传输数据的代码如下面所示：

```c
static inline u8 spi_xfer(u8 d)
{
	i32 r;
	int cnt = 0;
	REG32(spi, SPI_REG_TXFIFO) = d;
	do {
		cnt++;
		r = REG32(spi, SPI_REG_RXFIFO);
	} while (r < 0);
	return (r & 0xFF);
}
```

CPU 向传输数据寄存器发送一个字节的数据，并尝试接收一个字节的数据。rxdata 的第31位是 FIFO 队列是否为空标志位，当第 31 位置 1 时表明 FIFO 队列为空，此时对应得到 `r < 0`，因此需要再次询问 rxdata 寄存器直到 FIFO 队列不再为空，此时最低的一个字节即为接收到的数据。

当主端仅仅需要读取数据时，可以通过向从端发送0xFF的 sd_dummy 函数完成：

```c
static inline u8 sd_dummy(void)
{
	return spi_xfer(0xFF);
}
```

## 命令格式

SD 卡的命令传输格式有比较严格的限制，代码如下所示：

```c
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
			//printf("sd:cmd: %x\r\n", r);
			goto done;
		}
	} while (--n > 0);
	printf("sd_cmd: timeout\n");
done:
	return (r & 0xFF);
}
```

在传输前首先将片选模式寄存器的模式置为 HOLD 模式，表明要进行数据的传输，并等待一个周期（通过 `sd_dummy` 实现传输）。

```c
	REG32(spi, SPI_REG_CSMODE) = SPI_CSMODE_HOLD;
	sd_dummy();
```

接下来首先命令 cmd，所有的命令的最高位需要保证为1表明开始传输，

```c
	spi_xfer(cmd);
```

从高位往低位传输参数，

```c
	spi_xfer(arg >> 24);
	spi_xfer(arg >> 16);
	spi_xfer(arg >> 8);
	spi_xfer(arg);
```

最后传输 crc 校验码，事实上 crc 在 SPI 总线模式下是不启用的。

```c
	spi_xfer(crc);
```

对于每一个 CMD 命令，存在一个命令的返回值，如下图所示：

![img](http://rjhcoding.com/images/R1_FORMAT.png)

只有当最高位为 0 时，才是收到了命令的返回值，此时将命令的低 8 位返回。若读取 1000 次依然没有读取到正确的命令返回值，说明 CMD 命令超时。

```c
n = 1000;
	do {
		r = sd_dummy();
		if (!(r & 0x80)) {
			//printf("sd:cmd: %x\r\n", r);
			goto done;
		}
	} while (--n > 0);
	printf("sd_cmd: timeout\n");
done:
	return (r & 0xFF);
```



## 驱动初始化

在使用驱动进行数据传输之前，首先需要对 SD 卡进行驱动初始化，首先是一些寄存器的设置

* `fmt` 寄存器设置为 0x80000，这个是标准 SD 卡模式的数值
* `csdef` 寄存器设置为 1，表明选择该 SD 卡进行读入输出
* `csid` 寄存器设置为 0，表明开启片选模式
* `sckdiv` 寄存器设置为 3000，该数值会影响 SPI 频率大小，实际上只要保证工作频率在 100k - 400kHz SD 即可正常工作
* `csmode` 片选模式设置为 OFF，等待 10 个周期，再将片选模式改为 AUTO



接下来按照下面的顺序发送 CMD 命令

* CMD0
  * 重置 SD 卡状态
  * 传输命令 `sd_cmd(0x40，0，0x95)`
  * 返回值需要得到1，表明当前 SD 卡处于空闲状态
* CMD8
  * 检测 SD 卡的版本号
  * 传输命令 `sd_cmd(0x48, 0x000001AA, 0x87)`
  * 返回值需要位1，表明当前 SD 卡处于空闲状态
  * 电压模式应该在 2.7-3.6V 之间
* ACMD41
  * 发送主机信息并且开始 SD 卡初始化进程
  * 首先需要先发送 CMD55 表明下一个命令是特定的应用进程
    * 传输命令 `sd_cmd(0x77, 0, 0x65)`
  * 传输命令 `sd_cmd(0x69, 0x40000000, 0x77)`
  * 返回值应该需要为 0，表明 SD 卡已经不处于空闲状态
* CMD58
  * 读取 OCR 寄存器
  * 传输命令 `sd_cmd(0x7A, 0, 0xFD)`
  * 返回值需要为 0，表明没有异常
  * SD 卡需要已经上电好的状态
* CMD16
  * 设置块大小，我们设置块大小为 512 Bytes
  * 传输命令 `sd_cmd(0x50, 0x200, 0x15)`
  * 返回值需要为 0

## 数据块传输

我们对于 SD 卡块的读取采用了性能较好的多块读命令 CMD18，而写则仅仅采用了单块写命令 CMD24，一方面是因为多块写的稳定性不是特别好，另一方面在评测用板子上有部分 SD 卡并不支持设置块数量的 CMD23 指令。

实际上我们全部实现了单块读写和多块读写函数，根据实际表现情况选取了 CMD18 和 CMD24。

### CMD18

![img](http://elm-chan.org/docs/mmc/m/rm.png)

* 首先发送 CMD18 命令，分析命令的返回值，若不为 0 说明命令错误
* 接着不停读取字节直到读取到 `0xFE` 表明接收到读数据包的开始
* 接收 512 字节数据，2 字节 crc 码
* 持续上面的过程直到希望停止接收数据，此时需要发送 CMD12 命令
* 等待 1 到 8 个字节直到接收到命令返回值
* SD 卡将持续忙碌，此时读取的字节一定为 0。当接收到数据字节为 0xFF 后表明 SD 卡不再忙碌，本次 CMD18 命令结束

### CMD24

![img](http://elm-chan.org/docs/mmc/m/ws.png)

* 首先发送 CMD24 命令，分析命令的返回值，若不为 0 说明命令错误
* 发送超过一个字节的 0xFF 之后开始发送字节 0xFE 表明开始数据包的传输
* 传输 512 字节数据，2 字节 crc 码
* 接收数据反馈，若后返回值二进制最后5位值为5说明数据接收成功
* SD 卡将持续忙碌，此时读取的字节一定为 0。当接收到数据字节为 0xFF 后表明 SD 卡不再忙碌，本次 CMD24 命令结束

#### 总结

* 完成了 SD 卡驱动的编写，并经过大量的测试证明驱动的稳定性是非常好的
* 由于时间和技术原因并没有完成 DMA 模式下的驱动，在未来如果有时间会进行改进