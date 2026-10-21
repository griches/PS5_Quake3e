# Licences

The repository as a whole, and any title built from it, is offered under the GNU
General Public License, version 3 ([`LICENSE`](LICENSE)). Its files come from
projects under three licences, each compatible with that; every source file names
its own in its header, and may also be used under that licence alone. This is the
summary.

| Path | Licence | Text |
| --- | --- | --- |
| `ps5/q3/` (the PS5 layer for Quake3e), `ps5/engine/` (changes to Quake3e's engine: split screen), `ps5/ui/` (changes to ioquake3's menu code), `ps5/CMakeLists.txt`, `ps5/tools/build-ui.sh` | GPL-2.0-or-later | [`licenses/GPL-2.0.txt`](licenses/GPL-2.0.txt) |
| `ps5/tools/png-to-title-dds.py` (encoder from PS5 RetroArch, by Mihawk) | GPL-3.0-or-later | [`licenses/GPL-3.0.txt`](licenses/GPL-3.0.txt) |
| `ps5/src/` and the rest of `ps5/tools/` (from the PS5 Vulkan Template, by Mihawk, after Sascha Willems' Vulkan examples) | MIT | [`licenses/MIT.txt`](licenses/MIT.txt) |

## What is built from elsewhere

Nothing below is stored in this repository; the build takes each from its own
project, under that project's licence.

| Part | Project | Licence |
| --- | --- | --- |
| The engine, with the `ps5/engine/` changes | [Quake3e](https://github.com/ec-/Quake3e), from id Software's Quake III Arena source | GPL-2.0-or-later |
| The menu code `ps5/ui/` changes | [ioquake3](https://github.com/ioquake/ioq3), from the same source | GPL-2.0-or-later |
| The Vulkan driver | RADV from [PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa), linked by [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan) | MIT (Mesa); GPL-3.0 (PS5_Vulkan's tools) |
| The platform layer and SDK | [PS5_PayloadSDK](https://github.com/mihawk-99/PS5_PayloadSDK), a fork of ps5-payload-dev/sdk | GPL-3.0 |

## A built title

A built `eboot.bin` links the GPL-3.0 platform layer with GPL-2.0-or-later engine
code, so a title as distributed is under **GPL-3.0**. Anyone who distributes a
build must make the source of every part available at the revisions it was built
from.

## What is not covered

- **Game data.** Quake III Arena's `pak` files are id Software's and are not part
  of this repository or of any licence above. See the README for where to get them.
- **Artwork.** `ps5/sce_sys/icon0.png`, `pic0.dds`, `pic1.dds` and `ps5/art/` show
  the Quake III Arena logo and artwork, which belong to id Software. They are not
  offered under any licence above.
- **Trademarks.** "Quake" and "id Software" are trademarks of id Software LLC, a
  ZeniMax Media company. "PlayStation" and "PS5" are trademarks of Sony Interactive
  Entertainment Inc. This project is not affiliated with or endorsed by either.
