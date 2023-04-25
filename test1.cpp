#include <stdio.h>

int main()
{
	int n = 1000000000;
	int *p = new int[n];
	for(int i=0; i<n; ++i)
	{
		p[i] = i;
	}
	delete[] p;
	return 0;
}
