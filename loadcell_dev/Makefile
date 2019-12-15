KERNEL_DIR = /lib/modules/4.19.75-v7+/build

obj-m := loadcell_dev.o

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
