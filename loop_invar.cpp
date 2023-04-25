#include <stdio.h>
#include <stdlib.h>

/**
 * A loop with a store to an invariant address.
 */
int main()
{
    int *a = (int *)malloc(sizeof(int));
    for (int i = 0; i < 2000000000; ++i)
    {
        *a = i % 100;
    }
    free(a);
}
