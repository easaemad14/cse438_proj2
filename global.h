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
 * No project is complete without macros!
 */
#define flush_buffer(B, S) memset(B, '\0', S)

/*
 * Global Varialbes
 */
#define PG_SIZE 64
#define NUM_PAGES 512

/*
 * VERY IMPORTANT:
 * We cannot rely on dynamic allocation of our major number, because the user
 * space program needs to be able to know what this is.
 */
#define MAJOR_NUM 100

/*
 * Let's define some i2c functions
 */
#define IOCTL_READ _IOWR(MAJOR_NUM, 0, char *)
#define IOCTL_WRITE _IOWR(MAJOR_NUM, 1, char *)

#endif //__GLOBAL_H_
