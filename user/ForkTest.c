#include <Syscall.h>

void userMain(void) {
	int id = 0;
	volatile int i;
	if ((id = syscallFork()) == 0) {
		if ((id = syscallFork()) == 0) {
			for (i = 0; i < 500; i++) {
				syscallPutchar('a');
			}
		} else {
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
