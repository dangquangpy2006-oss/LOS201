#!/bin/sh

echo "===== ASM1 TASK 1 TEST ====="

echo "[1] Check/Load driver"

if ! lsmod | grep -q asgn1_driver; then
    insmod /usr/lib/modules/asgn1_driver.ko
fi

lsmod | grep asgn1_driver

echo "[2] Check device"
ls -l /dev/asgn1

echo "[3] Run ioctl test"
/usr/bin/asgn1_ioctl_test

echo "[4] Unload driver"
rmmod asgn1_driver
echo "[5] Check module after unload"
lsmod | grep asgn1_driver

echo "[6] Check device after unload"
ls -l /dev/asgn1
echo "===== TEST FINISHED ====="

