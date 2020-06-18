#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define GPIO_BASE_ADDR 0x3F200000
#define GPIO_BSC1_ADDR 0x3F804000
#define GPFSEL0  0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x34

#define I2C_MAJOR_NUMBER 510
#define I2C_DEV_NAME "i2c_ioctl"

#define IOCTL_MAGIC_NUMBER          'k'
#define IOCTL_I2C_READ                  _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_I2C_WRITE                 _IOWR(IOCTL_MAGIC_NUMBER,1, int)
#define IOCTL_I2C_SET_BAUD              _IOWR(IOCTL_MAGIC_NUMBER,2, int)
#define IOCTL_I2C_SET_CLOCK_DIVIDER     _IOWR(IOCTL_MAGIC_NUMBER,3, int)
#define IOCTL_I2C_SET_SLAVE_ADDRESS     _IOWR(IOCTL_MAGIC_NUMBER,4, int)
#define IOCTL_I2C_READ_REG_RS           _IOWR(IOCTL_MAGIC_NUMBER,5, int)
#define IOCTL_I2C_WRITE_REG_RS          _IOWR(IOCTL_MAGIC_NUMBER,6, int)


#define BCM2835_BSC_C 			0x0000 /*!< BSC Master Control */
#define BCM2835_BSC_S 			0x0004 /*!< BSC Master Status */
#define BCM2835_BSC_DLEN		0x0008 /*!< BSC Master Data Length */
#define BCM2835_BSC_A 			0x000c /*!< BSC Master Slave Address */
#define BCM2835_BSC_FIFO		0x0010 /*!< BSC Master Data FIFO */
#define BCM2835_BSC_DIV			0x0014 /*!< BSC Master Clock Divider */
#define BCM2835_BSC_DEL			0x0018 /*!< BSC Master Data Delay */
#define BCM2835_BSC_CLKT		0x001c /*!< BSC Master Clock Stretch Timeout */

/* Register masks for BSC_C */
#define BCM2835_BSC_C_I2CEN 	0x00008000 /*!< I2C Enable, 0 = disabled, 1 = enabled */
#define BCM2835_BSC_C_INTR 		0x00000400 /*!< Interrupt on RX */
#define BCM2835_BSC_C_INTT 		0x00000200 /*!< Interrupt on TX */
#define BCM2835_BSC_C_INTD 		0x00000100 /*!< Interrupt on DONE */
#define BCM2835_BSC_C_ST 		0x00000080 /*!< Start transfer, 1 = Start a new transfer */
#define BCM2835_BSC_C_CLEAR_1 	0x00000020 /*!< Clear FIFO Clear */
#define BCM2835_BSC_C_CLEAR_2 	0x00000010 /*!< Clear FIFO Clear */
#define BCM2835_BSC_C_READ 		0x00000001 /*!<	Read transfer */

/* Register masks for BSC_S */
#define BCM2835_BSC_S_CLKT 		0x00000200 /*!< Clock stretch timeout */
#define BCM2835_BSC_S_ERR 		0x00000100 /*!< ACK error */
#define BCM2835_BSC_S_RXF 		0x00000080 /*!< RXF FIFO full, 0 = FIFO is not full, 1 = FIFO is full */
#define BCM2835_BSC_S_TXE 		0x00000040 /*!< TXE FIFO full, 0 = FIFO is not full, 1 = FIFO is full */
#define BCM2835_BSC_S_RXD 		0x00000020 /*!< RXD FIFO contains data */
#define BCM2835_BSC_S_TXD 		0x00000010 /*!< TXD FIFO can accept data */
#define BCM2835_BSC_S_RXR 		0x00000008 /*!< RXR FIFO needs reading (full) */
#define BCM2835_BSC_S_TXW 		0x00000004 /*!< TXW FIFO needs writing (full) */
#define BCM2835_BSC_S_DONE 		0x00000002 /*!< Transfer DONE */
#define BCM2835_BSC_S_TA 		0x00000001 /*!< Transfer Active */

#define BCM2835_ST_CS 			0x0000 /*!< System Timer Control/Status */
#define BCM2835_ST_CLO 			0x0004 /*!< System Timer Counter Lower 32 bits */
#define BCM2835_ST_CHI 			0x0008 /*!< System Timer Counter Upper 32 bits */
#define BCM2835_ST_BASE			0x3000
#define BCM2835_BSC_FIFO_SIZE   16 /*!< BSC FIFO size */

#define BCM2835_CORE_CLK_HZ		250000000
#define MAP_FAILED -1

#define BCM2835_I2C_REASON_OK   	      0x00      /*!< Success */
#define BCM2835_I2C_REASON_ERROR_NACK     0x01      /*!< Received a NACK */
#define BCM2835_I2C_REASON_ERROR_CLKT     0x02      /*!< Received Clock Stretch Timeout */
#define BCM2835_I2C_REASON_ERROR_DATA     0x04       /*!< Not all data is sent / received */

struct writeData {
    char* buf;
    int len;
};

static DEFINE_MUTEX(data_mutex);
static void __iomem *gpio_base;
static void __iomem *gpio_bsc1_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev0;

volatile unsigned int *bsc_c;
volatile unsigned int *bsc_s;
volatile unsigned int *bsc_dlen;
volatile unsigned int *bsc_a;
volatile unsigned int *bsc_fifo;
volatile unsigned int *bsc_div;
volatile unsigned int *bsc_del;
volatile unsigned int *bsc_clkt;

volatile unsigned int i2c_byte_wait_us=-1;

unsigned int read_from(volatile unsigned int* addr);

int i2c_open(struct inode *inode, struct file *filp){
    // volatile unsigned int * cdiv;
    printk(KERN_ALERT "I2C driver open!\n");

    gpio_base =      ioremap(GPIO_BASE_ADDR,0x60);
    gpio_bsc1_base = ioremap(GPIO_BSC1_ADDR,0x60);

    gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);

    gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);

    gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);

    gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

    bsc_c =    (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_C);
    bsc_s =    (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_S);
    bsc_dlen = (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_DLEN);
    bsc_a =    (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_A);
    bsc_fifo = (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_FIFO);
    bsc_div =  (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_DIV);
    bsc_del =  (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_DEL);
    bsc_clkt = (volatile unsigned int *)(gpio_bsc1_base + BCM2835_BSC_CLKT);

    *gpsel0 |= (7<<6); //GPIO2
    *gpsel0 &= ~(3<<6); //ALT0 -> SDA

    *gpsel0 |= (7<<9); //GPIO3
    *gpsel0 &= ~(3<<9); //ALT0 -> SCL

    // bcm2835_gpio_fsel(2, 0x04); /* SDA */
    // bcm2835_gpio_fsel(3, 0x04); /* SCL */
    
    i2c_byte_wait_us = (*bsc_div / BCM2835_CORE_CLK_HZ) * 1000000 * 9;
    return 0;
}

int i2c_release(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "I2C driver closed\n");

    *gpsel0 |= (7<<6); //GPIO2
    *gpsel0 &= ~(7<<6); //INPUT

    *gpsel0 |= (7<<9); //GPIO3
    *gpsel0 &= ~(7<<9); //INPUT

    iounmap((void *)gpio_base);
    return 0;
}

long i2c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    struct writeData *buf;
    int data,divider,remaining,i,reason,len=1;
    
    switch(cmd){
        // case IOCTL_I2C_READ:
        //     copy_from_user( (void *)&readBuf, (const void *)arg, sizeof(readBuf));
        //     data=bcm2835_i2c_read(readBuf[0],1);
        // break;

        case IOCTL_I2C_WRITE:
            copy_from_user( (void *)&buf, (const void *)arg, sizeof(buf));
            printk(KERN_ALERT "%s",buf->buf);
            remaining = len;
            i = 0;
            reason = BCM2835_I2C_REASON_OK;

            /* Clear FIFO */
            mutex_lock(&data_mutex);
            *bsc_c = ((*bsc_c) & ~BCM2835_BSC_C_CLEAR_1) | BCM2835_BSC_C_CLEAR_1;
            mutex_unlock(&data_mutex);

            /* Clear Status */
            mutex_lock(&data_mutex);
            *bsc_s = BCM2835_BSC_S_CLKT | BCM2835_BSC_S_ERR | BCM2835_BSC_S_DONE;
            mutex_unlock(&data_mutex);

            /* Set Data Length */
            mutex_lock(&data_mutex);
            *bsc_dlen = len;
            mutex_unlock(&data_mutex);

            /* pre populate FIFO with max buffer */
            while( remaining && ( i < BCM2835_BSC_FIFO_SIZE ) )
            {
                // bcm2835_peri_write_nb(fifo, buf[i]);
                *bsc_fifo = buf->buf[i];
                i++;
                remaining--;
            }
            
            /* Enable device and start transfer */
            mutex_lock(&data_mutex);
            // bcm2835_peri_write(control, BCM2835_BSC_C_I2CEN | BCM2835_BSC_C_ST);
            *bsc_c = BCM2835_BSC_C_I2CEN | BCM2835_BSC_C_ST;
            mutex_unlock(&data_mutex);

            /* Transfer is over when BCM2835_BSC_S_DONE */
            while(!(read_from(bsc_s) & BCM2835_BSC_S_DONE ))
            {
                while ( remaining && (read_from(bsc_s) & BCM2835_BSC_S_TXD ))
                {
                /* Write to FIFO */
                // bcm2835_peri_write(fifo, buf[i]);
                mutex_lock(&data_mutex);
                *bsc_fifo = buf->buf[i];
                mutex_unlock(&data_mutex);
                i++;
                remaining--;
                }
            }

            /* Received a NACK */
            if (read_from(bsc_s) & BCM2835_BSC_S_ERR)
            {
                reason = BCM2835_I2C_REASON_ERROR_NACK;
            }

            /* Received Clock Stretch Timeout */
            else if (read_from(bsc_s) & BCM2835_BSC_S_CLKT)
            {
                reason = BCM2835_I2C_REASON_ERROR_CLKT;
            }

            /* Not all data is sent */
            else if (remaining)
            {
                reason = BCM2835_I2C_REASON_ERROR_DATA;
            }

            // bcm2835_peri_set_bits(control, BCM2835_BSC_S_DONE , BCM2835_BSC_S_DONE);
            mutex_lock(&data_mutex);
            *bsc_c = ((*bsc_c) & ~BCM2835_BSC_S_DONE) | BCM2835_BSC_S_DONE;
            mutex_unlock(&data_mutex);
            return reason;
            // bcm2835_i2c_write(writeBuf[0],1);
        break;

        case IOCTL_I2C_SET_BAUD:
            copy_from_user( (void *)&data, (const void *)arg, sizeof(data));
            mutex_lock(&data_mutex);
            divider = (BCM2835_CORE_CLK_HZ / data) & 0xFFFE;
            *bsc_div = divider;
            mutex_unlock(&data_mutex);
        break;

        case IOCTL_I2C_SET_CLOCK_DIVIDER:
            copy_from_user( (void *)&data, (const void *)arg, sizeof(data));
            mutex_lock(&data_mutex);
            *bsc_div = data;
            mutex_unlock(&data_mutex);
        break;

        case IOCTL_I2C_SET_SLAVE_ADDRESS:
            copy_from_user( (void *)&data, (const void *)arg, sizeof(data));
            printk(KERN_ALERT "slave %d",data);
            mutex_lock(&data_mutex);
            *bsc_a = data;
            mutex_unlock(&data_mutex);
        break;

        // case IOCTL_I2C_READ_REG_RS:
        //     bcm2835_i2c_read_register_rs();
        // break;

        // case IOCTL_I2C_WRITE_REG_RS:
        //     bcm2835_i2c_write_register_rs();
        // break;
    }
    return 0;
}

static struct file_operations i2c_fops = {
    .owner = THIS_MODULE,
    .open = i2c_open,
    .release = i2c_release,
    .unlocked_ioctl = i2c_ioctl
};

int __init i2c_init(void){
    if(register_chrdev(I2C_MAJOR_NUMBER, I2C_DEV_NAME, &i2c_fops)<0)
        printk(KERN_ALERT "I2C driver initialization failed\n");
    else
        printk(KERN_ALERT "I2C driver initialization successful\n");
    
    return 0;
}

void __exit i2c_exit(void){
    unregister_chrdev(I2C_MAJOR_NUMBER, I2C_DEV_NAME);
    printk(KERN_ALERT "I2C driver exit done\n");
}

module_init(i2c_init);
module_exit(i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kwangho Kim");
MODULE_DESCRIPTION("This is the hello world example for device driver in system programming lecture");

unsigned int read_from(volatile unsigned int* addr)
{
    unsigned int ret;
    mutex_lock(&data_mutex);
    ret = *addr;
    mutex_unlock(&data_mutex);
    return ret;
}
