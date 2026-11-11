# Slime Pet

Slime companion with hops, pickup cuddles and soft positional sounds.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [slime-pet-0.5.0.msmod](content/slime-pet-0.5.0.msmod), then drag and drop the `.msmod` file onto the [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) window. You can also use **Add mods…** (older loader builds: **Install mod package**).

Select the mod and its required frameworks, click **Prepare files**, then **Launch**.

Required mods: [sound-framework](https://github.com/swooping-mods/sound-framework), [unit-runtime](https://github.com/swooping-mods/unit-runtime), [pickup-hands](https://github.com/swooping-mods/pickup-hands), [hints-framework](https://github.com/swooping-mods/hints-framework).

## Controls

- Slime Pet 0.5.0 — Aim at BUY to adopt, or at your nearby slime to pick it up; E again sets it down. Exclusive interactions and native control hints. F6 recalls.
- Start Game > Slot 1 > Invite Only > Play. The starter/North room contains blue and gold terminals.
- Exit normally to restore the private profile. Use --keep-world/--use-world for disposable save/reload checks.

## Compatibility

Version 0.5.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
