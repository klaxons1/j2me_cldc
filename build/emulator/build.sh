#!/bin/bash
#
# build.sh - builds the J2ME MIDP 2.0 emulator for Windows.
#
# The default target is a 32-bit (i386) Windows PE executable. This is
# the ABI that the KVM 32-bit cell-based heap requires, and 32-bit
# executables run natively on 64-bit Windows (WoW64) as well.
#
# The build is performed with the Zig C compiler used as a
# cross-compiler ("zig cc -target i686-windows-gnu"), which bundles
# clang plus the mingw-w64 headers/libraries, so no separate toolchain
# installation is required.
#
# Usage:
#   ./build.sh [ZIG_BIN] [TARGET]
#
#   ZIG_BIN   path to the zig binary (default: auto-detect)
#   TARGET    x86-windows-gnu (default)
#
# Output: dist/j2me-win/midp.exe plus the MIDP 2.0 FCS runtime files.

set -e

TOP="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$TOP/build/emulator"
OBJDIR="$BUILD/obj"
DIST="$TOP/dist/j2me-win"

ZIG="${1:-}"
TARGET="${2:-x86-windows-gnu}"
EXENAME="${3:-midp.exe}"

# 32-bit targets need -m32; 64-bit (x86_64-windows-gnu) does not.
M32_FLAGS=()
case "$TARGET" in
    x86-*) M32_FLAGS=(-m32) ;;
esac

# keep object files separate per target
if [ "$TARGET" != "x86-windows-gnu" ]; then
    OBJDIR="$BUILD/obj-$TARGET"
fi

if [ -z "$ZIG" ]; then
    if [ -x "$HOME/.tools/zigvenv/lib/python3.11/site-packages/ziglang/zig" ]; then
        ZIG="$HOME/.tools/zigvenv/lib/python3.11/site-packages/ziglang/zig"
    elif command -v zig >/dev/null 2>&1; then
        ZIG="zig"
    else
        ZIG="$(python3 -c 'import ziglang, os; print(os.path.join(os.path.dirname(ziglang.__file__), "zig"))' 2>/dev/null)"
    fi
fi
if [ -z "$ZIG" ] || [ ! -x "$ZIG" ]; then
    echo "error: zig compiler not found (install with: pip install ziglang)" >&2
    exit 1
fi
echo "Using compiler: $ZIG (target: $TARGET)"
"$ZIG" version

mkdir -p "$OBJDIR" "$DIST"

# ---------------------------------------------------------------------
# 1. Generate the native function table from the prebuilt classes
# ---------------------------------------------------------------------
python3 "$TOP/tools/gen_native_table/gen_native_table.py" \
    "$TOP/midp2.0fcs/classes" "$OBJDIR/nativeFunctionTable.c" \
    --csrc "$TOP/kvm/VmCommon/src" "$TOP/kvm/VmWin/src" \
          "$TOP/kvm/VmExtra/src" \
          "$TOP/midp2.0fcs/src/share/native" \
          "$TOP/midp2.0fcs/src/win32/native" \
    || { echo "native table generation failed"; exit 1; }

# ---------------------------------------------------------------------
# 2. Compile everything
# ---------------------------------------------------------------------

MIDP="$TOP/midp2.0fcs"

DEFS=(
    -DWIN32 -DGCC -DROMIZING=0 -DMAXIMUMHEAPSIZE=1000000
    -DDEFAULTHEAPSIZE=1000000
    -DENABLE_JAVA_DEBUGGER=0 -DUSE_DEBUG_COLLECTOR=0 -DENABLEPROFILING=0
    -DINCLUDE_I18N -DUSE_KVM=1 -DGENERIC_IO_WAIT_TIME=10 -DPADTABLE=1
    "-DPLATFORMNAME=\"j2me\"" "-DRELEASE=\"J2ME x64\""
    "-DIMPL_VERSION=\"2.0\"" "-DFULL_VERSION=\"2.0.01\""
    -D_CRT_SECURE_NO_WARNINGS
)

# MIDP's win32/native must precede kvm/VmWin/h so that machine_md.h resolves
# to the MIDP build (the KVM stub #defines GetAndStoreNextKVMEvent away).
INCS=(
    -I"$MIDP/src/win32/native" -I"$MIDP/src/win32/native/i18n"
    -I"$MIDP/src/share/native" -I"$MIDP/src/share/native/kvm"
    -I"$MIDP/src/share/native/i18n"
    -I"$TOP/kvm/VmCommon/h" -I"$TOP/kvm/VmWin/h" -I"$TOP/kvm/VmExtra/h"
    -I"$TOP/kvm/VmCommon/src" -I"$TOP/jam/h"
)

CFLAGS=( -std=gnu89 -O2 -fno-strict-aliasing "${DEFS[@]}" "${INCS[@]}" )

cc() {
    local src="$1"
    local name
    name="$(basename "$src" .c)"
    name="${name%.cpp}"
    "$ZIG" cc -target "$TARGET" "${M32_FLAGS[@]}" "${CFLAGS[@]}" -c "$src" -o "$OBJDIR/$name.o"
}

KVM_SRC="$TOP/kvm/VmCommon/src"
KVM_WIN="$TOP/kvm/VmWin/src"
KVM_EXTRA="$TOP/kvm/VmExtra/src"
MIDP_SHARE="$MIDP/src/share/native"
MIDP_WIN="$MIDP/src/win32/native"

# --- KVM core
for f in cache.c class.c fields.c frame.c garbage.c collector.c global.c \
         interpret.c execute.c loader.c native.c nativeCore.c iowaitShim.c \
         pool.c thread.c events.c hashtable.c profiling.c StartJVM.c \
         verifier.c verifierUtil.c log.c stackmap.c kni.c; do
    cc "$KVM_SRC/$f"
done

# --- KVM file/zip loader + sockets + async (VmExtra)
for f in loaderFile.c jar.c inflate.c socketProtocol.c async.c resource.c; do
    cc "$KVM_EXTRA/$f"
done

# --- KVM Windows runtime
for f in runtime_md.c runtime2_md.c socketProtocol_md.c; do
    cc "$KVM_WIN/$f"
done

# --- floating point (CLDC 1.1 Math + float opcodes)
for f in fp_math.c; do
    cc "$KVM_SRC/$f"
done
for f in "$KVM_EXTRA/fp"/*.c; do
    cc "$f"
done

# --- generated native table
cc "$OBJDIR/nativeFunctionTable.c"

# --- MIDP shared native
for f in property.c configuration.c imageDecode.c pngDecode.c \
         defaultLCDUI.c menus.c popup.c commandState.c exitInternal.c \
         midpStartup.c SystemOutputStream.c ResourceInputStream.c \
         JarReader.c storage.c RandomAccessStream.c storageFile.c \
         socketProtocol.c datagramProtocol.c commProtocol.c phonesim.c \
         pushregistry.c midlet.c audiornd.c vibrate.c crc32.c \
         screenGrabber.c; do
    cc "$MIDP_SHARE/$f"
done
for f in "$MIDP_SHARE/kvm"/*.c; do
    cc "$f"
done
for f in "$MIDP_SHARE/i18n"/localeMethod.c "$MIDP_SHARE/i18n"/conv.c; do
    cc "$f"
done

# --- MIDP Windows native
for f in nativeGUI.c images.c text.c graphics.c staticGraphics.c \
         dirent.c midlet_md.c socketProtocol_md.c datagramProtocol_md.c \
         commProtocol_md.c mmaevt.c mmatone.c waveout.c screengrab.c \
         pvibrate.c; do
    cc "$MIDP_WIN/$f"
done
for f in "$MIDP_WIN/i18n"/locale.c "$MIDP_WIN/i18n"/genConv.c; do
    cc "$f"
done

# ---------------------------------------------------------------------
# 3. Link
# ---------------------------------------------------------------------
"$ZIG" cc -target "$TARGET" "${M32_FLAGS[@]}" -O2 \
    -o "$DIST/$EXENAME" \
    "$OBJDIR"/*.o \
    -luser32 -lgdi32 -lwinmm -lwsock32 -limm32 -lcomdlg32 -lkernel32 -static
echo "Built $DIST/$EXENAME"

# ---------------------------------------------------------------------
# 4. Assemble the distribution
# ---------------------------------------------------------------------
rm -rf "$DIST/lib" "$DIST/games"
mkdir -p "$DIST/lib" "$DIST/games"

# base classes (loaded from the classpath in this build)
cp -r "$MIDP/classes" "$DIST/lib/classes"

# configuration + images
for f in system.config internal.config; do
    if [ -f "$MIDP/lib/$f" ]; then
        cp "$MIDP/lib/$f" "$DIST/lib/"
    fi
done
cp "$MIDP"/lib/*.png "$DIST/lib/" 2>/dev/null || true

# storage root
mkdir -p "$DIST/appdb"
touch "$DIST/appdb/.keep"

# documentation
cp "$BUILD/README.txt" "$DIST/README.txt"

# example MIDlets
cp "$MIDP"/example/*.jar "$DIST/games/" 2>/dev/null || true
cp "$MIDP"/example/*.jad "$DIST/games/" 2>/dev/null || true

echo "Distribution assembled in $DIST"
ls -la "$DIST"
