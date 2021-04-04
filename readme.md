# SystemProgramming Device Driver

## 목차
- Server
- I2C
- SPI
- PWM

## Needs
- equal or more than 3 raspberry Pi 3+

## Before Start
``` Shell
$ sudo apt update
$ sudo apt upgrade
$ sudo apt install raspberrypi-kernel-headers
```

## Server
``` Shell
$ gcc server.c -o server.out -lpthread
$ ./server.out [PORT]
```

## I2C Device Driver
``` Shell
$ cd i2c
$ make
$ sudo rmmod i2c_ioctl
$ sudo insmod i2c_ioctl.ko
```

## PIR Device Driver
``` Shell
$ cd pir
$ make
$ sudo rmmod pir_ioctl
$ sudo insmod pir_ioctl.ko
```

## SPI Device Driver
``` Shell
$ cd spi
$ make
$ sudo rmmod spi_ioctl
$ sudo insmod spi_ioctl.ko
```

## ...or just run i2c_spi_pir.sh
``` Shell
$ sh i2c_spi_pir.sh 1 1 1
```

