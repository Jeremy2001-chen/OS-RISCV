INCLUDES := -I../../include

target = KernelVector.o Process.o Trap.o Timer.o Trampoline.o ElfLoader.o Syscall.o Sysarg.o Sleep.o exec.o Thread.o Yield.o Clone.o

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk