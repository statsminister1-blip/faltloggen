#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Gör en hel USB-fil (startprogram, partitionstabell, programmet) som skrivs på adress 0x0.
# Användning: tools/merge.sh <byggmapp> <ut.bin>
#   byggmappen är den som angavs med --build-path till arduino-cli (där Faltloggen.ino.bin ligger).
# Kräver esptool (pip install esptool) och boot_app0.bin från ESP32-kärnan (BOOT_APP0, annars sökt i ~/.arduino15).
set -e
B=$1; OUT=$2
APP0=${BOOT_APP0:-$(find "$HOME/.arduino15" /tmp/ard/data -name boot_app0.bin -path "*3.0.7*" 2>/dev/null | head -1)}
ET=${ESPTOOL:-esptool.py}
$ET --chip esp32s3 merge_bin -o "$OUT" --flash_mode keep --flash_freq keep --flash_size 16MB \
  0x0 "$B/Faltloggen.ino.bootloader.bin" 0x8000 "$B/Faltloggen.ino.partitions.bin" \
  0xe000 "$APP0" 0x10000 "$B/Faltloggen.ino.bin"
