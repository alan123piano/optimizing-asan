#include <stdio.h>

int main()
{
    // induce a segfault
    int *x = nullptr;
    printf("hello world. magic number: %d\n", *x);
    return 0;
}
