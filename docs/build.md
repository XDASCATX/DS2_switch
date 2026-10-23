# Сборка, упаковка, установка

Что здесь проверено в текущем окружении (облачный контейнер Ubuntu 24.04, 2026-10-23): **сборка ПК**
(`build-pc.sh`/`ninja`), **сборка Switch** (`build-switch.sh`), **упаковка** (`package-release.sh`),
**ПК-прогон** (`run-pc.sh`) — всё это выполнялось при выпуске v53–v56. **Codegen** и **сборка тулчейна/Mesa из
исходников** в этой сессии не повторялись — их описание взято из журналов [handoff/02](handoff/02-host-build.md),
[handoff/08](handoff/08-session-2026-10-19.md), [handoff/10](handoff/10-switch-build.md) и v46/v49 в
[handoff/16](handoff/16-switch-gpu-optimization.md).

## 1. Что нужно

| Компонент | Версия (проверено) | Откуда | Зачем |
|---|---|---|---|
| ОС | Ubuntu 24.04.5 LTS x86-64 | — | всё |
| clang / clang++ | 20.1.2 (`clang-20`, libstdc++ 13.3) | apt | хост: `rexglue`, ПК-сборка |
| CMake / Ninja | 4.4.4 / 1.13.2 | pip | все сборки |
| Python | 3.13 (+ `cryptography`, `Pillow` для разбора кадров) | apt/pip | инструменты |
| devkitA64 | GCC 16.1.0, binutils 2.46, newlib 4.6.0.20260123 (= devkitA64 r30) | **собран из исходников** `tools/switch/toolchain/` в `/opt/devkitpro` | Switch |
| libnx / switch-tools | v4.12.0 / master | из исходников | Switch |
| portlibs | zlib 1.3.1, zstd 1.5.7, expat 2.7.1 | из исходников | Switch |
| Mesa NVK | danfromtico/mesa-switch `0ff52079ca9e` + `tools/switch/toolchain/mesa-switch-ds2-perf.patch` (4 коммита) | вне репозитория: `DS2_MESA_SRC` | Vulkan на Switch |
| Rust | **ровно 1.91** (`/usr/lib/rust-1.91`; другая версия → E0514 с готовыми rlib) + bindgen | apt + сборка std | NAK (компилятор шейдеров NVK) |
| Файлы игры | Xbox 360 DS2 SotFS, TitleID 465307E4 + DLC «Compatibility Pack 4» | **у владельца**, никогда не в git | codegen, ПК-прогоны, консоль |

Официальные серверы devkitPro и rustup из облачной среды недоступны — поэтому тулчейн собран из исходников
([handoff/10](handoff/10-switch-build.md), `tools/switch/toolchain/README.md`). На машине с доступом к
`pkg.devkitpro.org` можно поставить devkitA64 штатно, **но Mesa NVK всё равно собирается из исходников** с патчем.

## 2. Пути и переменные

Все пути — в `tools/build/env.sh` (источник истины, `. tools/build/env.sh`). Значения по умолчанию указывают на
этот контейнер:

| Переменная | По умолчанию | Что |
|---|---|---|
| `DS2_ROOT` | корень репозитория | — |
| `DEVKITPRO` | `/opt/devkitpro` | тулчейн |
| `DS2_MESA_SRC` | `/home/claude/swtc/src/mesa-switch-head` | дерево Mesa (ветка с 4 патчами) |
| `DS2_MESA_SDK` | `/home/claude/swtc/mesa-sdk-head-ds2-v35` | установленный SDK Mesa, с которым линкуется NRO |
| `MESA_SWITCH_RUSTC` | `/usr/lib/rust-1.91/bin/rustc` | только 1.91 |
| `GAME_ROOT` | `out/run/game_root` | копия папки игры для ПК |
| `DS2_DLC` | `out/run/dlc/D4B91B6B…` | пакет DLC для ПК-прогонов |

В новом окружении переопределите переменные перед `. tools/build/env.sh` (например `DS2_MESA_SDK=/x/mesa-sdk`).

## 3. Порядок с нуля

```sh
# 0. Сторонние исходники SDK (в git только изменённые файлы sdk/thirdparty)
python3 tools/fetch_thirdparty.py                 # ReXGlue v0.10.0 c94f5eb с подмодулями; нюансы — handoff/02
# 1. Хост-инструмент rexglue (codegen), ~25 мин на -j1
cmake -S sdk -B out/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-20 \
  -DCMAKE_CXX_COMPILER=clang++-20 -DCMAKE_C_FLAGS=-march=x86-64-v3 -DCMAKE_CXX_FLAGS=-march=x86-64-v3
cmake --build out/host --target rexglue -j1      # -> sdk/out/linux-amd64/rexglue
# 2. Файлы игры: папка игры владельца -> assets/game_root (Default.xex) и out/run/game_root (ПК)
# 3. Codegen (часы; пишет app/generated/diag-dataonly, не в git)
MANIFEST=app/ds2_manifest.diag-dataonly.toml tools/run_codegen_diag.sh <tag>   # лог out/logs/diag/<tag>.log
# 4. Тулчейн Switch и Mesa (один раз; см. tools/switch/toolchain/README.md)
WORK=/path tools/switch/toolchain/fetch-sources.sh && WORK=/path tools/switch/toolchain/build-devkita64.sh
WORK=/path tools/switch/toolchain/build-libnx-tools.sh && WORK=/path tools/switch/toolchain/build-portlibs.sh
#    Mesa: дерево mesa-switch 0ff52079ca9e + git am tools/switch/toolchain/mesa-switch-ds2-perf.patch,
#    meson-сборка builddir-switch (кросс-файл tools/switch/toolchain/mesa_cross_switch_linux.ini), затем
tools/build/build-mesa.sh <имя-sdk>              # устанавливает SDK; затем DS2_MESA_SDK=<путь>
```

Шаги 3–4 в этой сессии не повторялись. Codegen обязательно с манифестом **diag-dataonly** — его ждёт
`app/CMakeLists.txt` (`DS2_GENERATED_DIR`); `tools/build/patch-generated.py` дописывает в старый
сгенерированный код `REX_FUNC_COUNT` (чтобы не перегенерировать).

## 4. Повседневная сборка (проверено)

```sh
tools/build/build-switch.sh      # out/app-switch/ds2.nro (+ ELF без strip: out/app-switch/ds2), ~5–15 мин при правке SDK
tools/build/build-pc.sh          # sdk/out/linux-amd64/ds2 + librexgpu-xenos.so
nice ninja -C out/app-host rexgpu-xenos   # быстрее, если менялся только графический плагин (ПК)
```

- `-j2` по умолчанию (`JOBS`): больше — `cc1plus` падает от нехватки памяти (7 ГБ контейнер).
- Полная перекомпиляция сгенерированного кода под Switch — ~2 ч (`ds2_register.cpp` один ~45 мин).
- Быстрая проверка, что файл компилируется, без всей сборки (~15 с): `tools/build/compile-check.sh
  sdk/src/graphics/native/native_renderer.cpp` (команда из ninja-графа ПК-сборки, объект в /tmp; проверено).
- Лог сборки версии сохраняют: `out/app-switch-build-vNN.log`.

## 5. Упаковка и выдача на консоль

```sh
tools/build/package-release.sh NN
#   out/app-switch/releases/vNN/{ds2.nro, ds2.elf, ds2.toml, SHA256SUMS}  (уже существующую версию не перезапишет)
#   /mnt/user-data/outputs/vNN/part_00.. (по 14 МБ) + ds2.toml — для передачи на Mac владельца
```

TOML берётся из `config/toml-history/15-ds2.toml.vNN` (его создают до упаковки). ELF версии хранится для
символизации стеков с консоли — не удалять.

Передача на Mac (в этой среде — через «мост» к компьютеру владельца): части → `SW1_ds2/_xferNN/`, склейка
`cat part_* > switch/vNN/ds2.nro`, проверка `shasum -a 256` против `SHA256SUMS`, части — в `_to_delete/`
(удалять на Mac этой сессии нельзя). Владелец копирует `ds2.nro` и `ds2.toml` в `sdmc:/switch/ds2/`.

## 6. Раскладка SD-карты (консоль)

```
sdmc:/switch/ds2/ds2.nro, ds2.toml
sdmc:/switch/ds2/game_root/        файлы игры (Default.xex, GameData*.bdt/bhd, …)
sdmc:/switch/ds2/user/             сохранения и установленный DLC (создаёт рантайм)
sdmc:/switch/ds2/logs/             ds2_NNN.log, rex/rex_perfil.log, rex/rex_stderr.log
sdmc:/switch/ds2/mesa_cache_v33/   дисковый кэш шейдеров Mesa (MESA_SHADER_CACHE_DIR из ds2_mesa_env)
sdmc:/switch/ds2/native_dump/      снимки кадра по L3+R3 (v55+)
```
Запуск — из hbmenu поверх игры (режим приложения, полная память). Подробности ранних версий — handoff/10, 11.

## 7. Типичные ошибки

| Симптом | Причина / что делать |
|---|---|
| `cc1plus` killed | мало памяти: `JOBS=1` или `-j2` |
| E0514 при сборке Mesa | не та версия rustc: нужен 1.91 |
| `libvulkan.a` ~2 МБ | установился не тот артефакт: см. `build-mesa.sh` (должно быть ~48 МБ) |
| CMake: `No generated code in ...diag-dataonly` | не было codegen или другой каталог; см. шаг 3 |
| ПК-прогон убит (OOM) | остались `/dev/shm/xenia_memory_*` от прошлых запусков — удалить, когда ds2 не запущен |
| `pkill -f ds2` убивает свой shell | убивать по `pgrep -x ds2` |
| Игра на ПК не прошла титул | скрипт ввода по секундам: при медленной загрузке (параллельная сборка) `60:start` приходит раньше титула |

## 8. Публичный тестовый выпуск 0.56 (из v56)

Только для подготовки релиза — **не пересобирает код игры и не меняет `switch/v56/`**. На Mac владельца есть
`switch/v56/ds2.nro` (sha256 `fca3c3be…671f`) и исходный TOML `config/toml-history/15-ds2.toml.v56`.
`prepare-public-nro.py` заменяет *только* ASET-иконку и NACP-метаданные (автор `xDASCATx`, версия `0.56`),
проверяет побайтное совпадение исполняемой секции; публичный NRO имеет новый sha256. Иконка — квадратный JPEG
256×256 из официальной обложки DS2 SotFS; исходник, иконку и бинарники не коммитить.

```sh
python3 -I tools/build/prepare-public-nro.py switch/v56/ds2.nro /path/to/icon-256.jpg \
  out/public-0.56/release/ds2.nro
cp -n config/toml-history/15-ds2.toml.v56 out/public-0.56/release/ds2.toml
python3 -I tools/build/make-forwarder.py out/public-0.56/release/ds2.nro \
  out/public-0.56/release/ds2-0.56-forwarder.nsp \
  --keys /private/path/prod.keys --packer /path/to/hacbrewpack \
  --loader-dir /path/to/NTON/nton/assets/exefs
```

NSP — отдельный shortcut, **не содержит NRO/ключи/файлы игры**, Title ID `0100D52000560000`, путь на SD
`sdmc:/switch/ds2/ds2.nro`. Загрузчик закреплён на NTON `c5d543a91b7c` (main/npdm проверяются по SHA-256),
упаковщик hacBrewPack `745b16ecfc9c`; исходники скачиваются каждый в отдельный каталог вне git. `prod.keys`
читаются только локально упаковщиком, никогда не передавать в git или GitHub Releases. На Mac 2026-10-23:
публичный NRO sha256 `67027bfb…f14745f`, TOML `640795b1…463e09`, NSP `c5887c1c…7ca2a0b` (277 720 байт,
PFS0 с 3 NCA, plaintext-ключи не найдены). Входной NACP содержит 0.56/xDASCATx, а исполняемая секция NRO
побайтно совпадает с v56. Зашифрованное содержимое NCA независимо не декодировали; реальный запуск NSP и
скорость 0.56 возможны только на консоли владельца. PC-проверка кадра в игре была выполнена для v56.

Текст публичного выпуска — [../RELEASE_0.56.md](../RELEASE_0.56.md) (только английский), README `main` — тоже
только английский. Не выдавать NSP за проверенный, пока владелец не установит его на консоли.
