#include<stdio.h>

void main()
{
	int i,j,prod;
	for(i=1;i<11;i++)
	{
		printf("\n'%d'Multiplication table", i);
		for(j=1;j<11;j++)
		{
		    prod=i*j;
		    printf("\n%d x %d = %d", i, j, prod);
		}
		printf("\n\n");
	}

}
