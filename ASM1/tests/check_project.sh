#!/bin/sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)

for f in task1_ioctl/asgn1_driver.c          task1_ioctl/asgn1_ioctl.h          task1_ioctl/test_asgn1.sh          task2_misc/counter_driver.c          task2_misc/test_counter.sh
do
    test -f "$ROOT/$f" || {
        echo "[FAIL] $f"
        exit 1
    }
    echo "[PASS] $f"
done

echo "[PASS] ASM1 source inventory"
