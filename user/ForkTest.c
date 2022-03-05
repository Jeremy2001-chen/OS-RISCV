#include <Syscall.h>
#include <Fork.h>

void userMain(void) {
	int id = 0;
	volatile int i;
	if ((id = fork()) == 0) {
		if ((id = fork()) == 0) {
			syscallPutchar('m');
			for (i = 0; i < 500; i++) {
				syscallPutchar('a');

			}
		} else {
			syscallPutchar('o');
			for (i = 0; i < 500; i++) {
				syscallPutchar('b');

			}
		}
	} else {
        syscallPutchar('n');
		for (i = 0; i < 500; i++) {
            syscallPutchar('c');
		}
	}
}
