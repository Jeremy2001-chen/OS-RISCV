kernel_dir	:= 	kernel
linker_dir	:= 	linkscript
user_dir	:= 	user
target_dir	:= 	target
driver_dir	:= 	$(kernel_dir)/driver
memory_dir	:= 	$(kernel_dir)/memory
boot_dir	:=	$(kernel_dir)/boot
init_dir	:=	$(kernel_dir)/init

linkscript	:= 	$(linker_dir)/Qemu.ld
vmlinux_img	:=	$(target_dir)/vmlinux.img

modules := 	$(kernel_dir) $(user_dir)
objects	:=	$(boot_dir)/*.o \
			$(init_dir)/*.o \
			$(driver_dir)/*.o \
			$(memory_dir)/*.o
			
#$(user_dir)/*.x

.PHONY: build clean $(modules) run

build: $(modules)
	$(LD) $(LDFLAGS) -T $(linkscript) -o $(vmlinux_img) $(objects)

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(target_dir)/*

run: clean build
	$(QEMU) -kernel $(vmlinux_img) $(QEMUOPTS)

include include.mk
