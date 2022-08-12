#include <Debug.h>
#include <SyscallId.h>
#include <Driver.h>
#include <Type.h>

void getSyscallMessage(int id, u64 pc) {
    switch (id)
    {
        case SYSCALL_CLONE:
            printf("syscall Clone, pc=%lx\n", pc);
            break;
        case SYSCALL_PIPE2:
            printf("syscall pipe2, pc=%lx\n", pc);
            break;
        case SYSCALL_PROCESS_KILL:
            printf("syscall process kill, pc=%lx\n", pc);
            break;
        case SYSCALL_THREAD_KILL:
            printf("syscall thread kill, pc=%lx\n", pc);
            break;
        default:
            break;
    }
}