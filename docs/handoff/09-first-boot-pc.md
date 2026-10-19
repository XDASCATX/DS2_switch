# 09. Первый запуск на ПК: от сгенерированного C++ до меню и вступительного ролика

Шаг 1 гайда NFSMW-NX («сначала запустить игру на ПК»). Всё сделано в облачной Linux-среде без GPU: Ubuntu 24.04, 2 ядра, 7 ГБ RAM. Vulkan — программный (Mesa llvmpipe/lavapipe). Звук — ALSA-устройство `null`. Окно — SDL `offscreen`. SDK и ReXGlue не менялись.

## Результат

| Прогон | Что проверено | Итог |
|---|---|---|
| run1–run3 | host-приложение, только `Default.xex` | XEX загружен, 84 126 функций зарегистрировано, ядро/VFS подняты, главный поток игры стартовал, первое падение — незарегистрированная функция |
| run6–run7 | все файлы игры, без GPU | 90–150 с без падений; потоки движка (Dantelion2, Havok, MOMainThread, NexusRevolution…), аудиоцикл XAudio |
| run9 | GPU-плагин `xenos` + Vulkan | **отрисованы кадры игры**: значок загрузки, окно автосохранения, логотип FromSoftware, титульный экран «PRESS START BUTTON»; 240 с без падений |
| run10 | виртуальный геймпад | START принят, создано сохранение («Saving content»), стоп на окне «нужен compatibility pack» |
| run11–run12 | + DLC Compatibility Pack 4 | первые экраны настроек (Screen Options, Game Options), затем **вступительный ролик** (`prologue.wmv`, субтитры) — 480 с без падений |
| run13 | START во время ролика | ролик пропущен → экран загрузки мира (описание предмета) → **3D-сцена Междумирья** (ступени, трава, деревья, кинематографичные полосы); 600 с без падений. Кадр окрашен в сильный красный — причина не установлена |

Кадры лежат у владельца в `SW1_ds2/logs/2026-10-19/boot/`, ключевые — в `boot/key/`. В репозиторий они не попадают: это производное от игровых ресурсов.

## Host-приложение (`app/`)

- `app/CMakeLists.txt` — собственный, а не шаблон `rexglue init`: тот подключает `generated/rexglue.cmake`, который перезапускает codegen с рабочим `ds2_manifest.toml`. Папка сгенерированного кода — `DS2_GENERATED_DIR` (по умолчанию `generated/diag-dataonly`). `ds2` кладётся рядом с `librexruntime.so`; путь к imgui — как в nfsmw-nx.
- `src/ds2_app.h`:
  - `OnLoadXexImage` → `game:\Default.xex`: SDK проверяет `default.xex` на ФС хоста до VFS, а на регистрозависимой ФС (Linux, SD-карта Switch) строчное имя не находится.
  - `OnPreSetup` → виртуальный геймпад.
  - `OnPostSetup` → установка DLC.
  - `OnPostLaunchModule`/`OnShutdown` → захват кадров.
- `src/ds2_dlc.cpp` — `--ds2_install_dlc=<пакет>`: вызывает готовый, но нигде не вызываемый `ContentManager::InstallContent` (STFS → папка `user/0000000000000000/465307E4/00000002/<имя>` + `.header` с маской лицензий).
- `src/ds2_input_script.cpp` — `--ds2_input_script="100:start,115:a,…"`: синтетический геймпад, перенесён из nfsmw-nx (`nfsmw_prueba_entrada.cpp`, GPL-3.0) без их автопилота.
- `src/ds2_capture.cpp` — `--ds2_capture_dir`: раз в N мс `Presenter::CaptureGuestOutput` → PPM, в лог — доля непустых пикселей.

## Что пришлось исправить в наборе точек входа (диагностическая конфигурация)

Каждое падение воспроизводилось и закрывалось записью или исправлением инструмента; всё восстанавливается скриптами.

| Падение | Причина | Исправление |
|---|---|---|
| `Call to invalid or unregistered function 0x825D92C0` | инструмент data-only считал все таблицы переходов 4-байтными; короткие таблицы (типы 1–3) ложно закрывали соседние vtable | учитываются только `kAbsolute` (тип 0); +6 точек |
| `… unregistered function 0x82663140` | указатели на функции, которые строятся в коде (`lis`+`addi`, колбэки на стеке) | режим `--code-built`, `config/ds2_overrides.codeptr.toml` |
| `Unresolved branch 0x8300F098 → 0x8300F0B8` | 527 из 565 «указателей из кода» оказались встроенными в `.text` таблицами переходов (после `bctr`) | фильтр: адрес таблицы из trace или первое слово — адрес в `.text` |
| `Unresolved branch 0x82E6533C → 0x82E650AC` (и ещё ~40 мест) | 16 data-only адресов — по-видимому, funclet'ы C++-исключений: на них ссылаются таблицы EH, и они переходят обратно в родительскую функцию | `tools/ds2_find_split_culprits.py` находит такие записи по `REX_FATAL` в сгенерированном коде → `config/ds2_dataonly_exclude.txt` |
| 4 `REX_FATAL` у `0x82FADEA8…` (были с прогона 5) | листовой код после `blr` без PDATA приклеен к соседней функции | `"0x82FADE98" = {}` в `ds2_overrides.diag.toml` |
| `… unregistered function 0x8249EFF0` | функция стоит сразу после `bl` к невозвратной функции (`_CxxThrowException` и обёртки), а не после терминатора | `bl` к функции, после вызова которой где-то стоит нулевое выравнивание, считается терминатором (13 таких функций) |

Итог (codegen run12): CONFIG = 52 (proposed + diag) + 244 data-only + 43 code-built (с пересечением — 335), валидация пройдена, **в сгенерированном C++ 0 `REX_FATAL`**.

Совет на будущее: после каждого codegen смотреть `REX_FATAL` в сгенерированном коде (`tools/ds2_find_split_culprits.py`). Код выхода codegen 0 этого не гарантирует: эмиттер пишет `REXCODEGEN_ERROR "Unresolved conditional branch"` и вставляет `REX_FATAL`, но завершается успешно.

## Окружение для запуска (не в репозитории)

- `game_root` — копия папки `DS2` владельца (все файлы, кроме `Thumbs.db`, сверены с `audit/game_files.sha256`). Перенос `GameData*.bdt` и DLC — кусками по 190 МиБ через временный файл `SW1_ds2/_xfer/chunk.bin` (после переноса обнулён, не удалён).
- `apt install mesa-vulkan-drivers` (llvmpipe), `~/.asoundrc`: `pcm.!default { type null }`.
- `cmake --build out/app-host --target ds2 rexgpu-xenos`.
- Запуск (`out/run/run.sh`):
  `ds2 --game_data_root=<DS2> --user_data_root=<user> --video_driver=offscreen --headless=true --gpu_plugin=xenos --ds2_install_dlc=<D4B91B6B…> --ds2_input_script=… --ds2_capture_dir=…`

## Не сделано / ограничения

- 3D-сцена (run13) окрашена в сильный красный: причина не установлена (эмуляция GPU — форматы/пост-обработка — или освещение сцены). Управление персонажем не проверялось.
- Программный Vulkan медленный — производительность на ПК не оценивалась.
- Не перенесены: Calibration 1.15 (сейв-пакет CON), лицензия DLC проверялась только тем, что игра его открыла.
- Ничего не делалось для Switch: сборка NRO (devkitA64), лимиты Horizon, нативный рендерер, перевод шейдеров.
- Рабочий манифест по-прежнему без overrides; результат получен на диагностической конфигурации.
