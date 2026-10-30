# Lab 2 - Testing Checklist

## Before testing
1. Build the module with make.
2. Check ARM architecture with file lab2_driver.ko.
3. Place the updated module in the QEMU root filesystem.
4. Boot QEMU and confirm the module is loaded.

## Read and write
- Confirm /dev/lab2 exists.
- Write data using printf.
- Read the data back using cat.
- Verify the clear command empties the buffer.

## B1
- Run test_seek_arm.
- Check valid seeks, SEEK_END and invalid offsets.

## B2
- Run test_reverse_read.sh.
- Check normal, reversed and invalid modes.

## B3
- Run test_overwrite.sh.
- Compare Sysfs and Procfs counters.

## System health
Run check_all.sh after copying the test scripts
into the QEMU root filesystem.

A successful build alone is not proof of runtime correctness.
