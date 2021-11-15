obj-m := pseudo_platform_device.o pseudo_platform_driver.o

BEAGLEBONE_LINUX_KERNEL_DIR = /media/hdd/jstand_jakubstandarski/jstand/$\
courses/embedded_linux/beaglebone_demo_files/linux/
BEAGLEBONE_CFLAGS = ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf-


HOST_LINUX_KERNEL_DIR = /lib/modules/$(shell uname -r)/build/
HOST_CFLAGS =


ifdef BEAGLEBONE
LINUX_KERNEL_DIR = $(BEAGLEBONE_LINUX_KERNEL_DIR)
CFLAGS = $(BEAGLEBONE_CFLAGS)
else
LINUX_KERNEL_DIR = $(HOST_LINUX_KERNEL_DIR)
CFLAGS = $(HOST_CFLAGS)
endif


build:
	make $(CFLAGS) -C ${LINUX_KERNEL_DIR} M=${PWD} modules -j4

clean:
	make -C ${LINUX_KERNEL_DIR} M=${PWD} clean

help:
	make $(CFLAGS) -C ${LINUX_KERNEL_DIR} M=${PWD} help
