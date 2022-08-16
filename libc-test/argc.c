#include <stdio.h>
#include <stdlib.h>

// /musl-gcc/bin/gcc a.c -static
// ./a.out
int main(int argc, char* argv) {
    int a, b;
    FILE* file = fopen("argc.out", "w");
    if (file == NULL) {
        printf("No such file!\n");
        exit(0);
    }
    printf("argc=%d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("%lx\n", argv[i]);
        printf("%s\n", argv[i]);
    }
    exit(0);
    fclose(file);
    return 0;
}
