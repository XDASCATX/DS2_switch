# 10. Сборка под Nintendo Switch: тулчейн из исходников и первый NRO

Цель шага — собрать DS2 как настоящий Switch-homebrew (NRO), как в nfsmw-nx (`docs/nfsmw/building.md`, шаг 6).
SDK и ReXGlue не менялись: в SDK уже есть Switch-бэкенд (`sdk/cmake/rexglue_switch.cmake`, `switch_compat/`,
`*_switch.cpp`), он включается тулчейн-файлом `tools/switch/cmake/switch-devkitA64.cmake`.

## Блокер и как он снят

Сборка под Switch требует devkitA64 (GCC для `aarch64-none-elf` с newlib devkitPro), libnx, switch-tools
(`elf2nro`, `nacptool`) и статический Vulkan-драйвер NVK из Mesa. Официальные пакеты devkitPro
(`pkg.devkitpro.org`, `downloads.devkitpro.org`, Docker Hub `devkitpro/devkita64`) и rustup из облачной среды
недоступны (curl → 000/403). Доступны: пул пакетов Ubuntu (`archive.ubuntu.com`), `git clone` публичных репозиториев
GitHub, crates.io через cargo, PyPI.

Поэтому всё собрано из исходников тех же версий, что в devkitA64 r30 (`devkitPro/buildscripts`, target 3) — скрипты в
`tools/switch/toolchain/` (порядок и источники — `tools/switch/toolchain/README.md`):

| Компонент | Версия | Проверка |
|---|---|---|
| binutils / GCC / newlib | 2.46 / 16.1.0 / 4.6.0.20260123, патчи devkitPro | `aarch64-none-elf-gcc (devkitA64) 16.1.0`; multilib `. large pic` |
| libnx | v4.12.0 | `libnx.a`, `libnxd.a`, `switch.specs`, `switch.ld` |
| switch-tools, general-tools | master | `elf2nro`, `nacptool`, `bin2s` … |
| zlib / zstd / expat | 1.3.1 / 1.5.7 / 2.7.1 | `portlibs/switch/lib` |
| Mesa NVK | mesa-switch `1a8c1a66d6f` + `mesa-switch-nfsmw.patch` (как в nfsmw-nx) | `libvulkan.a` 53 МБ, все объекты `ELF 64-bit ARM aarch64` |
| Rust | Ubuntu rustc 1.91 + std для `aarch64-unknown-linux-gnu`, собранная `-Zbuild-std` | NAK (`libnak_rs.a`) собран |

Сквозная проверка тулчейна: `switch-examples/graphics/printing/hello-world` → `hello-world.nro`, `_start` по адресу 0.

Две поправки, которых нет у devkitPro (у них сборка идёт поверх существующей установки):
- `limits.h` GCC, созданный на стадии 1 до установки newlib, не подключает `limits.h` newlib → нет `PATH_MAX`
  (падает libnx в `path_buf.h`). Скрипт пересоздаёт его из `limitx.h + glimits.h + limity.h`.
- expat 2.7 без источника энтропии на Horizon не собирается → `-DXML_POOR_ENTROPY` (Mesa использует его только для
  driconf XML).

## Изменения в приложении (`app/`)

- `app/CMakeLists.txt`: на Switch `rexglue_configure_target(ds2 GPU_PLUGINS xenos)` (плагин GPU линкуется статически)
  и `rexglue_switch_add_nro(ds2 …)`; `RUNTIME_OUTPUT_DIRECTORY` только для ПК.
- `app/src/ds2_app.h`:
  - `OnConfigurePaths` — переносимая раскладка: без `--game_data_root` берётся `<папка NRO>/game_root`, без
    `--user_data_root` — `<папка NRO>/user` (как `nfsmw_app.h`).
  - `OnPostInitLogging` — `gpu_plugin = xenos`, если не задан в командной строке или `ds2.toml`.
- `app/src/ds2_dlc.cpp` — относительные пути DLC берутся от папки NRO; при пустом `ds2_install_dlc` ставится всё
  из `<папка NRO>/dlc/`.

## Сборка

```sh
cmake -S app -B out/app-switch -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/tools/switch/cmake/switch-devkitA64.cmake \
  -DREXGLUE_SWITCH_NVK_SDK=<WORK>/mesa-sdk/opt/devkitpro/portlibs/switch
cmake --build out/app-switch --target ds2 -j2
```

Без LTO/PGO/порядка функций (они есть в nfsmw-nx и требуют 16 ГБ RAM и профиля с консоли).

## Раскладка на SD-карте

```
sdmc:/switch/ds2/
    ds2.nro
    ds2.toml            необязательно (cvars)
    game_root/          папка игры владельца (Default.xex, GameData*.bdt, …)
    dlc/                необязательно: пакет Compatibility Pack 4 (D4B91B6B…); без него игра стоит на титуле
    user/, logs/        создаются при запуске
```

Запуск — из Homebrew Menu в режиме title takeover (удерживать R при запуске игры) или через forwarder: в режиме
апплета (альбом) памяти не хватит (`sdk/src/ui/runtime_switch.cpp`, гостевая память одна 512 МБ).

## Результат (2026-10-20)

- `out/app-switch/ds2.nro` — 103 377 093 байт, sha256 `c0d917c972f0b1dbd503f64f03669f3dd6b859e23099b9349d42f76d9d008bd6`
  (сгенерированный код `generated/diag-dataonly`, коммит этого документа). ELF: AArch64 PIE, `_start` = 0
  (проверка из `docs/nfsmw/toolchain.md`), 83 634 функции игры, ни одного неопределённого или weak-undefined символа,
  `.text` 81 МБ; NRO: магия `NRO0`, секция ASET (иконка libnx по умолчанию + NACP).
- Сборка: SDK для Switch (532 шага) — без ошибок; 351 шаг приложения (336 файлов сгенерированного кода) — ~70 мин
  на 2 ядрах, без ошибок.
- ПК-сборка после изменений `app/` пересобрана и проверена (run14, 150 с: титульный экран, `gpu_plugin = xenos` по
  умолчанию, без фатальных ошибок).
- NRO, `ds2.toml` и инструкция переданы владельцу в `SW1_ds2/switch/` (в GitHub не попадают: это производное от
  игры).

**На консоли не проверено**: у сборочной среды нет доступа к Switch и к эмулятору. Следующий шаг — запуск владельцем по
`README_SD.txt` и разбор `logs/`, `sdmc:/switch/rex_crash.log`, отчёта Atmosphère (адреса → функции:
`aarch64-none-elf-addr2line -f -C -i -e out/app-switch/ds2`, неурезанный ELF хранится в `out/app-switch/`).

Известные риски на консоли: память (NRO 103 МБ + гостевые 512 МБ + стеки потоков по 16 МБ), время компиляции
шейдеров NVK при первом запуске, красный оттенок 3D-сцены с ПК (вероятно, общий для эмуляции GPU), отсутствие
LTO/PGO (производительность).
