#include <stdio.h>

void set_ptr(int *yabo, bool b, int ddd)
{
    char w[10];
    w[0] = 4;
    int lll = *yabo;
    int *x;
    int zzz = ddd;
    // set_ptr top
    if (b)
    {
        // set_ptr branch
        x = yabo;
	int y = *x;
    }
    else
    {
	x = yabo;
	int y = *x;
    }
    int *ptr3 = x;
    int y = *ptr3;
}

int f2()
{
	char j = 0;
	int w[10];
	for(int i=0; i < 10; ++i)
	{
		w[i] = 45;
	}
	return w[0];
}

int main()
{
	char x[2];
	int y = x[3];
	return 0;
}
