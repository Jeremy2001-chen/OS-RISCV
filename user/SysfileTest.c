#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>

char message[10];
int userMain(int argc, char **argv) {
    int fd = open("/testfile", O_RDONLY);
    printf("fd1=%d\n", fd);
    char buf[10]={0};
    int n = read(fd, buf, sizeof(buf));
    printf("[reading file %d bytes]: %s\n", n, buf);

    int fd2 = open("/createfile", O_WRONLY|O_CREATE_GLIBC);
    printf("fd2=%d\n", fd2);
    char to_write[]="testcreatefile";
    n = write(fd2, "testcreatefile", sizeof(to_write));
    printf("[writing file %d bytes]\n", n);

    close(fd2);

    fd2 = open("/createfile", O_RDONLY);
    char to_read[sizeof(to_write)+1];
    n =  read(fd2, to_read, sizeof(to_read));
    printf("[reading file %d bytes]: %s\n", n, to_read);
    for(;;);
}