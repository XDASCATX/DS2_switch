# Архитектура порта DS2 SotFS → Nintendo Switch

Статус сведений: **[код]** — проверено по исходникам репозитория; **[замер]** — измерено (где — указано);
**[вывод]** — анализ, не проверка; **[предположение]** — требует проверки.

## 1. Идея в одном абзаце

Порт — не эмулятор. Исполняемый файл Xbox 360 `Default.xex` (PowerPC, TitleID `465307E4`) один раз
переводится в C++ инструментом **ReXGlue codegen** (статическая рекомпиляция). Получившийся код (~84 тыс.
функций `sub_XXXXXXXX`) компилируется вместе с **ReXGlue SDK** — средой выполнения, которая реализует ядро Xbox 360
(xboxkrnl/XAM на хосте), память, потоки, звук XMA, ввод, файловую систему и графику. На Switch всё это собирается
devkitA64 в один статический NRO; графика идёт через Vulkan-драйвер **NVK** (Mesa для Switch). **[код]**

Основа взята из порта Need for Speed: Most Wanted (**nfsmw-nx**, StevensND): тот же SDK с правками под Switch,
тот же способ сборки. Своё у DS2 — приложение `app/`, конфигурация codegen `config/`, правки SDK (список —
[reference/sdk-changes.md](reference/sdk-changes.md)) и **нативный рендерер** (`sdk/src/graphics/native/`). **[код]**

## 2. Компоненты

```
 Default.xex ──(rexglue codegen, хост Linux)──▶ app/generated/diag-dataonly/*.cpp   (не в git: код из игры)
        ▲ манифест app/ds2_manifest.diag-dataonly.toml + overrides config/*.toml
        │
 ┌──────┴──────────────────────────── ds2.nro (Switch) / ds2 + .so (ПК) ──────────────────────────────┐
 │ app/src (Ds2App)          sdk/src/kernel (xboxkrnl, xam)   sdk/src/system (XThread, память ядра)    │
 │  пути, DLC, watchdog,     sdk/src/core (guest memory,      sdk/src/audio (XMA → FFmpeg)             │
 │  частоты, профайлер       потоки, Switch-слои *_switch)    sdk/src/input, sdk/src/filesystem (VFS)  │
 │                                                                                                    │
 │ Графический плагин «xenos» (sdk/src/graphics):                                                     │
 │   GraphicsSystem + CommandProcessor (кольцо PM4 от D3D игры, регистры Xenos)                        │
 │     ├─ VulkanCommandProcessor — эмуляция Xenos/EDRAM (Xenia)       ← по умолчанию, fallback         │
 │     └─ NativeCommandProcessor — нативный рендерер DS2 (v51+)       ← ds2_native_renderer = true     │
 │   SpirvShaderTranslator: микрокод Xenos → SPIR-V (во время игры)                                    │
 │   Presenter (sdk/src/ui/vulkan) → окно Switch (nwindow) / offscreen на ПК                           │
 └────────────────────────────────────────────────────────────────────────────────────────────────────┘
        │ Vulkan
        ▼
 NVK (Mesa, danfromtico/mesa-switch + tools/switch/toolchain/mesa-switch-ds2-perf.patch) → Tegra X1 (Maxwell)
```

| Подсистема | Где | Справка |
|---|---|---|
| Приложение DS2 | `app/src/` | [reference/app.md](reference/app.md) |
| Codegen и его конфигурация | `sdk/src/codegen/`, `app/ds2_manifest*.toml`, `config/` | [reference/codegen.md](reference/codegen.md) |
| Нативный рендерер | `sdk/src/graphics/native/` | [reference/native-renderer.md](reference/native-renderer.md) |
| Эмуляция Xenos (Xenia) с правками порта | `sdk/src/graphics/vulkan/`, `pipeline/` | [reference/emulation-gpu-path.md](reference/emulation-gpu-path.md) |
| Switch-слой SDK (память, потоки, профайлер, частоты) | `sdk/src/core/*_switch*`, `sdk/src/ui/switch_*` | [reference/switch-platform.md](reference/switch-platform.md) |
| Все правки SDK относительно nfsmw-nx | — | [reference/sdk-changes.md](reference/sdk-changes.md) |
| Mesa / NVK | вне репозитория + патч | [reference/mesa-nvk.md](reference/mesa-nvk.md) |
| Инструменты | `tools/` | [reference/tools.md](reference/tools.md) |

## 3. Путь кадра (нативный рендерер, текущий вариант) **[код]**

1. Игра (рекомпилированный D3D9 Xbox 360 внутри `generated`) пишет команды PM4 в кольцо в гостевой памяти и
   «пинает» GPU записью регистра.
2. Поток `GPU Commands` (базовый `CommandProcessor::WorkerThreadMain`) читает кольцо: пакеты type0 — запись
   регистров (`WriteRegister`), type3 — команды (`DRAW_INDX*`, `EVENT_WRITE*`, `XE_SWAP`, `WAIT_REG_MEM`…).
3. Рисование → `NativeCommandProcessor::IssueDraw`: шейдеры Xenos (микрокод из гостевой памяти) переводятся в SPIR-V
   (`SpirvShaderTranslator`, кэш по хешу микрокода и модификации), собирается `VkPipeline` (в отдельном потоке,
   v55+), вершины/индексы копируются в буфер кадра, константы и fetch-константы — туда же (dynamic UBO, v56),
   цели рисования — Vulkan-изображения по ключу (база EDRAM, шаг в сэмплах, формат хранения).
4. Resolve (копия EDRAM → текстура в памяти) → `IssueCopy`: копия изображения в «resolve-текстуру» по адресу
   назначения; игра потом читает эту текстуру — рендерер подставляет изображение, гостевая память не трогается.
5. `XE_SWAP` → `IssueSwap`: resolve-изображение по адресу из fetch-константы 0 рисуется в изображение презентера
   (с гамма-рампой игры), командный буфер кадра отправляется (`vkQueueSubmit`), презентер выводит кадр.

Подробно и с ограничениями — [reference/native-renderer.md](reference/native-renderer.md).

## 4. Два графических пути

| | Эмуляция Xenos (Xenia) | Нативный рендерер |
|---|---|---|
| Включение | по умолчанию | `ds2_native_renderer = true` (в текущем `ds2.toml` — да) |
| EDRAM | эмулируется (общая память 512 МБ, переносы владения между форматами/MSAA, resolve-шейдеры) | нет; RT — обычные изображения; переинтерпретация только 3 видов |
| Скорость на консоли **[замер]** | 11–16 кадр/с (v50, разгон CPU 2397 / GPU 1267) | ~20 кадр/с (v55, CPU 1963 / GPU 1267) |
| Картинка **[замер, консоль]** | верная (с оговорками v47–v50) | лучше по словам владельца; есть дефекты (см. known-issues) |
| Статус | fallback, не развивается (решение владельца 2026-10-23) | основной, развивается |

## 5. Платформы

| | ПК (Linux, облачный контейнер) | Nintendo Switch (консоль владельца) |
|---|---|---|
| Сборка | `tools/build/build-pc.sh` → `sdk/out/linux-amd64/ds2` + `librexgpu-xenos.so` | `tools/build/build-switch.sh` → `out/app-switch/ds2.nro` |
| Vulkan | lavapipe (программный, ~0,5–1 кадр/с в игре) | NVK на Tegra X1, через nvservices |
| Запуск | `tools/run/run-pc.sh` без экрана, скрипт ввода, захват кадров | владелец, hbmenu, `sdmc:/switch/ds2/` |
| Что проверяется | корректность картинки, отсутствие падений, логика | скорость, память, реальные дефекты |
| Таймер GPU | нс | тики 614,4 МГц (поправка в коде, v55) |

ПК-прогоны **не говорят ничего о скорости консоли**; картинка ПК — эталон корректности нативного пути.
Подробности окружения — [build.md](build.md), проверок — [testing.md](testing.md).

## 6. Конфигурация

Все настройки — cvar (`REXCVAR_DEFINE_*`), задаются в `ds2.toml` рядом с NRO или `--имя=значение` на ПК.
Описание структуры TOML и ключевых параметров — [config.md](config.md); полный список —
[handoff/17a-cvars-list.md](handoff/17a-cvars-list.md); история TOML всех версий — `config/toml-history/`.

## 7. Наблюдаемость

Журнал `logs/ds2_NNN.log` (+ строки `[native]`, `[native gpu]`, `[ds2 wd]`), профайлер `logs/rex/rex_perfil.log`
(CPU потоков, выборка стеков 1 кГц, частоты, вызовы libnx), `rex_stderr.log` (частоты, SaltyNX), снимок кадра по
L3+R3. Как читать — [performance.md](performance.md).
