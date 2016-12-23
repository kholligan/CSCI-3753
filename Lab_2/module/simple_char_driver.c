#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024
#define DEVICE_NAME "simple_char_driver"


static char device_buffer[BUFFER_SIZE];

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace 		buffer *buffer */
	unsigned long bytes = 0;
	
	bytes = sizeof(buffer);	

	copy_to_user(buffer, &device_buffer, bytes);



	(*offset) += bytes;
	printk(KERN_ALERT "Read: %s", device_buffer);
	printk(KERN_ALERT "Number of bytes read from file: %ld\n", bytes);

	return 0;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */

	unsigned long bytes = length;
	
	copy_from_user(&device_buffer, buffer, bytes);
	
	printk(KERN_ALERT "Wrote: %s", device_buffer);
        printk(KERN_ALERT "Number of bytes written to file: %ld\n", bytes);

	return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	static int count = 0;
	printk(KERN_ALERT "The file is open\n");
	printk(KERN_ALERT "The file has been opened %d times before\n",count++);
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	static int count = 0;
	printk(KERN_ALERT "The file is closed\n");
        printk(KERN_ALERT "The file has been closed %d times before\n",count++);
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
	.open = simple_char_driver_open,
	.release = simple_char_driver_close,
	.read = simple_char_driver_read,
	.write = simple_char_driver_write
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_ALERT "The %s function was called\n",__FUNCTION__);
	/* register the device */
	register_chrdev(240,DEVICE_NAME,&simple_char_driver_file_operations);
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	printk(KERN_ALERT "The %s function was called\n",__FUNCTION__);
	/* unregister  the device using the register_chrdev() function. */
	unregister_chrdev(240, DEVICE_NAME);
	return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
