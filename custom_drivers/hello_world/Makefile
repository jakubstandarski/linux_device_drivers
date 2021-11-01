obj-m := hello_world_driver.o

linux_kernel_dir = /media/hdd/jstand_jakubstandarski/jstand/courses/$\
embedded_linux/beaglebone_demo_files/linux/

all:
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C\
	${linux_kernel_dir} M=${PWD} modules -j4

clean:
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C\
	${linux_kernel_dir} M=${PWD} clean

help:
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C\
	${linux_kernel_dir} M=${PWD} help
