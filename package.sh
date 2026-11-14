#!/usr/bin/env bash
set -euo pipefail

export PS3DEV="${PS3DEV:-/mnt/d/ps3dev-sdk}"
export PSL1GHT="${PSL1GHT:-$PS3DEV}"
export PORTLIBS="${PORTLIBS:-$PS3DEV/portlibs/ppu}"
export PATH="$PS3DEV/bin:$PS3DEV/ppu/bin:$PATH"

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

TITLE_ID="CPC4PS300"
CONTENT_ID="UP0001-CPC4PS300_00-0000000000000000"
PKG_DIR="$ROOT/build/pkg/$TITLE_ID"
DIST_DIR="$ROOT/dist"

if [ ! -f "$ROOT/cpc_ps3.elf" ]; then
    "$ROOT/build.sh"
fi

rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/USRDIR/rom" "$PKG_DIR/USRDIR/resources" "$DIST_DIR"

cp -f "$ROOT/cpc_ps3.elf" "$PKG_DIR/USRDIR/cpc_ps3.elf"
ppu-strip "$PKG_DIR/USRDIR/cpc_ps3.elf"
sprxlinker "$PKG_DIR/USRDIR/cpc_ps3.elf"
make_self_npdrm "$PKG_DIR/USRDIR/cpc_ps3.elf" "$PKG_DIR/USRDIR/EBOOT.BIN" "$CONTENT_ID"
rm -f "$PKG_DIR/USRDIR/cpc_ps3.elf"

sfo -f "$ROOT/sfo.xml" "$PKG_DIR/PARAM.SFO" --title="CPC4PS3" --appid="$TITLE_ID"

cp -f "$ROOT/assets/ICON0.PNG" "$PKG_DIR/ICON0.PNG"
if [ -f "$ROOT/assets/PIC1.PNG" ]; then
    cp -f "$ROOT/assets/PIC1.PNG" "$PKG_DIR/PIC1.PNG"
fi

cp -f "$ROOT/rom/"*.rom "$PKG_DIR/USRDIR/rom/"
cp -f "$ROOT/rom/system.cpr" "$PKG_DIR/USRDIR/rom/"
cp -f "$ROOT/resources/keymap_us.map" "$PKG_DIR/USRDIR/resources/"
cp -f "$ROOT/assets/font.ttf" "$PKG_DIR/USRDIR/font.ttf"

pkg --contentid="$CONTENT_ID" "$PKG_DIR/" "$DIST_DIR/CPC4PS3.pkg"
cp -f "$DIST_DIR/CPC4PS3.pkg" "$DIST_DIR/CPC4PS3.gnpdrm.pkg"
package_finalize "$DIST_DIR/CPC4PS3.gnpdrm.pkg"

echo "PKG pronti in $DIST_DIR:"
ls -lh "$DIST_DIR/CPC4PS3.pkg" "$DIST_DIR/CPC4PS3.gnpdrm.pkg"
