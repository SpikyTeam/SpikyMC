#!/usr/bin/env bash
# Build the AppImage from the CMake install tree (mirrors the CI linux action).
set -euo pipefail

ROOT=/mnt/c/Users/mel1x/Desktop/SpikyMC
APPDIR="$HOME/spikymc-appdir"
BUILD="$HOME/spikymc-build"
QT_PLUGINS=$(qmake6 -query QT_INSTALL_PLUGINS)
export PATH="$HOME/bin:$PATH"

rm -rf "$APPDIR"
cmake --install "$BUILD" --config Release --prefix "$APPDIR" >/dev/null

sharun lib4bin --hard-links --with-hooks --dst-dir "$APPDIR" "$APPDIR"/bin/* "$QT_PLUGINS"/*/*.so >/dev/null

find "$APPDIR" -name '*gamemode*' -delete

echo 'SPIKYMC_DISABLE_GLVULKAN=1' >> "$APPDIR"/.env
echo 'QT_QPA_PLATFORMTHEME=xdgdesktopportal' >> "$APPDIR"/.env

ln -sf team.spiky.mc.SpikyMC.metainfo.xml "$APPDIR"/share/metainfo/team.spiky.mc.SpikyMC.appdata.xml
ln -sf share/applications/team.spiky.mc.SpikyMC.desktop "$APPDIR"
ln -sf share/icons/hicolor/256x256/apps/team.spiky.mc.SpikyMC.png "$APPDIR"
mv "$APPDIR"/sharun "$APPDIR"/AppRun

OUT=/mnt/c/Users/mel1x/Desktop/SpikyMC-Linux-x86_64.AppImage
rm -f "$OUT" "$OUT.zsync"
mkappimage --updateinformation 'gh-releases-zsync|SpikyTeam|SpikyMC|latest|SpikyMC-Linux-x86_64.AppImage.zsync' "$APPDIR" "$OUT"
ls -lh "$OUT" "$OUT.zsync"
