# DARK SOULS II: Scholar of the First Sin — Switch 0.56

**First public test build (v56). Not a stable or 30 FPS release.** The 0.56 executable is v56 with only the NRO icon and application metadata changed for this public package. Its compiled game code is byte-for-byte identical to v56. The `ds2.toml` is byte-for-byte identical to the v56 test configuration.

The last **console-tested** build was v55: approximately 20 FPS in tested areas, with dips to 8 FPS while loading. v56/0.56 reached the menu and an in-game frame on a PC (lavapipe), but **has not yet been tested on the Switch**. Please report regressions rather than assuming that performance has improved.

## Downloads

- **`ds2.nro`** — Switch application. Required.
- **`ds2.toml`** — v56 configuration. Required for the tested renderer and clock settings.
- **`ds2-0.56-forwarder.nsp`** — optional HOME Menu shortcut; requires `ds2.nro` at `sdmc:/switch/ds2/ds2.nro`. Does not include the game, NRO or console keys. NSP Title ID: `0100D52000560000`.
- **`SHA256SUMS`** — checksums for the release assets.

You must provide your own Xbox 360 copy of *Scholar of the First Sin* (Title ID `465307E4`, `Default.xex` and game data) and its Compatibility Pack 4 DLC. Copy `ds2.nro` and `ds2.toml` into `sdmc:/switch/ds2/`, the game's original directory structure into `sdmc:/switch/ds2/game_root/`, and the Compatibility Pack 4 STFS package into `sdmc:/switch/ds2/dlc/` for the first launch. Run with full application memory via Homebrew Menu title takeover or the optional NSP forwarder, **not** Album/app-mode. See the [installation instructions](README.md#download-and-install).

**Clock warning:** `ds2.toml` requests CPU **1963 MHz** and GPU **1267 MHz** using a supported clock sysmodule. Memory clock is left unchanged. These are high, non-stock frequencies. Only use them on suitable hardware; after a crash the override may need to be cleared manually. Detailed diagnostics, profiling and frame capture are enabled by default.

## Changes from v55

- Reduced CPU work in the native renderer: per-thread `mtmsrd` state, dynamic uniform-buffer offsets, descriptor reuse, faster GPU register writes and texture untiling, and sub-resolve lookup caching.
- Enabled fused multiply-add in pixel shaders (vertex shaders remain unchanged).
- Added sampling of expensive GPU draw calls once per 900 frames.

These changes were **not measured on the console**. The native renderer remains the primary path; Xenos GPU emulation is retained as a fallback.

## Known issues / feedback requested

The 30+ FPS goal has not been reached; loading stutters, incorrect bonfire lighting near the merchant, and loot visible through walls were reported with v55. Real occlusion queries previously caused enemy flicker and remain disabled. Please test Majula (sea and town views), combat and area transitions and include the logs under `sdmc:/switch/ds2/logs/`. L3+R3 held for about a second captures a problematic frame under `native_dump/`. Do not publish game files, saves or `prod.keys` with bug reports.

The forwarder NSP has not yet been validated on a physical Switch. It is an unsigned homebrew shortcut, which may require additional system support and may carry a console-ban risk. Use the NRO through title takeover if you do not want a forwarder.

## Verification and provenance

- v56 NRO: `fca3c3bef28e46e7d44b3d0dc189d7e564ee8bb4e339302bc921bf1beee8671f` (the earlier privately tested PC build).
- Public 0.56 NRO: `67027bfb435fce0d505f98fd061641285a44d75289c94332b77ffae00f14745f` (only NRO icon/NACP changed; executable section SHA-256: `b6b56b342638a688d1ba9d5440448f85d3eba7954fb9b110e733e8b5a478bb41`).
- `ds2.toml`: `640795b195a7099738af51670a8a7b025146bd29c05f6c80e7742b7d01463e09` (unchanged from v56).
- Forwarder: [NTON](https://github.com/rlaphoenix/NTON) loader (`c5d543a91b7c`), packaged with [hacBrewPack](https://github.com/rlaphoenix/hacBrewPack) (`745b16ecfc9c`). Icon adapted from the official [Steam store cover](https://store.steampowered.com/app/335300/) without committing any artwork or keys to this repository. [Third-party notices](THIRD_PARTY_NOTICES.md).

Unofficial, community test release maintained by **xDASCATx**. This project is not affiliated with or endorsed by FromSoftware, Bandai Namco, Microsoft or Nintendo.
