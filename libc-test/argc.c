#include <stdio.h>
#include <stdlib.h>

// /musl-gcc/bin/gcc a.c -static
// ./a.out
int main(int argc, char** argv) {
    int a, b;
    FILE* file = fopen("argc.out", "w");
    if (file == NULL) {
        printf("No such file!\n");
        exit(0);
    }
    fprintf(file, "argc=%d\n", argc);
    for (int i = 0; i < argc; i++) {
        fprintf(file, "argv[%d]: %s\n", i, argv[i]);
    }
    fclose(file);
    return 0;
}
