# Справочник: эмуляционный графический путь (Xenia/Vulkan) — fallback

Статус: **не развивается** с 2026-10-23 (решение владельца, [decisions.md](../decisions.md) D11, D14). Включается
`ds2_native_renderer = false`; рабочие значения ключей — `config/toml-history/15-ds2.toml.v54_emulation`
(= v50). Последний замер на консоли: v50, 11–16 кадр/с при CPU 2397 / GPU 1267.

## Как устроен (по коду SDK, Xenia)

`VulkanCommandProcessor` (`sdk/src/graphics/vulkan/command_processor.cpp`) исполняет те же пакеты PM4, но
эмулирует EDRAM Xenos: общая память GPU 512 МБ (`shared_memory`), кэш render target (`render_target_cache`) с
переносами владения между форматами/MSAA (transfer draws), resolve через вычислительные шейдеры, текстуры из
общей памяти с наблюдением за записью (write watch). Узкое место на Switch (замеры v31–v50,
[handoff/16](../handoff/16-switch-gpu-optimization.md)): переносы EDRAM 20–39 мс, resolve 7–12, перезагрузка текстур ~6.

## Правки порта (ключи cvar)

| Ключ | Что | Версия |
|---|---|---|
| `render_target_transfer_skip`, `_skip_kinds` | пропуск видов переносов (MODE 7 = без 8 проходов битов трафарета; A/B-тесты v49/v50) | test29, v49 |
| `render_target_transfer_elide_overwritten`, `_elide_extended` | не переносить, что draw перезапишет целиком (в т.ч. глубина, полноэкранные проходы, с учётом W) | v32–v34 |
| `execute_unclipped_draw_vs_on_cpu` | высота очисток по вершинам (`draw_extent_estimator`) | v32 |
| `render_target_direct_resolve` | resolve одним проходом (`shaders/ds2/direct_resolve_fast_32bpp.comp`) | v33 |
| `vulkan_max_frames_in_flight`, `vulkan_shared_memory_hoist_uploads` | 2 кадра в полёте; загрузки в начало отправки | v35–v36 |
| `readback_resolve`, `readback_resolve_max_bytes` | CPU-копии маленьких resolve | v36 |
| `vulkan_render_target_budget_mb`, `render_target_max_height` | освобождение RT, высота ≤ 1024 (Маджула) | v47–v48 |
| `render_target_msaa_alias` | 1x/4x на общей памяти (эксперимент) | v51 |
| `gpu_zone_timing*`, `vulkan_barrier_stats`, `vulkan_memory_report_frames`, `rt_transfer_stats*` | измерители | v31–v38 |
| `resolve_dump_*` | дампы resolve кадра (`tools/analysis/ds2_resolve_dump.py`) | v37 |
| `edram_diagnostic_mode` | режимы MODE 0…10 (`app/src/ds2_mode_overlay.cpp`, круг L+R+ZL) | diag14 |

FSI-путь (fragment shader interlock) на NVK зависал на загрузке (тест 24) — не использовать.
