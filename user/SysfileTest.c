#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>

char message[10];
int userMain(int argc, char **argv) {
    int fd = syscallOpen("/testfile", O_RDONLY);
    printf("fd1=%d\n", fd);
    char buf[10]={0};
    int n = syscallRead(fd, buf, sizeof(buf));
    printf("[reading file %d bytes]: %s\n", n, buf);

    int fd2 = syscallOpen("/createfile", O_WRONLY|O_CREATE);
    printf("fd2=%d\n", fd2);
    char to_write[]="testcreatefile";
    n = syscallWrite(fd2, "testcreatefile", sizeof(to_write));
    printf("[writing file %d bytes]\n", n);

    syscallClose(fd2);

    fd2 = syscallOpen("/createfile", O_RDONLY);
    char to_read[sizeof(to_write)+1];
    n =  syscallRead(fd2, to_read, sizeof(to_read));
    printf("[reading file %d bytes]: %s\n", n, to_read);
    for(;;);
}