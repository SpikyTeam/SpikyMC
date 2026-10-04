#!/bin/bash

LAUNCHER_APPID="team.spiky.mc.SpikyMC"

svg2png() {
    input_file="$1"
    output_file="$2"
    width="$3"
    height="$4"

    inkscape -w "$width" -h "$height" -o "$output_file" "$input_file"
}

if command -v "inkscape" && command -v "icotool" && command -v "oxipng"; then
    # Windows ICO
    d=$(mktemp -d)

    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_16.png" 16 16
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_24.png" 24 24
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_32.png" 32 32
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_48.png" 48 48
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_64.png" 64 64
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_128.png" 128 128
    svg2png ${LAUNCHER_APPID}.svg "$d/spikymc_256.png" 256 256

    oxipng --opt max --strip all --alpha --interlace 0 "$d/spikymc_"*".png"

    rm spikymc.ico && icotool -o spikymc.ico -c \
        "$d/spikymc_256.png"  \
        "$d/spikymc_128.png"  \
        "$d/spikymc_64.png"   \
        "$d/spikymc_48.png"   \
        "$d/spikymc_32.png"   \
        "$d/spikymc_24.png"   \
        "$d/spikymc_16.png"
else
    echo "ERROR: Windows icons were NOT generated!" >&2
    echo "ERROR: requires inkscape, icotool and oxipng in PATH"
fi

# replace icon in themes
cp -v ${LAUNCHER_APPID}.svg "../launcher/resources/multimc/scalable/launcher.svg"
