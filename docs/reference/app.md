# Справочник: приложение DS2 (`app/`)

Приложение — тонкий слой над рантаймом ReXGlue: класс приложения, пути, DLC, телеметрия, диагностика.
Код игры — в `app/generated/<конфигурация>/` (генерирует codegen, в git не входит). Всё — **[код]**.

## Сборка (`app/CMakeLists.txt`)

- Не включает `generated/rexglue.cmake` (тот перезапускает codegen с рабочим манифестом); каталог кода задаёт
  `DS2_GENERATED_DIR`, по умолчанию `app/generated/diag-dataonly` (манифест `app/ds2_manifest.diag-dataonly.toml`).
- `ds2_recomp` (OBJECT, PCH `ds2_pch.h`, `-gline-tables-only`) + исполняемый `ds2`. На Switch плагин GPU «xenos»
  линкуется статически (`rexglue_configure_target(ds2 GPU_PLUGINS xenos)`), NRO — `rexglue_switch_add_nro`.
- На ПК `ds2` кладётся рядом с `librexruntime.so` (`sdk/out/linux-amd64/`), плагин — `librexgpu-xenos.so`.

## Файлы `app/src/`

| Файл | Назначение | Ключевое | cvar |
|---|---|---|---|
| `main.cpp` | точка входа: `REX_DEFINE_APP(ds2, Ds2App::Create)` | — | — |
| `ds2_app.h` | класс `Ds2App : rex::ReXApp`, хуки жизненного цикла | `OnConfigurePaths` (game_root/user рядом с exe, на Switch `sdmc:/switch/ds2/`), `OnPostInitLogging` (умолчания `gpu_plugin=xenos`, Switch `host_thread_stack_mb=6`; платформенные ключи, счётчики, оверлей), `OnLoadXexImage` (`game:\Default.xex` — регистр важен), `OnPreSetup` (скрипт ввода), `OnPostSetup` (DLC, масштаб времени), `OnPostLaunchModule` (захват, watchdog, оверлей) | — |
| `ds2_watchdog.cpp/.h` | телеметрия загрузки/зависаний; платформенные ключи Switch до старта; **частоты сессии** (v55) | поток раз в секунду: `[ds2 wd] snapshot/hb/file/STALL`, `ds2_wd_last.txt` (переживает принудительное закрытие), пробы запаса ядра, гистограмма блоков; `ApplyPlatformSwitches` — стеки профайлера, `RexSwitchRelojesFijar`, SaltyNX, гранула памяти, nvservices, `ds2_mesa_env` | `ds2_watchdog*`, `ds2_switch_*` (в т.ч. `cpu/gpu/mem_mhz`), `ds2_mesa_env`, `ds2_export_stats` |
| `ds2_switch_platform.cpp/.h` | запросы, требующие `switch.h` (конфликт typedef с заголовками rex) | память процесса, статистика ядра (блоки, sysres, окна таблиц страниц), пробы запаса блоков и таблиц, лимиты ресурсов, гранула гостевой памяти, transfer memory nvservices, мост SaltyNX | — |
| `ds2_func_counts.cpp/.h` | счётчики вызовов всех функций игры (`REX_FUNC_COUNT`) + применение `ds2_msr_per_thread` и `ds2_fpcr_ftz_fixed` до первого гостевого кода | окна в `logs/ds2_llamadas.bin`, декодер `tools/analysis/ds2_func_counts.py` | `ds2_func_counts*`, `ds2_msr_per_thread`, `ds2_fpcr_ftz_fixed` |
| `ds2_editor_wait.cpp` | хуки ожидания редактора персонажа (v42–v44, `REX_HOOK_RAW`) — диагностика/обходы; после v46 не нужны, выключены | `sub_8239A2E8` «UI свободен?», `sub_825EC618`/`sub_825EFDF0` (скрипт) | `ds2_editor_*` |
| `ds2_input_script.cpp/.h` | виртуальный геймпад по сценарию для ПК-тестов (из nfsmw-nx, без автопилота) | `"сек:кнопка,сек-сек:кнопка"`, удержание, правый стик; масштаб гостевого времени | `ds2_input_script`, `ds2_guest_time_scalar*` |
| `ds2_capture.cpp/.h` | периодический захват выходного кадра в PPM (ПК) | `Presenter::CaptureGuestOutput` | `ds2_capture_*` |
| `ds2_dlc.cpp/.h` | установка DLC (STFS) при старте — без «Compatibility Pack 4» игра не уходит с титула | `ContentManager::InstallContent` | `ds2_install_dlc` |
| `ds2_mode_overlay.cpp/.h` | надпись MODE N для диагностических режимов GPU эмуляции (L+R+ZL) | только эмуляционный путь | `edram_diagnostic_mode` |

## Манифесты codegen (`app/*.toml`)

| Файл | Роль |
|---|---|
| `ds2_manifest.toml` | исходный (без overrides) — валидацию не проходит, оставлен как точка отсчёта |
| `ds2_manifest.diag-overrides.toml` | + overrides из разбора 50 ошибок |
| `ds2_manifest.diag-dataonly.toml` | **рабочий**: + точки входа, найденные по данным/коду, + дыры кода (`config/ds2_overrides.gaps.toml`); его каталог `generated/diag-dataonly` собирается |

Подробнее о codegen — [codegen.md](codegen.md).

## Перед изменением

- Хуки `REX_HOOK_RAW(sub_X)` заменяют функцию игры при линковке (weak-символы); адрес должен существовать в
  сгенерированном коде.
- Всё, что должно действовать до первой гостевой инструкции (msr, FTZ, гранула памяти, nvservices) — только в
  `OnPostInitLogging` (cvar `kInitOnly`).
- `switch.h` нельзя включать в файлы с заголовками rex — для этого `ds2_switch_platform.cpp`.
