#!/bin/sh
set -eu

DEV=/dev/lab2
MODE=/sys/class/lab2_class/lab2/reverse_read

original=$(cat "$MODE")
trap 'printf "%s" "$original" > "$MODE"' EXIT

printf abc > "$DEV"
printf 1 > "$MODE"

result=$(cat "$DEV")
[ "$result" = "cba" ] || exit 1
echo "[PASS] Reverse read: cba"

printf 0 > "$MODE"

result=$(cat "$DEV")
[ "$result" = "abc" ] || exit 1
echo "[PASS] Normal read: abc"

if (printf 2 > "$MODE") 2>/dev/null; then
    echo "[FAIL] Invalid value accepted"
    exit 1
fi

echo "[PASS] Invalid value rejected"
