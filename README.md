<p align="center">
  <img src="https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/335300/library_hero.jpg" alt="DARK SOULS II: Scholar of the First Sin" width="100%">
</p>

<p align="center">
  <img src="https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/335300/logo.png" alt="DARK SOULS II: Scholar of the First Sin" width="520">
</p>

<h1 align="center">DARK SOULS II: Scholar of the First Sin — Nintendo Switch</h1>

<p align="center">
  <img src="https://img.shields.io/badge/static%20recompilation-PPC→ARM64-1F1F1F?style=for-the-badge" alt="Static recompilation">
  <img src="https://img.shields.io/badge/Horizon-libnx-E60012?style=for-the-badge&logo=nintendoswitch&logoColor=white" alt="Horizon libnx">
  <img src="https://img.shields.io/badge/Vulkan-Mesa%20NVK-AC162C?style=for-the-badge&logo=vulkan" alt="Vulkan NVK">
  <img src="https://img.shields.io/badge/renderer-native-5C2D91?style=for-the-badge" alt="Native renderer">
</p>

An experimental port of the **Xbox 360** release (Title ID `465307E4`) using static PowerPC-to-C++ recompilation with ReXGlue and a custom Vulkan renderer on Mesa NVK. This is not a PC-version port or an Xbox 360 emulator.

> [!IMPORTANT]
> **0.56 (v56) is the first public test build, not a finished or 30 FPS release.** v56 reached the menu and an in-game frame in PC tests; it has **not yet been tested on a Switch**. In the last console-tested build (v55), the game ran at approximately 20 FPS in the tested areas and dropped to 8 FPS during loading. See [known issues](docs/known-issues.md) before installing.

## Download and install

Download `ds2.nro` and `ds2.toml` from the [0.56 release](https://github.com/XDASCATX/DS2_switch/releases/tag/v0.56). Optionally download `ds2-0.56-forwarder.nsp` to start from the HOME Menu **without opening Sphaira**. The NSP is only a shortcut: it does not contain the NRO, the game, or console keys.

You must supply your **own Xbox 360 copy** of *Scholar of the First Sin* with `Default.xex` (the base executable for Title ID `465307E4`), game data and the **Compatibility Pack 4** DLC package. No game files or DLC are included. Copy them to the SD card as follows:

```text
sdmc:/switch/ds2/
  ds2.nro
  ds2.toml
  game_root/Default.xex
  game_root/GameData1.bdt, GameData1.bhd, GameData2.bdt, GameData2.bhd, ...
  dlc/<your Compatibility Pack 4 STFS file>    # needed on first launch
  user/                                         # created by the port; saves and installed DLC
  logs/                                         # created by the port
```

Keep the game's original directory structure inside `game_root/` (`font/`, `movie/`, `shader/`, `sound/`, etc.). The first launch may take several minutes while the DLC is installed. Start `ds2.nro` with full application memory: Homebrew Menu via title takeover, or install the optional NSP forwarder. **Do not launch in applet/Album mode.** The forwarder is fixed to `sdmc:/switch/ds2/ds2.nro`; moving the NRO breaks it. Its Switch Title ID is `0100D52000560000` (not the Xbox 360 Title ID). Installing an unsigned forwarder requires a compatible homebrew setup and may carry a console-ban risk; the NRO does not require the forwarder.

> [!WARNING]
> The supplied `ds2.toml` is **identical to the v56 test configuration**: it requests **CPU 1963 MHz / GPU 1267 MHz** through a supported clock sysmodule and leaves the memory clock unchanged (`0`). These are high, non-stock clocks; use only on hardware prepared for them. On a crash, the clock override may remain until it is cleared by the console's clock controls or a restart. The configuration also enables detailed logging and profiling; logs are written to `sdmc:/switch/ds2/logs/` and can grow over time.

## What to test

Please report FPS and image issues in Majula (sea and town views), combat, fog gates and area transitions. If possible, include `logs/ds2_*.log`, `logs/rex/rex_perfil.log` and `logs/rex/rex_stderr.log` with the version number. Press **L3+R3 for about one second** to capture a problematic frame under `native_dump/` (this briefly pauses the game). Do not share saves, game files, your `prod.keys` or private identifiers in public reports.

Known issues from v55 include lighting around the bonfire near the merchant, glowing loot visible through walls, and loading stutters. The console performance and any regressions of **v56/0.56 remain unverified**. Actual 30+ FPS and a lower-clock profile are future goals, not release claims.

## How it works

`Default.xex` is translated to approximately 84,000 C++ functions by ReXGlue codegen. ReXGlue's runtime supplies Xbox 360 kernel services, guest memory, audio, input and file access on the Switch. The native renderer reads the game's PM4 command stream, translates Xenos shaders to SPIR-V at runtime and renders with Vulkan/NVK. Xenos/EDRAM emulation remains an optional fallback. The port was built on the [NFSMW-NX](https://github.com/StevensND/nfsmw-nx) Switch layer and SDK.

The source code, configuration and build instructions are in [docs/README.md](docs/README.md). Generating the game code and building your own NRO require your own game copy, devkitA64, libnx and the patched Mesa NVK; see [docs/build.md](docs/build.md). Game files, generated game code, NRO/NSP binaries and console keys are **not committed to this repository**.

## Credits and notices

Project maintainer: **xDASCATx**. Based on [ReXGlue](https://github.com/rexglue/rexglue-sdk), [Xenia](https://xenia.jp), [NFSMW-NX](https://github.com/StevensND/nfsmw-nx), [mesa-switch](https://github.com/danfromtico/mesa-switch), [libnx](https://github.com/switchbrew/libnx) and their contributors. The forwarder uses the [NTON](https://github.com/rlaphoenix/nton) loader based on nx-hbloader. See [third-party notices](THIRD_PARTY_NOTICES.md) and [license](LICENSE).

Unofficial fan project; not affiliated with or endorsed by FromSoftware, Bandai Namco, Microsoft or Nintendo. Game names and the official promotional cover art used for the optional forwarder's icon belong to their respective owners.
