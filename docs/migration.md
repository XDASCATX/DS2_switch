# Перенос проекта в новое окружение / к другому исполнителю

Перенос в чистое окружение **не проверялся**. План составлен по фактическому содержимому этого окружения
(облачный контейнер, 2026-10-23).

## 1. Что переносить

| Категория | Что | Где сейчас | Как получить в новом месте |
|---|---|---|---|
| **Обязательно (разработка)** | репозиторий (код, конфигурация, документация, TOML всех версий) | git, ветка `diag/switch-level-load` | `git clone https://github.com/XDASCATX/DS2_switch -b diag/switch-level-load` (приватный — доступ даёт владелец) |
| **Обязательно (сборка)** | файлы игры Xbox 360 DS2 SotFS + DLC «Compatibility Pack 4» | у владельца; здесь `out/run/game_root`, `out/run/dlc`, `assets/game_root` | только от владельца (никогда не публиковать); сверка хешей — `audit/game_files.sha256` |
| **Обязательно (сборка)** | тулчейн devkitA64 r30-эквивалент, libnx v4.12.0, portlibs | `/opt/devkitpro` (468 МБ, собран из исходников) | `tools/switch/toolchain/*.sh` или штатный devkitPro (версии — build.md) |
| **Обязательно (сборка)** | Mesa NVK: дерево + установленный SDK | `/home/claude/swtc/src/mesa-switch-head`, `/home/claude/swtc/mesa-sdk-head-ds2-v35` | клонировать mesa-switch `0ff52079ca9e`, `git am tools/switch/toolchain/mesa-switch-ds2-perf.patch`, собрать (Rust 1.91) |
| **Обязательно (сборка)** | `rexglue` (codegen) и сгенерированный код | `sdk/out/linux-amd64/rexglue`, `app/generated/diag-dataonly` (~350 МБ) | собрать `rexglue`, прогнать codegen (часы) — или перенести `app/generated/` архивом (производное от игры: только приватно) |
| **Полезно (диагностика)** | ELF всех версий | `out/app-switch/releases/vNN/ds2.elf` | архив; без них стеки консоли старых версий не символизируются |
| **Полезно** | ПК-сохранение (Междумирье у костра) | `out/run/user/` (1,2 ГБ вместе с кэшем) | архив; иначе ПК-прогон начнётся с титула без продолжения |
| **Полезно** | логи консоли v53–v55, снимок capture_01 | у владельца `SW1_ds2/ds2_logs/` | от владельца |
| **Историческое** | дампы и кадры ПК-прогонов, логи codegen | `out/run/dump*`, `out/run/frames`, `out/logs` (~3 ГБ) | по желанию |
| **Не переносить** | кэши сборки | `out/app-host`, `out/app-switch` (кроме releases), `out/host`, `sdk/out` | пересобираются |
| **Не переносить** | временные файлы | `/tmp`, `/dev/shm/xenia_memory_*`, `out/run/cache` | — |

Секретов в репозитории и документации нет; доступ к GitHub и к Mac владельца — через среду, не через файлы.

## 2. Доступы и зависимости среды

- GitHub: чтение/запись ветки (push) — токен среды/владельца.
- Связь с Mac владельца (в этой среде — «мост» Claude Desktop): передача NRO/TOML в `SW1_ds2/switch/vNN/`, чтение
  логов из `SW1_ds2/ds2_logs/`. Без моста — владелец сам копирует файлы.
- Сеть: GitHub, PyPI, crates.io, archive.ubuntu.com достаточно; серверы devkitPro не нужны при сборке из исходников.

## 3. Контрольный список

1. Клонировать репозиторий, ветка `diag/switch-level-load`; прочитать `CLAUDE.md`, `docs/HANDOFF.md`.
2. Установить зависимости хоста (build.md §1; apt-список — handoff/02), `python3 tools/fetch_thirdparty.py`.
3. Собрать или установить devkitA64/libnx/portlibs; собрать Mesa с патчем; задать `DEVKITPRO`, `DS2_MESA_SRC`,
   `DS2_MESA_SDK`, `MESA_SWITCH_RUSTC` перед `. tools/build/env.sh`.
4. Получить файлы игры от владельца; проверить `sha256sum -c audit/game_files.sha256` из папки владельца
   `DARK.SOULS…` (пути в списке — `./DS2/…`, `./DS2.SotFS.CompatibilityPack4/…`, `./DS2.SotFS.Calibrations.1.15/…`).
5. Собрать `rexglue`, прогнать codegen с `app/ds2_manifest.diag-dataonly.toml` (или развернуть архив `app/generated`).
6. `tools/build/build-pc.sh`; ПК-прогон до кадра в игре ([testing.md](testing.md)) — сравнить с эталоном.
7. `tools/build/build-switch.sh`; сравнить поведение с последней выданной версией (тот же TOML) на консоли.
8. Готово к работе, если: NRO собирается, ПК-кадр верный, консоль воспроизводит результаты последней версии.

## 4. Ограничения

- Скорость, память и дефекты проверяются только на консоли владельца (Switch с Atmosphère, Horizon OC); ПК — нет.
- Частоты 1963/1267 зависят от разгонного модуля владельца (Horizon OC); на консоли без него override не сработает.
- Пересборка сгенерированного кода под Switch — ~2 ч; codegen — часы; Mesa — десятки минут.
