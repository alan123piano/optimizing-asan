#include <stdio.h>

/**
 * A loop with a frequent path store and an infrequent path which changes the
 * address of the store instruction. The infrequent path may generate out-of-
 * bounds indices for the array.
 */
int main()
{
    int A[100];
    int j = 0;
    for (int i = 0; i < 1000000000; ++i)
    {
        A[j] = -1;
        if (i % 10000000 == 9000000)
        {
            j = (i / 10000000) % 100 + 100;
        }
    }
}
