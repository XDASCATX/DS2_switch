# 07. Следующие шаги

## 0. Где продолжать

Облачная песочница слабая: 1 ядро, 4 ГБ RAM, ~10 ГБ диска. Codegen идёт ~14 мин на прогон, devkitPro нет. Для codegen она годится. Для сборки сгенерированного C++ (сотни МБ кода; у NFSMW LTO-сборка требует ~16 ГБ RAM) и для сборки NRO под Switch нужна своя машина: x86-64 Linux/WSL, 8+ ядер, 16–32 ГБ RAM, devkitA64.

## 1. Восстановить рабочую папку

1. Собрать `rexglue` по `02-host-build.md`.
2. Положить файлы из этого пакета:
   - `tools/*.py`, `tools/run_codegen_monitored.sh` → `ds2-switch/tools/`
   - `tests/test_xexinfo.py` → `ds2-switch/tests/`
   - `config/ds2_manifest.toml` → `ds2-switch/app/ds2_manifest.toml`
   - `config/gitignore.txt` → `ds2-switch/.gitignore`
3. Скопировать свой `Default.xex` в `ds2-switch/assets/game_root/Default.xex` и сверить SHA-256 (`7d36c5f1…2d745`).
4. Проверки:
   ```bash
   python3 -m unittest discover -s tests            # 5 тестов, OK
   python3 tools/ds2_xexinfo.py assets/game_root/Default.xex
   python3 tools/ds2_imagedump.py assets/game_root/Default.xex -o out/ds2_image.bin --info
   python3 tools/ds2_make_elf.py out/ds2_image.bin out/ds2_image.elf
   ```
5. `git add -A && git commit`.

## 2. Восстановить анализ 50 ошибок (без перезапуска codegen)

```bash
mkdir -p out/logs out/reports
awk '{printf "  0x%s from 0x%s: b 0x%s from 0x%s - target not in any function\n",$1,$2,$1,$2}' \
    config/unresolved_pairs.txt > out/logs/runner.out   # файл из пакета
python3 tools/ds2_analyze_unresolved.py   # self-check 50/50, отчёт out/reports/unresolved_targets.txt
python3 tools/ds2_classify_unresolved.py  # НЕ запускался ни разу — сверить с 05
```

Главное в `ds2_classify_unresolved.py` — колонка `exits_after_declaring`: появятся ли после объявления адаптеров новые неизвестные хвостовые цели.

## 3. Решения, нужные от владельца проекта

Каждое из этих действий по договорённости делается только с отдельного согласия:

1. **Применить overrides** из `config/ds2_overrides.proposed.toml`. Сначала посмотреть, как `app/nfsmw_manifest.toml` подключает `huecos.toml`.
2. **Перезапустить codegen** (`tools/run_codegen_monitored.sh`, ~14 мин). Ожидаемо: эти 50 ошибок уйдут, могут появиться новые (см. риски в `05`). Повторять цикл «анализ → overrides → codegen», пока валидация не пройдёт.
3. По желанию — **прогон с `--log-level trace`**, чтобы подтвердить, почему GapFill пропустил эти участки. Если это ошибка SDK, решить: чинить SDK или продолжать overrides.
4. **Версия игры:** базовый XEX как есть или с title update 1.10 (тогда нужен `.xexp`).

## 4. Дальше по исходному плану (после успешной валидации)

1. Codegen генерирует C++ → собрать на ПК-хосте (Linux/Windows) по примеру NFSMW-NX (`docs/porting-another-game.md`: сначала ПК).
2. Запустить в тестовом окружении, добиться инициализации ядра Xbox 360 (импорты `xboxkrnl`/`xam`: 353 + 218), памяти, потоков, файловой системы с `game:\` на папку `DS2`.
3. Графика: D3D9/Xenos через бэкенд SDK; шейдеры `.fxo`/`.bnd` уже скомпилированы — понадобится перевод.
4. Звук (FMOD `.fsb`, XAudio2), ввод, видео (`prologue.wmv`).
5. Перенос на Switch (слой Horizon в SDK), сборка NRO, оптимизация до 30 FPS.

Ни один из этих этапов не начат.
