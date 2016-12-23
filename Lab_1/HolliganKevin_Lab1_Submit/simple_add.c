#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_simple_add(int number1, int number2, int *result)
{
	int add;
	printk(KERN_ALERT "Number1: %d\n",number1);
	printk(KERN_ALERT "Number2: %d\n",number2);
	add = number1 + number2;
	*result = add;
	return 0;
}
