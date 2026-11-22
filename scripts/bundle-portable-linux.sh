#!/usr/bin/env bash
# Build a self-contained Linux folder for SpikyMC.
# Usage: bundle-portable-linux.sh <path-to-spikymc-binary> <dest-dir>
set -euo pipefail

BIN=${1:?binary path required}
DEST=${2:?destination directory required}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
QT_PLUGINS=$(qmake6 -query QT_INSTALL_PLUGINS)

rm -rf "$DEST"
mkdir -p "$DEST/lib" "$DEST/plugins"
cp "$BIN" "$DEST/spikymc"
chmod +x "$DEST/spikymc"

# Everything the binary needs, except the C library and the dynamic loader.
collect() {
    ldd "$1" 2>/dev/null | awk '/=>/ {print $3}' | while read -r lib; do
        [ -f "$lib" ] || continue
        case "$(basename "$lib")" in
            libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libutil.so.*|libresolv.so.*|ld-linux*|linux-vdso*|libnss_*) continue ;;
        esac
        cp -n "$lib" "$DEST/lib/"
    done
}

collect "$DEST/spikymc"
# Qt pulls in more (ICU, pcre2, harfbuzz, ...); follow the first pass again.
for lib in "$DEST"/lib/*.so*; do
    collect "$lib"
done

for plugin_dir in platforms imageformats iconengines styles tls generic networkinformation; do
    if [ -d "$QT_PLUGINS/$plugin_dir" ]; then
        mkdir -p "$DEST/plugins/$plugin_dir"
        find "$QT_PLUGINS/$plugin_dir" -maxdepth 1 -name '*.so' -exec cp -n {} "$DEST/plugins/$plugin_dir/" \;
    fi
done
# The wayland platform plugin is optional; keep only xcb to stay small.
rm -f "$DEST/plugins/platforms/libqwayland"*.so

patchelf --set-rpath '$ORIGIN/lib' "$DEST/spikymc"

printf '[Paths]\nPrefix = .\nPlugins = plugins\n' > "$DEST/qt.conf"
cp "$ROOT/launcher/qtlogging.ini" "$DEST/"
cp "$ROOT/LICENSE" "$ROOT/COPYING.md" "$DEST/"
cp -r "$ROOT/dist/SpikyMC-1.2.0/jars" "$DEST/jars" 2>/dev/null || true
printf 'Portable build: keep all data in this folder.\n' > "$DEST/portable.txt"

(cd "$DEST" && find . -type f | sed 's|^\./||' | sort > manifest.txt)

echo "bundle done: $DEST"
