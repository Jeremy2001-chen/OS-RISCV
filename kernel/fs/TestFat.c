#include <Defs.h>
#include <string.h>
#include <Sysfile.h>
#include <Debug.h>

char test_content_to_write[10] = "abcdefghi";
char test_content_to_read[10] = {0};

void testfat() {
    printf("[testfat] testing fat.......\n");
    Dirent* testfile = create(AT_FDCWD, "/testfile", T_FILE, O_CREATE_GLIBC | O_RDWR);
    if (testfile == NULL) {
        panic("[testfat] create file error\n");
    }
    printf("create file finish\n");
    int ret = ewrite(testfile, 0, (u64)test_content_to_write, 0, 9);
    if (ret != 9) {
        panic("[testfat] write file error\n");
    }

    printf("write file finish\n");
    testfile = ename(AT_FDCWD, "/testfile", true);
    if (testfile == NULL) {
        panic("[testfat] open file error\n");
    }
    eread(testfile, 0, (u64)test_content_to_read, 0, 9);
    if (strncmp(test_content_to_write, test_content_to_read, 114514)==0) {
        printf("[testfat]  testfat passed\n");
    } else {
        panic("[testfat]  testfat failed\n");
    }
}