#! /usr/bin/zsh

if [ $1 = "1" ]; then
    cd ./i2c
    make &
    cd ..
fi

if [ $2 = "1" ]; then
    cd ./spi
    make &
    cd ..
fi

if [ $3 = "1" ]; then
    cd ./pir
    make &
    cd ..
fi

wait

cd ./i2c
sudo rmmod i2c_ioctl
echo "     Inserting i2c device drvier"
sudo insmod i2c_ioctl.ko
cd ..
cd ./spi
sudo rmmod spi_ioctl
echo "     Inserting spi device drvier"
sudo insmod spi_ioctl.ko
cd ..
cd ./pir
sudo rmmod pir_ioctl
echo "     Inserting pir device drvier"
sudo insmod pir_ioctl.ko
cd ..