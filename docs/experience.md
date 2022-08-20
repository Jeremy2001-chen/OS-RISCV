# 经验分享

## GDB 调试方式

* 启动两个 `shell`
* 一个 `shell` 执行 `make gdb`，等待通信
* 另一个 `shell` 执行 `gdb-multiarch target/vmlinux.img` 加载符号表

```shell
(gdb) target extended-remote localhost:1234
(gdb) add-inferior
(gdb) inferior 2
(gdb) attach 2
(gdb) set schedule-multiple on
(gdb) c
```

## 软件移植

当我们需要将软件从其他体系结构移植到我们的 RISC-V 内核上时，一定是需要进行非常多的适配的，因此在这里我们给出移植过程中的一些方法，希望可以给大家一个启发。

最理想的方式是我们可以使用本地 `gcc` 编译程序的需要移植的程序，这种好处是非常显而易见的，因为我们可以比较容易的得到当前文件的控制流，最便捷的方式是在代码中插入下面的代码：

```c
printf("%s %d\n", __FILE__, __LINE__);
```

可以将这一行代码变为宏，这有助于我们的调试。

当我们不容易将代码直接编译时，我们可以先尝试将该软件的可执行文件跑在同样为 RISC-V 的虚拟机环境上（在这次大赛上我们使用了提供的 Debian 虚拟机），之后可以使用虚拟机上的 `strace` 命令，通过与本机系统调用参数以及返回值的对比找到背后的问题所在。