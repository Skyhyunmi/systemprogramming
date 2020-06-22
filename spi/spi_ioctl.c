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

#define GPIO_BASE_ADDR                  0x3F200000
#define BCM2835_SPI0_BASE               0x3F204000
#define BCM2835_SPI1_BASE				0x3F215080
#define BCM2835_SPI2_BASE				0x3F2150C0
#define GPFSEL0  0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x34

#define SPI_MAJOR_NUMBER 511
#define SPI_DEV_NAME "spi_ioctl"

#define IOCTL_MAGIC_NUMBER          'l'
#define IOCTL_SPI_SET_CLOCK_DIVIDER     _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_SPI_SET_DATA_MODE         _IOWR(IOCTL_MAGIC_NUMBER,1, int)
#define IOCTL_SPI_TRANSFER              _IOWR(IOCTL_MAGIC_NUMBER,2, int)
#define IOCTL_SPI_TRANSFER_NB           _IOWR(IOCTL_MAGIC_NUMBER,3, int)
#define IOCTL_SPI_WRITE_NB              _IOWR(IOCTL_MAGIC_NUMBER,4, int)
#define IOCTL_SPI_TRANSFER_N            _IOWR(IOCTL_MAGIC_NUMBER,5, int)
#define IOCTL_SPI_CHIP_SELECT           _IOWR(IOCTL_MAGIC_NUMBER,6, int)
#define IOCTL_SPI_CHIP_SELECT_POLARITY  _IOWR(IOCTL_MAGIC_NUMBER,7, int)
#define IOCTL_SPI_WRITE                 _IOWR(IOCTL_MAGIC_NUMBER,8, int)

#define BCM2835_SPI0_CS                      0x0000 /*!< SPI Master Control and Status */
#define BCM2835_SPI0_FIFO                    0x0004 /*!< SPI Master TX and RX FIFOs */
#define BCM2835_SPI0_CLK                     0x0008 /*!< SPI Master Clock Divider */
#define BCM2835_SPI0_DLEN                    0x000c /*!< SPI Master Data Length */
#define BCM2835_SPI0_LTOH                    0x0010 /*!< SPI LOSSI mode TOH */
#define BCM2835_SPI0_DC                      0x0014 /*!< SPI DMA DREQ Controls */

/* Register masks for SPI0_CS */
#define BCM2835_SPI0_CS_LEN_LONG             0x02000000 /*!< Enable Long data word in Lossi mode if DMA_LEN is set */
#define BCM2835_SPI0_CS_DMA_LEN              0x01000000 /*!< Enable DMA mode in Lossi mode */
#define BCM2835_SPI0_CS_CSPOL2               0x00800000 /*!< Chip Select 2 Polarity */
#define BCM2835_SPI0_CS_CSPOL1               0x00400000 /*!< Chip Select 1 Polarity */
#define BCM2835_SPI0_CS_CSPOL0               0x00200000 /*!< Chip Select 0 Polarity */
#define BCM2835_SPI0_CS_RXF                  0x00100000 /*!< RXF - RX FIFO Full */
#define BCM2835_SPI0_CS_RXR                  0x00080000 /*!< RXR RX FIFO needs Reading (full) */
#define BCM2835_SPI0_CS_TXD                  0x00040000 /*!< TXD TX FIFO can accept Data */
#define BCM2835_SPI0_CS_RXD                  0x00020000 /*!< RXD RX FIFO contains Data */
#define BCM2835_SPI0_CS_DONE                 0x00010000 /*!< Done transfer Done */
#define BCM2835_SPI0_CS_TE_EN                0x00008000 /*!< Unused */
#define BCM2835_SPI0_CS_LMONO                0x00004000 /*!< Unused */
#define BCM2835_SPI0_CS_LEN                  0x00002000 /*!< LEN LoSSI enable */
#define BCM2835_SPI0_CS_REN                  0x00001000 /*!< REN Read Enable */
#define BCM2835_SPI0_CS_ADCS                 0x00000800 /*!< ADCS Automatically Deassert Chip Select */
#define BCM2835_SPI0_CS_INTR                 0x00000400 /*!< INTR Interrupt on RXR */
#define BCM2835_SPI0_CS_INTD                 0x00000200 /*!< INTD Interrupt on Done */
#define BCM2835_SPI0_CS_DMAEN                0x00000100 /*!< DMAEN DMA Enable */
#define BCM2835_SPI0_CS_TA                   0x00000080 /*!< Transfer Active */
#define BCM2835_SPI0_CS_CSPOL                0x00000040 /*!< Chip Select Polarity */
#define BCM2835_SPI0_CS_CLEAR                0x00000030 /*!< Clear FIFO Clear RX and TX */
#define BCM2835_SPI0_CS_CLEAR_RX             0x00000020 /*!< Clear FIFO Clear RX  */
#define BCM2835_SPI0_CS_CLEAR_TX             0x00000010 /*!< Clear FIFO Clear TX  */
#define BCM2835_SPI0_CS_CPOL                 0x00000008 /*!< Clock Polarity */
#define BCM2835_SPI0_CS_CPHA                 0x00000004 /*!< Clock Phase */
#define BCM2835_SPI0_CS_CS                   0x00000003 /*!< Chip Select */

#define BCM2835_SPI_BIT_ORDER_LSBFIRST  0  /*!< LSB First */
#define BCM2835_SPI_BIT_ORDER_MSBFIRST  1   /*!< MSB First */

#define BCM2835_SPI_MODE0               0  /*!< CPOL = 0, CPHA = 0 */
#define BCM2835_SPI_MODE1               1  /*!< CPOL = 0, CPHA = 1 */
#define BCM2835_SPI_MODE2               2         /*!< CPOL = 1, CPHA = 0 */
#define BCM2835_SPI_MODE3               3   /*!< CPOL = 1, CPHA = 1 */

#define BCM2835_SPI_CS0                 0     /*!< Chip Select 0 */
#define BCM2835_SPI_CS1                 1     /*!< Chip Select 1 */
#define BCM2835_SPI_CS2                 2     /*!< Chip Select 2 (ie pins CS1 and CS2 are asserted) */
#define BCM2835_SPI_CS_NONE             3  /*!< No CS, control it yourself */


struct Data {
    char* tbuf;
    char* rbuf;
    char* buf;
    uint32_t len;
    uint8_t data;
    uint32_t ret;
    uint8_t active;
};


static DEFINE_MUTEX(data_mutex);

static void __iomem *gpio_base;
static void __iomem *spi0_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev0;

volatile unsigned int *spi0_cs;
volatile unsigned int *spi0_fifo;
volatile unsigned int *spi0_clk;
volatile unsigned int *spi0_dlen;
volatile unsigned int *spi0_ltoh;
volatile unsigned int *spi0_dc;


uint32_t read_from(volatile unsigned int* addr);
uint32_t read_from_nb(volatile unsigned int* addr);
void set_bits(volatile unsigned int* addr, int value, int mask);
void p_write(volatile unsigned int* addr, int value);
void p_write_nb(volatile unsigned int* addr, int value);

int spi_open(struct inode *inode, struct file *filp){
    // volatile unsigned int * cdiv;
    printk(KERN_ALERT "SPI driver open!\n");

    gpio_base = ioremap(GPIO_BASE_ADDR,0x60);
    spi0_base = ioremap(BCM2835_SPI0_BASE,0x60);

    gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

    spi0_cs   = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_CS);
    spi0_fifo = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_FIFO);
    spi0_clk  = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_CLK);
    spi0_dlen = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_DLEN);
    spi0_ltoh = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_LTOH);
    spi0_dc   = (volatile unsigned int *)(spi0_base + BCM2835_SPI0_DC);

    *gpsel0 |= (7<<(3*9)); //GPIO9
    *gpsel0 &= ~(3<<(3*9)); //ALT0 -> MISO

    *gpsel1 |= (7); //GPIO10
    *gpsel1 &= ~(3); //ALT0 -> MOSI

    *gpsel1 |= (7<<3); //GPIO11
    *gpsel1 &= ~(3<<3); //ALT0 -> CLK

    *gpsel0 |= (7<<24); //GPIO8
    *gpsel0 &= ~(3<<24); //ALT0 -> CE0

    *gpsel0 |= (7<<21); //GPIO7
    *gpsel0 &= ~(3<<21); //ALT0 -> CE1

    // bcm2835_gpio_fsel(2, 0x04); /* SDA */
    // bcm2835_gpio_fsel(3, 0x04); /* SCL */

    p_write(spi0_cs,0);

    p_write_nb(spi0_cs, BCM2835_SPI0_CS_CLEAR);
    return 0;
}

int spi_release(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "SPI driver closed\n");

    *gpsel0 |= (7<<(3*9)); //GPIO9
    *gpsel0 &= ~(7<<(3*9)); //ALT0 -> MISO

    *gpsel1 |= (7); //GPIO10
    *gpsel1 &= ~(7); //ALT0 -> MOSI

    *gpsel1 |= (7<<3); //GPIO11
    *gpsel1 &= ~(7<<3); //ALT0 -> CLK

    *gpsel0 |= (7<<24); //GPIO8
    *gpsel0 &= ~(7<<24); //ALT0 -> CE0

    *gpsel0 |= (7<<21); //GPIO7
    *gpsel0 &= ~(7<<21); //ALT0 -> CE1

    iounmap((void *)gpio_base);
    iounmap((void *)spi0_base);
    return 0;
}

long spi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    struct Data *buf;
    uint32_t RXCnt,TXCnt,i;
    uint8_t shift;

    switch(cmd){
        case IOCTL_SPI_SET_CLOCK_DIVIDER:
            copy_from_user( (void *)&buf, (const void *)arg, sizeof(buf));
            p_write(spi0_clk,buf->data);
        break;
        
        case IOCTL_SPI_SET_DATA_MODE:
            copy_from_user( (void *)&buf, (const void *)arg, sizeof(buf));
            set_bits(spi0_cs, (buf->data) << 2, BCM2835_SPI0_CS_CPOL | BCM2835_SPI0_CS_CPHA);
        break;

        case IOCTL_SPI_TRANSFER:
            copy_from_user( (void *)&buf, (void *)arg, sizeof(buf));
            set_bits(spi0_cs, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);
            set_bits(spi0_cs, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);
            while (!(read_from(spi0_cs) & BCM2835_SPI0_CS_TXD))
	            ;
            p_write(spi0_fifo,buf->data);

            while (!(read_from_nb(spi0_cs) & BCM2835_SPI0_CS_DONE))
	            ;
            /* Read any byte that was sent back by the slave while we sere sending to it */
            buf->ret = read_from_nb(spi0_fifo);
            /* Set TA = 0, and also set the barrier */
            set_bits(spi0_cs, 0, BCM2835_SPI0_CS_TA);
            copy_to_user( (void *)arg, (void *)&buf, sizeof(buf));
            // return ret;
        break;

        case IOCTL_SPI_TRANSFER_NB:
            copy_from_user( (void *)&buf, (void *)arg, sizeof(buf));
            
            transfer:
            TXCnt=0;
            RXCnt=0;

            /* Clear RX fifos */
            set_bits(spi0_cs, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

            /* Set TA = 1 */
            set_bits(spi0_cs, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

            /* Use the FIFO's to reduce the interbyte times */
            while(TXCnt < (buf->len) || RXCnt < (buf->len)){
                while(((read_from(spi0_cs) & BCM2835_SPI0_CS_TXD))&&(TXCnt < buf->len )){
                    p_write_nb(spi0_fifo, (buf->tbuf)[TXCnt]);
                    TXCnt++;
                }
                while(((read_from(spi0_cs) & BCM2835_SPI0_CS_RXD))&&( RXCnt < buf->len )){
                    (buf->rbuf)[RXCnt] = read_from_nb(spi0_fifo);
                    RXCnt++;
                }
            }
            /* Wait for DONE to be set */
            while (!(read_from_nb(spi0_cs) & BCM2835_SPI0_CS_DONE))
            ;
            copy_to_user((void *)arg,&buf,sizeof(buf));
            /* Set TA = 0, and also set the barrier */
            set_bits(spi0_cs, 0, BCM2835_SPI0_CS_TA);
            
        break;

        case IOCTL_SPI_WRITE_NB:
            copy_from_user( (void *)&buf, (void *)arg, sizeof(buf));

            /* Clear TX and RX fifos */
            set_bits(spi0_cs, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

            /* Set TA = 1 */
            set_bits(spi0_cs, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

            for (i = 0; i < buf->len; i++)
            {
                /* Maybe wait for TXD */
                while (!(read_from(spi0_cs) & BCM2835_SPI0_CS_TXD))
                    ;
                
                /* Write to FIFO, no barrier */
                p_write_nb(spi0_fifo, buf->tbuf[i]);
                
                /* Read from FIFO to prevent stalling */
                while (read_from(spi0_cs) & BCM2835_SPI0_CS_RXD)
                    (void) read_from_nb(spi0_fifo);
            }
            
            /* Wait for DONE to be set */
            while (!(read_from_nb(spi0_cs) & BCM2835_SPI0_CS_DONE)) {
                while (read_from(spi0_cs) & BCM2835_SPI0_CS_RXD)
                    (void) read_from_nb(spi0_fifo);
            };

            /* Set TA = 0, and also set the barrier */
            set_bits(spi0_cs, 0, BCM2835_SPI0_CS_TA);
        break;

        case IOCTL_SPI_TRANSFER_N:
            copy_from_user( (void *)&buf, (void *)arg, sizeof(buf));
            for(i=0;i<buf->len;i++){
                buf->tbuf[i] = buf->buf[i];
                buf->rbuf[i] = buf->buf[i];
            }
            goto transfer;
        break;

        case IOCTL_SPI_CHIP_SELECT:
            copy_from_user( (void *)&buf, (const void *)arg, sizeof(buf));
            set_bits(spi0_cs,buf->data,BCM2835_SPI0_CS_CS);
        break;

        case IOCTL_SPI_CHIP_SELECT_POLARITY:
            copy_from_user( (void *)&buf, (const void *)arg, sizeof(buf));
            shift = 21 + buf->data;
            set_bits(spi0_cs, (buf->active) << shift, 1 << shift);
        break;

        case IOCTL_SPI_WRITE:
            copy_from_user( (void *)&buf, (void *)arg, sizeof(buf));
            /* Clear TX and RX fifos */
            set_bits(spi0_cs, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

            /* Set TA = 1 */
            set_bits(spi0_cs, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

            /* Maybe wait for TXD */
            while (!(read_from(spi0_cs) & BCM2835_SPI0_CS_TXD))
                ;

            /* Write to FIFO */
            p_write_nb(spi0_fifo,  (uint32_t) (buf->data) >> 8);
            p_write_nb(spi0_fifo,  (buf->data) & 0xFF);


            /* Wait for DONE to be set */
            while (!(read_from_nb(spi0_cs) & BCM2835_SPI0_CS_DONE))
            ;

            /* Set TA = 0, and also set the barrier */
            set_bits(spi0_cs, 0, BCM2835_SPI0_CS_TA);
        break;

    }
    return 0;
}

static struct file_operations spi_fops = {
    .owner = THIS_MODULE,
    .open = spi_open,
    .release = spi_release,
    .unlocked_ioctl = spi_ioctl
};

int __init spi_init(void){
    if(register_chrdev(SPI_MAJOR_NUMBER, SPI_DEV_NAME, &spi_fops)<0)
        printk(KERN_ALERT "SPI driver initialization failed\n");
    else
        printk(KERN_ALERT "SPI driver initialization successful\n");
    
    return 0;
}

void __exit spi_exit(void){
    unregister_chrdev(SPI_MAJOR_NUMBER, SPI_DEV_NAME);
    printk(KERN_ALERT "SPI driver exit done\n");
}

module_init(spi_init);
module_exit(spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kwangho Kim");
MODULE_DESCRIPTION("This is the hello world example for device driver in system programming lecture");

uint32_t read_from(volatile unsigned int* addr)
{
    unsigned int ret;
    mutex_lock(&data_mutex);
    ret = *addr;
    mutex_unlock(&data_mutex);
    return ret;
}

uint32_t read_from_nb(volatile unsigned int* addr)
{
    unsigned int ret;
    ret = *addr;
    return ret;
}

void set_bits(volatile unsigned int* addr, int value, int mask){
    mutex_lock(&data_mutex);
    *addr = ((*addr) & ~mask) | (value & mask);
    mutex_unlock(&data_mutex);
}

void p_write(volatile unsigned int* addr, int value){
    mutex_lock(&data_mutex);
    *addr = value;
    mutex_unlock(&data_mutex);
}

void p_write_nb(volatile unsigned int* addr, int value){
    *addr = value;
}