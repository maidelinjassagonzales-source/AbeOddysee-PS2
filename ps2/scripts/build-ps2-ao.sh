#!/bin/bash
# Compila Abe's Oddysee para PS2 (RELIVE_PS2_GAME=AO).
#   build-ps2-ao.sh        -> versión de pruebas para PCSX2 (lee los datos con host:) -> out/ao/ODDYSEE.ELF
#   build-ps2-ao.sh disc   -> versión para PS2 real / ISO / OPL                      -> out/ao/disc/ODDYSEE.ELF
. /etc/profile.d/ps2dev.sh
SRC=${SRC:-/mnt/c/Users/dgvpr/OneDrive/Escritorio/PS2Exoddus/alive_reversing}
OUTROOT=${OUTROOT:-/mnt/c/Users/dgvpr/OneDrive/Escritorio/PS2Exoddus/out/ao}
TC=$(find "$PS2DEV" -name "ps2dev.cmake" | head -1)
if [ "$1" = "disc" ]; then
  BUILD=${BUILD:-/root/build-ps2-ao-disc}; HOSTFS=OFF; OUT=$OUTROOT/disc; LOG=/root/build-ao-disc.log
else
  BUILD=${BUILD:-/root/build-ps2-ao}; HOSTFS=ON; OUT=$OUTROOT; LOG=/root/build-ao.log
fi
if [ ! -f "$BUILD/build.ninja" ]; then
  cmake -S "$SRC" -B "$BUILD" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TC" \
    -DCMAKE_BUILD_TYPE=Release \
    -DRELIVE_PS2_GAME=AO \
    -DRELIVE_PS2_HOSTFS=$HOSTFS \
    -DSDL3_DIR="$PS2SDK/ports/lib/cmake/SDL3" || exit 1
fi
cmake --build "$BUILD" --target relive -j"$(nproc)" -- -k 0 > "$LOG" 2>&1
echo "exit=$?"
echo "errors: $(grep -c 'error:' "$LOG")"
grep 'error:' "$LOG" | sed 's|.*/alive_reversing/||' | sort | uniq -c | sort -rn | head -60
grep -q 'undefined reference' "$LOG" && grep -o "undefined reference to .*" "$LOG" | sort | uniq -c | sort -rn | head -40
if [ -f "$BUILD/Source/relive/relive" ] && ! grep -q 'error:' "$LOG"; then
  mkdir -p "$OUT"
  mips64r5900el-ps2-elf-strip -o "$OUT/ODDYSEE.ELF" "$BUILD/Source/relive/relive"
  mips64r5900el-ps2-elf-size "$BUILD/Source/relive/relive" | tail -1
  echo "ELF en $OUT/ODDYSEE.ELF"
fi
