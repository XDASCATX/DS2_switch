# RC-ver062 — перенос контекста и возобновление

Дата передачи: 2026-10-25. Этот файл — подробный handoff по прямой просьбе владельца; краткая RC-запись остаётся в `docs/RC-ver062.md`.

## 1. Состояние на паузе

- Владелец разрешил объединить crash-fix, CPU/GPU-оптимизации и ограниченный native vertex path в RC62: «объедини оба этапа и стартуй на RC62». Затем дал команду «пауза» и попросил сохранить контекст для новой сессии после смены API-токена.
- Рабочая копия: `/Users/cat/Desktop/SW1_ds2`, ветка `diag/switch-level-load`. Изменения не закоммичены; много правок и untracked-файлов относятся ещё к RC57–61. Не делать reset/clean/stash и не считать весь diff исключительно RC62.
- **Готового RC62 NRO/ELF, финального pack/index и forwarder нет.** В `switch/rc-ver062/` пока только предварительный `ds2.toml`. Ничего не опубликовано.
- Codegen RC62 завершён: 679 файлов, отдельный каталог `app/generated/rc62`; повторять его не нужно.
- Сборка generated C++ прервана на `ds2_register.cpp`. Первый запуск получил OOM (`cc1plus: Killed`), повторный шёл `-j1`.
- Важно: TaskStop остановил локальный `limactl`, но компилятор остался в VM. При подготовке этого handoff отдельно завершены оставшиеся `cmake` PID419195 и `cc1plus` PID419221; повторная проверка не показала `cc1plus`, `ninja -j 1 ds2_recomp` или RC62 cmake build. Сборка сейчас действительно остановлена. VM оставлена включённой.
- На последней проверке VM: RAM 3,8 GiB, swap нет; после остановки доступно 3,4 GiB. **Только один компилятор одновременно**, включая host builder и standalone cross-compile.

## 2. Неизменяемые требования

- Русский язык, краткие ответы; измерения отдельно от гипотез.
- Цель владельца: стабильные **60 FPS без потери корректной картинки**, 30 FPS — промежуточный рубеж. Никакого обещания FPS по одной компиляции.
- `Default.xex` не менять. Игру, generated C++, NRO/ELF, дампы не коммитить. Старые RC-сборки не перезаписывать.
- Не выполнять игровой ПК-прогон. Корректность/FPS/стабильность проверяет владелец на Switch.
- Не публиковать release и не делать push/commit без соответствующего решения владельца.
- Один краткий RC-файл и один TOML с независимыми rollback-переключателями. Подробный текущий файл — отдельно запрошенный контекст, не замена RC-записки.
- Не печатать, не копировать и не коммитить содержимое API-токенов и `ds2_logs/prod.keys`. Ключи packer получает только локально по пути.
- Старую паузу RC57 до 2026-10-28 не трактовать как запрет уже явно разрешённой разработки RC62. Текущая пауза снимается новой командой владельца продолжить.

## 3. Подтверждённые данные RC61

Владелец на Switch подтвердил: световая ошибка в бухте и нижняя полупрозрачная полоса ушли; субъективно несколько FPS прибавилось. Стабильность не подтверждена: три игровых вылета.

Отчёты `01792889243_03db12780bd84000.log`, `01792889993_0100d52000560000.log`, `01792890348_0100d52000560000.log` имеют одинаковые 22 return offsets. Причина: fatal indirect guest call `0x82CB81D8`, LR `0x82C97598`, затем abort/User Break `0xCB59 (2345-0101)`. Нет доказательств DEVICE_LOST. Это не отдельная ошибка forwarder.

Точный thunk 0x82CB81D8, 24 байта: `lwz r3,0(r4); mr r6,r7; lwz r11,0(r3); lwz r10,40(r11); mtctr r10; bctr`. Соседние 0x82CB81C0/0x82CB81F0 используют другие слоты. Нельзя alias на соседа, stub или подавление fatal.

Чистое окно RC61: CPU frame 40,61 ms, CPU draw 19,21 ms, descriptors 4,61 ms; GPU 31,23 ms, material 14,55 ms, prepass 6,29 ms. CPU и GPU частично перекрываются — не складывать. Частоты менялись от CPU1020/GPU307,2 до CPU2193/GPU1267,2; прямое сравнение RC без выравнивания условий некорректно. Capture: 1243 draws (608 material, 443 prepass), запись длилась 55,8 s; её commands163,66/draw191,97 ms нельзя считать нормальным frame time.

RC61 library: индекс открылся за 104,54 ms, но 0 hits/256 misses. Смешанный pack/index — лишь прежняя гипотеза, а не доказанная причина.

## 4. Что изменено для RC62

### Crash и metadata

- `config/ds2_overrides.gaps.toml`: отдельная entry `"0x82CB81D8" = {}`.
- `out/rc62-codegen.toml`: отдельный output `app/generated/rc62`, исходный XEX прежний.
- Проверены generated `sub_82CB81D8` с vtable+40 и его регистрация `{0x82CB81D8, sub_82CB81D8}`.
- `app/CMakeLists.txt`: metadata `0.62 RC62`, название `Dark Souls II: Scholar of the First Sin`, автор `xDASCATx`, `app/resources/icon.jpg`. TitleID forwarder остаётся `0100d52000560000`, target `sdmc:/switch/ds2/ds2.nro`.

### CPU

- Новый `sdk/include/rex/graphics/native/upload_content_cache.h`, тест `tests/test_upload_content_cache.cpp` ранее прошёл.
- Frame-local immutable upload cache: hash + полный memcmp, tag/size, проверка used range и alignment, reset при переиспользовании frame slot после fence. Повторное использование constants/indices, scratch vectors для descriptor writes/key. CVar `ds2_native_cpu_cache`, счётчики hits/saved MB.
- Проверить финальный diff и descriptor pointers после роста scratch vectors. `put_float` использует `array<float,256*4>`; `Shader::ConstantRegisterMap` содержит bitmap `256/64`, цикл копирует четыре блока по64 — текущая структура рассчитана на256 registers на stage. Не увеличивать/менять вслепую; проверить поведение dynamic addressing и финальную сборку.

### GPU / native inputs — эксперимент, НЕ подтверждён Switch

- `native_vertex_layout.h`: консервативный layout по анализу и SDK disassembly. Только unconditional vfetch-prefix из r0.x, без loops/calls/jumps/memexport, с границами числа bindings/words/stride/offset. После первоначального нулевого coverage helper изменён: разрешены cnop/serialize, fetch constant берётся из каждого fetch operand, а не только binding.
- Native renderer: VkVertexInput raw R32_UINT, существующий endian/format unpack остаётся в shader. Ограничены primitive/index/base/min/max и upload bounds; pipeline key содержит layout; bind vertex buffers; неподдерживаемые состояния и translation/pipeline failure возвращаются на legacy. Upload buffer имеет VERTEX_BUFFER usage. Лог `[native] vertex draws/frame` различает hardware/legacy.
- Это НЕ полная предварительная конверсия vertex данных в float. Runtime coverage и выигрыш неизвестны.
- Исходные shader templates могут содержать stride0; игра патчит vfetch. Zero eligible в offline sources не доказывает нулевой runtime coverage. `--native-test` симулирует stride только в тестовом input; эти патченные microcode нельзя включать в игровой pack.
- Новая `Modification::kVersion = 13`, bits `vertex.native_vertex_inputs` и `pixel.alpha_test_always`; translator fetch получает raw words через SPIR-V inputs.
- Alpha specialization убирает только заведомо Always/disabled guest alpha test, не shader kill и не alpha-to-coverage.
- Профиль: раздельные budgets prepass64/material96/post32, всего192 групп. Проверить query bounds и смысл логов: sparse samples, не frame average.
- **Native path требует ревью и тестов до включения.** Предварительный TOML сейчас включает все три новые CVar в true. Если semantic/SPIR-V validation не пройдены — выключить `ds2_native_vertex_inputs` перед передачей, честно указать это. Наличие кода не доказывает работоспособности.

### Library

- `spirv_library.{h,cpp}`: `Open(path, expected_signature=0)`, проверка pack ABI/signature до lookup, проверка signature индексных записей, `last_failure()` с miss reasons. Проверить reset failure state при Open errors; runtime fallback обязателен.
- `sdk/src/graphics/CMakeLists.txt`: signature зависит в том числе от `native_vertex_layout.h`. После ЛЮБОГО изменения ABI/translator/helper заново configure/build host и Switch, пересоздать pack/index. Сигнатуры должны совпасть.
- Host builder `tools/shaders/build_library.cpp`: alpha/native variants, selftest/index/native-test.
- Старый raw `/home/cat.guest/ds2-toolchain/rc62-shaders-raw.spvlib`: 35165 translated/roundtrip verified, 1368 unique, signature `63EA348D361CCC11`, eligible_sources=0. **Он устарел относительно последнего helper; это НЕ финальный RC62 pack, его не выдавать.**
- RC60/61: 33867 externally validated variants, 40 исключённых OpPhi failures, все1368 source shaders представлены. Нужен `spirv-val --target-env vulkan1.2 --scalar-block-layout`; без scalar-block-layout возникает ложное массовое отклонение UBO.
- Итоговое имя RC62: `ds2_shaders_rc62.spvlib` и `ds2_shaders_rc62.spvlib.idx`, совпадает с TOML; не путать с прежним `ds2_shaders.spvlib`.

## 5. Среда — уже установлена, переустановка не нужна

Host macOS, shell zsh. Linux ARM64 VM Lima: **ds2-build**. Репозиторий доступен в VM по тому же абсолютному пути `/Users/cat/Desktop/SW1_ds2`.

- devkitPro `/opt/devkitpro`, GCC `/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++` (16.1.0).
- CMake/Ninja, libnx, portlibs уже использованы текущим build.
- Правильный NVK SDK: `/home/cat.guest/ds2-toolchain/mesa-sdk-ds2-rc58/opt/devkitpro/portlibs/switch` (Mesa26.2.4 DS2 patched). Не подменять RC57 Mesa.
- Switch build `/home/cat.guest/ds2-toolchain/rc62-switch-build`, CMake source `.../SW1_ds2/app`, Release, generated `.../app/generated/rc62`.
- Host SDK build `/home/cat.guest/ds2-toolchain/host-build`, source `.../SW1_ds2/sdk`, Release.
- Host binaries `/Users/cat/Desktop/SW1_ds2/sdk/out/linux-arm64/rexglue` и `rex-shader-library` запускаются **в Linux VM**, не macOS.
- Shader inputs уже извлечены: `/home/cat.guest/ds2-toolchain/rc60-shader-inputs-v2/manifest.txt`. Не надо повторно извлекать игру.
- `spirv-val` в VM: `/usr/bin/spirv-val`.
- **Не запускать общий `tools/build/build-switch.sh` для RC62**: он рассчитан на другие `out/app-switch` и generated/diag-dataonly. `tools/build/env.sh` содержит старые `/home/claude/...` defaults. Для RC62 использовать команды ниже с явными путями и сохранённым cache.

## 6. Возобновление в новой сессии

Смена API-токена не требует пересборки среды, смены ветки или удаления build directory. Токен задавать привычным защищённым способом вне репозитория/переписки. Этот файл не содержит токенов. Открыть новую сессию в том же каталоге, передать ей этот файл и явно снять паузу (готовый текст в конце).

### Проверка и запуск VM — на macOS

```sh
cd /Users/cat/Desktop/SW1_ds2
limactl list
# Только если ds2-build остановлена:
limactl start ds2-build
# Войти в VM:
limactl shell ds2-build
```

`limactl start` не запускать второй раз, если VM уже Running. Дальнейшие блоки выполняются ВНУТРИ VM:

```sh
export ROOT=/Users/cat/Desktop/SW1_ds2
export TC=/home/cat.guest/ds2-toolchain
export DEVKITPRO=/opt/devkitpro
export PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:$PATH
cd "$ROOT"
git branch --show-current
# Проверить отсутствие старой сборки, прежде чем запускать новую:
ps -eo pid,ppid,args | grep -E 'cc1plus|cmake --build|ninja' | grep -v grep
free -h
```

Если обнаружен живой build, не запускать конкурирующий. Остановка локального limactl не гарантирует остановки VM children; проверять процессы в самой VM. Не killall: затрагивать только подтверждённую RC62 process tree.

### Продолжить generated C++ без очистки

```sh
cmake --build "$TC/rc62-switch-build" --target ds2_recomp -j1
```

Это инкрементальная сборка: законченные object files сохраняются. Большой `ds2_register.cpp` может долго компилироваться. При OOM зафиксировать ошибку, проверить RAM/параллельные процессы; не выдавать старый NRO и не запускать многопоточную повторную сборку.

### Host tests и новый pack — ПОСЛЕ завершения предыдущего compiler

Сначала закончить ревью translator/native/helper, затем обновить host configure и builder:

```sh
cmake -S "$ROOT/sdk" -B "$TC/host-build"
cmake --build "$TC/host-build" --target rex-shader-library -j1
export BUILDER="$ROOT/sdk/out/linux-arm64/rex-shader-library"
# Новое имя для каждого возобновления, без перезаписи прошлых проверок:
export RUN="$TC/rc62-resume-$(date +%Y%m%d-%H%M%S)"
mkdir "$RUN"
"$BUILDER" --selftest "$RUN/selftest"
"$BUILDER" --native-test "$TC/rc60-shader-inputs-v2/manifest.txt" "$RUN/native-test"
```

Нужны успешные selftest и native-test. Проверить ВСЕ полученные `.spv`:

```sh
find "$RUN/native-test" -name '*.spv' -print0 | 
  xargs -0 -r -n1 spirv-val --target-env vulkan1.2 --scalar-block-layout
```

Количество успешно переведённых тестовых templates — не actual runtime coverage. Нулевой результат или ошибки не игнорировать.

```sh
"$BUILDER" "$TC/rc60-shader-inputs-v2/manifest.txt" "$RUN/raw.spvlib"
python3 -I "$ROOT/out/rc62-resume-support/filter_spvlib.py" \
  "$RUN/raw.spvlib" "$RUN/ds2_shaders_rc62.spvlib" "$RUN/validation.json"
python3 -I "$ROOT/out/rc62-resume-support/validate_spvlib.py" "$RUN/ds2_shaders_rc62.spvlib"
"$BUILDER" --index "$RUN/ds2_shaders_rc62.spvlib" "$RUN/ds2_shaders_rc62.spvlib.idx"
"$BUILDER" --inspect "$RUN/ds2_shaders_rc62.spvlib"
```

Перед фильтрацией нужен успешный builder exit. После неё проверить отчёт: `excluded_only_unique=0`, причины rejected известны; не выкидывать silently весь shader или все новые native/alpha варианты. Runtime неизвестного варианта всегда translator fallback, никакого пропуска draw. Сохранить signature из builder и проверить её соответствие Switch. Script copies в `out/rc62-resume-support/` сохранены из временной сессии, каталог игнорируется git, но переживёт смену токена. Не запускать старые временные `rc62_*.py` edit-scripts повторно: изменения уже внесены, они не являются build steps.

### Финальная Switch сборка — после стабилизации исходников

Существующий cache содержит правильный NVK SDK. Явно закрепить настройки:

```sh
cmake -S "$ROOT/app" -B "$TC/rc62-switch-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/tools/switch/cmake/switch-devkitA64.cmake" \
  -DDS2_GENERATED_DIR="$ROOT/app/generated/rc62" \
  -DDS2_DISPLAY_VERSION='0.62 RC62' \
  -DREXGLUE_SWITCH_NVK_SDK="$TC/mesa-sdk-ds2-rc58/opt/devkitpro/portlibs/switch"
cmake --build "$TC/rc62-switch-build" --target ds2 -j1
ls -lh "$TC/rc62-switch-build/ds2" "$TC/rc62-switch-build/ds2.nro"
```

Не выполнять этот блок параллельно host compiler. Это сборка, не игровой ПК-прогон. Успех команды ещё не подтверждает FPS или изображение.

## 7. Перед выдачей владельцу

1. Полное ревью native fetch semantics, register/predicate/control-flow guards, endian indices, index units, vertex bounds, SPIR-V locations и pipeline key. Проверить legacy fallback и descriptor scratch lifetime. Если есть сомнения — выключить native inputs в итоговом TOML.
2. Пройти финальные tests/compile; проверить ELF64 AArch64/entry0, NRO0, embedded NACP title/author/version0.62RC62/JPEG, pack/header/index/signature. Предварительная standalone renderer cross-compile ранее прошла, но это не проверка последних правок и не полный link.
3. В `switch/rc-ver062/` копировать только реально новые проверенные NRO, TOML, pack и idx, не перезаписывать файлы прошлых RC и уже существующие outputs без осмотра. Итоговые имена: `ds2.nro`, `ds2.toml`, `ds2_shaders_rc62.spvlib`, `ds2_shaders_rc62.spvlib.idx`, `ds2-forwarder.nsp`, `SHA256SUMS`.
4. Forwarder — после финального NRO через `tools/build/make-forwarder.py`; CLI: `nro output --keys <local-path> --packer <local-executable> --loader-dir <pinned-NTON-exefs>`. Нужные локальные packer/loader пути заново найти, не угадывать. Не читать и не выводить prod.keys. Не переиспользовать RC61 NSP с ложным новым номером.
5. Обновить краткий `docs/RC-ver062.md` точными результатами, составом/хэшами, rollback keys и статусом «Switch-тест ожидается». Текущие provisional true в TOML не равны утверждённому набору.
6. Владелец проверяет: холодный/повторный запуск и library hits/miss reasons, бой с прежними вылетами, свет/полосу в бухте/костёр, обычные кадры после прогрева при фиксированных частотах, hardware-input vs legacy counters. Capture времени не считать обычным FPS. После отзыва — выводы о стабильности и скорости.

Независимые новые откаты с перезапуском:

```toml
ds2_native_cpu_cache = false
ds2_native_vertex_inputs = false
ds2_native_alpha_specialization = false
```

Сохранять рабочие RC61 resolve fixes (`ds2_native_resolve_logical_size`, `ds2_native_resolve_color_convert`). Не возвращать RC57 render-area/fast-math эксперименты. `RGBA16F x4` в логах не путать с MSAA×4. D24FS8 не менять без отдельного доказанного анализа.

## 8. Сообщение для новой сессии

> Прочитай CLAUDE.md, docs/handoff/RC-ver062-context.md и docs/RC-ver062.md. Снимаю паузу, продолжай разрешённую разработку и сборку RC62 с текущей рабочей копии на diag/switch-level-load. Сначала проверь отсутствие оставшихся компиляторов внутри Lima ds2-build. Не переустанавливай среду, не повторяй codegen и не запускай старый общий build-switch.sh. Одна компиляция за раз, -j1. Проверь экспериментальный native vertex path, пересобери и валидируй shader pack/index из финальных исходников, затем новый NRO/forwarder в отдельном rc-ver062. Без игрового ПК-прогона и публикации. Цель 60 FPS без ухудшения картинки; никаких обещаний до моего Switch-теста.
