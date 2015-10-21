#ifndef __I2C_FLASH_H_
#define __I2C_FLASH_H_
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
#include <linux/workqueue.h>

/*
 * Variable definitions
 */
#define WQ_NAME "i2c_workqueue"
#define DEVICE_ADDR 0x54
#define DEVICE_NAME "i2c_flash"
#define I2CMUX 29 //gpio29 (see schematic image)
#define LED_PIN 26

//Our character device for read/write ops to EEPROM
typedef struct{
	struct work_struct my_work;
	struct file *filp;
	char wbuf[PG_SIZE];
	size_t npages;
} wq_t;

struct fdev{
	struct cdev fcdev;
	unsigned int ppos;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	struct workqueue_struct *dev_wq;
};

/*
 * File definitions
 */
//FOPS
loff_t i2c_seek(struct file *filp, loff_t offset, int whence);
ssize_t i2c_read(struct file *filp, char *buf, size_t count, loff_t *ppos);
ssize_t i2c_write(struct file *filp, const char *buf, size_t count, loff_t *ppos);
int i2c_ioctl_dep(struct inode *node, struct file *filp, unsigned int nioctl, unsigned long param);
long i2c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int i2c_open(struct inode *node, struct file *filp);
int i2c_release(struct inode *node, struct file *filp);

//Workqueue
void wq_read(struct work_struct *pwork);
void wq_write(struct work_struct *pwork);

#endif //__I2C_FLASH_H_
