#include <Printf.h>
#include <Syscall.h>
#include <SyscallLib.h>
#include <uLib.h>
#include <userfile.h>
#include <LibMain.h>
typedef long Align;

union header {
    struct {
        union header* ptr;
        uint size;
    } s;
    Align x;
};

typedef union header Header;

void free(void* ap);

void* malloc(uint nbytes) ;