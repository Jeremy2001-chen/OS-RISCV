kernel_dir	:= 	kernel
linker_dir	:= 	linkscript
user_dir	:= 	user
target_dir	:= 	target
utility_dir	:=  utility
driver_dir	:= 	$(kernel_dir)/driver
memory_dir	:= 	$(kernel_dir)/memory
boot_dir	:=	$(kernel_dir)/boot
trap_dir	:=	$(kernel_dir)/trap
lock_dir    :=  $(kernel_dir)/lock
system_dir  :=  $(kernel_dir)/system

linkscript	:= 	$(linker_dir)/Qemu.ld
vmlinux_img	:=	$(target_dir)/vmlinux.img
vmlinux_bin :=  os.bin
vmlinux_asm	:= 	$(target_dir)/vmlinux_asm.txt
disk_img	:=  $(utility_dir)/disk.img

modules := 	$(kernel_dir) $(user_dir)
objects	:=	$(boot_dir)/*.o \
			$(driver_dir)/*.o \
			$(memory_dir)/*.o \
			$(trap_dir)/*.o \
			$(lock_dir)/*.o \
			$(system_dir)/*.o \
			$(user_dir)/*.x

.PHONY: build clean $(modules) run

all : build

build: $(modules)
	mkdir -p $(target_dir)
	$(LD) $(LDFLAGS) -T $(linkscript) -o $(vmlinux_img) $(objects)
	$(OBJDUMP) -S $(vmlinux_img) > $(vmlinux_asm)
	$(OBJCOPY) -O binary $(vmlinux_img) $(vmlinux_bin)
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \

sifive: clean build
	$(OBJCOPY) -O binary $(vmlinux_img) /srv/tftp/vm.bin

fs:
	dd if=$(disk_img) of=target/disk.img bs=4096 count=512

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(target_dir)/*

run: clean build fs
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS)

asm: clean build fs
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -d in_asm

int: clean build fs
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -d int

gdb: clean build fs
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -nographic -s -S

include include.mk
