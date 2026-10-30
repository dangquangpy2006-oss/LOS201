#!/bin/sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)

for f in kernel/zImage kernel/vexpress-v2p-ca9.dtb          kernel/.config rootfs/initramfs.cpio.gz          uboot/u-boot.bin
do
    test -s "$ROOT/$f" || {
        echo "[FAIL] $f"
        exit 1
    }
    echo "[PASS] $f"
done

gzip -t "$ROOT/rootfs/initramfs.cpio.gz"
echo "[PASS] Initramfs gzip"
