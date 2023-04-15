#include <stdio.h>

int main()
{
    char x[2] = {'a', 'b'};
    for (int i = 0; i < 100000; ++i)
    {
        printf("hello world. magic number: %d\n", x[0]);
    }
    return 0;
}
