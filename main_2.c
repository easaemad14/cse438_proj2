/**********************************************************************************
 *File: 	main_2.c
 *Author: 	Easa El Sirgany (easaemad14@gmail.com)
 *ASU ID:	1001361972
 *
 *Description:	This is the test project for our I2C character device. The tests
 		that are performed here are created by myself in order to test my
		work (there was no suggestions on the testing implementation on
		the instruction pdf.
 *********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "global.h"

/*
 * IOCTL Functions called from main()
 */
ioctl_erase_EEPROM(int fild){
	int i, ret;
	char buff[PG_SIZE];
	flush_buffer(buff, PG_SIZE);

	for(i = 0; i < NUM_PAGES; ){
		ret = ioctl(fild, IOCTL_WRITE, buff);

		if(!ret)
			i++;
		else{
			printf("Unable to write from userspace!\n");
			printf("Trying again in one usecond.\n");
			sleep(1);
		}
	}
}

ioctl_write_london(int fild){
	//Throw some works on my queue
	//No need to change file position, it is automated in the reading and writing process
	if(ioctl(fild, IOCTL_WRITE, "London") < 0)
		printf("Unable to write \"London\" from userspace!");
	if(ioctl(fild, IOCTL_WRITE, "o") < 0)
		printf("Unable to write 'o' from userspace!");
	if(ioctl(fild, IOCTL_WRITE, "n") < 0)
		printf("Unable to write 'n' from userspace!");
	if(ioctl(fild, IOCTL_WRITE, "d") < 0)
		printf("Unable to write 'd' from userspace!");
	if(ioctl(fild, IOCTL_WRITE, "o") < 0)
		printf("Unable to write 'o' from userspace!");
	if(ioctl(fild, IOCTL_WRITE, "n") < 0)
		printf("Unable to write 'n' from userspace!");
}

ioctl_read_london(int fild, int npg){
	int i, ret;
	char buff[PG_SIZE];

	for(i = 0; i < npg; i++){
		ret = ioctl(fild, IOCTL_READ, &buff);

		if(!ret){
			printf("%s\n", buff);
			i++;
		}
		else{
			printf("Unable to read from kernel space!\n");
			printf("Trying again in one usecond.\n");
			sleep(1);
		}
	}
}

int main(){
	int fd;

	fd = open("/dev/i2c_flash", O_RDWR);
	if(fd < 0){
		printf("Can't open i2c_flash device!\n");
		return -1;
	}

	//Test my program with ioctl functions
	ioctl_erase_EEPROM(fd);
	if(lseek(fd, 0, SEEK_SET) < 0)
		printf("ERROR at lseek in main!\n");
	ioctl_write_london(fd);
	ioctl_read_london(fd, 6);

	return 0;
}
