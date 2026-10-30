# LOS201 - Lab 2

## Student
SE201072 - Nguyen Dang Quang

## Character Device Driver
- Device: /dev/lab2
- Major number: 240
- Class: lab2_class
- Procfs: /proc/lab2_info

## Features
- Character device read and write
- Mutex synchronization
- Procfs statistics
- Sysfs attributes
- B1: Seek support
- B2: Reverse-read mode
- B3: Overwrite counter

## Build
cd ~/embedded_lab2/driver
make

## Testing on QEMU
cat /proc/lab2_info
ls /sys/class/lab2_class/lab2/

## Reverse Read
printf "abc" > /dev/lab2
echo 1 > /sys/class/lab2_class/lab2/reverse_read
cat /dev/lab2

Expected: cba

## Overwrite Counter
After loading the module:
echo first > /dev/lab2
echo second > /dev/lab2
echo third > /dev/lab2

Check:
cat /sys/class/lab2_class/lab2/overwrite_count

Expected: 2 (if the buffer was initially empty)

## Module Cleanup
rmmod lab2_driver
dmesg | tail -n 20
