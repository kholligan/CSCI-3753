#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

static char buffer[256];

char menu(void);

int main(void)
{
	long *offset = 0;
        char command[512];
	char *filename = "/dev/simple_char_driver";
	size_t nbytes;
	ssize_t bytes_written;
	ssize_t bytes_read;
	
        int file = open(filename,O_RDWR);
		
        while(true)
        {
                char input = menu();
                if( input == 'e' )
                {
                        printf("Exiting...");
                        break;
                }
                else
                {
                        switch(input){
                                case 'r':
                                        printf("Data read from device:\n");
                                        //strcpy(command,"tail /var/log/syslog");
                                        nbytes = sizeof(buffer);
					bytes_read = read(file, buffer, nbytes);
					//read(file,buffer,1024,0);
                                        //system(command);
                                        break;
                                case 'w':
                                        printf("Enter data to write to device:\n");
                                        //fgets(buffer,1024,stdin);
					strcpy(buffer, "Test writing\n");
					nbytes = strlen(buffer);
                                        bytes_written = write(file, buffer, nbytes);
                                        break;
                                case 'e':
                                        printf("Exiting...\n\n");
                                        break;
                        }
                }
        }

        close(file);

        return 0;

}

char menu(void)
{
        //char inputChar[1024];
        char inputChar;	
	bool done = false;

        do {

                printf("Please enter a character corresponding to a menu item:\n");
                printf("Press 'r' to read from the device,\n");
                printf("Press 'w' to write to the device,\n");
                printf("Press 'e' to exit from the device:\n");

                //while ((inputChar = getchar()) != EOF && inputChar != '\n');
		//fgets(inputChar, 1024, stdin);
		
		inputChar = getchar();
		
                /*if( (strcmp(inputChar, "r\n") != 0) || (strcmp(inputChar, "w\n") != 0) || (strcmp(inputChar, "e\n")!=0) ) {
                        done = true;
                        break;
                }*/
		if( inputChar == 'w' || inputChar == 'r' || inputChar == 'e'){
			done = true;
			break;
		}
                else
                        printf("Please enter a valid choice.\n");
        }while (!done);

        return inputChar;
}

