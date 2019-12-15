sudo rmmod loadcell_dev.ko
sudo rm -rf /dev/loadcell_dev

make clean
make
sudo insmod loadcell_dev.ko
sudo mknod -m 666 /dev/loadcell_dev c 502 100
