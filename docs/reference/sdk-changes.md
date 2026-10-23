# Справочник: правки SDK, сделанные портом DS2

База SDK — ReXGlue v0.10.0 (`c94f5eb`) с правками nfsmw-nx (Switch-слой, профайлер, звук), импортирована
коммитом `9faaf22`. Список ниже — `git diff --stat 9faaf22 HEAD -- sdk` (49 файлов, +9,5 тыс. строк) по группам.
Правки nfsmw-nx описаны в `docs/nfsmw/` (их документация, частично устаревшая для DS2).

| Группа | Файлы | Что | Версия | Справка |
|---|---|---|---|---|
| Нативный рендерер | `src/graphics/native/*`, `src/graphics/plugin_main.cpp`, `src/graphics/CMakeLists.txt` | свой GraphicsSystem/CommandProcessor | v51–v56 | [native-renderer.md](native-renderer.md) |
| Переводчик шейдеров | `include/.../spirv_builder.h`, `src/.../spirv_builder.cpp`, `spirv_translator.h/.cpp` | флаг `allow_contraction` / `SetPixelShaderContraction` (FMA в PS) | v56 | native-renderer.md §8 |
| Vulkan-функции | `include/rex/ui/vulkan/functions/device_1_0.inc` | +2 функции в таблице устройства | v51+ | — |
| Эмуляция EDRAM: render target cache | `src/graphics/vulkan/render_target_cache.cpp/.h`, `pipeline/render_target/cache.cpp/.h` | пропуск переносов, которые draw перезапишет (`render_target_transfer_elide_*`), пропуск по видам (`_skip`, `_skip_kinds`), прямой resolve, освобождение RT при нехватке/бюджете, ограничение высоты, MSAA-алиасинг (тест) | v32–v51 | [emulation-gpu-path.md](emulation-gpu-path.md) |
| Эмуляция: command processor | `src/graphics/vulkan/command_processor.cpp/.h`, `deferred_command_buffer.*` | зоны времени GPU, статистика барьеров/памяти, загрузки в начало отправки, readback resolve, дампы resolve, журнал draw | v31–v40 | emulation-gpu-path.md |
| Оценка экстента draw | `src/graphics/util/draw_extent_estimator.cpp/.h` | высота по вершинам (VS на CPU) для очисток/полноэкранных проходов | v32–v34 | emulation-gpu-path.md |
| Прямой resolve | `src/graphics/shaders/ds2/direct_resolve_fast_32bpp.comp` + 4 скомпилированных `ds2_direct_resolve_*_cs.h` | resolve одним проходом из EDRAM в память | v33 | `tools/ds2/gen_direct_resolve_shaders.sh` |
| Общая память GPU | `src/graphics/shared_memory.cpp`, `vulkan/shared_memory.cpp/.h`, `include/.../shared_memory.h` | sparse-буфер на Switch (NVK sparse), освобождение RT при отказе | v35, v47 | — |
| Текстуры эмуляции | `vulkan/texture_cache.cpp`, `pipeline/texture/cache.h` | мелкие правки статистики | v3x | — |
| Гостевая память Switch | `src/core/guest_memory_switch.cpp/.h` | выравнивание окна/алиасов по 2 МБ, гранула подтверждения, слияние отображений, результат последней операции | diag7–diag12 | [switch-platform.md](switch-platform.md) |
| Потоки | `src/system/xthread.cpp` | `host_thread_stack_mb` (16 → 6 на Switch по умолчанию из приложения) | diag12 | handoff/11 |
| Память ядра | `src/system/xmemory.cpp` | `diag_disable_gpu_write_watch` (диагностика) | diag | — |
| Профайлер / Switch UI | `src/ui/switch_perf.cpp`, `src/ui/switch_sysclk.cpp` | выборка стеков по cvar, круг A/B режимов, `RexSwitchStickClicksHeld` (L3+R3); фиксированные частоты сессии `RexSwitchRelojesFijar` | разные, v55 | switch-platform.md |
| Счётчики и замки | `include/rex/ppc/func.h`, `function.h`, `context.h`, `include/rex/hook.h`, `src/core/mutex.cpp` | `REX_FUNC_COUNT`, счётчики экспорта ядра (`REX_EXPORT`), `rex_msr_global_lock`, FPCR: счётчик переключений и фиксированный FTZ | v39–v41 | [codegen.md](codegen.md) |
| Codegen | `src/codegen/function_scanner.cpp`, `function_graph.cpp`, `resources/templates/codegen/pch_h.inja` | 16-битные таблицы переходов; `REX_FUNC_COUNT`; области mtmsrd | v41, v46 | codegen.md |
| XAM UI | `src/kernel/xam/xam_ui.cpp` | системная клавиатура Switch (swkbd) для `XamShowKeyboardUI` | v45 | — |
| Звук | `src/audio/xma_decoder.cpp`, `xma_context.cpp`, `include/rex/audio/xma/*.h` | диагностика «немых» контекстов XMA (`audio_xma_estado`) | v40 | — |

Как обновлять этот список: `git diff --stat 9faaf22 HEAD -- sdk`; при новой правке SDK — строка сюда и в
профильный справочник.
