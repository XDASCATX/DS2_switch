# 02. Сборка host-бинарника `rexglue`

`rexglue` — генератор кода ReXGlue: читает XEX, анализирует PowerPC-код и пишет C++. Он собирается под хост (здесь Linux x86-64) и к Switch отношения не имеет.

## Проверенная конфигурация

Ubuntu 24.04, 1 ядро, ~4 ГБ RAM. Результат: `BUILD_EXIT=0`, 857/858 шагов, бинарник `sdk/out/linux-amd64/rexglue` (≈4,4 МБ, ELF x86-64), `--help` и `--version` → 0, версия `0.10.0.0-dev.unknown`. Время сборки ~25 мин с `-j1`.

| Компонент | Версия |
|---|---|
| clang / clang++ | **20.1.2** (`clang-20`, Ubuntu archive) |
| стандартная библиотека | **libstdc++ 13.3** (не libc++) |
| CMake | 4.4.4 (через `pip`) |
| Ninja | 1.13.2 (через `pip`) |
| Стандарт | C++23 (как в SDK, не менялся) |

## Шаги восстановления

```bash
# 1. Исходники
git clone --depth 1 https://github.com/StevensND/nfsmw-nx nfsmw-nx
mkdir ds2-switch && cd ds2-switch && git init
cp -r ../nfsmw-nx/sdk ../nfsmw-nx/tools ../nfsmw-nx/LICENSE ../nfsmw-nx/THIRD_PARTY_NOTICES.md .
mkdir -p docs assets/game_root app tests
cp ../nfsmw-nx/docs/{building,porting-another-game,platform-notes,glossary}.md docs/

# 2. Зависимости
apt-get install -y clang-20 lld-20 pkg-config \
  libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev libxfixes-dev libxss-dev \
  libxkbcommon-dev libwayland-dev libasound2-dev libudev-dev libgl1-mesa-dev libxtst-dev \
  libxrender-dev libdecor-0-dev libdrm-dev libgbm-dev libpulse-dev \
  libx11-xcb-dev libxcb1-dev libxcb-xkb-dev libxcb-randr0-dev libxcb-shm0-dev \
  libxcb-util-dev libxcb-keysyms1-dev time
pip install --break-system-packages cmake ninja cryptography

# 3. Сторонние библиотеки (ПЕРЕД этим применить патч к tools/fetch_thirdparty.py, см. ниже)
python3 tools/fetch_thirdparty.py
# SDL3 нужен каталог .git с HEAD, а не пустой файл-маркер:
rm -f sdk/thirdparty/sdl3/.git && mkdir sdk/thirdparty/sdl3/.git \
  && echo 0000000000000000000000000000000000000000 > sdk/thirdparty/sdl3/.git/HEAD

# 4. Конфигурация и сборка
cmake -S sdk -B out/host -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20 \
  -DCMAKE_C_FLAGS=-march=x86-64-v3 -DCMAKE_CXX_FLAGS=-march=x86-64-v3
cmake --build out/host --target rexglue -j1
sdk/out/linux-amd64/rexglue --help
```

## Ошибки, на которые наткнулись, и их причины

Каждая была реальным падением; исправлена только установленная причина.

| # | Симптом | Причина | Исправление |
|---|---|---|---|
| 1 | CMake: `Git submodule 'cli11' is not initialized` (потом libmspack и др.) | `sdk/thirdparty/CMakeLists.txt` проверяет наличие `<submodule>/.git`; `fetch_thirdparty.py` копирует файлы без `.git` | Пустые маркеры `.git` (патч в скрипте). Дерево сверено с чистым ReXGlue v0.10.0 (коммит `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518`, с подмодулями): **18 900/18 900 файлов на месте**, отличаются 5 — это документированные изменения порта (`CMakeLists.txt`, `ffmpeg-overlay/codec_list.c`, `FFmpeg/config.h`, `FFmpeg/libavcodec/fft_template.c`, `mdct_template.c`) |
| 2 | `fetch_thirdparty.py` падает | Битый symlink (moltenvk) на не-Windows хосте | Пропускать несуществующие пути (патч) |
| 3 | SDL3: `GetGitRevisionDescription.cmake:143 … REGEX REPLACE needs at least 6 arguments` | SDL3 читает git HEAD | Каталог `sdl3/.git` с нулевым `HEAD`. Влияет только на строку ревизии SDL |
| 4 | SDL3: `Couldn't find dependency package for XCURSOR / XTEST`, `x11-xcb` | Нет dev-пакетов X11/xcb | Установить пакеты из шага 2 |
| 5 | `sdk/src/core/memory.cpp`: `_mm_shuffle_epi8` требует SSSE3 | Интринсик без target-атрибута | `-march=x86-64-v3` |
| 6 | `map_parser.h:78: no template named 'expected' in namespace 'std'` | clang 18 сообщает `__cpp_concepts = 201907`, а libstdc++ 13 включает `<expected>` только при `>= 202002` | Перейти на **clang 20** (`__cpp_concepts = 202002`); проверено пробной программой с `std::expected` и `clock_cast` |
| 7 | `chrono.h:131: explicit specialization of undeclared template struct 'clock_time_conversion'` | Возникла при попытке собрать с libc++ 18: в нём нет `std::chrono::clock_time_conversion`, а SDK подставляет свою замену только под `#ifdef __APPLE__` | Отказаться от libc++, вернуться на libstdc++ с clang 20 |

Прочее:
- В `sdk/include/rex/string/numeric.h` в какой-то момент появилась правка под libc++ (`defined(_LIBCPP_VERSION)`), происхождение которой не установлено. В рабочей конфигурации она ничего не меняла (макрос не определён) и была откачена; в коммиты не попала.
- Бинарник кладётся в `sdk/out/linux-amd64/`, а не в `out/host/` — это задано SDK.

## Патч к `tools/fetch_thirdparty.py`

Две вставки (оригинал — из NFSMW-NX):

1. В цикле копирования, сразу после строки `src = os.path.join(dirpath, name)`:
   ```python
                   if not os.path.exists(src):  # broken symlink (e.g. moltenvk) on non-Windows hosts
                       continue
   ```
2. Перед финальным `print(f'{copied} files copied …`:
   ```python
           for d in os.listdir(target):  # CMake only checks that <submodule>/.git exists
               full = os.path.join(target, d)
               if os.path.isdir(full) and not os.path.exists(os.path.join(full, '.git')):
                   open(os.path.join(full, '.git'), 'w').close()
   ```

## Возможности `rexglue` 0.10.0

- Подкоманды: `codegen`, `init`, `recompile-tests`.
- `codegen [OPTIONS] [config]`: `config` — путь к TOML-манифесту (иначе ищется в cwd); `--target NAME` (DLL-цель, можно повторять); `--ignore-stamp` (перегенерировать, даже если входы совпадают со stamp).
- Режима «только анализ / dry-run» **нет**: любой запуск пишет в `out_directory_path` и stamp.
- Глобально есть `--log-level` (включая `trace`) и `--log-file`.
