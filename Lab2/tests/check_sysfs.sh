#!/bin/sh
set -eu

DIR=/sys/class/lab2_class/lab2

for attr in buffer_len buffer_size open_count read_count write_count status reverse_read overwrite_count
do
    test -r "$DIR/$attr" || {
        echo "[FAIL] Missing: $attr"
        exit 1
    }
    echo "[PASS] $attr"
done
