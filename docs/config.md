# Конфигурация: cvar и ds2.toml

## 1. Как это устроено **[код]**

- Каждый параметр — cvar, объявлен макросом `REXCVAR_DEFINE_BOOL/INT32/STRING/…(имя, умолчание, категория,
  описание)` в `sdk/src`, `sdk/include` или `app/src`; читается `REXCVAR_GET(имя)`.
- Рантайм читает `<папка исполняемого файла>/ds2.toml` (`sdk/src/ui/rex_app.cpp`: `exe_dir / (GetName() + ".toml")`).
  На Switch это `sdmc:/switch/ds2/ds2.toml`. На ПК параметры задаются аргументами `--имя=значение`.
- Жизненный цикл: `.lifecycle(kInitOnly)` — читается до старта (менять только в TOML), `kRequiresRestart` —
  после перезапуска; остальные формально «live», но меняется ли поведение на лету — зависит от того, читает ли
  код значение каждый раз.
- Меню настроек на консоли: **L+R+→** ([handoff/17-switch-settings-menu.md](handoff/17-switch-settings-menu.md)).
  Договорённость: тестовые переключатели — через TOML, не через меню.
- Ранние умолчания приложения: `app/src/ds2_app.h` (`gpu_plugin=xenos`, на Switch `host_thread_stack_mb=6`).
- Полный список всех ~320 cvar — [handoff/17a-cvars-list.md](handoff/17a-cvars-list.md) (генерируется:
  `python3 tools/analysis/cvars.py . /tmp/cv.json && python3 tools/analysis/cvars_table.py /tmp/cv.json`; после
  добавления cvar — перегенерировать).

## 2. Файлы TOML

- `config/toml-history/15-ds2.toml.vNN` — TOML каждой выпущенной версии (не менять задним числом). Ранние: `11-*`
  (тесты 13–26), `14-*` (режимы MODE).
- Текущая версия — **`config/toml-history/15-ds2.toml.v56`** (её же `package-release.sh` кладёт как `ds2.toml`).
- С v55 — **один TOML на версию** (просьба владельца); A/B-варианты — только по явной необходимости.
- Новая версия: скопировать TOML предыдущей → `15-ds2.toml.vNN`, исправить шапку (что изменено), добавить новые
  ключи с комментарием «vNN: …».

## 3. Текущий ds2.toml (v56) по группам

| Группа | Ключи | Смысл |
|---|---|---|
| Журнал и наблюдение | `log_level = "info"`, `ds2_watchdog`, `ds2_switch_stack_sampling`, `ds2_export_stats` | журнал, watchdog `[ds2 wd]`, стеки в профайлере, счётчики экспорта |
| Оверлей | `ds2_switch_saltynx = true` | FPS для Status Monitor / Reverse-NX через SaltyNX |
| Mesa | `ds2_mesa_env` | `MESA_SHADER_CACHE_DIR=sdmc:/switch/ds2/mesa_cache_v33`, `NOUVEAU_HORIZON_BO_CACHE_MB=16`, `NVK_SWITCH_PERF_LOG=1` |
| Память Switch | `ds2_switch_nvmap_mb = 64`, `ds2_switch_commit_granule_kb = 2048`, `ds2_watchdog_probe_tables = 0`, `nfsmw_io_rangos_mb = 0` | найдено в тестах 12–21 (handoff/11): без этого — 2001-0103 и нехватка кучи |
| Диагностика (выкл.) | `ds2_func_counts`, `ds2_editor_*`, `audio_xma_estado`, `ds2_fpcr_ftz_fixed` | эксперименты v39–v45, выключены |
| Потоки | `ds2_msr_per_thread = true` (v56) | области `mfmsr/mtmsrd` без общего мьютекса SDK |
| Частоты | `ds2_switch_cpu_mhz = 1963`, `ds2_switch_gpu_mhz = 1267`, `ds2_switch_mem_mhz = 0` | override через sysmodule частот (Horizon OC `hoc:clk` / sys-clk), снимается при чистом выходе; 0 = не трогать |
| Нативный рендерер | `ds2_native_renderer = true` и `ds2_native_*` | см. ниже |

Ключи нативного рендерера (`sdk/src/graphics/native/native_renderer.cpp`, в скобках — умолчание в коде):

| Ключ | Умолч. | В v56 | Что |
|---|---|---|---|
| `ds2_native_renderer` | false | true | нативный рендерер вместо эмуляции (перезапуск) |
| `ds2_native_log_every` | 300 | 300 | период строк статистики (кадры) |
| `ds2_native_gpu_profile` | false | true | метки времени GPU по проходам |
| `ds2_native_gpu_draw_profile_every` | 0 | 900 | один кадр из N — метки по каждому draw |
| `ds2_native_hitch_ms` | 100 | 100 | порог журнала рывков |
| `ds2_native_mips` | true | true | мипы текстур (v54) |
| `ds2_native_max_anisotropy` | 0 | 0 | потолок анизотропии |
| `ds2_native_early_z` | true | true | kEarlyHint для шейдеров без kill/глубины/альфа-покрытия (v55) |
| `ds2_native_async_pipelines` | true | true | конвейеры в отдельном потоке (v55) |
| `ds2_native_pass_barriers` | false | false | полный барьер перед каждым проходом (как до v55) — только для проверки |
| `ds2_native_occlusion_queries` | false | false | настоящие occlusion queries (v54; с ними мигали враги) |
| `ds2_native_fast_pixel_math` | true | true | FMA в пиксельных шейдерах (v56, перезапуск) |
| `ds2_native_capture_combo` | true | true | снимок кадра по L3+R3 |
| `ds2_native_texture_keep_frames` | 600 | — | освобождать текстуры без обращений N кадров |
| `ds2_native_gamma_ramp` | true | — | гамма-рампа игры при показе |
| `ds2_native_textures` | true | — | false = белая текстура везде (проверка геометрии) |
| `ds2_native_dump_dir`, `ds2_native_dump_swap` | "" / 0 | — | дамп N-го «тяжёлого» кадра (ПК/разработка) |

Эмуляционные ключи (`render_target_*`, `vulkan_*`, `readback_resolve`, `gpu_zone_timing*`…) действуют только при
`ds2_native_renderer = false`; их значения для эмуляции — в `15-ds2.toml.v54_emulation`.
