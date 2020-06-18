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

#define PIR_MAJOR_NUMBER 505
#define PIR_DEV_NAME "pir_ioctl"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x34

#define IOCTL_MAGIC_NUMBER          'j'
#define IOCTL_CMD_GET_STATUS    _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_CMD_GET_STATUS2    _IOWR(IOCTL_MAGIC_NUMBER,1, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev0;

int pir_open(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "PIR driver open!\n");

    gpio_base = ioremap(GPIO_BASE_ADDR,0x60);
    gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
    gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    *gpsel2 |= (7); //GPIO20
    *gpsel2 &= ~(7);
    *gpsel2 |= (7<<3); //GPIO21
    *gpsel2 &= ~(7<<3);
    return 0;
}

int pir_release(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "PIR driver closed\n");
    iounmap((void *)gpio_base);
    return 0;
}

long pir_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    long read;
    switch(cmd){
        case IOCTL_CMD_GET_STATUS:
            read = (*gplev0)>>20 & 1;
            return read;
            break;

        case IOCTL_CMD_GET_STATUS2:
            read = (*gplev0)>>21 & 1;
            return read;
            break;
    }
    return 0;
}

static struct file_operations pir_fops = {
    .owner = THIS_MODULE,
    .open = pir_open,
    .release = pir_release,
    .unlocked_ioctl = pir_ioctl
};

int __init pir_init(void){
    if(register_chrdev(PIR_MAJOR_NUMBER, PIR_DEV_NAME, &pir_fops)<0)
        printk(KERN_ALERT "PIR driver initialization failed\n");
    else
        printk(KERN_ALERT "PIR driver initialization successful\n");
    
    return 0;
}

void __exit pir_exit(void){
    unregister_chrdev(PIR_MAJOR_NUMBER, PIR_DEV_NAME);
    printk(KERN_ALERT "PIR driver exit done\n");
}

module_init(pir_init);
module_exit(pir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kwangho Kim");
MODULE_DESCRIPTION("This is the PIR Detection Device Driver");
