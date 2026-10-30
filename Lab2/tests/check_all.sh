#!/bin/sh
set -eu
DIR=$(cd "$(dirname "$0")" && pwd)

sh "$DIR/check_module.sh"
sh "$DIR/check_device.sh"
sh "$DIR/check_sysfs.sh"
sh "$DIR/check_procfs.sh"

echo "[PASS] Lab 2 health checks"
