#!/bin/sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CONFIG="$ROOT/kernel/.config"

test -f "$CONFIG"
echo "Kernel configuration:"
grep -E '^(CONFIG_(MODULES|DEVTMPFS|POSIX_MQUEUE)=|# CONFIG_(MODULES|DEVTMPFS|POSIX_MQUEUE) is not set)' "$CONFIG" || true
echo "[OK] Configuration inspected"
