INCLUDES	:=	-I./include/

USER_LIB	:= LibMain.o

USER_TARGET	:= ProcessA.x ProcessB.x ForkTest.x

.PHONY: bintoc build clean

bintoc:
	gcc BinaryToC.c -o BinaryToC
	chmod +x BinaryToC

build: bintoc $(USER_LIB) $(USER_TARGET)

%.x: %.b.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	./BinaryToC -f $< -o $@

%.b: Entry.o %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/User.ld $^
	$(OBJDUMP) -D $@ > $@.txt

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~ *.d *.x *.b.txt BinaryToC

include ../include.mk
