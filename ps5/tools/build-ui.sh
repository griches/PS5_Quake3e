#!/usr/bin/env bash
# Quake3e for the PlayStation 5 - the title's menus: ioquake3's q3_ui with this
# port's graphics menu, built as a QVM and packed as baseq3/zz-ps5-ui.pk3.
#
#   ps5/tools/build-ui.sh        (run in the Linux VM, or through tools/vm.sh build-ui)
#
# The stock ui.qvm (pak8.pk3) lists 4:3 modes from 1999 and has no FPS switch.
# ioquake3's lists what the engine's r_availableModes names (ps5/q3/ps5_glimp.c:
# the console's three display modes); ps5/ui/ui_video.c adds "Show FPS" to it,
# ps5/ui/ui_preferences.c adds "Auto Switch Weapons" to Game Options, and
# ps5/ui/ui_mfield.c opens the console's on-screen keyboard from a text field
# (ps5/ui/q3_ui.patch is the change against the pinned ioquake3). A pure server's
# own ui.qvm takes over while connected to it.
#
# ioquake3 is the vendor/ioq3 submodule, at the pinned revision. It is never
# changed: its code/ and cmake/ are copied to build/ui-src/ and this port's files
# go over the copy.
#
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
[[ -f $root/vendor/ioq3/CMakeLists.txt || -n ${IOQ3_DIR:-} ]] || git -C "$root" submodule update --init vendor/ioq3
ioq3=$(cd -- "${IOQ3_DIR:-$root/vendor/ioq3}" && pwd)
src="$root/build/ui-src"
build="$root/build/ui"
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
mkdir -p "$src"
rsync -a --delete --exclude=/.git --exclude=/build "$ioq3/" "$src/"
cp "$ps5"/ui/*.c "$src/code/q3_ui/"
# the host's own compiler builds the QVM tools (q3lcc, q3asm), not the console's
if [[ ! -f $build/build.ninja ]]; then
    env -u CC -u CXX cmake -S "$src" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SERVER=OFF -DBUILD_CLIENT=OFF \
        -DBUILD_RENDERER_GL1=OFF -DBUILD_RENDERER_GL2=OFF -DBUILD_GAME_LIBRARIES=OFF -DBUILD_GAME_QVMS=ON \
        -DCMAKE_C_COMPILER=gcc > "$build.configure.log" 2>&1 || { cat "$build.configure.log" >&2; exit 1; }
fi
ninja -C "$build" uiqvm_baseq3 > "$build.log" 2>&1 || { grep -iE "error" "$build.log" >&2; exit 1; }
out="$root/dist/$title_id/baseq3"
mkdir -p "$out"
rm -f "$out/zz-ps5-ui.pk3"
(cd "$build/Release/baseq3" && python3 -c 'import sys,zipfile; z=zipfile.ZipFile(sys.argv[1],"w",zipfile.ZIP_DEFLATED); z.write("vm/ui.qvm"); z.close()' "$out/zz-ps5-ui.pk3")
printf '==> %s (%s bytes)\n' "$out/zz-ps5-ui.pk3" "$(stat -c %s "$out/zz-ps5-ui.pk3")"
