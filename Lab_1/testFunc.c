#include <stdio.h>

int testAdd(int num1, int num2, int *result)
{
	int add;
	add = num1 + num2;
	*result = add;
	return 0;
}

int main()
{
	int n1 = 40;
	int n2 = 2;
	int result;
	int x;

	x = testAdd(n1,n2, &result);

	printf("The result is: %d\n",result);
        printf("System call return %d\n", x);
}
