#include <defs.h>
#include <string.h>
#include <Sysfile.h>
#include <debug.h>

char test_content_to_write[10] = "abcdefghi";
char test_content_to_read[10] = {0};

void testfat() {
#ifdef ZZY_DEBUG
    printf("[testfat] testing fat.......\n");
#endif
    struct dirent* testfile = create("/testfile", T_FILE, O_CREATE | O_RDWR);
    if (testfile == NULL) {
        panic("[testfat] create file error\n");
    }
    MSG_PRINT("create file finish");
    int ret = ewrite(testfile, 0, (u64)test_content_to_write, 0, 9);
    if (ret != 9) {
        panic("[testfat] write file error\n");
    }

    MSG_PRINT("write file finish");
    eunlock(testfile);
    eput(testfile);
    MSG_PRINT("eput file finish");
    testfile = ename("/testfile");
    if (testfile == NULL) {
        printf("[testfat] open file error\n");
    }
    DEC_PRINT(testfile->ref);
    eread(testfile, 0, (u64)test_content_to_read, 0, 9);
    if (strncmp(test_content_to_write, test_content_to_read, 114514)==0) {
        printf("[testfat]  testfat passed\n");
    } else {
        printf("[testfat]  testfat failed\n");
    }
}