#!/bin/sh

echo "===== COUNTER DRIVER TEST ====="

echo "[1] Load driver"
insmod /usr/lib/modules/counter_driver.ko

echo "[2] Check module"
lsmod | grep counter_driver

echo "[3] Check device"
ls -l /dev/counter
echo "[4] Read counter"
cat /dev/counter

echo "[5] Read counter again"
cat /dev/counter

echo "[6] Set counter to 10"
echo 10 > /dev/counter

echo "[7] Read counter after write"
cat /dev/counter
echo "[8] Check proc info"
cat /proc/counter_info
echo "[9] Reset counter to 0"
echo 0 > /dev/counter

echo "[10] Check proc after reset"
cat /proc/counter_info
echo "[11] Unload driver"
rmmod counter_driver

echo "[12] Check module after unload"
lsmod | grep counter_driver

echo "[13] Check device after unload"
ls -l /dev/counter

echo "[14] Check proc after unload"
ls -l /proc/counter_info

echo "===== TEST FINISHED ====="
