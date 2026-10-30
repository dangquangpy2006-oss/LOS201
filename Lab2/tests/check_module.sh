#!/bin/sh
set -eu

grep -q '^lab2_driver ' /proc/modules || {
    echo "[FAIL] lab2_driver not loaded"
    exit 1
}

echo "[PASS] Kernel module loaded"
grep '^lab2_driver ' /proc/modules
