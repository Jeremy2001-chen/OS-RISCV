#include <defs.h>
#include <string.h>
#include <sysfile.h>

char test_content_to_write[10] = "abcdefghi";
char test_content_to_read[10] = {0};

void testfat() {
#ifdef DEBUG
    printf("[testfat] testing fat.......\n");
#endif
    struct dirent* testfile = create("/testfile", T_FILE, O_CREATE | O_RDWR);
    if (testfile == NULL) {
        panic("[testfat] create file error\n");
    }
    int ret = ewrite(testfile, 0, (u64)test_content_to_write, 0, 9);
    if (ret != 9) {
        panic("[testfat] write file error\n");
    }
    eput(testfile);
    testfile = ename("/testfile");
    if (testfile == NULL) {
        printf("[testfat] open file error\n");
    }
    eread(testfile, 0, (u64)test_content_to_read, 0, 9);
    if (strncmp(test_content_to_write, test_content_to_read, 114514)==0) {
        printf("[testfat]  testfat passed\n");
    } else {
        printf("[testfat]  testfat failed\n");
    }
}