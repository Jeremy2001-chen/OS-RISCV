#include <stdio.h>
#include <stdlib.h>

int main() {
    int a, b;
    FILE* file = fopen("a_plus_b.in", "r");
    if (file == NULL) {
        printf("No such file!\n");
        exit(0);
    }
    fscanf(file, "%d%d", &a, &b);
    printf("a+b=%d\n", a + b);
    fclose(file);
    return 0;
}
