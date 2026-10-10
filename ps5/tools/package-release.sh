#!/usr/bin/env bash
# Quake3e for the PlayStation 5 - pack the built title folder as a release archive.
#
#   ps5/tools/package-release.sh VERSION      (through tools/vm.sh package-release 1.0.0)
#
# Writes dist/Quake3-PS5-vVERSION.zip: the title folder dist/<TITLE_ID>/ as the
# console takes it (eboot.bin, sce_sys/, sce_module/, baseq3/zz-ps5-ui.pk3,
# licenses/), with SOURCE.txt beside eboot.bin naming the revision of every part
# the build linked. A built title is under GPL-3.0: that file is how a release
# says where its source is. No game data goes in.
#
# SPDX-License-Identifier: MIT
set -euo pipefail
version=${1:?usage: package-release.sh VERSION}
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
app="$root/dist/$title_id"
for file in eboot.bin sce_sys/param.json sce_sys/icon0.png sce_module/libc.prx baseq3/zz-ps5-ui.pk3; do
    [[ -f $app/$file ]] || { echo "missing $app/$file: run build.sh and build-ui.sh first" >&2; exit 2; }
done
if compgen -G "$app/baseq3/pak*.pk3" > /dev/null; then
    echo "game data in $app/baseq3: a release never carries it" >&2
    exit 2
fi
[[ -z $(git -C "$root" status --porcelain) ]] || { echo "uncommitted changes: a release is built from a commit" >&2; exit 2; }
provenance="$vulkan/.deps/native/radv-release/PROVENANCE.txt"
mkdir -p "$app/licenses"
cp "$root/licenses/GPL-3.0.txt" "$app/licenses/GPL-3.0.txt"
cp "$root/licenses/MIT.txt" "$app/licenses/MIT.txt"
cp "$root/LICENSE.md" "$app/licenses/LICENSE.md"
cat > "$app/SOURCE.txt" <<SOURCE
Quake III Arena for PlayStation 5 (Quake3e), version $version
A homebrew port. Not affiliated with id Software or Sony Interactive Entertainment.

This title is distributed under the GNU General Public License, version 3
(licenses/GPL-3.0.txt). Its source is these projects at these revisions:

The port (PS5 layer, menu changes, build)
  $(git -C "$root" remote get-url origin 2>/dev/null || echo "this repository")
  $(git -C "$root" rev-parse HEAD)
The engine: Quake3e (GPL-2.0-or-later)
  https://github.com/ec-/Quake3e
  $(git -C "$root/vendor/Quake3e" rev-parse HEAD)
The menu code: ioquake3 (GPL-2.0-or-later), with ps5/ui/ of the port over it
  https://github.com/ioquake/ioq3
  $(git -C "$root/vendor/ioq3" rev-parse HEAD)
The Vulkan driver: RADV, from PS5_Mesa (MIT)
  https://github.com/mihawk-99/PS5_Mesa
  $(sed -n 's/^revision: //p' "$provenance")
The link recipe, CRT, libc.prx and packaging: PS5_Vulkan (GPL-3.0)
  https://github.com/mihawk-99/PS5_Vulkan
  $(git -C "$vulkan" rev-parse HEAD)
The platform layer and SDK: PS5_PayloadSDK (GPL-3.0), over ps5-payload-dev/sdk v0.42
  https://github.com/mihawk-99/PS5_PayloadSDK
  title: $(sed -n 's/^sdk_revision=//p' "$ps5/tools/setup-sdk.sh")
  RADV archive: $(sed -n 's/^sdk: //p' "$provenance")
Compiler: $(${PS5_CLANG:-clang} --version | head -1)

The Quake III Arena logo and artwork in sce_sys/ belong to id Software and are
not covered by the licences above. No game data is included: see the port's
README for where pak0.pk3 to pak8.pk3 come from.

eboot.bin sha256: $(sha256sum "$app/eboot.bin" | cut -d' ' -f1)
SOURCE
out="$root/dist/Quake3-PS5-v$version.zip"
rm -f "$out"
(cd "$root/dist" && zip -q -r -X "$out" "$title_id" -x '*/.DS_Store')
printf '==> %s (%s bytes)\n' "$out" "$(stat -c %s "$out")"
unzip -l "$out" | tail -n +4 | awk '{print "    " $1, $4}'
