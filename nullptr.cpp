#include <sanitizer/asan_interface.h>
#include <stdio.h>

__attribute__((no_sanitize("address")))
int main()
{
    int *x = nullptr;
    printf("Magic number: %d\n", *x);
    return 0;
}
