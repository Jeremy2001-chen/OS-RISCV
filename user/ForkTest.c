#include <Syscall.h>

void userMain(void) {
	int id = 0;
	int i;
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
		for (i = 0; i < 500; i++) {
			syscallPutchar('c');
		}
	}
}
