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
#include "global.h"

int main(){
	int fd, ret, i;
	char buff[PG_SIZE];
	memset(buff, 0, PG_SIZE);

	fd = open("/dev/%s", DEVICE_NAME);
	if(fd < 0){
		printf("Can't open device: %s!\n", DEVICE_NAME);
		return -1;
	}

	//First thing I want to do is write all zeros to the EEPROM
	for(i = 0; i < NUM_PAGES;){
		ret = write(fd, buff, sizeof(buff));

		if(!ret)
			i++;
		else{
			printf("Unable to write from userspace!\n");
			pritnf("Trying again in one usecond.\n");
			sleep(1);
		}
	}

	//Next, let's write "London" horizontally and vertically
	ret = lseek(fd, 0, SEEK_SET); //Go back to the first line

	ret = write(fd, "London", 6);
	ret = write(fd, "o", 1);
	ret = write(fd, "n", 1);
	ret = write(fd, "d", 1);
	ret = write(fd, "o", 1);
	ret = write(fd, "n", 1);

	//Now let's read our beautiful map (six pages)
	ret = lseek(fd, 0, SEEK_SET);
	ret = read(fd, buff, 6);
	buff[6] = '\0'; //So we don't overflow
	printf("%s\n", buff);

	memset(buff, '\0', sizeof(buff));
	ret = read(fd, buff, 1); //o
	printf("%c\n", buff[0]);
	ret = read(fd, buff, 1); //n
	printf("%c\n", buff[0]);
	ret = read(fd, buff, 1); //d
	printf("%c\n", buff[0]);
	ret = read(fd, buff, 1); //o
	printf("%c\n", buff[0]);
	ret = read(fd, buff, 1); //n
	printf("%c\n", buff[0]);

	return 0;
}
