#!/usr/bin/env bash
set -euo pipefail
export PATH="$HOME/bin:$PATH"
OUT=/mnt/c/Users/mel1x/Desktop/SpikyMC-Linux-x86_64.AppImage
rm -f "$OUT" "$OUT.zsync"
VERSION=1.2.0 mkappimage \
    --updateinformation 'gh-releases-zsync|SpikyTeam|SpikyMC|latest|SpikyMC-Linux-x86_64.AppImage.zsync' \
    "$HOME/spikymc-appdir" "$OUT"
ls -lh "$OUT" "$OUT.zsync"
