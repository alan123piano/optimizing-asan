#include <stdio.h>

/**
 * A loop with a frequent path store and an infrequent path which changes the
 * address of the store instruction. The infrequent path may generate out-of-
 * bounds indices for the array.
 *
 * If argc > 1, then the program will not issue out-of-bounds instructions.
 * This is so that profiling can be executed properly.
 */
int main(int argc, char *argv[])
{
    int A[100];
    int j = 0;
    for (int i = 0; i < 1000000000; ++i)
    {
        A[j] = -1;
        if (i % 10000000 == 9000000)
        {
            j = (i / 10000000) % 100;
            if (argc <= 1)
                j += 100;
        }
    }
}
