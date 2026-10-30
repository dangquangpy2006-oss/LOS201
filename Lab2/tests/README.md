# Lab 2 - B1 B2 B3 Testing Guide

## B1 - Seek Support
Test file: test_seek.c

Compile for ARM:
arm-linux-gnueabihf-gcc -static test_seek.c -o test_seek

Expected:
- Valid seek succeeds
- Invalid seek returns EINVAL
- SEEK_END works correctly

## B2 - Reverse Read
Test file: test_reverse_read.sh

Expected:
- reverse_read=1 returns reversed data
- reverse_read=0 returns normal data
- Invalid values are rejected

## B3 - Overwrite Counter
Test file: test_overwrite.sh

Expected:
- Overwrite counter increases correctly
- Sysfs and Procfs show matching values

## Verification
Build the updated driver and test on QEMU ARM.
