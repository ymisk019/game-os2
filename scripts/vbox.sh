#!/usr/bin/env bash
# Create (once) and start a VirtualBox VM for Yazan OS.
# Usage: scripts/vbox.sh [path/to/yazanos.iso]
# On Windows (Git Bash) VBoxManage is usually at "/c/Program Files/Oracle/VirtualBox/VBoxManage.exe".
set -euo pipefail
ISO="${1:-build/yazanos.iso}"
VM="Yazan OS"
VBM="${VBOXMANAGE:-VBoxManage}"

[ -f "$ISO" ] || { echo "ISO not found: $ISO (run 'make iso' first)"; exit 1; }
mkdir -p build

if ! "$VBM" showvminfo "$VM" >/dev/null 2>&1; then
    "$VBM" createvm --name "$VM" --ostype Other_64 --register
    "$VBM" modifyvm "$VM" --memory 256 --vram 32 --cpus 1 --ioapic on \
        --graphicscontroller vboxvga --firmware bios \
        --boot1 dvd --boot2 none --boot3 none --boot4 none \
        --uart1 0x3F8 4 --uartmode1 file "$PWD/build/serial.log"
    "$VBM" storagectl "$VM" --name IDE --add ide
fi

"$VBM" storageattach "$VM" --storagectl IDE --port 0 --device 0 --type dvddrive --medium "$ISO"
"$VBM" startvm "$VM"
echo "Serial/debug log: build/serial.log"
