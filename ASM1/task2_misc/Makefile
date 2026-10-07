obj-m += counter_driver.o

KDIR := /home/nguyendangquang/embedded_lab1/kernel/linux-5.15
PWD := $(shell pwd)

ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
