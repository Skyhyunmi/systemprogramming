#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define SPI_MAJOR_NUMBER 511
#define SPI_MINOR_NUMBER 102
#define SPI_DEV_PATH_NAME "/dev/spi_ioctl"

#define IOCTL_MAGIC_NUMBER          'l'
// #define IOCTL_SPI_SET_BIT_ORDER         _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_SPI_SET_CLOCK_DIVIDER     _IOWR(IOCTL_MAGIC_NUMBER,0, int)
#define IOCTL_SPI_SET_DATA_MODE         _IOWR(IOCTL_MAGIC_NUMBER,1, int)
/* Reads a single byte to SPI */
#define IOCTL_SPI_TRANSFER              _IOWR(IOCTL_MAGIC_NUMBER,2, int)
/* Reads an number of bytes to SPI */
#define IOCTL_SPI_TRANSFER_NB           _IOWR(IOCTL_MAGIC_NUMBER,3, int)
/* Reads an number of bytes to SPI */
#define IOCTL_SPI_WRITE_NB              _IOWR(IOCTL_MAGIC_NUMBER,4, int)
#define IOCTL_SPI_TRANSFER_N            _IOWR(IOCTL_MAGIC_NUMBER,5, int)
#define IOCTL_SPI_CHIP_SELECT           _IOWR(IOCTL_MAGIC_NUMBER,6, int)
#define IOCTL_SPI_CHIP_SELECT_POLARITY  _IOWR(IOCTL_MAGIC_NUMBER,7, int)
#define IOCTL_SPI_WRITE                 _IOWR(IOCTL_MAGIC_NUMBER,8, int)

#define BCM2835_SPI_MODE0               0  /*!< CPOL = 0, CPHA = 0 */
#define BCM2835_SPI_MODE1               1  /*!< CPOL = 0, CPHA = 1 */
#define BCM2835_SPI_MODE2               2         /*!< CPOL = 1, CPHA = 0 */
#define BCM2835_SPI_MODE3               3   /*!< CPOL = 1, CPHA = 1 */

#define BCM2835_SPI_CS0                 0     /*!< Chip Select 0 */
#define BCM2835_SPI_CS1                 1     /*!< Chip Select 1 */
#define BCM2835_SPI_CS2                 2     /*!< Chip Select 2 (ie pins CS1 and CS2 are asserted) */
#define BCM2835_SPI_CS_NONE             3  /*!< No CS, control it yourself */

#define BCM2835_SPI_CLOCK_DIVIDER_65536  0       /*!< 65536 = 3.814697260kHz on Rpi2, 6.1035156kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_32768  32768   /*!< 32768 = 7.629394531kHz on Rpi2, 12.20703125kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_16384  16384   /*!< 16384 = 15.25878906kHz on Rpi2, 24.4140625kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_8192   8192    /*!< 8192 = 30.51757813kHz on Rpi2, 48.828125kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_4096   4096    /*!< 4096 = 61.03515625kHz on Rpi2, 97.65625kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_2048   2048    /*!< 2048 = 122.0703125kHz on Rpi2, 195.3125kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_1024   1024    /*!< 1024 = 244.140625kHz on Rpi2, 390.625kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_512    512     /*!< 512 = 488.28125kHz on Rpi2, 781.25kHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_256    256     /*!< 256 = 976.5625kHz on Rpi2, 1.5625MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_128    128     /*!< 128 = 1.953125MHz on Rpi2, 3.125MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_64     64      /*!< 64 = 3.90625MHz on Rpi2, 6.250MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_32     32      /*!< 32 = 7.8125MHz on Rpi2, 12.5MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_16     16      /*!< 16 = 15.625MHz on Rpi2, 25MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_8      8       /*!< 8 = 31.25MHz on Rpi2, 50MHz on RPI3 */
#define BCM2835_SPI_CLOCK_DIVIDER_4      4       /*!< 4 = 62.5MHz on Rpi2, 100MHz on RPI3. Dont expect this speed to work reliably. */
#define BCM2835_SPI_CLOCK_DIVIDER_2      2       /*!< 2 = 125MHz on Rpi2, 200MHz on RPI3, fastest you can get. Dont expect this speed to work reliably.*/
#define BCM2835_SPI_CLOCK_DIVIDER_1      1       /*!< 1 = 3.814697260kHz on Rpi2, 6.1035156kHz on RPI3, same as 0/65536 */

struct Data {
    char* tbuf;
    char* rbuf;
    char* buf;
    int len;
    int data;
    int ret;
    int active;
};
int analog_read(int channel);
int fd1;
struct Data *buf;

int main(){
	dev_t spi_dev;

    int channel = 0;
    buf = malloc(sizeof(struct Data));
    buf->rbuf = malloc(sizeof(char)*32);
    buf->tbuf = malloc(sizeof(char)*32);
    buf->len = 30;
	spi_dev = makedev(SPI_MAJOR_NUMBER, SPI_MINOR_NUMBER);
    mknod(SPI_DEV_PATH_NAME, S_IFCHR|0666, spi_dev);

    fd1 = open(SPI_DEV_PATH_NAME, O_RDWR);
    if(fd1<0){
        printf("fail to open spi\n");
        return -1;
    }
    buf->data = BCM2835_SPI_MODE0;
    ioctl(fd1, IOCTL_SPI_SET_DATA_MODE, &buf);

    buf->data = BCM2835_SPI_CLOCK_DIVIDER_64;
	ioctl(fd1, IOCTL_SPI_SET_CLOCK_DIVIDER, &buf);

    buf->data = BCM2835_SPI_CS0;
    ioctl(fd1, IOCTL_SPI_CHIP_SELECT, &buf);

    buf->data = BCM2835_SPI_CS0;
    buf->active = 0;
    ioctl(fd1, IOCTL_SPI_CHIP_SELECT_POLARITY, &buf);
    

	while(1){
        sleep(1);
        printf("%d\n",analog_read(channel));
	}
}

int analog_read(int channel){
    buf->data=0;
    buf->tbuf[0] = 0x06;
    buf->tbuf[1] = (channel & 0x07)<<6;
    buf->tbuf[2] = 0x0;
    buf->len=3;
    ioctl(fd1, IOCTL_SPI_TRANSFER_NB, &buf);
    return (buf->rbuf[2] | ((buf->rbuf[1] & 0x0F) << 8));
}