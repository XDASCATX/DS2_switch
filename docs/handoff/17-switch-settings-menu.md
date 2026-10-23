# 17. Меню L+R+→ («Debug Menu») и параметры сборки DS2

Проверено по исходникам ветки `diag/switch-level-load` (v32 и новее), а не по документации nfsmw-nx.

## Что открывается и как

Комбинации обрабатывает `SwitchInputDriver::DispatchMenuShortcuts` (`sdk/src/input/switch/switch_input_driver.cpp`):
L+R+направление превращается в нажатие функциональной клавиши ПК в потоке UI.

| Кнопки | Клавиша ПК | Что это |
|---|---|---|
| L + R + ↑ | F3 | debug overlay: FPS, время кадра (`debug_overlay.cpp`); L+R+правый стик двигают его |
| **L + R + →** | **F4** | **Settings — меню всех cvar** (`settings_overlay.cpp`) |
| L + R + ↓ | ` (тильда) | консоль лога; команды `find <подстрока>`, `echo`, а также `имя значение` для смены cvar |
| L + R + ← | F7 | достижения |
| L + R + ZL | — | круг A/B-тестов GPU (`switch_perf.cpp`), для измерений |

Пока зажаты L+R, D-pad и ZL до меню не доходят (`switch_imgui_input.h`). В меню: A — выбрать, B — назад, D-pad — ходить,
левый стик — прокрутка, касание экрана — клик.

## Откуда берутся пункты меню

- Каждый пункт — это cvar, объявленный макросом `REXCVAR_DEFINE_BOOL/INT32/UINT32/INT64/UINT64/DOUBLE/STRING` в SDK
  или в приложении (`rex/cvar.h`). Меню строит дерево категорий из второго аргумента («GPU», «GPU/Vulkan», «DS2»…),
  поиск — по имени и описанию.
- Значок рядом с именем — `Lifecycle`: `[live]` (по умолчанию для всех макросов), `[restart]`
  (`.lifecycle(kRequiresRestart)`), `[init-only]` (`kInitOnly`; в меню показан серым, менять нельзя).
- **`[live]` — только метка.** Смена применяется сразу лишь тогда, когда код читает `REXCVAR_GET(...)` при каждом
  использовании. Многие параметры читаются один раз при создании объекта: например `native_2x_msaa`,
  `depth_float24_round` читаются в `VulkanRenderTargetCache::Initialize` — в меню «live», а на деле действуют только
  после перезапуска.
- Скрыть пункты может только приложение (`OcultarAjustesEnMenu`); DS2 ничего не скрывает.
- Изменения из меню действуют до выхода из игры. Кнопка внизу «Save to config» вызывает `SaveConfig` и **перезаписывает
  ds2.toml** списком всех значений, отличных от значений по умолчанию (`SerializeToTOML`: комментарии пропадут, зато
  попадут и значения, которые выставило само приложение) — на консоли её лучше не нажимать, toml мы готовим сами.

Всего в сборке 269 cvar: 187 с меткой live, 33 restart, 49 init-only. Полный список: [17a-cvars-list.md](17a-cvars-list.md).

## TOML (`sdmc:/switch/ds2/ds2.toml`)

- Любой cvar задаётся по имени: `имя = значение` (`cvar.cpp: ApplyTomlTable`). **Таблицы склеивают имя через «_»**:
  `[gpu] vsync = false` превращается в `gpu_vsync`, а это уже другой (несуществующий) параметр. Поэтому в наших toml все
  ключи пишутся на верхнем уровне.
- Неизвестное имя не ошибка: в логе появляется `Config: unknown cvar '...'` (уровень warning). В логе v32 таких строк нет —
  все ключи `15-ds2.toml.v32` существуют.
- Приоритет: командная строка > переменные окружения > toml > значение по умолчанию; init-only после инициализации не
  меняется (`Cannot modify init-only flag`).

## Что есть в DS2, а чего нет (по сравнению с nfsmw-nx)

- Нет нативного рендерера nfsmw-nx (`nfsmw_nativo_*`, `nfsmw_nativo_set4_diferencias`, `nfsmw_nativo_nvk_*`),
  нет библиотеки шейдеров XenosRecomp/DXC: DS2 рисует через эмуляцию Xenos в SDK (Vulkan, путь host render targets),
  шейдеры транслируются во время игры (`async_shader_compilation`, кэш Mesa — `MESA_SHADER_CACHE_DIR` в `ds2_mesa_env`).
- Есть общие механизмы Switch-слоя: профайлер `switch_perf.cpp` (`rex_perfil.log`), `nfsmw_io_rangos_mb` (кэш чтений
  файлов), `present_hilo_*` (поток презентации), `host_present_*`, логирование (`log_*`).
- Только D3D12 (на Switch ничего не делают): вся категория `GPU/D3D12`, `UI/D3D12`, и **`occlusion_query_enable`** —
  его читает только `d3d12/command_processor.cpp`. Строка `occlusion_query_enable = false` в наших toml ни на что не
  влияет.

## Параметры, которые реально полезны для поиска узкого места

| Параметр | Где читается | Когда действует | Что даёт |
|---|---|---|---|
| `gpu_zone_timing` | command_processor (Vulkan) | init-only | GPU-время по зонам в `rex_perfil.log` (переносы, resolve, текстуры, draw игры, барьеры). Timestamp на каждый draw добавляет сериализацию: абсолютные цифры завышены, доли верны |
| `render_target_transfer_skip` | на каждом transfer-draw | live | 1 = без 8 проходов битов stencil (нет VK_EXT_shader_stencil_export в NVK); 2/4 — диагностика, ломает картинку |
| `render_target_transfer_elide_overwritten` | на каждом обновлении RT | live | пропуск переносов, которые draw целиком перезаписывает (v32) |
| `execute_unclipped_draw_vs_on_cpu` | на каждом draw (оценка высоты RT) | live | высота RT для очисток по вершинам (v32) |
| `render_target_direct_resolve` (новый, v33) | на каждом resolve | live | resolve одним проходом без дампа в буфер EDRAM |
| `rt_transfer_stats`, `rt_transfer_trace`, `render_target_direct_resolve_validate` | ПК, разработка | live | гистограммы/трасса переносов и resolve; сверка прямого resolve с обычным побайтно |
| `edram_diagnostic_mode` | приложение | init-only | режимы «без части GPU-работы» для замеров (не для игры) |
| `ds2_mesa_env` | приложение | init-only | переменные Mesa/NVK: `MESA_SHADER_CACHE_DIR`, `NVK_SWITCH_PERF_LOG`, `NOUVEAU_HORIZON_BO_CACHE_MB`… |
| `vsync` | graphics_system / command_processor | live | синхронизация с vblank гостя |
| `async_shader_compilation`, `vulkan_async_skip_incomplete_frames` | pipeline cache | live | фоновая компиляция; кадры с заглушками пропускаются |
| `texture_cache_memory_limit_*` | texture cache | restart | лимиты кэша текстур (у нас 128/256 МБ ради кучи) |
| `clear_memory_page_state` | конец кадра | live | обновление состояния страниц, записанных GPU |
| `vulkan_submit_on_primary_buffer_end` | command_processor | live | отправка на конце первичного буфера PM4 |

## Как пользоваться для замеров

Меню удобно, чтобы проверить, что реально работает (значение, источник), и для live-параметров переключать их в одной
сессии (правило A/B из `nfsmw-nx/docs/measuring.md`: чередовать в одной сессии, а не сравнивать разные запуски). Для
init-only/restart — только через toml и перезапуск.
