# Справочник: инструменты (`tools/`)

Все скрипты читают только то, что им передано; файлы игры и их производные пишутся в `out/` (вне git).

## Сборка и выдача — `tools/build/`, `tools/run/`

| Файл | Что | Проверено в этой среде |
|---|---|---|
| `build/env.sh` | все пути и переменные (источник истины) | да |
| `build/build-switch.sh` | конфигурация (первый раз) и сборка `out/app-switch/ds2.nro` | да (v53–v56) |
| `build/build-pc.sh` | ПК: `ds2` + `librexgpu-xenos.so` | да |
| `build/compile-check.sh <файл>` | компиляция одного файла ПК-сборки (~15 с), объект в /tmp | да |
| `build/package-release.sh NN` | `releases/vNN` + части по 14 МБ для Mac | да |
| `build/prepare-public-nro.py` | публикация 0.56: заменить только NRO-иконку/NACP (имя, автор, версия), сверить неизменность исполняемой секции | Mac: v56 проверен |
| `build/make-forwarder.py` | NSP-shortcut → `sdmc:/switch/ds2/ds2.nro` из локальных `prod.keys`, hacBrewPack и закреплённого NTON-загрузчика; ключи в пакет не копируются | Mac: NSP собран/структура проверена; консоль ожидает теста |
| `build/build-mesa.sh <имя>` | пересборка и установка SDK Mesa | в журналах (v34–v35) |
| `build/patch-generated.py <dir>` | `REX_FUNC_COUNT` в старом сгенерированном коде (вызывают build-скрипты) | да |
| `run/run-pc.sh <tag> <сек> [--cvar=…]` | ПК-прогон без экрана, логи `out/run/logs/<tag>.*` | да |
| `run/pc-args-example.txt` | типовые аргументы эмуляционного пути | — |

## Тулчейн Switch — `tools/switch/`

`toolchain/` (fetch-sources, build-devkita64, build-libnx-tools, build-portlibs, build-mesa-nvk; кросс-файлы
meson; патчи Mesa), `cmake/switch-devkitA64.cmake` (тулчейн-файл CMake), `gm-sim/` (симулятор гостевой памяти
Switch против модели адресного пространства Horizon), `hbl-ds2/` (hbloader с системным ресурсом; не используется).

## Codegen и анализ XEX — `tools/*.py`, `tools/*.sh`

| Файл | Что |
|---|---|
| `fetch_thirdparty.py` | сторонние исходники SDK в `sdk/thirdparty` (из nfsmw-nx; нюансы — handoff/02) |
| `fase1_extraer.py` | извлечение ISO Xbox 360 (из nfsmw-nx; для DS2 файлы дал владелец папкой) |
| `run_codegen_diag.sh <tag>` | прогон codegen с журналом и хешами входов (`MANIFEST=…`) |
| `run_codegen_monitored.sh` | то же с замером времени/памяти |
| `ds2_xexinfo.py` | проверка заголовка XEX (TitleID, вход, шифрование, импорты); тест `tests/test_xexinfo.py` |
| `ds2_imagedump.py`, `ds2_make_elf.py` | расшифровка образа XEX → `out/ds2_image.bin` → ELF для `llvm-objdump` |
| `ds2_analyze_unresolved.py`, `ds2_classify_unresolved.py` | разбор `UnresolvedCall` (второй **не запускался** — см. шапку) |
| `ds2_trace_match.py`, `ds2_trace_nodes.py`, `ds2_trace_cause.py` | разбор trace-журнала codegen (Scan/GapFill) |
| `ds2_find_dataonly_entries.py` | точки входа из данных/кода → `config/ds2_overrides.dataonly.toml` и `config/ds2_overrides.codeptr.toml` |
| `ds2_find_split_culprits.py` | какие объявления разрезали настоящие функции |
| `huecos.py` (+ `huecos_excluir.txt`, `huecos_functions.toml`) | дыры кода между функциями → `config/ds2_overrides.gaps.toml` |
| `calientes.py` | горячие функции по rex_perfil.log (из nfsmw-nx) |
| `ds2/gen_direct_resolve_shaders.sh` | компиляция шейдера прямого resolve (эмуляция) |
| `compile_check/` | компиляция только сгенерированного кода (проверка без приложения) |

## Анализ логов и данных — `tools/analysis/`

| Файл | Что |
|---|---|
| `threads.py` | горячие функции/стеки потока по блокам rex_perfil.log, с addr2line (v56) |
| `perf.py` | сводка rex_perfil.log эмуляционного пути (зоны GPU) |
| `gaps.py` | паузы между строками журнала |
| `ds2_func_counts.py` | декодер `ds2_llamadas.bin`, сравнение окон |
| `ds2_resolve_dump.py` | дампы resolve (эмуляция) → PNG |
| `check_u16_jump_tables.py` | проверка 16-битных таблиц переходов в сгенерированном коде |
| `cvars.py`, `cvars_table.py` | список cvar → JSON → `docs/handoff/17a-cvars-list.md` |
| `bnd4.py`, `hdr2spv.py` | распаковка BND4 (шейдеры игры), SPIR-V из заголовка |

## Исследование шейдеров — `tools/shaders/`

Драйвер XenosRecomp и DXC-скрипт ([handoff/18-native-renderer-study](../handoff/18-native-renderer-study.md)); не часть сборки.

## Наследие nfsmw-nx — `tools/nfsmw-legacy/`

Перенесено 2026-10-23 без изменений: `build.ps1`, `codegen.sh`, `copia_literal.py`, `llamadas_directas.py`,
`comprobar_clave_textura.py`, `lee_antes.py`, `editions/`, `french-dub/`, их `README.md`. Они относятся к NFSMW
(манифест `nfsmw_manifest.toml`, издания NFSMW, французская озвучка) и в DS2-сборке не используются; ссылки внутри
их README ведут на структуру репозитория nfsmw-nx. Оставлены как референс (идеи: прямые вызовы для LTO, PGO).
