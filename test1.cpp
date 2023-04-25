#include <stdio.h>
#include <stdlib.h>

int main()
{
	int n = 1000000000;
	int *p = (int*)malloc(n*sizeof(int));
	for(int i=0; i<n; ++i)
	{
		p[i] = i;
	}
	free(p);
	return 0;
}
