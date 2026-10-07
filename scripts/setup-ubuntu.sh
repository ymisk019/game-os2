#!/usr/bin/env bash
# Install everything needed to build and run Yazan OS on Ubuntu/Debian (also works in WSL2).
set -e
sudo apt update
sudo apt install -y build-essential grub-pc-bin grub-efi-amd64-bin grub-common xorriso mtools \
                    qemu-system-x86 ovmf
echo "Done. Now run: make iso && make run"
