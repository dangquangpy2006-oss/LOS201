#!/bin/sh
set -eu

DEV=/dev/lab2
COUNT=/sys/class/lab2_class/lab2/overwrite_count

printf clear > "$DEV"
before=$(cat "$COUNT")

echo first > "$DEV"
echo second > "$DEV"
echo third > "$DEV"

after=$(cat "$COUNT")
expected=$((before + 2))

[ "$after" -eq "$expected" ] || {
    echo "[FAIL] Expected $expected, got $after"
    exit 1
}

grep -F "Overwrites : $expected" /proc/lab2_info >/dev/null

echo "[PASS] Overwrite counter increased by 2"
echo "[PASS] Procfs matches Sysfs"
