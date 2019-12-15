#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LOADCELL_MAJOR_NUMBER	502
#define LOADCELL_DEV_NAME		"loadcell"

#define GPIO_BASE_ADDR	0x3F200000
#define GPFSEL0	0x00
#define GPSET0	0x1C
#define GPCLR0	0x28
#define GPLEV0	0x34

#define DOUT	5
#define PD_SCK	6

#define TRUE 1
#define FALSE 0

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

long getCount(void);

int loadcell_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LOADCELL driver open!!\n");
	
	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
	
	// gpio5(HX711_DOUT) = in, gpio6(HX711_CLK) = out
	*gpsel0 |= (1<<18);
	*gpset0 |= (1<<6);
	usleep_range(60,100);
	// gpio6 out -> init to 0
	*gpclr0 |= (1<<6);
	usleep_range(1,50);
		//usleep_range(1000,1000);

	return 0;
}

int loadcell_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "LOADCELL driver closed!!\n");
	iounmap((void *)gpio_base);
	return 0;
}

ssize_t loadcell_read(struct file *filp, char *buf, size_t size, loff_t *f_pos){
	long count = -1;
	while(count < 0 || count >= 0x7fffff){
		count = getCount();
	}
	copy_to_user(buf,&count,4);
	
	return size;
}

long getCount(void){
	int i;
	long count = 0;
	while((*gplev0 & (1<<5))>>5 == 1){
		//*gpclr0 |= (1<<6);
		//usleep_range(1000,1000);
	}
	
	for(i=0;i<24;i++){
		*gpset0 |= (1<<6);
		usleep_range(1,50);
		count = count << 1;
		*gpclr0 |= (1<<6);
		if((*gplev0 & (1<<5))>>5) count++;
		usleep_range(1,50);
		//if((*gplev0 & (1<<5))>>5) count++;
	}
	*gpset0 |= (1<<6);
	usleep_range(1,50);
	*gpclr0 |= (1<<6);
	
	if (count & 0x800000) {
		count |= (long) ~0xffffff;
	}
	
	printk("%ld",count);
	return count;
}

static struct file_operations loadcell_fops= {
	.owner = THIS_MODULE,
	.read = loadcell_read,
	.open = loadcell_open,
	.release = loadcell_release
};

int __init loadcell_init(void){
	if(register_chrdev(LOADCELL_MAJOR_NUMBER, LOADCELL_DEV_NAME, &loadcell_fops) < 0){
		printk(KERN_ALERT "LOADCELL driver initialization fail\n");
	}
	else{
		printk(KERN_ALERT "LOADCELL driver initialization success\n");
	}
	return 0;
}

void __exit loadcell_exit(void){
	unregister_chrdev(LOADCELL_MAJOR_NUMBER, LOADCELL_DEV_NAME);
	printk(KERN_ALERT "LOADCELL driver exit done\n");
}

module_init(loadcell_init);
module_exit(loadcell_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");
