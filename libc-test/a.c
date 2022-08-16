#include <stdio.h>

// /musl-gcc/bin/gcc a.c -static
// ./a.out
int main() {
    int a, b;
    scanf("%d%d", &a, &b);
    printf("a+b=%d\n", a + b);
    return 0;
}
