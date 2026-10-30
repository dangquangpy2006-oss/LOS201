#!/bin/sh
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
LOG="$ROOT/SE201072_NguyenDangQuang_Lab03/logs"

for f in valgrind_report.txt strace_full.txt          perf_stat.txt gdb_session.txt arm_performance.txt
do
    test -f "$LOG/$f" || {
        echo "[FAIL] Missing $f"
        exit 1
    }
    echo "[PASS] $f"
done
