#!/usr/bin/env bash
# Boot an ISO headless in QEMU, wait for a string on the serial port, take a
# screenshot, and report PASS/FAIL.   (used by GitHub Actions; works locally too)
#   usage: scripts/qemu-test.sh <iso> <name> <expected-text> [forbidden-text]
# Needs: qemu-system-x86_64, socat.  Output: out/<name>.log and out/<name>.ppm
set -u
ISO="$1"; NAME="$2"; EXPECT="$3"; FORBID="${4:-}"
OUT=out
mkdir -p "$OUT"
rm -f "$OUT/$NAME.log" "$OUT/$NAME.ppm" "$OUT/$NAME.sock"

qemu-system-x86_64 -cdrom "$ISO" -m 256M -vga std -display none \
    -no-reboot -no-shutdown \
    -serial "file:$OUT/$NAME.log" \
    -monitor "unix:$OUT/$NAME.sock,server,nowait" &
QPID=$!

for _ in $(seq 1 45); do
    sleep 1
    grep -q "$EXPECT" "$OUT/$NAME.log" 2>/dev/null && break
    kill -0 "$QPID" 2>/dev/null || break          # QEMU exited (e.g. triple fault with -no-reboot)
done
sleep 2

if [ -S "$OUT/$NAME.sock" ]; then
    echo "screendump $OUT/$NAME.ppm" | socat - "UNIX-CONNECT:$OUT/$NAME.sock" >/dev/null 2>&1
    sleep 1
    echo quit | socat - "UNIX-CONNECT:$OUT/$NAME.sock" >/dev/null 2>&1
fi
kill "$QPID" 2>/dev/null; wait "$QPID" 2>/dev/null

echo "----- serial log ($NAME) -----"
cat "$OUT/$NAME.log" 2>/dev/null
echo "------------------------------"

if ! grep -q "$EXPECT" "$OUT/$NAME.log" 2>/dev/null; then
    echo "FAIL [$NAME]: expected text not found: $EXPECT"; exit 1
fi
if [ -n "$FORBID" ] && grep -q "$FORBID" "$OUT/$NAME.log"; then
    echo "FAIL [$NAME]: found forbidden text: $FORBID"; exit 1
fi
echo "PASS [$NAME]"
