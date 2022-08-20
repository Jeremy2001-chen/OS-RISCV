kernel_dir	:= 	kernel
linker_dir	:= 	linkscript
user_dir	:= 	user
target_dir	:= 	target
utility_dir	:=	utility

linkscript	:= 	$(linker_dir)/Qemu.ld
vmlinux_img	:=	$(target_dir)/vmlinux.img
vmlinux_bin	:=	os.bin
vmlinux_asm	:= 	$(target_dir)/vmlinux_asm.txt
dst		:=	/mnt
fs_img		:=	fs.img

modules := 	$(kernel_dir) $(user_dir)
objects := $(kernel_dir)/*/*.o $(user_dir)/*.x

.PHONY: build clean $(modules) run
GDB := n

ifeq ($(MAKECMDGOALS), gdb)
	GDB = y
endif

all : build

build: $(modules)
	mkdir -p $(target_dir)
	$(LD) $(LDFLAGS) -T $(linkscript) -o $(vmlinux_img) $(objects)
	$(OBJDUMP) -S $(vmlinux_img) > $(vmlinux_asm)
	$(OBJCOPY) -O binary $(vmlinux_img) $(vmlinux_bin)

sifive: clean build
	$(OBJCOPY) -O binary $(vmlinux_img) /srv/tftp/vm.bin
	
fat: $(user_dir)
	if [ ! -f "$(fs_img)" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=$(fs_img) bs=512k count=256; fi
	mkfs.vfat -F 32 $(fs_img); 
	@sudo mount $(fs_img) $(dst)
	# @if [ ! -d "$(dst)/bin" ]; then sudo mkdir $(dst)/bin; fi
	# @sudo cp README $(dst)/README
	@sudo cp -r user/mnt/* $(dst)/
	# @sudo cp -r home $(dst)/
	@sudo cp -r root/** $(dst)/
	# @for file in $$( ls user/_* ); do \
	# 	sudo cp $$file $(dst)/$${file#$U/_};\
	# 	sudo cp $$file $(dst)/bin/$${file#$U/_}; done
	@sudo umount $(dst)

new-lib:
	cd ../SysY-compiler && riscv64-unknown-elf-g++ -static main.cpp -o my-gcc && cp my-gcc ../OS-RISCV/libc-test

new: clean new-lib fat run

umount:
	sudo umount $(dst)

mount:
	sudo mount -t vfat $(fs_img) $(dst)
	
$(modules):
	$(MAKE) build GDB=$(GDB) --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(target_dir)/*

comp: clean build
	$(QEMU) -kernel $(vmlinux_img) $(BOARDOPTS)

run: clean build
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS)

asm: clean buil.d
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -d in_asm

int: clean build
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -d int

gdb: clean build
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS) -nographic -s -S

include include.mk
