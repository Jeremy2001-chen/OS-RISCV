#include <Syscall.h>
#include <Printf.h>
#include <userfile.h>
#include <uLib.h>

char* fmtname(char* name) {
    static char buf[STAT_MAX_NAME + 1];
    int len = strlen(name);

    // Return blank-padded name.
    if (len >= STAT_MAX_NAME)
        return name;
    memmove(buf, name, len);
    memset(buf + len, ' ', STAT_MAX_NAME - len);
    buf[STAT_MAX_NAME] = '\0';
    return buf;
}

const char * stat_map[]={
  [T_DIR]   "T_DIR",
  [T_FILE]  "T_FILE",
  [T_DEVICE]"D_DEVICE"
};

void ls(char* path) {
    int fd;
    struct stat st;

    MSG_PRINT("before ls");
    if ((fd = syscallOpen(path, 0)) < 0) {
        printf("ls: cannot open %s\n", path);
        return;
    }
    DEC_PRINT(fd);
    if (syscallFstat(fd, &st) < 0) {
        printf("ls: cannot stat %s\n", path);
        syscallClose(fd);
        return;
    }
    MSG_PRINT("finish Fstat");
    if (st.type == T_DIR) {
        while (syscallReaddir(fd, &st) == 1) {
            printf("%s %s\t\t\t%d\n", fmtname(st.name), stat_map[st.type], st.size);
        }
    } else {
        printf("%s %s\t\t\t%d\n", fmtname(st.name), stat_map[st.type], st.size);
    }
    syscallClose(fd);
}

int userMain(int argc, char **argv) {
    printf("[ls test]\n");
    printf("argc=%d\n",argc);
    for(int i=0;i<argc;++i){
        printf("%s ",argv[i]);
    }
    printf("\n");
    int i;

    if (argc < 2) {
        ls(".");
        return 0;
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    return 0;
}
