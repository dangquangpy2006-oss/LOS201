obj-m += sms_sensor_driver.o

KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
PWD := $(shell pwd)

.PHONY: all arm clean

all: arm

arm:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- clean
