# Quake III Arena for PlayStation 5 (Quake3e)

[![stars](https://img.shields.io/github/stars/griches/PS5_Quake3e?style=flat)](https://github.com/griches/PS5_Quake3e/stargazers)
[![release](https://img.shields.io/github/v/release/griches/PS5_Quake3e)](https://github.com/griches/PS5_Quake3e/releases/latest)
[![licence](https://img.shields.io/badge/licence-GPL--3.0%20as%20built-blue)](LICENSING.md)

A PS5 homebrew port of [Quake3e](https://github.com/ec-/Quake3e), an improved
Quake III Arena engine, drawing with its Vulkan renderer on RADV through the
[PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate) stack.
It plays with the DualSense, with gyro aim if you want it, or with a USB mouse
and keyboard, and two to four players can share the screen in split screen.

It needs a PS5 that already runs homebrew, and your own copy of the game's data.
No game data, Sony software or keys are included.

## Status

Tested on one console, on 2026-10-10 (the mouse and keyboard on 2026-10-11). "Seen" means it ran there and I watched it
or read it in the console's log; nothing else is claimed.

| Area | Seen on the console | Not yet verified |
| --- | --- | --- |
| Start-up and exit | Boots to the menu, loads all nine pak files, quits cleanly to the home screen | |
| Rendering | Single player plays at 3840x2160 on a 59.94 Hz display | Whether it holds 60 fps everywhere; 120 Hz (the test display stayed at 60) |
| Video modes | The menu lists 3840x2160, 2560x1440 and 1920x1080; a live switch to 1920x1080 went through | 2560x1440 |
| Split screen | Two, three and four players, 2026-10-21: each joins and plays; four players and three bots started from the menu; three players' scoreboard quarter, free for all and team deathmatch (players 2 and 3 on the teams chosen for them); the camera with two players, cutting between players and bots; the halves' display in its own shape; a locked 120 fps at 3840x2160 on a 120 Hz display with one to four players and three bots (q3dm4, q3dm17), never a missed refresh | Players 3 and 4 on their own controllers; team games; how far past 120 it could go |
| Input | DualSense: sticks, buttons, triggers; a button rebound in the controls menu, saved across launches. A USB mouse (movement, three buttons, the wheel) and a USB keyboard, 2026-10-11. The gyro, aiming in play and moving the menu pointer, and the touch pad's tap and double tap, 2026-10-21 | The two sensitivity sliders and invert look; the mouse speed slider; a mouse's side buttons; a Bluetooth mouse or keyboard |
| Sound | 48 kHz stereo effects | Music (no Ogg Vorbis in this build) |
| Network | Name lookup works; the Internet server list fills (over 1,000 servers); a server's own pak files downloaded | A full online or LAN match, start to finish |
| On-screen keyboard | The console's keyboard opens from a menu | The field being cleared before the typed text goes in |
| Home screen | The Quake III icon, once the title was removed from the home screen and added again (the console had kept its first cached copy; the game's folder and data survived the removal) | The background art and the title's name showing |

Known limits:

- The game modules run in the bytecode interpreter; there is no JIT yet.
- No fast (HTTP) downloads, so servers that need extra pak files will crawl or stall.
- A keyboard types the US layout's characters, whatever the keyboard's own.
- Stepping sideways on the left stick is on or off, not analog.
- The menus are ioquake3's; a pure server's own menus take over while connected.
- Split screen is on the console's own server only (with or without bots), not
  online. Each part of the screen shows the game's own heads-up display, drawn for
  a 4:3 screen: in the two players' halves each picture keeps its shape (the top
  and bottom of the display at the top and bottom of the half); in quarters it is
  stretched as on a widescreen in single player, with a round crosshair.

## Install

> **Read this first: a title ID shared with two PS3 emulators.** PS5SX3 and
> KB-RPCS3, both ports of the RPCS3 PlayStation 3 emulator, use the same title ID
> as this port, `PPSA99303`, and so the same folder, `/data/homebrew/PPSA99303`.
> A console holds one app per title ID: installing Quake replaces the emulator,
> and removing Quake removes the folder with the emulator's games and saves in it.
> This has happened to players through a homebrew store. If you have either
> emulator installed, don't install this port. Install it by hand, as below, so
> you can see what is in `/data/homebrew/PPSA99303` before you copy anything;
> with a store, read its warning before installing or removing. Telling apps that
> share a title ID apart, and never overwriting or deleting another app, is for
> each store's developers to handle.

You need a PS5 that already runs homebrew: an enabler such as etaHEN,
ShadowMountPlus (it shows titles in `/data/homebrew` on the home screen) and an FTP
server (ftpsrv, port 2121). Setting that up is not covered here.

1. **Get the title.** Download `Quake3-PS5-v1.3.0.zip` from the
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
| Turning the pad (the gyro) | Look, when gyro aim is on | Pointer, always |
| D-pad | Arrow keys | Move between items |
| CROSS, L3 | Jump | CROSS: select; on a text field, the on-screen keyboard |
| CIRCLE | Crouch | Back; also backs out of connecting or loading |
| SQUARE, R2 | Fire | SQUARE: clear a binding; R2: click |
| TRIANGLE | Use item | The on-screen keyboard |
| L1 / R1 | Previous / next weapon | |
| L2 | Zoom | |
| OPTIONS | Menu | Back |
| Touch pad: click | Scores | |
| Touch pad: tap | Level the view (the gyro's reset) | Pointer to the middle (the gyro's reset) |
| Touch pad: double tap | Gyro aim on / off | Gyro aim on / off |

These are the defaults. Every button, trigger and D-pad direction can be given
another action in Setup, Controls (CROSS on an action, then the button; SQUARE
clears it). The sticks and OPTIONS are fixed. Look sensitivity is two sliders,
horizontal and vertical, with invert look beside them.

**Gyro aim.** Turn the DualSense to aim, as with a mouse; the right stick still
looks too. It is off at first. Switch it on in Setup, Controls, Look (**gyro
aim**), or double tap the touch pad (two quick taps, not clicks) at any time; a
double tap again switches it off. **Gyro speed** on the same page sets how far
the view turns for each turn of the pad. In the menus the pad moves the pointer
whether gyro aim is on or not.

To reset the gyro, hold the pad where it is comfortable and tap the touch pad
once: in play the view levels, and in a menu the pointer goes to the middle of
the screen. The gyro carries on from where the pad is. Turning left or right has
no fixed middle in play, so a tap levels the view but keeps the way you face.

Setup, Controls shows these as **TAP** (center view) and **DOUBLE TAP** (gyro aim
on / off). A button can be bound to either beside the touch pad.

A mouse and a keyboard plugged into the console work beside the pad, bound as
the game binds them by default (W, A, S and D, fire on the left button, the
weapons on the wheel and the number keys). The keyboard types into the console
(the key left of 1), a chat line and the menus' text fields. Setup, Controls
shows a pad button and a mouse or keyboard key side by side for an action, and
a new one takes the place of its own kind. The mouse has its own speed slider
there (`in_ps5MouseSpeed`); the sensitivities, invert look and smooth look are
the stick's and the mouse's alike. `in_ps5Mouse 0` and
`in_ps5Keyboard 0` turn them off.

The port's own cvars: `in_ps5LookSpeed`, `in_ps5MenuSpeed`, `in_ps5Mouse`, `in_ps5MouseSpeed`, `in_ps5Keyboard`,
`in_ps5Gyro` (gyro aim, `0` or `1`), `in_ps5GyroSpeed` (the view turns this many times as far as the pad, 0.1 to 5, 2 at first),
`r_ps5Height` (2160, 1440 or 1080 when the menu has not chosen) and `r_ps5Refresh`.

## Split screen

Two to four players play on one console: two have half the screen each, side by
side; three or four have a quarter each, and with three the last quarter is the
scoreboard, kept up to date for everyone (the players on the console marked P1,
P2 and P3 in their controllers' colours). With two or three players a camera can
take a part of the screen too: it follows each player in turn from behind, ten
seconds each (bots too), with their name under it and no status bar.

1. Turn on another controller for each player and press its PS button. The
   console asks who is using it: pick a user or a guest. The players after the
   first are those users, in the order they signed in, and take their names.
2. In the main menu choose **SPLIT SCREEN**, then a game type and a map, as for a
   skirmish.
3. On the next page choose **Players** (2, 3 or 4, at the top of the right-hand
   column): the first slots on the left are **Player 1** and the others on the
   console. The other slots are as in a skirmish: Bot, Open, or "----" for none,
   with the bots' skill at the top. In a team game, choose each player's team
   there (they start on the same team). **Camera** adds the camera (not with four
   players: two and the camera have quarters, with the scoreboard in the last).
4. **Fight**.

Both controllers play with player 1's controls (Setup, Controls), and each
player's gyro aim is their own: double tap the touch pad to switch it. OPTIONS on
either controller opens the menu, which pauses the game, and either controller
can work it. The mouse, the keyboard and the on-screen keyboard are player 1's.

The other players join as the map loads and leave when you leave the game; a
player without a controller stands where they start. Each player
hears the game from where they stand: a sound is heard from the nearer player.
From the console, `player 2 <command>` runs a command as player 2 (for example
`player 2 team red`).

The split screen cvars: `cl_splitPlayers` (the players on the next local map,
set by the menu), `cl_splitName2` (player 2's name, from their user),
`cl_splitModel2` (player 2's model; empty picks one player 1 doesn't wear),
`cl_splitTeam2` (player 2's team in a team game, set by the menu) and
`in_ps5Gyro2` (player 2's gyro aim). Players 3 and 4 have their own (`cl_splitName3`
and so on). `cl_splitCamera` adds the camera (set by the menu); its view is
`cl_cameraRange` (100) behind the player, `cl_cameraAngle` around them,
`cl_cameraThirdPerson` and `cl_cameraDraw2D` (0: no status bar).

## How it is put together

Quake3e's sources come from the `vendor/Quake3e` submodule (pinned at `2b375bd`),
copied to `build/engine-src/` with the files in `ps5/engine/` over the copy
(`ps5/engine/quake3e.patch` is the difference: split screen). The engine's SDL and
unix files are left out and `ps5/q3/` stands in for them:

| File | Replaces | Does |
| --- | --- | --- |
| `ps5_sys.c` | `unix/unix_main.c` | launch, the main loop, the log, the exit through the shell, name lookup, the libc calls the console lacks |
| `ps5_shared.c` | `unix/unix_shared.c` | time, file listing, files and folders FTP can reach |
| `ps5_glimp.c` | `sdl/sdl_glimp.c` | RADV's entry point, the `VK_KHR_display` surface, the display modes |
| `ps5_input.c` | `sdl/sdl_input.c` | the DualSense as Quake's keys, mouse and joystick, its gyro and touch pad; a USB mouse and keyboard; the on-screen keyboard |
| `ps5_snd.c` | `sdl/sdl_snd.c` | the mixer's buffer through the console's 48 kHz output |

Split screen in the engine (`ps5/engine/`): each local player has a client of
their own (the client state, the connection, the cgame VM, the buttons), and the
code works on the current player's, so the stock `cgame.qvm` runs once for each
player, told that their half is the whole screen. The players after the first
connect to the local server on loopback channels of their own and load the map
after player 1, using the world player 1's cgame loaded. Input events carry the
player they are from; another player's bound commands run as theirs (the
`player` command). The mixer hears each sound from the nearest player and drops
the copies the other cgames start. The local players count as one for pausing.
`MAX_LOCAL_PLAYERS` (4) sizes all of it, and the screen's layout has quarters for
three or four.

`ps5/src/platform.c` is the template's PS5 layer (log, pad, mouse, keyboard, sound, exit).
The SDK has no stub for the console's mouse library, so `ps5/stubs/` names its
calls for the link.

The menus are a second build: ioquake3's `q3_ui` from the `vendor/ioq3` submodule
(pinned at `83a7762`), copied to `build/ui-src/` with the files in `ps5/ui/` over the
copy (`ps5/ui/q3_ui.patch` is the difference), compiled to a QVM and packed as `baseq3/zz-ps5-ui.pk3`. The changes: the graphics menu lists the
console's display modes and has a Show FPS switch, Game Options has an Auto Switch Weapons
switch (`cg_autoswitch`, on by default), the controls menu shows and binds
the DualSense's buttons beside a mouse's and a keyboard's and has the two look sensitivities and the gyro's switch and speed, text fields open the
on-screen keyboard, there is no CD key prompt at start-up, and the credits describe
this build, and **SPLIT SCREEN** in the main menu starts a skirmish with the first
slots for the players on the console.

## Building

The stack builds on Linux. On macOS this repository drives a Lima VM:

```bash
git submodule update --init   # Quake3e and ioquake3, at their pinned revisions
ps5/tools/vm.sh build      # the title: dist/PPSA99303/
ps5/tools/vm.sh build-ui   # the menus: dist/PPSA99303/baseq3/zz-ps5-ui.pk3
ps5/tools/vm.sh package-release 1.3.0   # dist/Quake3-PS5-v1.3.0.zip, from a clean commit
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

See [`LICENSING.md`](LICENSING.md). In short: the port's code is GPL-2.0-or-later, the
template's files are MIT, and a built title is GPL-3.0. The Quake III Arena logo
and artwork in `ps5/sce_sys/` and `ps5/art/` belong to id Software and are not
covered by those licences.

This is an independent homebrew project, not affiliated with or endorsed by id
Software, ZeniMax Media or Sony Interactive Entertainment. Running homebrew needs
a modified console, which may void its warranty or breach the platform's terms of
service.
