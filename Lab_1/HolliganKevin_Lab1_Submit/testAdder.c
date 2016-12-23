#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
        int num1 = 40;
        int num2 = 2;
        int result;
        long int x;

        x = syscall(319, num1, num2, &result);

        printf("The result is: %d\n",result);
        printf("System call return %ld\n", x);
}
