#include <stdio.h>
#include <sanitizer/asan_interface.h>

//void *__asan_region_is_poisoned(void *beg, size_t size);

void foo(int *ptr)
{
	int x = *ptr;
	if(*ptr % 2 == 0)
		*ptr = 5;
}

int set_ptr(int *yabo, int *qabo, bool b)
{

	int *x;
	qabo = yabo;
	int y = 0;
	(*yabo)++;
    // set_ptr top
    if (b)
    {
        // set_ptr branch
        x = yabo;
	y = *x;
	*x = 45;
    }
    else
    {
	x = qabo;
	y = *x;
	*x = 750;
    }
    int *ptr3 = x;
    y = *ptr3;
    return y;
}

/*int f2()
{
	int w[10];
	for(int i=0; i < 10; ++i)
	{
		w[i] = 45;
	}
	return w[0];
}*/

int main()
{
	/*long long *p = new long long[4];
	for(long long i=0; i<4; ++i)
	{
		p[i] = i;
	}
	delete[] p;*/
	char p[10];
	p[0] = p[0] + 1; 
	for(int i=0; i<11; ++i)
	{
		++p[i];
	}


	/*int *r = new int(7777);
	int *d = new int[4];
	d[0] = 55;
	for(int i=0; i<4; ++i)
	{
		d[i] = i;
	}
	delete[] d;
	delete r;*/
	
	//*d = 4444444;
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

