#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

long helloworld_syscall(void)
{
	return syscall(318);
}

int main(int argc, char *argv[])
{
	long int x = helloworld_syscall();
	printf("System call return %ld\n", x);
	return 0;

}
