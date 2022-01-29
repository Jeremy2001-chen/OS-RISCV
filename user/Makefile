INCLUDES	:=	-I../include/

USER_LIB	:=

.PHONY:	build clean

build: $(USER_LIB)

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *.o *~

include ../include.mk
