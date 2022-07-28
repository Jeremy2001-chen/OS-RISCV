INCLUDES := -I../../include

target = $(shell ls *.c *.S | awk '{gsub(/\.c|\.S/, ".o"); print $0}')

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk