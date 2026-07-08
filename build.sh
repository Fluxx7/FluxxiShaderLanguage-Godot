#!/usr/bin/env bash
# Convenience build wrapper around SCons for the FSL GDExtension.
#
#   ./build.sh [target] [arch]
#     target : template_debug (default) | template_release | editor
#     arch   : arm64 | x86_64   (default: host arch)
#
# Builds with debug_symbols for non-release targets (so Godot crash backtraces
# symbolicate), generates compile_commands.json for IntelliSense, then copies
# the arch-suffixed artifact to the plain name the .gdextension actually loads.
set -euo pipefail
cd "$(dirname "$0")"

TARGET="${1:-template_debug}"
ARCH="${2:-$(uname -m)}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# Shared content-addressed object cache (honored by godot-cpp/SConstruct via
# CacheDir). Lets builds with different flags (e.g. debug_symbols on/off) reuse
# previously compiled objects instead of recompiling from scratch. Export the
# same SCONS_CACHE in your own shell so manual `scons` runs share this cache too.
export SCONS_CACHE="${SCONS_CACHE:-$(pwd)/.scons_cache}"

EXTRA=(compiledb=yes)
if [ "$TARGET" != "template_release" ]; then
  EXTRA+=(debug_symbols=yes)
fi

echo "==> scons platform=macos arch=$ARCH target=$TARGET ${EXTRA[*]} -j$JOBS"
scons platform=macos arch="$ARCH" target="$TARGET" "${EXTRA[@]}" -j"$JOBS"

# scons installs an arch-suffixed dylib, but fluxxishaderlang.gdextension loads
# the non-suffixed filename. Mirror it so the freshly built lib is the one Godot opens.
BINDIR="project/addons/fluxxishaderlang/bin/macos"
SRC="$BINDIR/libfluxxishaderlang.macos.${TARGET}.${ARCH}.dylib"
DST="$BINDIR/libfluxxishaderlang.macos.${TARGET}.dylib"
if [ -f "$SRC" ]; then
  # Atomic replace: write to a temp then rename so $DST gets a fresh inode.
  # Overwriting in place (cp -f) reuses the inode, leaving stale code-signature
  # validation state in the kernel's page cache -> Godot dies with
  # "SIGKILL (Code Signature Invalid) / Invalid Page" unless you wait for the
  # vnode to be evicted. rename() swaps the dir entry atomically and avoids it.
  TMP="$DST.tmp.$$"
  cp -f "$SRC" "$TMP"
  mv -f "$TMP" "$DST"
  echo "==> $(basename "$SRC") -> $(basename "$DST")"
else
  echo "!! expected artifact not found: $SRC" >&2
  exit 1
fi
echo "==> done"
