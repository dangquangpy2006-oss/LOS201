#!/bin/sh
set -eu

test -c /dev/lab2 || {
    echo "[FAIL] /dev/lab2 missing"
    exit 1
}

echo "[PASS] Character device exists"
ls -l /dev/lab2
