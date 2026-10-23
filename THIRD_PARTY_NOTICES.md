# Third-party notices

This project builds on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) and the Nintendo Switch porting work of [NFSMW-NX](https://github.com/StevensND/nfsmw-nx). Their respective licenses apply to the components derived from them. The project's own sources are under [GPL-3.0](LICENSE); the SDK has its own [BSD-3-Clause license](sdk/LICENSE). The NRO is linked with additional third-party libraries and Mesa NVK.

| Project | Use | License |
|---|---|---|
| [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) (with portions from [Xenia](https://xenia.jp)) | Xbox 360 runtime, code generator, graphics command processing (`sdk/`) | BSD-3-Clause and third-party licenses (`sdk/LICENSE`) |
| [NFSMW-NX](https://github.com/StevensND/nfsmw-nx) | Switch-specific SDK layer, build approach and tooling | Licenses of its respective components |
| [mesa-switch](https://github.com/danfromtico/mesa-switch) / [Mesa](https://mesa3d.org) | Vulkan/NVK driver linked into the NRO, with a DS2 patch | Per-file Mesa licenses (principally MIT) |
| [libnx](https://github.com/switchbrew/libnx) | Horizon integration | ISC |
| [FFmpeg](https://ffmpeg.org) | WMV3 and XMA decoding in the SDK | LGPL-2.1-or-later (configuration without GPL-only components) |
| [libmspack](https://www.cabextract.org.uk/libmspack/) | LZX decompression | LGPL-2.1 |
| [glslang](https://github.com/KhronosGroup/glslang) | Shader compilation | BSD-3-Clause and others |
| [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools) | SPIR-V processing | Apache-2.0 |
| [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) | Vulkan memory | MIT |
| [fmt](https://github.com/fmtlib/fmt), [spdlog](https://github.com/gabime/spdlog) | Formatting and logging | MIT |
| [xxHash](https://github.com/Cyan4973/xxHash) | Cache keys | BSD-2-Clause |
| [SIMDe](https://github.com/simd-everywhere/simde) | Portable intrinsics | MIT |
| [toml++](https://github.com/marzer/tomlplusplus) | Configuration | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | Debug UI | MIT |
| [o1heap](https://github.com/pavel-kirienko/o1heap) | Allocator | MIT |

Additional ReXGlue third-party dependencies and their licenses are supplied by the pinned upstream sources fetched with `tools/fetch_thirdparty.py`. The modified third-party files in this repository are under `sdk/thirdparty/`; the rest are fetched from ReXGlue v0.10.0. The sources, the build configuration and the LGPL components' changes are documented in [docs/build.md](docs/build.md).

## NSP forwarder

The optional NSP includes a small homebrew loader, **not** the game's executable. Its prebuilt `main` and `main.npdm` are from [NTON](https://github.com/rlaphoenix/NTON), commit `c5d543a91b7c`, based on [nx-hbloader](https://github.com/switchbrew/nx-hbloader) and Forwarder-Mod. The forwarder loader is distributed under the following ISC notice (also present in NTON's `rom/LICENSE.md`):

> Copyright 2017-2018 nx-hbloader Authors
>
> Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
>
> THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

The NSP build tool is [hacBrewPack](https://github.com/rlaphoenix/hacBrewPack), commit `745b16ecfc9c` (GPL-2.0); the tool itself is **not** included in the NRO or NSP. Console `prod.keys` are used locally for packaging only and must never be uploaded or committed.

## Trademarks and artwork

DARK SOULS II and its official promotional cover art (the optional forwarder's icon, adapted from [Steam's official store assets](https://store.steampowered.com/app/335300/)) belong to their respective owners. Nintendo Switch, Xbox 360 and other marks belong to their respective owners. This unofficial port is not affiliated with or endorsed by them.
