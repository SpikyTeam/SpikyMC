#!/bin/bash
# Portable Windows bundle: copy every non-system dependency plus Qt plugins.
# ponytail: assumes MSYS2 clang64 Qt; switch to windeployqt if Qt ever becomes static.
DEST="${1:?usage: bundle-portable.sh <dist dir>}"
export PATH=/c/msys64/clang64/bin:/c/msys64/usr/bin:$PATH
BIN="/c/msys64/clang64/bin"
PLUGINS="$BIN/../share/qt6/plugins"

# plugins are found by Qt relative to qt.conf (it lives in the dist root)
for plugin in \
    platforms/qwindows.dll \
    styles/qmodernwindowsstyle.dll \
    imageformats/qgif.dll imageformats/qico.dll imageformats/qjpeg.dll imageformats/qsvg.dll imageformats/qwbmp.dll imageformats/qwebp.dll \
    iconengines/qsvgicon.dll \
    tls/qcertonlybackend.dll tls/qopensslbackend.dll tls/qschannelbackend.dll \
    networkinformation/qnetworklistmanager.dll \
    generic/qtuiotouchplugin.dll; do
    mkdir -p "$DEST/$(dirname "$plugin")"
    cp -f "$PLUGINS/$plugin" "$DEST/$plugin" 2>/dev/null || echo "skip plugin $plugin"
done

# resolve deps of every binary in the bundle, plugins included (qsvg needs Qt6Svg.dll)
deps() { ldd "$1" 2>/dev/null | awk '/=>/ { print $1 }'; }

queue=()
while read -r f; do
    while read -r d; do queue+=("$d"); done < <(deps "$f")
done < <(find "$DEST" -type f \( -name '*.exe' -o -name '*.dll' \))

while [ ${#queue[@]} -gt 0 ]; do
    dll="${queue[0]}"
    queue=("${queue[@]:1}")
    case "$dll" in
        api-ms-win*|ext-ms*) continue ;;
    esac
    [ -e "$DEST/$dll" ] && continue
    [ -e "$BIN/$dll" ] || continue
    cp -f "$BIN/$dll" "$DEST/" || exit 1
    while read -r sub; do queue+=("$sub"); done < <(deps "$BIN/$dll")
done

echo "bundle done: $DEST"
