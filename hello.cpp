#include <stdio.h>

void set_ptr(int *ptr)
{
    // set_ptr top
    *ptr = 10;
    if (*ptr % 2 == 0)
    {
        // set_ptr branch
        *ptr = 11;
    }
}

int main()
{
    int *num = new int(5);
    set_ptr(num);
    return 0;
}
