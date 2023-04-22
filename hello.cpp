#include <stdio.h>
#include <sanitizer/asan_interface.h>

void set_ptr(int *yabo, int *qabo, bool b)
{
	int *x;
	int y = 0;
    // set_ptr top
    if (b)
    {
        // set_ptr branch
        x = yabo;
	y = *x;
    }
    else
    {
	x = yabo;
	y = *x;
    }
    int *ptr3 = x;
    y = *ptr3;
}

/*int f2()
{
	char j = 0;
	int w[10];
	for(int i=0; i < 10; ++i)
	{
		w[i] = 45;
	}
	return w[0];
}*/

int main()
{
	int p[4];
	for(int i=0; i<4; i++)
	{
		p[i] = i;
	}

	//int *lll = new int;
	//*lll = 4444444;
	/*void *y = __asan_region_is_poisoned(p, 4*sizeof(int));
	if(y != nullptr)
	{
		printf("bad\n");
	}*/
	//delete[] p;
	//delete lll;

	
	return 0;
}

