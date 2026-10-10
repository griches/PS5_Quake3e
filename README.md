# Quake III Arena for PlayStation 5 (Quake3e)

[![stars](https://img.shields.io/github/stars/griches/PS5_Quake3e?style=flat)](https://github.com/griches/PS5_Quake3e/stargazers)
[![release](https://img.shields.io/github/v/release/griches/PS5_Quake3e)](https://github.com/griches/PS5_Quake3e/releases/latest)
[![licence](https://img.shields.io/badge/licence-GPL--3.0%20as%20built-blue)](LICENSE.md)

A PS5 homebrew port of [Quake3e](https://github.com/ec-/Quake3e), an improved
Quake III Arena engine, drawing with its Vulkan renderer on RADV through the
[PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate) stack.

It needs a PS5 that already runs homebrew, and your own copy of the game's data.
No game data, Sony software or keys are included.

## Status

Tested on one console, on 2026-10-10. "Seen" means it ran there and I watched it
or read it in the console's log; nothing else is claimed.

| Area | Seen on the console | Not yet verified |
| --- | --- | --- |
| Start-up and exit | Boots to the menu, loads all nine pak files, quits cleanly to the home screen | |
| Rendering | Single player plays at 3840x2160 on a 59.94 Hz display | Whether it holds 60 fps everywhere; 120 Hz (the test display stayed at 60) |
| Video modes | The menu lists 3840x2160, 2560x1440 and 1920x1080; a live switch to 1920x1080 went through | 2560x1440 |
| Input | DualSense: sticks, buttons, triggers; a button rebound in the controls menu, saved across launches | The two sensitivity sliders and invert look |
| Sound | 48 kHz stereo effects | Music (no Ogg Vorbis in this build) |
| Network | Name lookup works; the Internet server list fills (over 1,000 servers); a server's own pak files downloaded | A full online or LAN match, start to finish |
| On-screen keyboard | The console's keyboard opens from a menu | The field being cleared before the typed text goes in |
| Home screen | The Quake III icon, once the title was removed from the home screen and added again (the console had kept its first cached copy; the game's folder and data survived the removal) | The background art and the title's name showing |

Known limits:

- The game modules run in the bytecode interpreter; there is no JIT yet.
- No fast (HTTP) downloads, so servers that need extra pak files will crawl or stall.
- No USB keyboard or mouse: the DualSense is the only input.
- Stepping sideways on the left stick is on or off, not analog.
- The menus are ioquake3's; a pure server's own menus take over while connected.

## Install

You need a PS5 that already runs homebrew: an enabler such as etaHEN,
ShadowMountPlus (it shows titles in `/data/homebrew` on the home screen) and an FTP
server (ftpsrv, port 2121). Setting that up is not covered here.

1. **Get the title.** Download `Quake3-PS5-v1.0.0.zip` from the
   [latest release](https://github.com/griches/PS5_Quake3e/releases/latest) and
   unzip it. It holds one folder, `PPSA99303`.
2. **Get the game data** (not included, see the table below): `pak0.pk3` from your
   own copy of Quake III Arena, and `pak1.pk3` to `pak8.pk3` from the 1.32 point
   release.
3. **Put the nine pak files** in `PPSA99303/baseq3/`, beside `zz-ps5-ui.pk3`.
4. **Copy the folder to the console** over FTP, so that it becomes
   `/data/homebrew/PPSA99303/`:

   ```text
   /data/homebrew/PPSA99303/eboot.bin
   /data/homebrew/PPSA99303/sce_sys/...
   /data/homebrew/PPSA99303/sce_module/libc.prx
   /data/homebrew/PPSA99303/baseq3/pak0.pk3 ... pak8.pk3
   /data/homebrew/PPSA99303/baseq3/zz-ps5-ui.pk3
   ```
5. **Launch "Quake III Arena"** from the home screen. If the tile does not appear,
   let ShadowMountPlus rescan.

To update, close the game and replace `eboot.bin`, `sce_sys/` and
`baseq3/zz-ps5-ui.pk3`; your pak files and `baseq3/q3config.cfg` (your settings)
stay. If the home screen keeps an old icon after an update, remove the title from
the home screen and let it be added again: the folder and its data are kept.

## Game data

Not included, and never to be committed here.

| File | From |
| --- | --- |
| `pak0.pk3` | A copy of Quake III Arena you own (the CD, [Steam](https://store.steampowered.com/app/2200/Quake_III_Arena/) or [GOG](https://www.gog.com/en/game/quake_iii_arena)), in its `baseq3` folder |
| `pak1.pk3` to `pak8.pk3` | id Software's free 1.32 point release. ioquake3 hosts the files, behind id's licence agreement, at <https://ioquake3.org/extras/patch-data/> (the download's `baseq3` folder) |

All nine are needed. A `pak1.pk3` to `pak8.pk3` from any version other than 1.32
is refused at start-up, and so is a copy with only `pak0.pk3`.

Extra start-up commands can go in `/data/homebrew/PPSA99303/cmdline.txt` (one
line, for example `+connect 192.168.0.10`).

## Controls

| DualSense | In play | In menus |
| --- | --- | --- |
| Left stick | Move | Pointer |
| Right stick | Look | Pointer |
| D-pad | Arrow keys | Move between items |
| CROSS, L3 | Jump | CROSS: select; on a text field, the on-screen keyboard |
| CIRCLE | Crouch | Back; also backs out of connecting or loading |
| SQUARE, R2 | Fire | SQUARE: clear a binding; R2: click |
| TRIANGLE | Use item | The on-screen keyboard |
| L1 / R1 | Previous / next weapon | |
| L2 | Zoom | |
| OPTIONS | Menu | Back |
| Touch pad | Scores | |

These are the defaults. Every button, trigger and D-pad direction can be given
another action in Setup, Controls (CROSS on an action, then the button; SQUARE
clears it). The sticks and OPTIONS are fixed. Look sensitivity is two sliders,
horizontal and vertical, with invert look beside them.

The port's own cvars: `in_ps5LookSpeed`, `in_ps5MenuSpeed`,
`r_ps5Height` (2160, 1440 or 1080 when the menu has not chosen) and `r_ps5Refresh`.

## How it is put together

Quake3e's sources are compiled unchanged from the `vendor/Quake3e` submodule
(pinned at `2b375bd`). The engine's SDL and unix files are left out and
`ps5/q3/` stands in for them:

| File | Replaces | Does |
| --- | --- | --- |
| `ps5_sys.c` | `unix/unix_main.c` | launch, the main loop, the log, the exit through the shell, name lookup, the libc calls the console lacks |
| `ps5_shared.c` | `unix/unix_shared.c` | time, file listing, files and folders FTP can reach |
| `ps5_glimp.c` | `sdl/sdl_glimp.c` | RADV's entry point, the `VK_KHR_display` surface, the display modes |
| `ps5_input.c` | `sdl/sdl_input.c` | the DualSense as Quake's keys, mouse and joystick; the on-screen keyboard |
| `ps5_snd.c` | `sdl/sdl_snd.c` | the mixer's buffer through the console's 48 kHz output |

`ps5/src/platform.c` is the template's PS5 layer (log, pad, sound, exit).

The menus are a second build: ioquake3's `q3_ui` from the `vendor/ioq3` submodule
(pinned at `83a7762`), copied to `build/ui-src/` with the files in `ps5/ui/` over the
copy (`ps5/ui/q3_ui.patch` is the difference), compiled to a QVM and packed as `baseq3/zz-ps5-ui.pk3`. The changes: the graphics menu lists the
console's display modes and has a Show FPS switch, the controls menu shows and binds
the DualSense's buttons and has the two look sensitivities, text fields open the
on-screen keyboard, there is no CD key prompt at start-up, and the credits describe
this build.

## Building

The stack builds on Linux. On macOS this repository drives a Lima VM:

```bash
git submodule update --init   # Quake3e and ioquake3, at their pinned revisions
ps5/tools/vm.sh build      # the title: dist/PPSA99303/
ps5/tools/vm.sh build-ui   # the menus: dist/PPSA99303/baseq3/zz-ps5-ui.pk3
ps5/tools/vm.sh package-release 1.0.0   # dist/Quake3-PS5-v1.0.0.zip, from a clean commit
ps5/tools/vm.sh deploy     # FTP to the console (needs the stack's control payload)
ps5/tools/vm.sh run        # launch and follow the console's log
```

Beside this folder it expects the stack: `PS5_Vulkan` with its RADV archive built,
and `PS5_PayloadSDK` (PS5_VulkanTemplate's `ps5/tools/bootstrap.sh` sets both up).

Build the stack with **LLVM 22**: clang 18 drops the hidden visibility of Mesa's
weak entry points on the PS5 target, and the title then fails to package.

The console needs its homebrew environment running: an enabler such as etaHEN,
ShadowMountPlus (titles in `/data/homebrew`), and an FTP server on port 2121.

Home-screen art is in `ps5/sce_sys/`: `icon0.png` (512x512), `pic0.dds` (behind the
title on the home screen) and `pic1.dds` (the splash while it starts), both
3840x2160. `ps5/tools/png-to-title-dds.py` converts a picture to that format.

## Credits

- **id Software** made Quake III Arena and released its source under the GPL.
- **[Quake3e](https://github.com/ec-/Quake3e)** and its contributors: the engine.
- **[ioquake3](https://ioquake3.org/)** and its contributors: the menu code.
- **Mihawk** ([mihawk-99](https://github.com/mihawk-99)): PS5_VulkanTemplate,
  PS5_Vulkan, PS5_Mesa (RADV on the console) and the PS5_PayloadSDK fork, which
  this port is built on, and PS5_vkQuake, which showed the way.
- **Mesa** and its RADV developers; **ps5-payload-dev** for the payload SDK;
  **Sascha Willems** for the Vulkan examples the template grew from.

## Licences

See [`LICENSE.md`](LICENSE.md). In short: the port's code is GPL-2.0-or-later, the
template's files are MIT, and a built title is GPL-3.0. The Quake III Arena logo
and artwork in `ps5/sce_sys/` and `ps5/art/` belong to id Software and are not
covered by those licences.

This is an independent homebrew project, not affiliated with or endorsed by id
Software, ZeniMax Media or Sony Interactive Entertainment. Running homebrew needs
a modified console, which may void its warranty or breach the platform's terms of
service.
