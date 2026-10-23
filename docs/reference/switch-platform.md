# Справочник: Switch-слой SDK и особенности консоли

Большая часть слоя — из nfsmw-nx (`docs/nfsmw/platform-notes.md`); здесь — что важно для DS2. **[код]** если не
помечено.

## Память и ресурсы ядра

- `sdk/src/core/guest_memory_switch.cpp`: гостевое адресное пространство (4 ГБ Xbox 360) на Horizon — окно +
  алиасы/виды, подтверждение памяти кусками. Правки DS2 (diag7–diag12): окна и алиасы выровнены по 2 МБ (иначе таблица
  страниц на каждые 2 МБ), гранула подтверждения `ds2_switch_commit_granule_kb = 2048`, слияние соседних видов.
  Проверочный симулятор — `tools/switch/gm-sim/` (2103 → 512 таблиц страниц).
- Почему важно **[замер, тесты 12–21]**: под hbloader у процесса нет своего системного ресурса — таблицы страниц
  берутся из общей с системой кучи ядра; её исчерпание = 2001-0103 для всей системы.
- `ds2_switch_nvmap_mb = 64` — transfer memory nvservices (учёт объектов памяти GPU; libnx даёт 8 МБ).
- `host_thread_stack_mb = 6` (приложение, на Switch) — было 16 × 44 потока = 741 МБ кучи.
- `tools/switch/hbl-ds2/` — hbloader с собственным системным ресурсом (тест 19: не запустился; не используется).

## Потоки и процессор

- Процесс получает 3 ядра (маска 0x7). Главный поток игры, поток команд GPU, рабочие потоки игры, XMA, звук.
- Области `mfmsr/mtmsrd` — см. [codegen.md](codegen.md) §3; `ds2_msr_per_thread = true` с v56.

## Профайлер (`sdk/src/ui/switch_perf.cpp`)

Раз в 10 с в `logs/rex/rex_perfil.log`: FPS игры (по `RexSwitchPerfCount(0)` из `IssueSwap`), CPU каждого потока
(тики ядра, точно), выборка pc/lr/стека 1 кГц (поток высокого приоритета останавливает поток, читает контекст,
продолжает), частоты, температура, вызовы libnx через `--wrap` (fences, kickoff, NvMap, окно, сон). Адреса —
`imagen+0x…` → addr2line по ELF версии. Также: круг A/B режимов (L+R+ZL), функция `RexSwitchStickClicksHeld`
(L3+R3 для снимка нативного рендерера). Разбор — `tools/analysis/threads.py`, `perf.py`.

## Частоты (`sdk/src/ui/switch_sysclk.cpp`, `switch_apm.cpp`)

- Консоль стартует homebrew на штатных частотах портативного режима (CPU 1020, GPU 307,2) **[замер, v54]**.
- v55: `RexSwitchRelojesFijar(cpu, gpu, mem)` — override через sysmodule частот (ищутся `sys:clk`, `hoc:clk`, …;
  у владельца Horizon OC `hoc:clk` 2.5.1, API 2 **[замер v55]**), команда 8 SetOverride, снимается деструктором
  при чистом выходе (при аварийном — остаётся до перезагрузки/оверлея). Из TOML: `ds2_switch_cpu_mhz/gpu_mhz/mem_mhz`.
- Существующие механизмы nfsmw-nx: следование Reverse-NX (`SeguirModo`, по умолчанию выкл.), apm-профиль GPU 460,8
  (`switch_apm.cpp`).

## Оверлеи

`ds2_switch_saltynx = true` — FPS/разрешение для Status Monitor через SaltyNX (`switch_saltynx.cpp`); займ сессии
SaltySD.

## GPU

- Tegra X1 (Maxwell, 256 ядер), Vulkan через NVK (Mesa) поверх nvservices — [mesa-nvk.md](mesa-nvk.md).
- Таймер GPU: 614,4 МГц (NVK сообщает 1 нс) — поправка в нативном рендерере.
- Барьер NVK = WAIT_FOR_IDLE + сброс/инвалидация кэшей (по исходнику `nvk_cmd_buffer.c`).
- Каждый запрос fence — ioctl, даже если GPU уже закончил (nfsmw-nx, platform-notes).

## Экран и ввод

Окно — nwindow 1280×720 (портатив); ввод — SDL/hid. Системная клавиатура — swkbd (`xam_ui.cpp`, v45).
