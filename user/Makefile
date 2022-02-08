INCLUDES	:=	-I../include/

USER_LIB	:= Libos.o

USER_TARGET	:= Test.x

.PHONY:	build clean

build: $(USER_LIB) $(USER_TARGET)

%.x: %.b.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	chmod +x bintoc
	./bintoc $* $< > $@~ && mv -f $@~ $@

%.b: Entry.o %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/User.ld $^

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~ *.d

include ../include.mk
