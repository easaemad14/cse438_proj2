/*******************************************************************************************************
 *File: 	i2c_flash.c
 *Author: 	Easa El Sirgany (easaemad14@gmail.com)
 *ASU ID:	1001361972
 *
 *Description:	
 ******************************************************************************************************/
#include "global.h"
#include "i2c_flash.h"

/*
 * Global variables
 */
struct fdev *fdev_ptr;
struct class *fclass;
static struct device *fdev;

//A global state variable
int g_state;

//Virtual copy of our EEPROM, in order to perform asyncronous tasks
char *vEEPROM;

/*
 * Functions
 */
//FOPS functions
/*******************************************************************************************************
 * As a reminder, the file operations structure looks like this:
 * 	struct file_operations {
 *		struct module *owner;
 *              loff_t (*llseek) (struct file *, loff_t, int);
 *              ssize_t (*read) (struct file *, char *, size_t, loff_t *);
 *              ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
 *              int (*readdir) (struct file *, void *, filldir_t);
 *              unsigned int (*poll) (struct file *, struct poll_table_struct *);
 *              int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
 *              int (*mmap) (struct file *, struct vm_area_struct *);
 *              int (*open) (struct inode *, struct file *);
 *              int (*flush) (struct file *);
 *              int (*release) (struct inode *, struct file *);
 *              int (*fsync) (struct file *, struct dentry *, int datasync);
 *              int (*fasync) (int, struct file *, int);
 *              int (*lock) (struct file *, int, struct file_lock *);
 *              ssize_t (*readv) (struct file *, const struct iovec *, unsigned long, loff_t *);
 *              ssize_t (*writev) (struct file *, const struct iovec *, unsigned long, loff_t *);
 *	};
 *
 * With that being said, however, after some issues, I found this article explaining the replacement of
 * ioctl functions: http://lwn.net/Articles/119652/. In order to correctly (I think) do this, is to
 * check whether or not the HAVE_COMPAT_IOCTL is defined, and set the correct fops value accordingly.
 *
 * Similar to the last lab, we will be using the standard elements: owner, open, release, read, and
 * write, but we will be adding the implementation for the ioctl operation for this lab as well.
 ******************************************************************************************************/
loff_t i2c_seek(struct file *filp, loff_t offset, int whence){
	if(fdev_ptr != filp->private_data)
		return -EBADF;

	switch(whence){
		case SEEK_SET:
			if((offset < NUM_PAGES) && (offset >= 0)){
				fdev_ptr->ppos = offset;
				return fdev_ptr->ppos;
			}
			return -EOVERFLOW;
		case SEEK_CUR:
			if(((offset + fdev_ptr->ppos) < NUM_PAGES) &&
					((offset + fdev_ptr->ppos) >= 0)){
				fdev_ptr->ppos += offset;
				return fdev_ptr->ppos;
			}
			return -EOVERFLOW;
		case SEEK_END:
			if((offset > ~NUM_PAGES) && (offset <= 0)){
				fdev_ptr->ppos = NUM_PAGES - 1 + offset;
				return fdev_ptr->ppos;
			}
			return -EOVERFLOW;
		default:
			return -EINVAL;
	}
}

ssize_t i2c_read(struct file *filp, char *buf, size_t count, loff_t *ppos){
	int i;
	char tmpbuf[count];
	wq_t *tmpwork;

	switch(g_state){
		//Pseudo FSM
		case 0:
			for(i = 0; i < count; i++){
				sprintf(tmpbuf, &vEEPROM[fdev_ptr->ppos], PG_SIZE);
				fdev_ptr->ppos++;
			}

			if(copy_to_user(buf, tmpbuf, count))
				printk(KERN_WARNING "Data lost when writing to user!\n");

			g_state = EAGAIN;
			return 0;
		case EAGAIN:
			g_state = EBUSY;

			tmpwork = (wq_t *) kmalloc(sizeof(wq_t), GFP_KERNEL);
			tmpwork->filp = filp;
			if(copy_from_user(tmpwork->wbuf, buf, count))
				printk(KERN_WARNING "Data lost when reading from user!\n");
			tmpwork->npages = count/PG_SIZE; //Count is the number of bytes
			INIT_WORK((struct work_struct *)tmpwork, wq_read);

			if(queue_work(fdev_ptr->dev_wq, (struct work_struct *)tmpwork)){
				//Work is already in the queue
				kfree(tmpwork);
			}

			//Don't break (we want to return -1 either way)
		case EBUSY:
		default:
			return -1;
	}
}

ssize_t i2c_write(struct file *filp, const char *buf, size_t count, loff_t *ppos){
	wq_t *tmpwork;
	tmpwork = (wq_t *) kmalloc(sizeof(wq_t), GFP_KERNEL);
	tmpwork->filp = filp;
	if(copy_from_user(tmpwork->wbuf, buf, count))
		printk(KERN_WARNING "Data lost when reading from user!\n");
	tmpwork->npages = count/PG_SIZE; //Count is the number of bytes

	INIT_WORK((struct work_struct *)tmpwork, wq_write);
	if(queue_work(fdev_ptr->dev_wq, (struct work_struct *)tmpwork)){
		kfree(tmpwork);
		return -1;
	}
	return 0;
}

int i2c_ioctl_dep(struct inode *node, struct file *filp, unsigned int nioctl, unsigned long param){
	loff_t *tmppos = (loff_t *) &(fdev_ptr->ppos);
	printk(KERN_WARNING "Using depricated ioctl function!");
	fdev_ptr = container_of(node->i_cdev, struct fdev, fcdev);
	filp->private_data = fdev_ptr;

	switch(nioctl){
		case IOCTL_READ:
			return i2c_read(filp, (char *)param, PG_SIZE, tmppos);
		case IOCTL_WRITE:
			return i2c_write(filp, (char *)param, PG_SIZE, tmppos);
		default:
			return -EINVAL;
	}
}

long i2c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	loff_t *tmppos = (loff_t *) &(fdev_ptr->ppos);

	switch(cmd){
		case IOCTL_READ:
			return i2c_read(filp, (char *)arg, PG_SIZE, tmppos);
		case IOCTL_WRITE:
			return i2c_write(filp, (char *)arg, PG_SIZE, tmppos);
		default:
			return -EINVAL;
	}
}

int i2c_open(struct inode *node, struct file *filp){
	fdev_ptr = container_of(node->i_cdev, struct fdev, fcdev);
	filp->private_data = fdev_ptr;
	return 0;
}

int i2c_release(struct inode *node, struct file *filp){
	fdev_ptr = filp->private_data;
	return 0;
}

struct file_operations flash_fops = {
	.owner = THIS_MODULE,
	.llseek = i2c_seek,
	.read = i2c_read,
	.write = i2c_write,
#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = i2c_ioctl,
#else
	.ioctl = i2c_ioctl_dep,
#endif
	.open = i2c_open,
	.release = i2c_release
};

//Workqueue functions
void wq_read(struct work_struct *pwork){
	int i;
	char addr[3];

	wq_t *tmpwork = (wq_t *) pwork;

	//While we are communicating with the device, turn the LED on
	gpio_set_value_cansleep(26, 1);

	for(i = 0; i < tmpwork->npages; i++){
		sprintf(addr, "%x", fdev_ptr->ppos);

		//Need to send memory address
		if(i2c_master_send(fdev_ptr->client, addr, 3) < 0)
			printk(KERN_ERR "Unable to send memory address to EEPROM!\n");

		if(i2c_master_recv(fdev_ptr->client, &vEEPROM[fdev_ptr->ppos], PG_SIZE) < 0)
			printk(KERN_ERR "Unable to read data from EEPROM!\n");

		fdev_ptr->ppos++;
	}

	//Turn off LED, and set our global variable state to OK
	gpio_set_value_cansleep(26, 0);
	g_state = 0;
}

void wq_write(struct work_struct *pwork){
	int i;
	char addr[3];

	wq_t *tmpwork = (wq_t *) pwork;

	//While we are communicating with the device, turn the LED on
	gpio_set_value_cansleep(26, 1);

	for(i = 0; i < tmpwork->npages; i++){
		sprintf(addr, "%x", fdev_ptr->ppos);

		//Need to send memory address
		if(i2c_master_send(fdev_ptr->client, addr, 3) < 0)
			printk(KERN_ERR "Unable to send memory address to EEPROM!\n");

		if(i2c_master_send(fdev_ptr->client, tmpwork->wbuf, PG_SIZE) < 0)
			printk(KERN_ERR "Unable to read data from EEPROM!\n");

		//We also need to show this data is written in our virtual EEPROM
		strcpy(&vEEPROM[fdev_ptr->ppos], tmpwork->wbuf);

		fdev_ptr->ppos++;
	}

	//Turn off LED, and set our global variable state to OK
	gpio_set_value_cansleep(26, 0);
	g_state = 0;
}

//init and exit functions
int __init i2c_init(void){
	//Set up our global variables
	vEEPROM = kcalloc(NUM_PAGES, PG_SIZE, GFP_KERNEL);
	g_state = EAGAIN;

	/*
	 * Standard character device implementation
	 */
	fdev_ptr = NULL; //For error checking later on in the function

	if(alloc_chrdev_region((dev_t *)MAJOR_NUM, 0, 1, DEVICE_NAME) < 0){
		printk(KERN_ERR "Can't register device!\n"); 
		return -1;
	}

	fclass = class_create(THIS_MODULE, DEVICE_NAME);

	fdev_ptr = kmalloc(sizeof(struct fdev), GFP_KERNEL);
	if (!fdev_ptr) {
		printk(KERN_ERR "Can't allocate space for device!\n");
		return -1;
	}

	cdev_init(&fdev_ptr->fcdev, &flash_fops);
	fdev_ptr->fcdev.owner = THIS_MODULE;
	if(cdev_add(&fdev_ptr->fcdev, MAJOR_NUM, 1) < 0){
		printk(KERN_ERR "Can't add device!\n");
		if(fdev_ptr == NULL)
			kfree(fdev_ptr);
		return -1;
	}

	fdev_ptr->ppos = 0;

	fdev = device_create(fclass, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_NAME);
	
	/*
	 * Now we get to some fun, project related stuff!
	 *
	 * I have this set up to print warning messages, and continue with operation, with
	 * the line of thought that if we are unable to set up our LED to blink while
	 * operations to the EEPROM are occuring, we will still be able to complete the
	 * actions.
	 */
	//Set up the LED on the correct pin (See Galileo schematics and instruction PDF)
	if(gpio_request(LED_PIN, "gpio_LED") < 0)
		printk(KERN_WARNING "Unable to request LED Pin!\n");

	if(gpio_direction_output(LED_PIN, 0) < 0)
		printk(KERN_WARNING "Unable to set LED direction!\n");

	//Set up the GPIO "MUX" (See the IO_schem image) to use I2C, as compared to GPIO
	if(gpio_request(I2CMUX, "I2CMUX") < 0)
		printk(KERN_WARNING "Unable to request GPIO for I2C!\n");

	if(gpio_direction_output(I2CMUX, 0) < 0)
		printk(KERN_WARNING "Unable to set GPIO for output!\n");

	//I2C implements a message based bus, which can't be done from inside IRQ handlers
	//tl;dr, use gpio_set_value_cansleep, as compared to gpio_set_value
	gpio_set_value_cansleep(I2CMUX, 0);

	//Set up our I2C adapter
	fdev_ptr->adapter = i2c_get_adapter(0);
	if(!(fdev_ptr->adapter))
		printk(KERN_WARNING "Unable to get I2C adapter!\n");

	//Set up our client
	fdev_ptr->client = (struct i2c_client *) kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	fdev_ptr->client->addr = DEVICE_ADDR;
	if(sprintf(fdev_ptr->client->name, DEVICE_NAME) < 0){
		printk(KERN_ERR "Unable to set client name!\n");
		return -1;
	}
	fdev_ptr->client->adapter = fdev_ptr->adapter;

	//Set up our work queue
	fdev_ptr->dev_wq = create_workqueue(WQ_NAME);

	return 0;
}

void __exit i2c_exit(void){
	//Start from the inside of the device
	flush_workqueue(fdev_ptr->dev_wq);
	destroy_workqueue(fdev_ptr->dev_wq);

	i2c_put_adapter(fdev_ptr->adapter);
	kfree(fdev_ptr->client);

	unregister_chrdev_region(MAJOR_NUM, 1);

	device_destroy(fclass, MKDEV(MAJOR_NUM, 0));
	cdev_del(&fdev_ptr->fcdev);
	kfree(fdev_ptr);
	
	class_destroy(fclass);
	
	//Don't forget about the GPIO and our vEEPROM
	gpio_free(I2CMUX);
	gpio_free(LED_PIN);
	kfree(vEEPROM);
}


module_init(i2c_init);
module_exit(i2c_exit);
MODULE_LICENSE("GPL v2");
