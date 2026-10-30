#!/bin/sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)

for f in app/main.c          app/sensor_thread.c          app/processor_thread.c          app/alert_thread.c          app/logger_thread.c          app/circular_buffer.c
do
    test -f "$ROOT/$f" || {
        echo "[FAIL] $f"
        exit 1
    }
    echo "[PASS] $f"
done

echo "[PASS] ASM2 source inventory"
