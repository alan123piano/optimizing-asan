#include <sanitizer/asan_interface.h>
#include <stdio.h>

// __attribute__((no_sanitize("address")))
void print(long long int int_ptr)
{
    int *num = (int *)(int_ptr);
    printf("hello world. magic number: %d\n", *num);
    int *num2 = (int *)(int_ptr);
    printf("magic number 2: %d\n", *num2);
}

int main()
{
    int *num = new int(101);
    print((long long int)num);
    return 0;
}
