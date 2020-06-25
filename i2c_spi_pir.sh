#! /usr/bin/zsh

arr=("i2c" "spi" "pir")

for i in ${arr[@]}; do
    if [ $1 = "1" ] && [ ${i} = "i2c" ]; then
        cd ./i2c
        make &
        if [  ]
        cd ..

    elif [ $2 = "1" ] && [ ${i} = "spi" ]; then
        cd ./spi
        make &
        cd ..

    elif [ $3 = "1" ] && [ ${i} = "pir" ]; then
        cd ./pir
        make &
        cd ..
    fi
done

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