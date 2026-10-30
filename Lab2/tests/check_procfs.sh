#!/bin/sh
set -eu

FILE=/proc/lab2_info
test -r "$FILE"

grep -q "Driver name" "$FILE"
grep -q "Buffer size" "$FILE"
grep -q "Write count" "$FILE"
grep -q "Overwrites" "$FILE"

echo "[PASS] Procfs information"
cat "$FILE"
