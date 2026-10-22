# 17a. Все параметры (cvar) сборки DS2

Сгенерировано скриптом из исходников (`REXCVAR_DEFINE_*` в `sdk/src`, `sdk/include`, `app`). Значок — как его
показывает меню настроек (L+R+→). «live» — это только метка: работает ли смена на лету, зависит от того, читает ли
код значение каждый раз (см. 17-switch-settings-menu.md).

| Категория | Имя | Тип | По умолчанию | Значок | Описание |
|---|---|---|---|---|---|
| - | `my_cmd` | command | `[]() { DoThing(` | [live] |  |
| - | `name` | command_args | `callback` | [live] |  |
| Audio | `audio_diag_prioridad_critica` | bool | `false` | [live] | Diagnostico (solo PC): Audio Worker y XMA Decoder a prioridad de tiempo critico, como en la Switch, donde van por encima de los hilos del ju |
| Audio | `audio_ffmpeg_simd` | bool | `true` | [live] | Diagnostico: false = el decodificador XMA de FFmpeg usa solo codigo C, sin NEON ni SSE (para comparar el audio) |
| Audio | `audio_maxqframes` | int32 | `8` | [live] |  |
| Audio | `audio_mute` | bool | `false` | [live] | Mute audio output |
| Audio | `audio_sdl_bomba` | bool | `false` | [live] | Diagnostico: un hilo pide una trama del juego cada 5,333 ms (como la bomba del driver de la Switch) y SDL deja de liberar el semaforo al con |
| Audio | `audio_sdl_rafaga_tramas` | int32 | `0` | [live] | Diagnostico: el driver SDL saca las tramas del juego de N en N, con N liberaciones seguidas, como el driver de la Switch con buferes de 4 tr |
| Audio | `audio_switch_bomba` | bool | `true` | [live] | Pedir al juego una trama cada 5,333 ms (bomba de 187,5 Hz, como MarathonRecomp-NX) en vez de 4 seguidas al consumir cada bufer de audout, qu |
| Audio | `audio_switch_tramas_en_cola` | int32 | `10` | [live] | Con audio_switch_bomba: tramas del juego que se mantienen en cola antes de audout  |
| Audio | `audio_volcado_desde_s` | int32 | `0` | [live] | Diagnostico: segundos de audio de cada cliente que se saltan antes del volcado |
| Audio | `audio_volcado_s` | int32 | `0` | [live] | Diagnostico: segundos del audio que entrega el juego (6 canales, antes del driver) que se guardan en audio_volcado_<cliente>.wav junto al ej |
| Audio | `audio_volcado_salida_desde_s` | int32 | `0` | [live] | Diagnostico: segundos de salida SDL que se saltan antes del volcado |
| Audio | `audio_volcado_salida_s` | int32 | `0` | [live] | Diagnostico: segundos de lo que el driver SDL entrega al dispositivo (con los silencios por falta de tramas) que se guardan en audio_salida. |
| Audio | `audio_volcado_xma_contextos` | int32 | `12` | [live] | Diagnostico: contextos XMA que se vuelcan como mucho |
| Audio | `audio_volcado_xma_desde_s` | int32 | `0` | [live] | Diagnostico: segundos desde el primer audio XMA antes de empezar a volcar |
| Audio | `audio_volcado_xma_mb` | int32 | `32` | [live] | Diagnostico: megas como mucho de audio XMA acumulado sin escribir |
| Audio | `audio_volcado_xma_min_s` | int32 | `2` | [live] | Diagnostico: segundos minimos de un sonido XMA que se guardan cuando termina |
| Audio | `audio_volcado_xma_s` | int32 | `0` | [live] | Diagnostico: segundos de cada contexto XMA que se guardan en xma_<contexto>_tramas/salida_<hz>.wav junto al ejecutable; 0 = nada |
| Audio | `audio_xma_en_trabajador` | int32 | `REX_XMA_EN_TRABAJADOR_PO` | [live] | Quien descodifica el XMA al llegar un kick (XMAEnableContext): 0 = el hilo que lo pide, como hasta ahora; 1 = el hilo XMA Decoder, que en la |
| Audio | `ffmpeg_verbose` | bool | `false` | [live] | Verbose FFmpeg output (debug and above) |
| Category | `my_flag` | bool | `false` | [live] | Description |
| Category | `my_int` | int32 | `42` | [live] | An integer setting |
| Category | `my_string` | string | `"default"` | [live] | A string setting |
| Clock | `clock_no_scaling` | bool | `false` | [live] | Disable clock scaling (inverted: false = scaling enabled) |
| Clock | `clock_source_raw` | bool | `false` | [live] | Use raw clock source without scaling |
| Codegen | `backward_scan_limit` | uint32 | `64` | [init-only] (в меню серым) | Max instructions to scan backward for jump table patterns |
| Codegen | `max_blocks_per_function` | uint32 | `10000` | [init-only] (в меню серым) | Safety limit on blocks per function |
| Codegen | `max_discovery_iterations` | uint32 | `1000` | [init-only] (в меню серым) | Max iterations for function discovery convergence |
| Codegen | `max_eh_ip_map_entries` | uint32 | `200` | [init-only] (в меню серым) | Max IP-to-state map entries |
| Codegen | `max_eh_states` | uint32 | `100` | [init-only] (в меню серым) | Max C++ EH states before rejecting handler |
| Codegen | `max_eh_try_blocks` | uint32 | `50` | [init-only] (в меню серым) | Max try blocks before rejecting handler |
| Codegen | `max_file_size_bytes` | uint32 | `1048576` | [init-only] (в меню серым) | Target source file size in bytes; sets how many recomp files a project is split into, chosen once and then persisted |
| Codegen | `max_jump_table_entries` | uint32 | `512` | [init-only] (в меню серым) | Max entries per detected jump table |
| Codegen | `max_resolve_iterations` | uint32 | `100` | [init-only] (в меню серым) | Max iterations for call target resolution |
| Codegen | `max_seh_scope_entries` | uint32 | `100` | [init-only] (в меню серым) | Max SEH scope table entries |
| Codegen | `max_vtable_iterations` | uint32 | `100` | [init-only] (в меню серым) | Max iterations for vtable discovery |
| Codegen | `progress_log_frequency` | uint32 | `100` | [init-only] (в меню серым) | Log progress every N functions |
| Console | `find` | command_args | `ConsoleFind` | [live] | List cvar/command names containing a substring |
| DS2 | `ds2_func_counts` | bool | `false` | [start] | Диагностика: счётчики вызовов каждой функции игры, раз в `ds2_func_counts_every_s` с в `logs/ds2_llamadas.bin` (декодер `tools/analysis/ds2_func_counts.py`) |
| DS2 | `ds2_func_counts_every_s` | int32 | `10` | [live] | Секунд в окне `ds2_func_counts` |
| DS2 | `ds2_msr_per_thread` | bool | `false` | [start] | Области `mfmsr/mtmsrd` как состояние потока вместо общего мьютекса SDK (PORTING.md 5.1 порта Dead Space 2) |
| DS2 | `ds2_fpcr_ftz_fixed` | bool | `false` | [start] | Flush-to-zero FPCR всегда включён (скалярный код тоже); FPCR не переписывается |
| DS2 | `ds2_capture_dir` | string | `""` | [live] | Diagnostic: folder where guest output frames are saved as PPM (empty = off) |
| DS2 | `ds2_capture_every_ms` | int32 | `2000` | [live] | Diagnostic: interval between captured frames, ms |
| DS2 | `ds2_capture_max` | int32 | `60` | [live] | Diagnostic: maximum number of captured frames |
| DS2 | `ds2_capture_save` | bool | `true` | [live] | Diagnostic: write the PPM files (false = only log size and non-black share of each capture) |
| DS2 | `ds2_input_script` | string | `""` | [init-only] (в меню серым) | Tests: virtual pad script in seconds since start, e.g. \"60:start,70:a,80-82:down\"  |
| DS2 | `ds2_install_dlc` | string | `""` | [init-only] (в меню серым) | STFS DLC packages to install at startup if missing, separated by ';'  |
| DS2 | `ds2_mesa_env` | string | `""` | [init-only] (в меню серым) | Mesa/NVK environment variables, 'NAME=value;NAME=value' |
| DS2 | `ds2_switch_commit_granule_kb` | int32 | `2048` | [live] |  |
| DS2 | `ds2_switch_nvmap_mb` | int32 | `0` | [init-only] (в меню серым) |  |
| DS2 | `ds2_switch_saltynx` | bool | `false` | [init-only] (в меню серым) | Switch: publish FPS/resolution to SaltyNX overlays (Status Monitor) and follow ReverseNX |
| DS2 | `ds2_watchdog` | bool | `true` | [init-only] (в меню серым) | Load/hang telemetry in the main log: snapshots of guest threads, open files and memory, file open/close events and stall alarms |
| DS2 | `ds2_watchdog_heartbeat_s` | int32 | `2` | [init-only] (в меню серым) | Seconds between one-line [ds2 wd] hb heartbeats: vblank counter and the shaders the GPU command processor last bound (0 = off) |
| DS2 | `ds2_watchdog_probe_pages` | int32 | `0` | [init-only] (в меню серым) | Switch DIAGNOSTIC: every heartbeat, test how many more small kernel mapping changes still  |
| DS2 | `ds2_watchdog_probe_tables` | int32 | `32` | [init-only] (в меню серым) | Switch DIAGNOSTIC: every heartbeat, test how many more kernel page tables can still be taken,  |
| DS2 | `ds2_watchdog_snapshot_s` | int32 | `10` | [init-only] (в меню серым) | Seconds between [ds2 wd] snapshots (0 = only on stalls) |
| DS2 | `edram_diagnostic_mode` | int32 | `0` | [init-only] (в меню серым) | Switch DIAGNOSTIC: GPU work left out for the whole session, by mode number as shown on screen  |
| Debug | `debug_overlay` | bool | `false` | [live] | Show overlay |
| Display | `resolution` | string | `""` | [restart] | Common resolution preset for both guest video mode and startup window (for example: 720p, 1080p, 1440p, 4k, 1280x720) |
| Display | `video_mode_height` | int32 | `720` | [restart] | Guest video mode height in pixels |
| Display | `video_mode_refresh_rate` | double | `60.0` | [restart] | Guest video mode refresh rate in Hz |
| Display | `video_mode_width` | int32 | `1280` | [restart] | Guest video mode width in pixels |
| Filesystem | `allow_game_relative_writes` | bool | `false` | [live] | Not useful to non-developers. Allows code to write to paths relative to game://. Used for generating test data to compare with original hard |
| Filesystem | `dump_path` | string | `""` | [live] | Specifies the directory to dump files to |
| Filesystem | `dump_source` | string | `""` | [live] | Specifies the file to dump from |
| Filesystem | `nfsmw_io_aviso_ms` | int32 | `8` | [live] |  |
| Filesystem | `nfsmw_io_cache_mb` | int32 | `0` | [live] |  |
| Filesystem | `nfsmw_io_rangos_max_mb` | int32 | `12` | [live] | Ninguna entrada de la cache de rangos pasa de estos MB. |
| Filesystem | `nfsmw_io_rangos_mb` | int32 | `64` | [live] |  |
| Filesystem | `nfsmw_io_rangos_min_kb` | int32 | `256` | [live] | Solo se cachean las lecturas de estos KB o mas. 256 = el punto donde la simulacion  |
| Filesystem | `nfsmw_io_rangos_min_mb` | int32 | `0` | [live] | OBSOLETO (build 131): usar nfsmw_io_rangos_min_kb. Si es > 0 manda sobre el de KB. |
| Filesystem | `nfsmw_io_resumen_s` | int32 | `15` | [live] |  |
| Filesystem | `nfsmw_io_trozo_mb` | int32 | `4` | [live] |  |
| Filesystem | `nfsmw_io_ventana_kb` | int32 | `256` | [live] |  |
| Filesystem | `nfsmw_io_ventanas_max` | int32 | `16` | [live] | Maximo de ficheros con lectura anticipada a la vez. |
| GPU | `anisotropic_override` | int32 | `3` | [live] | Forces anisotropic filtering for eligible textures. Higher values keep textures sharper at oblique angles, but increase texture sampling co |
| GPU | `async_shader_compilation` | bool | `true` | [live] | Compile shaders and create pipelines asynchronously in background threads. This reduces stutter but may cause brief visual artifacts while p |
| GPU | `backend` | string | `"d3d12"` | [live] | Render backendd3d12vulkan |
| GPU | `clear_memory_page_state` | bool | `true` | [live] | Refresh page-valid state from GPU-written memory at frame end. Disable for minor CPU overhead reduction, but may break memory coherency. |
| GPU | `depth_float24_convert_in_pixel_shader` | bool | `false` | [live] | Convert float24 depth in pixel shader |
| GPU | `depth_float24_round` | bool | `false` | [live] | Round float24 depth values |
| GPU | `depth_transfer_not_equal_test` | bool | `true` | [live] | Use not-equal test for depth transfer |
| GPU | `direct_host_resolve` | bool | `true` | [live] | Resolve from host render targets directly to shared memory when possible |
| GPU | `draw_resolution_scale_x` | int32 | `1` | [restart] | Draw resolution scale X (1 = no scaling) |
| GPU | `draw_resolution_scale_y` | int32 | `1` | [restart] | Draw resolution scale Y (1 = no scaling) |
| GPU | `dump_shaders` | string | `""` | [live] | Path to dump shaders to |
| GPU | `execute_unclipped_draw_vs_on_cpu` | bool | `false` | [live] | Execute unclipped draw vertex shader on CPU |
| GPU | `execute_unclipped_draw_vs_on_cpu_for_psi_render_backend` | bool | `true` | [live] | Execute unclipped draw VS on CPU for PSI render backend |
| GPU | `execute_unclipped_draw_vs_on_cpu_with_scissor` | bool | `false` | [live] | Execute unclipped draw VS on CPU with scissor |
| GPU | `force_convert_line_loops_to_strips` | bool | `false` | [live] | Force convert line loops to strips |
| GPU | `force_convert_quad_lists_to_triangle_lists` | bool | `false` | [live] | Force convert quad lists to triangle lists |
| GPU | `force_convert_triangle_fans_to_lists` | bool | `false` | [live] | Force convert triangle fans to lists |
| GPU | `gamma_render_target_as_unorm16` | bool | `true` | [live] | Use R16G16B16A16_UNORM for gamma render targets (more accurate than sRGB) |
| GPU | `gpu_3d_to_2d_texture` | bool | `true` | [live] | Sample problematic 3D textures through 2D-compatible wrappers |
| GPU | `gpu_allow_invalid_fetch_constants` | bool | `false` | [live] | Allow invalid fetch constants |
| GPU | `gpu_debug_markers` | bool | `false` | [live] | Insert debug markers into GPU command streams for tools like PIX and RenderDoc. Automatically enabled when RenderDoc is detected. |
| GPU | `gpu_plugin` | string | `""` | [live] |  |
| GPU | `half_pixel_offset` | bool | `true` | [live] | Enable half pixel offset |
| GPU | `mrt_edram_used_range_clamp_to_min` | bool | `true` | [live] | Clamp MRT EDRAM used range to minimum |
| GPU | `native_2x_msaa` | bool | `true` | [live] | Enable native 2x MSAA |
| GPU | `native_stencil_value_output` | bool | `true` | [live] | Enable native stencil value output |
| GPU | `non_seamless_cube_map` | bool | `false` | [live] | Use non-seamless cube map sampling |
| GPU | `occlusion_query_enable` | bool | `true` | [live] | Enable host occlusion query handling |
| GPU | `pre_mask_resolve_l2_block` | bool | `true` | [live] | Pre-mask scaled resolve L2 blocks to the write range before iterating |
| GPU | `primitive_processor_cache_min_indices` | int32 | `0` | [live] | Minimum indices for primitive processor cache |
| GPU | `query_occlusion_fake_sample_count` | int32 | `1000` | [live] | Fake sample count for occlusion queries |
| GPU | `readback_memexport` | bool | `true` | [live] | Enable CPU readback of shader memexport writes for guest memory coherency (can reduce correctness issues, but may add GPU/CPU sync cost) |
| GPU | `readback_memexport_fast` | bool | `true` | [live] | Use fast double-buffered memexport readback when possible, with automatic fallback to full synchronous readback |
| GPU | `readback_resolve` | string | `"none"` | [live] | Controls CPU readback of render-to-texture resolve results.  none: Disable readback (default)  fast: Read previous frame (delayed, copy ev |
| GPU | `readback_resolve_half_pixel_offset` | bool | `false` | [live] | When draw resolution scaling is active, sample from the center of each scaled block during resolve readback downscale |
| GPU | `resolution_scale` | int32 | `1` | [restart] | Draw resolution scale for both X and Y axes (same as setting draw_resolution_scale_x and draw_resolution_scale_y) |
| GPU | `resolve_resolution_scale_fill_half_pixel_offset` | bool | `true` | [live] | Fill half pixel offset during resolution scale resolve |
| GPU | `scale` | int32 | `1` | [live] | Resolution scale |
| GPU | `snorm16_render_target_full_range` | bool | `true` | [live] | Use full range for SNORM16 render targets |
| GPU | `store_shaders` | bool | `true` | [live] | Store shaders persistently and load them when loading games to avoid runtime spikes and freezes when playing the game not for the first time |
| GPU | `swap_post_effect` | string | `"none"` | [restart] | Swap post effect: none, fxaa, fxaa_extreme |
| GPU | `texture_cache_memory_limit_hard` | int32 | `768` | [restart] | Hard texture cache memory limit (MB) |
| GPU | `texture_cache_memory_limit_render_to_texture` | int32 | `24` | [restart] | Texture cache memory limit for render-to-texture (MB) |
| GPU | `texture_cache_memory_limit_soft` | int32 | `384` | [restart] | Soft texture cache memory limit (MB) |
| GPU | `texture_cache_memory_limit_soft_lifetime` | int32 | `30` | [live] | Soft texture cache memory limit lifetime (seconds) |
| GPU | `use_fuzzy_alpha_epsilon` | bool | `false` | [live] | Use approximate compare for alpha test values to prevent flickering on NVIDIA graphics cards |
| GPU | `vsync` | bool | `true` | [live] | Enable vertical sync |
| GPU/D3D12 | `d3d12_bindless` | bool | `true` | [restart] | Use bindless resources where available |
| GPU/D3D12 | `d3d12_dxbc_disasm` | bool | `false` | [live] | Dump DXBC disassembly |
| GPU/D3D12 | `d3d12_dxbc_disasm_dxilconv` | bool | `false` | [live] | Dump DXIL conversion disassembly |
| GPU/D3D12 | `d3d12_pipeline_creation_threads` | int32 | `-1` | [restart] | Number of pipeline creation threads (-1 for auto) |
| GPU/D3D12 | `d3d12_readback_memexport` | bool | `false` | [live] | Read data written by memory export in shaders on the CPU |
| GPU/D3D12 | `d3d12_readback_resolve` | bool | `false` | [live] | Read render-to-texture results on the CPU |
| GPU/D3D12 | `d3d12_submit_on_primary_buffer_end` | bool | `true` | [live] | Submit command list when PM4 primary buffer ends |
| GPU/D3D12 | `d3d12_tessellation_wireframe` | bool | `false` | [live] | Render tessellation as wireframe |
| GPU/D3D12 | `d3d12_tiled_shared_memory` | bool | `true` | [restart] | Use tiled shared memory on D3D12 |
| GPU/D3D12 | `native_stencil_value_output_d3d12_intel` | bool | `false` | [live] | Native stencil value output for Intel D3D12 |
| GPU/D3D12 | `render_target_path_d3d12` | string | `""` | [init-only] (в меню серым) | D3D12 render target implementation path |
| GPU/Shader | `draw_resolution_scaled_texture_offsets` | bool | `true` | [live] | Scale texture offsets with draw resolution |
| GPU/Shader | `dxbc_source_map` | bool | `false` | [live] | Generate source maps for DXBC |
| GPU/Shader | `dxbc_switch` | bool | `true` | [live] | Use switch statements in DXBC |
| GPU/Vulkan | `gpu_zone_timing` | bool | `false` | [init-only] (в меню серым) | Switch DIAGNOSTIC: measure GPU time of transfers, resolves, texture loads and output with timestamp queries; reported per frame in rex_perfi |
| GPU/Vulkan | `render_target_direct_resolve` | bool | `false` | [live] | Fast 32bpp resolves (the usual render-to-texture copy) straight from the host render targets into guest memory in one compute pass, instead  |
| GPU/Vulkan | `render_target_direct_resolve_validate` | int32 | `0` | [live] | DEVELOPMENT (with render_target_direct_resolve): also do every direct resolve the usual way and compare the bytes written to guest memory wh |
| GPU/Vulkan | `render_target_path_vulkan` | string | `""` | [init-only] (в меню серым) | Vulkan render target implementation path |
| GPU/Vulkan | `render_target_transfer_elide_overwritten` | bool | `false` | [live] | Skip render target ownership transfers into areas that the draw binding the render targets  |
| GPU/Vulkan | `render_target_transfer_skip` | int32 | `0` | [live] | Leave out render target ownership transfer draws (bitmask): 1 = stencil-bit passes, 2 = depth/stencil transfers, 4 = color transfers. Faster |
| GPU/Vulkan | `rt_transfer_stats` | int32 | `0` | [live] | DEVELOPMENT: log a histogram of render target ownership transfers (destination <- source key) every N render targets receiving transfers (0  |
| GPU/Vulkan | `rt_transfer_trace` | int32 | `0` | [live] | DEVELOPMENT (with rt_transfer_stats): log every transfer of 200 destination updates starting at this update number (0 = off) |
| GPU/Vulkan | `vulkan_async_skip_incomplete_frames` | bool | `true` | [live] | When async shader compilation is enabled, skip presenting frames that used placeholder pipelines to avoid visible flashing |
| GPU/Vulkan | `vulkan_dynamic_rendering` | bool | `true` | [live] | Use VK_KHR_dynamic_rendering for Vulkan GPU emulation when supported by the device (falls back to render passes otherwise) |
| GPU/Vulkan | `vulkan_force_convert_quad_lists_to_triangle_lists` | bool | `false` | [init-only] (в меню серым) | Force Vulkan quad list conversion to triangle lists in primitive processing, even when geometry shaders are available |
| GPU/Vulkan | `vulkan_force_expand_point_sprites_in_vs` | bool | `false` | [init-only] (в меню серым) | Force Vulkan point sprite expansion in the vertex shader, even when geometry shaders are available |
| GPU/Vulkan | `vulkan_force_expand_rectangle_lists_in_vs` | bool | `false` | [init-only] (в меню серым) | Force Vulkan rectangle list expansion in the vertex shader, even when geometry shaders are available |
| GPU/Vulkan | `vulkan_pipeline_creation_threads` | int32 | `-1` | [restart] | Number of pipeline creation threads for Vulkan async pipeline creation (-1 for auto) |
| GPU/Vulkan | `vulkan_readback_memexport` | bool | `false` | [live] | Read data written by memory export in shaders on the CPU |
| GPU/Vulkan | `vulkan_readback_resolve` | bool | `false` | [live] | Read render-to-texture results on the CPU |
| GPU/Vulkan | `vulkan_sparse_shared_memory` | bool | `true` | [restart] | Use sparse shared memory on Vulkan |
| GPU/Vulkan | `vulkan_submit_on_primary_buffer_end` | bool | `true` | [live] | Submit command buffer when PM4 primary buffer ends |
| GPU/Vulkan | `vulkan_tessellation_wireframe` | bool | `false` | [live] | Render tessellation as wireframe |
| Input | `guide_button` | bool | `false` | [live] | Enable guide button pass-through |
| Input | `hid_mappings_file` | string | `"gamecontrollerdb.txt"` | [live] | Path to SDL gamecontroller mappings file |
| Input | `input_backend` | string | `"sdl"` | [live] | Input backend: sdl, xinput |
| Input | `input_vibracion` | bool | `false` | [live] | Vibracion del mando (desactivada por defecto: a los mandos solo llega  |
| Input | `input_xbox_layout` | bool | `false` | [live] |  |
| Input | `mnk_mode` | bool | `false` | [live] | Enable keyboard/mouse controller emulation |
| Input | `mnk_mouse` | bool | `false` | [live] | Use the mouse for the right stick. Off means the right stick comes from the keybind_rstick_* keys only |
| Input | `mnk_sensitivity` | double | `1.0` | [live] | Mouse sensitivity for right stick |
| Kernel | `cl` | string | `""` | [live] | Specify additional command-line provided to guest |
| Kernel | `content_backup_root` | string | `""` | [live] | Carpeta donde dejar una copia de cada guardado, con una subcarpeta por perfil. Vacia = no se copia nada |
| Kernel | `headless` | bool | `false` | [live] | Don't display any UI, using defaults for prompts as needed |
| Kernel | `host_thread_stack_mb` | int32 | `16` | [live] | Host stack of each guest thread, MB. Measured on PC (DS2): at most ~3.6 MB used. On Switch each stack is a separate mapping that needs ~1 pa |
| Kernel | `ignore_thread_affinities` | bool | `true` | [live] | Ignores game-specified thread affinities |
| Kernel | `ignore_thread_priorities` | bool | `true` | [live] | Ignores game-specified thread priorities |
| Kernel | `kernel_cert_monitor` | bool | `false` | [live] | Enable cert monitor |
| Kernel | `kernel_debug_monitor` | bool | `false` | [live] | Enable debug monitor |
| Kernel | `kernel_pix` | bool | `false` | [live] | Enable PIX debugging support |
| Kernel | `license_mask` | uint32 | `0` | [live] | Set license mask for activated content |
| Kernel | `log_high_frequency_kernel_calls` | bool | `false` | [live] | Log kernel calls with the kHighFrequency tag |
| Kernel | `user_country` | uint32 | `103` | [live] | User's country ID |
| Kernel | `user_language` | uint32 | `1` | [live] | User's language ID |
| Log | `log_async` | bool | `false` | [live] |  |
| Log | `log_async_cola` | int32 | `8192` | [init-only] (в меню серым) | Mensajes que caben en la cola del log asincrono |
| Log | `log_file` | string | `""` | [live] | Log file path (empty = auto sequential naming) |
| Log | `log_flush_interval` | int32 | `1` | [init-only] (в меню серым) | Periodic flush interval in seconds (0 = off) |
| Log | `log_level` | string | `"info"` | [live] | Global log level: trace, debug, info, warn, error, critical, off |
| Log | `log_max_file_size_mb` | int32 | `5` | [init-only] (в меню серым) | Max log file size in MB before rotation |
| Log | `log_max_files` | int32 | `20` | [init-only] (в меню серым) | Max number of rotated log files to keep |
| Log | `log_noisy` | bool | `false` | [live] | Enable noisy/high-frequency log macros |
| Log | `log_verbose` | bool | `false` | [live] | Enable verbose logging (sets level to trace) |
| Memory | `diag_disable_gpu_write_watch` | bool | `false` | [init-only] (в меню серым) | DIAGNOSTIC: do not protect pages for GPU write watches (stale textures expected) |
| Memory | `protect_on_release` | bool | `false` | [live] | Protect released memory to prevent accesses |
| Memory | `protect_zero` | bool | `true` | [restart] | Protect the zero page from reads and writes |
| Memory | `scribble_heap` | bool | `false` | [live] | Scribble 0xCD into all allocated heap memory |
| Memory | `writable_executable_memory` | bool | `true` | [restart] | Allow executable memory to be writable |
| Perf | `perf_log_csv` | string | `""` | [live] | Path to write per-frame CSV log (empty = disabled) |
| Runtime | `cache_root` | string | `""` | [live] | Override shader cache path |
| Runtime | `game_data_root` | string | `""` | [live] | Override game data path |
| Runtime | `metadata_root` | string | `""` | [live] | Override metadata path |
| Runtime | `update_data_root` | string | `""` | [live] | Override update data path |
| Runtime | `user_data_root` | string | `""` | [live] | Override user data path |
| UI/D3D12 | `d3d12_adapter` | int32 | `-1` | [init-only] (в меню серым) | Index of the DXGI adapter to use (-1 for any physical, -2 for WARP) |
| UI/D3D12 | `d3d12_allow_variable_refresh_rate_and_tearing` | bool | `true` | [live] | Allow variable refresh rate and tearing |
| UI/D3D12 | `d3d12_break_on_error` | bool | `false` | [live] | Break on Direct3D 12 validation errors |
| UI/D3D12 | `d3d12_break_on_warning` | bool | `false` | [live] | Break on Direct3D 12 validation warnings |
| UI/D3D12 | `d3d12_debug` | bool | `false` | [init-only] (в меню серым) | Enable Direct3D 12 and DXGI debug layer |
| UI/D3D12 | `d3d12_queue_priority` | int32 | `1` | [live] | Graphics command queue priority (0=normal, 1=high, 2=realtime) |
| UI/Presenter | `host_present_from_non_ui_thread` | bool | `true` | [live] | Allow presentation from non-UI thread |
| UI/Presenter | `host_present_ignore_implicit_vsync` | bool | `true` | [live] | Presentar desde el hilo del juego aunque la superficie tenga vsync implicito  |
| UI/Presenter | `present_allow_overscan_cutoff` | bool | `false` | [restart] | Allow overscan cutoff based on safe area settings |
| UI/Presenter | `present_cas_additional_sharpness` | double | `rex::ui::Presenter::Gues` | [restart] | Additional CAS sharpness in [0, 1] |
| UI/Presenter | `present_dither` | bool | `false` | [restart] | Enable output dithering in the final present pass |
| UI/Presenter | `present_effect` | string | `"bilinear"` | [restart] | Guest output effect: bilinear |
| UI/Presenter | `present_esperar_adquisicion_en_cpu` | bool | `false` | [restart] | Esperar la imagen de la cadena de intercambio en la CPU (fence) en vez de con un semaforo en el pintado, para no parar el canal de la GPU (p |
| UI/Presenter | `present_fsr_max_upsampling_passes` | int32 | `rex::ui::Presenter::Gues` | [restart] | Maximum chained FSR EASU passes |
| UI/Presenter | `present_fsr_quality_mode` | string | `"auto"` | [restart] | Temporal FSR quality mode: auto, nativeaa, quality, balanced, performance, ultra_performance |
| UI/Presenter | `present_fsr_sharpness_reduction` | double | `rex::ui::Presenter::Gues` | [restart] | FSR RCAS sharpness reduction in stops |
| UI/Presenter | `present_hilo_espera_max_ms` | int32 | `50` | [live] | Plazo maximo de esa espera, en ms. Es una red de seguridad por si el presentador se colgara, no un mecanismo: al agotarse se descarta como a |
| UI/Presenter | `present_hilo_espera_media_max_us` | int32 | `1500` | [live] | Microsegundos que puede costarle de media el presentador al juego, por fotograma, antes de apagar el hilo propio. 1500 = 1,5 ms sobre un fot |
| UI/Presenter | `present_hilo_gracia_fotogramas` | int32 | `3000` | [live] | Fotogramas de gracia antes de que la rendicion pueda dispararse. Durante el arranque y las cargas los tres nucleos estan saturados y el pres |
| UI/Presenter | `present_hilo_prioridad` | int32 | `0x2C` | [live] |  |
| UI/Presenter | `present_hilo_propio` | bool | `REX_PRESENT_HILO_PROPIO_` | [restart] | Presentar en un hilo propio del presentador en vez de dentro del hilo que genera la imagen. El hilo del juego solo marca que hay imagen nuev |
| UI/Presenter | `present_hilo_sin_descartes` | bool | `true` | [live] | El hilo que genera la imagen espera a que el presentador coja el fotograma anterior en vez de tirarlo. Con el presentador sobrado no espera  |
| UI/Presenter | `present_hilo_ventana` | int32 | `600` | [live] | Fotogramas de la ventana con la que se juzga si el hilo de presentacion da abasto. Se mira la espera MEDIA, no las rachas: en la compilacion |
| UI/Presenter | `present_letterbox` | bool | `true` | [live] | Enable letterboxing for non-native aspect ratios |
| UI/Presenter | `present_perfil_pintado` | bool | `REX_PRESENT_PERFIL_PINTA` | [restart] | Medir trozo a trozo el tiempo que el hilo que pinta pasa dentro de PaintAndPresent (espera al envio antiguo, reset del pool, adquisicion, bu |
| UI/Presenter | `present_refresco_sellado_perezoso` | bool | `false` | [restart] | Quitar el vkQueueSubmit vacio que se hacia en cada refresco de la salida del juego y sellar solo cuando hay que destruir esa imagen (cambio  |
| UI/Presenter | `present_render_pass_clear` | bool | `true` | [live] | Clear render pass during presentation |
| UI/Presenter | `present_safe_area_x` | int32 | `90` | [live] | Horizontal safe area percentage (0-100) |
| UI/Presenter | `present_safe_area_y` | int32 | `90` | [live] | Vertical safe area percentage (0-100) |
| UI/Vulkan | `vulkan_allow_present_mode_fifo_relaxed` | bool | `true` | [live] | Allow FIFO relaxed present mode |
| UI/Vulkan | `vulkan_allow_present_mode_immediate` | bool | `REX_VULKAN_ALLOW_PRESENT` | [live] | Allow immediate present mode (no vsync) |
| UI/Vulkan | `vulkan_allow_present_mode_mailbox` | bool | `REX_VULKAN_ALLOW_UNSYNCE` | [live] | Allow mailbox present mode (triple buffering) |
| UI/Vulkan | `vulkan_device` | int32 | `-1` | [init-only] (в меню серым) | Vulkan device index (-1 for auto selection) |
| UI/Vulkan | `vulkan_log_debug_messages` | bool | `true` | [live] | Log Vulkan debug messages |
| UI/Vulkan | `vulkan_native_shader_features` | bool | `false` | [init-only] (в меню серым) | Habilitar capacidades disponibles para los shaders nativos experimentales |
| UI/Vulkan | `vulkan_prefer_fill_mode_non_solid` | bool | `true` | [init-only] (в меню серым) | Prefer physical devices supporting fillModeNonSolid when auto-selecting |
| UI/Vulkan | `vulkan_prefer_fragment_stores_and_atomics` | bool | `true` | [init-only] (в меню серым) | Prefer physical devices supporting fragmentStoresAndAtomics when auto-selecting |
| UI/Vulkan | `vulkan_prefer_geometry_shader` | bool | `true` | [init-only] (в меню серым) | Prefer physical devices supporting geometryShader when auto-selecting |
| UI/Vulkan | `vulkan_prefer_vertex_pipeline_stores_and_atomics` | bool | `true` | [init-only] (в меню серым) | Prefer physical devices supporting vertexPipelineStoresAndAtomics when auto-selecting |
| UI/Vulkan | `vulkan_require_fill_mode_non_solid` | bool | `!REX_PLATFORM_MAC` | [init-only] (в меню серым) | Require fillModeNonSolid support for Vulkan GPU emulation (disable to allow fallback to solid fill for line/point polygon modes) |
| UI/Vulkan | `vulkan_require_fragment_stores_and_atomics` | bool | `true` | [init-only] (в меню серым) | Deprecated and ignored for parity; fragmentStoresAndAtomics is always required for Vulkan GPU emulation |
| UI/Vulkan | `vulkan_require_geometry_shader` | bool | `!REX_PLATFORM_MAC` | [init-only] (в меню серым) | Require geometryShader support for Vulkan GPU emulation (disable to allow fallback primitive emulation paths) |
| UI/Vulkan | `vulkan_require_vertex_pipeline_stores_and_atomics` | bool | `true` | [init-only] (в меню серым) | Deprecated and ignored for parity; vertexPipelineStoresAndAtomics is always required for Vulkan GPU emulation |
| UI/Vulkan | `vulkan_validation_enabled` | bool | `false` | [init-only] (в меню серым) | Enable Vulkan validation layers |
| UI/Window | `fullscreen` | bool | `true` | [live] | Start the window in fullscreen mode |
| UI/Window | `monitor` | int32 | `0` | [restart] | Monitor index to display on (0 = default, 1 = primary, 2 = second monitor, etc.) |
| UI/Window | `video_driver` | string | `""` | [restart] | SDL video driver to use, such as \"wayland\" or \"x11\". Empty picks SDL's default for the session |
| UI/Window | `window_height` | int32 | `0` | [restart] | Startup window height in logical pixels (0 = use app default) |
| UI/Window | `window_width` | int32 | `0` | [restart] | Startup window width in logical pixels (0 = use app default) |
| crt | `rexcrt_heap_enable` | bool | `false` | [init-only] (в меню серым) | Enable o1heap-backed CRT heap for RtlAllocateHeap/Free/Size/ReAlloc |
| crt | `rexcrt_heap_size_mb` | uint32 | `256` | [init-only] (в меню серым) | Heap size in megabytes |
| {} | `echo` | command_args | `[](std::string_view args` | [live] |  |

Раскладка кнопок (`Input/Keybinds/Controller`, 25 шт.) опущена.
