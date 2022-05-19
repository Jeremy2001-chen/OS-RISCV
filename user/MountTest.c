#include <Printf.h>
#include <Syscall.h>
#include <uLib.h>
#include <userfile.h>

char *__argv[]={0};

int userMain(int argc, char** argv) {
    int rt, fd;

    rt = mount("fs.img", "/mnt", "vfat", 0, 0);
    assert(rt != -1);
    char buf[20]={0};
    fd = open("/mnt/1.txt", O_RDONLY);
    read(fd, buf, sizeof(buf));
    printf("%s\n", buf);
    return 0;
}