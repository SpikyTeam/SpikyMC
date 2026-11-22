#!/usr/bin/env bash
# Bundle the CMake install tree into a self-contained portable folder.
set -euo pipefail

SRC="$HOME/spikymc-install"
DEST="$HOME/spikymc-portable"
QT_PLUGINS=$(qmake6 -query QT_INSTALL_PLUGINS)

rm -rf "$DEST"
cp -a "$SRC" "$DEST"
mkdir -p "$DEST/lib" "$DEST/plugins"

collect() {
    ldd "$1" 2>/dev/null | awk '/=>/ {print $3}' | while read -r lib; do
        [ -f "$lib" ] || continue
        case "$(basename "$lib")" in
            libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libutil.so.*|libresolv.so.*|ld-linux*|libnss_*) continue ;;
        esac
        cp -n "$lib" "$DEST/lib/"
    done
}

collect "$DEST/bin/spikymc"
collect "$DEST/bin/spikymc_updater"
for lib in "$DEST"/lib/*.so*; do
    collect "$lib"
done

for dir in platforms imageformats iconengines styles tls generic networkinformation; do
    [ -d "$QT_PLUGINS/$dir" ] || continue
    mkdir -p "$DEST/plugins/$dir"
    find "$QT_PLUGINS/$dir" -maxdepth 1 -name '*.so' -exec cp -n {} "$DEST/plugins/$dir/" \;
done
rm -f "$DEST"/plugins/platforms/libqwayland*.so

patchelf --set-rpath '$ORIGIN/../lib' "$DEST/bin/spikymc"
patchelf --set-rpath '$ORIGIN/../lib' "$DEST/bin/spikymc_updater"

printf '[Paths]\nPrefix = ..\nPlugins = plugins\n' > "$DEST/bin/qt.conf"
printf 'Portable build: data stays in this folder.\n' > "$DEST/portable.txt"

(cd "$DEST" && find . \( -type f -o -type l \) | sed 's|^\./||' | sort > manifest.txt)

du -sh "$DEST"
