#include <stdio.h>

/**
 * A loop with a frequent path store and an infrequent path which changes the
 * address of the store instruction. There should be no invalid accesses.
 */
int main()
{
    int A[100];
    int j = 0;
    for (int i = 0; i < 1000000000; ++i)
    {
        if (i % 10000000 == 5000000)
        {
            j = (i / 10000000) % 100;
        }
        A[j] = -1;
    }
}
