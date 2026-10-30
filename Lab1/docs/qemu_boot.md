# Lab 1 - QEMU Boot Reference

Run commands from the repository root.

    qemu-system-arm -M vexpress-a9 -cpu cortex-a9 -m 512M -smp 2 -nographic -kernel Lab1/kernel/zImage -dtb Lab1/kernel/vexpress-v2p-ca9.dtb -initrd Lab1/rootfs/initramfs.cpio.gz -append "console=ttyAMA0,115200 rdinit=/init mem=512M"

This is a reference command. Boot options must match
the kernel and root filesystem configuration.

To quit QEMU, press Ctrl+A, then X.

Verification commands inside QEMU:

    uname -a
    cat /proc/cpuinfo
    cat /proc/meminfo

Build and boot evidence is stored in Lab1/logs/.
