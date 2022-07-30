driver_dir	:= driver
memory_dir	:= memory
boot_dir	:= boot
trap_dir	:= trap
lock_dir	:= lock
system_dir	:= system
utility_dir	:= utility
fs_dir		:= fs
yield_dir	:= yield
modules		:= $(shell ls -d */)

.PHONY:	build clean $(modules)

build: $(modules)

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../include.mk
