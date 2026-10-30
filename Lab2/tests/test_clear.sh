#!/bin/sh
set -eu

DEV=/dev/lab2
SYS=/sys/class/lab2_class/lab2

printf hello > "$DEV"
printf clear > "$DEV"

status=$(cat "$SYS/status")
length=$(cat "$SYS/buffer_len")

test "$status" = "EMPTY"
test "$length" = "0"

echo "[PASS] Clear command"
echo "[PASS] Buffer is empty"
