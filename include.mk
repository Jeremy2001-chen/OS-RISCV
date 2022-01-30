CROSS_COMPILE	:= riscv64-unknown-elf-
GCC				:= $(CROSS_COMPILE)gcc
CFLAGS 	= -Wall -Werror -O -fno-omit-frame-pointer -ggdb -g
CFLAGS 	+= -MD
CFLAGS 	+= -mcmodel=medany
CFLAGS 	+= -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS 	+= -I.
CFLAGS 	+= $(shell $(GCC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
OBJDUMP			:= $(CROSS_COMPILE)objdump
LD				:= $(CROSS_COMPILE)ld
LDFLAGS			:= -z max-page-size=4096
QEMU			:= qemu-system-riscv64

RUSTSBI			:=	./bootloader/sbi-qemu

CPUS := 2

QEMUOPTS = -machine virt -m 8M -nographic
QEMUOPTS += -smp $(CPUS)
QEMUOPTS += -bios $(RUSTSBI)
#QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0 
#QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0