CROSS_COMPILE	:= riscv64-unknown-elf-
GCC		:= $(CROSS_COMPILE)gcc
CFLAGS 	= -Wall -Werror -O -fno-omit-frame-pointer -ggdb -g 
# CFLAGS  += -mabi=lp64
CFLAGS 	+= -MD
CFLAGS 	+= -mcmodel=medany
CFLAGS 	+= -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS 	+= -I.
CFLAGS 	+= $(shell $(GCC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

OBJDUMP := $(CROSS_COMPILE)objdump
OBJCOPY	:= $(CROSS_COMPILE)objcopy
LD 	:= $(CROSS_COMPILE)ld
LDFLAGS	:= -z max-page-size=4096
QEMU	:= qemu-system-riscv64
OPENSBI	:=./bootloader/dynamic.bin
CPUS 	:= 5
OPTS 	= -machine sifive_u -m 1G -nographic
OPTS 	+= -smp $(CPUS)
OPTS 	+= -bios $(OPENSBI)

BOARDOPTS 	= $(OPTS) -drive file=sdcard.img,if=sd,format=raw 
QEMUOPTS 	= $(OPTS) -drive file=fs.img,if=sd,format=raw 
#QEMUOPTS 	+= -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0