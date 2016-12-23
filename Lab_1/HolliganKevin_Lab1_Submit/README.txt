Name: Kevin Holligan

Phone: (408) 465-6332

Email: kevin.holligan@colorado.edu // kholligan@gmail.com



Files:

Makefile - This is the makefile for the linux kernel. It includes all the relevant object files, including the added helloworld.o and simple_add.o This file is located in the Linux folder in arch/x86/kernel/

simple_add.c - This is the c code for the system call kernel instruction. It takes two numbers and a pointer. It adds the two numbers and stores them in the pointer, and prints the numbers and the result to the system log, then returns 0. This file is located in the Linux folder in arch/x86/kernel/

syscall_64.tbl - This is the 64 bit system call table. This is where the kernel looks up the system call reference number to know which syscall file to execute. 318 refers to the added helloworld and 319 refers to simple_add. This file is located in the Linux folder in arch/x86/syscalls/

syscalls.h - This is a header declaration file. It tells the kernel what the system call parameters are. This file is in the Linux folder in include/linux/

syslog.txt - This is a dump of the system log, which includes the results of calling simple_add. This is located in /var/log/
testAdder.c - This is the test program to test simple_add. It generates two numbers and passes them through the syscall. Then it prints the result of the numbers received stored in the pointer and prints the output of the syscall (0 when successful). 