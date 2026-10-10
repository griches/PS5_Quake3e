# Publication checklist (done)

- [x] **Artwork.** Decided by Gary, 2026-10-10: the Quake III Arena artwork stays in the
      build. `pic0.dds` and `pic1.dds` are his supplied `quake_iii_arena_4k.png` (id's
      official art); `icon0.png` is the Quake III logo rendered from Quake3e's
      `code/unix/quake3.svg`. Sources are in `ps5/art/`.
- [x] **Licence files.** `LICENSE.md` is still only the template's MIT text. Add the GPL-2.0
      and GPL-3.0 texts and say which folders are under which:
      `ps5/q3/` and `ps5/ui/` GPL-2.0-or-later; `ps5/src/` and most of `ps5/tools/` MIT;
      `ps5/tools/png-to-title-dds.py` GPL-3.0-or-later; a built title GPL-3.0.
- [x] **Reproducible sources.** Quake3e (`2b375bd`) and ioquake3 (`83a7762`) are pinned
      submodules in `vendor/`; a clean build from them gave the same menu module.
- [x] **Credits and status in the README.** Credit id Software, Quake3e, ioquake3 (the menu
      code) and the mihawk-99 stack (PS5_VulkanTemplate, PS5_Vulkan, PS5_Mesa,
      PS5_PayloadSDK). State what has run on a console and what has not.
- [x] **Game data instructions.** Say where the data comes from, with links and no files:
      `pak0.pk3` from a bought copy of Quake III Arena; `pak1.pk3` to `pak8.pk3` from the
      1.32 point release, which ioquake3 hosts at https://ioquake3.org/extras/patch-data/
      (under id's EULA).
- [x] **Quit screen.** The menus' credits screen links to ioquake3.org; reword it to credit
      id Software, Quake3e and ioquake3's menu code.
- [x] **Commit.** One commit on `main` holds the port, on top of the template's generated first commit (squash the two before publishing if a single root commit is wanted).
- [x] **Badges.** Stars, release and licence badges are under the README title (there is
      no CI workflow, so no CI badge). They render once the repository is public.
- [x] **Publish.** `griches/PS5_Quake3e` is public; `v1.0.0` is tagged and released with
      `Quake3-PS5-v1.0.0.zip` (2026-10-10).

Never commit: pak files, `klog/` (console address, user id, system log), `build/`, `dist/`.
