#!/usr/bin/env bash
# PS5 Vulkan Template - build the title into dist/<TITLE_ID>/.
#
#   ps5/tools/build.sh
#
# Configures ps5/ with the payload SDK's CMake toolchain (once), compiles what
# changed, links with RADV and packages the title folder (tools/link-title.sh),
# then lays the assets in (tools/build-assets.py).
#
# Quake3e is the vendor/Quake3e submodule, at the pinned revision, and is never
# changed: its tree is copied to build/engine-src/ and this port's engine files
# (ps5/engine/, at the same paths under code/: split screen) go over the copy.
# ps5/engine/quake3e.patch is that change against the pinned Quake3e.
#
#   PS5_VULKAN_DIR   the PS5_Vulkan checkout (default ../PS5_Vulkan): the RADV
#                    release archive, the link recipe, the native tool and libc.prx
#
# The payload SDK is this title's own pin of my fork (tools/setup-sdk.sh).
#   PS5_CLANG        the host clang the compiler wrappers drive (default: clang)
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT

set -euo pipefail

ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
[[ -f $root/vendor/Quake3e/code/qcommon/common.c || -n ${QUAKE3E_DIR:-} ]] || git -C "$root" submodule update --init vendor/Quake3e
upstream=$(cd -- "${QUAKE3E_DIR:-$root/vendor/Quake3e}" && pwd)
sdk=$(bash "$ps5/tools/setup-sdk.sh")
build="$root/build/ps5"
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
export PS5_CLANG=${PS5_CLANG:-$(command -v clang || true)}

# The engine's sources: the pinned Quake3e with ps5/engine/ over it. Copies keep their
# times, so only what changed is compiled again; the port's files are left out of the
# first copy, so it never undoes them, and one taken out of ps5/engine/ comes back.
quake3e="$root/build/engine-src"
mkdir -p "$quake3e"
rsync -a --delete --exclude=/.git $(cd "$ps5/engine" && find . -type f ! -name '*.patch' | sed 's|^\./|--exclude=/code/|') \
    "$upstream/" "$quake3e/"
rsync -a --exclude='*.patch' "$ps5/engine/" "$quake3e/code/"

if [[ -f $build/CMakeCache.txt ]] && ! grep -qx "QUAKE3E_DIR:PATH=$quake3e" "$build/CMakeCache.txt"; then
    rm -rf "$build" # configured against the bare submodule
fi
if [[ ! -f $build/build.ninja ]]; then
    mkdir -p "$build"
    cmake -S "$ps5" -B "$build" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$sdk/toolchain/prospero.cmake" \
        -DPS5_VULKAN_DIR="$vulkan" -DQUAKE3E_DIR="$quake3e" -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=OFF > "$build.configure.log" 2>&1 \
        || { cat "$build.configure.log" >&2; exit 1; }
fi
"$ninja" -C "$build" title
