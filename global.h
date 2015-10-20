#ifndef __GLOBAL_H_
#define __GLOBAL_H_
/**********************************************************************************
 *File: 	global.h
 *Author: 	Easa El Sirgany (easaemad14@gmail.com)
 *ASU ID:	1001361972
 *
 *Description:	Global header file for definitions that will be used by both main
 		and i2c_flash C files. For ioctl communication, as well as a few
		other shared variables, we will need to define these in both files.
 *********************************************************************************/
#include <linux/ioctl.h>

/*
 * Global Varialbes
 */
#define DEVICE_NAME "i2c_flash"
#define PG_SIZE 64
#define NUM_PAGES 512
extern int MAJOR_NUM;

/*
 * Let's define some i2c functions
 */
#define IOCTL_RW_SIZE _IOR(MAJOR_NUM, 0, int npgs)
#define IOCTL_READ _IOWR(MAJOR_NUM, 1, int start_pos)
#define IOCTL_WRITE _IOWR(MAJOR_NUM, 2, char *buffer)

#endif //__GLOBAL_H_
