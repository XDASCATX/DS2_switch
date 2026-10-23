# CLAUDE.md — порт Dark Souls II SotFS на Nintendo Switch

Прочитай этот файл целиком, затем **[docs/HANDOFF.md](docs/HANDOFF.md)** (текущее состояние и задачи).
Индекс всей документации — [docs/README.md](docs/README.md).

## Проект

Порт **Dark Souls II: Scholar of the First Sin** (только Xbox 360, TitleID `465307E4`) на **Nintendo Switch**
статической рекомпиляцией: `Default.xex` (PowerPC) → C++ (**ReXGlue codegen**, ~84 тыс. функций) + **ReXGlue SDK**
(ядро Xbox 360 на хосте, память, потоки, звук, ввод, графика) → один NRO (devkitA64). Графика — **собственный
нативный рендерер** на Vulkan/**NVK** (Mesa для Switch); старая эмуляция Xenos/EDRAM (Xenia) осталась как fallback.
Основа — порт NFSMW-NX (тот же SDK и сборка). Архитектура — [docs/architecture.md](docs/architecture.md).

**Цель владельца сейчас:** стабильные **30+ кадр/с везде** (Маджула, бой, подгрузка) при правильной картинке;
потом — снизить частоты консоли (сейчас игра ставит CPU 1963 / GPU 1267 МГц).

**Состояние (2026-10-23):** v55 на консоли — исследованные локации работают, ~20 кадр/с, предел — CPU (поток команд GPU и главный
поток игры). v56 (разгрузка CPU) выдана, **ждёт теста**. Тестовый выпуск 0.56 (= v56) собран локально с тем же
исполняемым кодом и TOML; NSP создан, `main` объединён локально, GitHub Release ещё не опубликован. По решению
владельца репозиторий остаётся PRIVATE, релиз доступен только людям с доступом. Детали — [docs/HANDOFF.md](docs/HANDOFF.md).

## Как идёт работа

Владелец играет на своей консоли и присылает логи (`SW1_ds2/ds2_logs/diag_tomlNN/` на его Mac). Ты: разбираешь
логи → меняешь код → проверяешь на ПК (lavapipe, без экрана) → собираешь NRO → выдаёшь версию vNN с одним
`ds2.toml` → ждёшь отзыва. «Исправлено» — только после его теста на консоли.

## Правила (обязательны)

1. Писать владельцу **по-русски и коротко**. Отделять замеры от выводов и предположений.
2. **Не менять `Default.xex`**. Ничего из игры (файлы, `app/generated/`, NRO/ELF, дампы) **не коммитить**.
3. Разработка — в ветке **`diag/switch-level-load`**; `main` обновлять только для публичного выпуска по запросу
   владельца (первый — 0.56 = v56). Коммит на новую тестовую версию: `vNN: <что>`; push в origin после проверки.
4. Прошлые версии не перезаписывать (`out/app-switch/releases/vNN`, `config/toml-history/15-ds2.toml.vNN`).
5. Каждое изменение поведения — cvar с безопасным умолчанием и ключом в TOML версии (откат без пересборки).
   Владельцу — **один** `ds2.toml` на версию; A/B — только если явно нужно.
6. Не жертвовать правильностью картинки ради кадров; целые проходы рендера не выключать без доказательства.
7. Перед выдачей — ПК-прогон до кадра в игре и сравнение с эталоном ([docs/testing.md](docs/testing.md)).
8. Длинных автоматических прохождений не делать; ПК-прогоны — короткие (~10 мин), после них
   `rm -f /dev/shm/xenia_memory_*`; процессы убивать по `pgrep -x ds2`, не `pkill -f`.
9. Перегенерация кода (codegen) и полная пересборка — часы: только когда без неё нельзя, предупредить владельца.

## Где что

| Путь | Что |
|---|---|
| `app/src/` | приложение DS2 (пути, DLC, watchdog, частоты, диагностика) — [docs/reference/app.md](docs/reference/app.md) |
| `sdk/src/graphics/native/native_renderer.cpp` | **нативный рендерер** (основная работа) — [docs/reference/native-renderer.md](docs/reference/native-renderer.md) |
| `sdk/` | ReXGlue SDK с правками nfsmw-nx и DS2 — [docs/reference/sdk-changes.md](docs/reference/sdk-changes.md) |
| `app/ds2_manifest.diag-dataonly.toml`, `config/ds2_overrides.*.toml` | рабочий манифест и overrides codegen — [docs/reference/codegen.md](docs/reference/codegen.md) |
| `config/toml-history/` | TOML каждой выпущенной версии (текущая — `15-ds2.toml.v56`) — [docs/config.md](docs/config.md) |
| `tools/build/`, `tools/run/` | сборка, упаковка, ПК-прогон — [docs/build.md](docs/build.md) |
| `tools/analysis/` | разбор логов консоли (`threads.py`, …) — [docs/reference/tools.md](docs/reference/tools.md) |
| `tools/switch/` | тулчейн из исходников, патч Mesa, симулятор памяти |
| `docs/handoff/` | журнал по этапам (подробные разборы тестов) |
| вне git | `assets/`, `out/run/game_root` (игра), `app/generated/` (код из игры), `out/` (сборки, релизы с ELF), Mesa `/home/claude/swtc`, тулчейн `/opt/devkitpro` |

## Сборка и проверка (кратко; полностью — [docs/build.md](docs/build.md))

```sh
. tools/build/env.sh                                   # пути (переопределяются переменными)
tools/build/compile-check.sh sdk/src/graphics/native/native_renderer.cpp   # ~15 с
tools/build/build-pc.sh   |  nice ninja -C out/app-host rexgpu-xenos     # ПК
tools/run/run-pc.sh <tag> 600 --ds2_native_renderer=true ...           # см. docs/testing.md
tools/build/build-switch.sh                            # out/app-switch/ds2.nro
cp config/toml-history/15-ds2.toml.v<prev> config/toml-history/15-ds2.toml.vNN  # править шапку и ключи
tools/build/package-release.sh NN                      # releases/vNN + части для Mac
```

## Технические ограничения, которые легко забыть

- ПК-прогон не говорит о скорости консоли. Таймер GPU на Switch — 614,4 МГц (учтено в коде с v55).
- Нативный рендерер: MSAA в пространстве сэмплов; переинтерпретация EDRAM — только глубина→8888 и 7e3↔unorm10;
  шейдеры переводятся во время игры; кубы/3D-текстуры — заглушка ([docs/known-issues.md](docs/known-issues.md)).
- `switch.h` не включать рядом с заголовками rex (конфликты typedef) — Switch-вызовы выносить в отдельный файл.
- Rust для Mesa — строго 1.91. Сборка `-j2` (больше — OOM).

## Документация: поддерживать актуальной

После каждой версии / отзыва владельца обновить: [docs/HANDOFF.md](docs/HANDOFF.md) (состояние, версия, задачи),
[docs/experiments.md](docs/experiments.md) (строка версии: коммит, TOML, sha256, где проверено, результат),
[docs/known-issues.md](docs/known-issues.md), журнал [docs/handoff/18-native-renderer.md](docs/handoff/18-native-renderer.md)
(замеры, находки). При изменении кода — затронутый `docs/reference/*.md`; при новых cvar — [docs/config.md](docs/config.md)
и `docs/handoff/17a-cvars-list.md` (`tools/analysis/cvars.py` + `cvars_table.py`). CLAUDE.md, HANDOFF и справочники не должны
противоречить друг другу и коду. Обновлять только то, что затронуто изменением.

## Агенты (Claude Code)

`.claude/agents/`: `console-log-analyst` (разбор логов консоли), `perf-investigator` (кандидаты оптимизаций),
`change-reviewer` (проверка перед выдачей). Роли и правила — [docs/agents.md](docs/agents.md). Для других ИИ —
[prompts/universal-handoff.md](prompts/universal-handoff.md).
