/*******************************************************************************************************
 *File: 	i2c_flash.h
 *Author: 	Easa El Sirgany (easaemad14@gmail.com)
 *ASU ID:	1001361972
 *
 *Description:	Header file for the I2C device file.
 ******************************************************************************************************/
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>		// No fragmentations in our memory allocation ;)
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/init.h>

//GPIO libs
#include <linux/gpio.h>
#include <asm/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

//Work Queue
#include<linux/workqueue.h>

/*
 * Variable definitions
 */
#define DEVICE_NAME "i2c_flash"
#define WQ_NAME "i2c_workqueue"
#define DEVICE_ADDR 0x54
#define I2CMUX 29 //gpio29 (see schematic image)
#define LED_PIN 26
#define PG_SIZE 64
#define NUM_PAGES 512

//Our character device for read/write ops to EEPROM
typedef struct{
	struct work_struct my_work;
	struct file *filp;
	char *wbuf;
	size_t npages;
} wq_t;

struct fdev{
	struct cdev fcdev;
	unsigned int ppos;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	struct workqueue_struct *dev_wq;
};

