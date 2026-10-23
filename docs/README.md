# Документация проекта DS2 → Switch

Точка входа для ИИ — [../CLAUDE.md](../CLAUDE.md). Текущее состояние — [HANDOFF.md](HANDOFF.md).

## Основное

| Документ | О чём |
|---|---|
| [HANDOFF.md](HANDOFF.md) | состояние, текущая версия, открытые задачи, с чего начать |
| [architecture.md](architecture.md) | компоненты, путь кадра, два графических пути, ПК и консоль |
| [build.md](build.md) | окружение, версии инструментов, сборка, упаковка, раскладка SD |
| [testing.md](testing.md) | ПК-прогоны, протокол теста на консоли, разбор логов |
| [config.md](config.md) | cvar, ds2.toml, ключи нативного рендерера |
| [performance.md](performance.md) | инструменты измерения, метод, замеры по версиям, резервы |
| [known-issues.md](known-issues.md) | открытые проблемы и ограничения |
| [experiments.md](experiments.md) | реестр всех версий: коммит, TOML, sha256, где проверено, результат |
| [history.md](history.md) | путь проекта по этапам и уроки |
| [decisions.md](decisions.md) | принятые решения и почему |
| [agents.md](agents.md) | роли ИИ-агентов |
| [migration.md](migration.md) | перенос в новое окружение, контрольный список |

## Справочник по коду — `reference/`

| Документ | Код |
|---|---|
| [reference/native-renderer.md](reference/native-renderer.md) | `sdk/src/graphics/native/` |
| [reference/app.md](reference/app.md) | `app/` |
| [reference/codegen.md](reference/codegen.md) | codegen, манифесты, `config/` |
| [reference/sdk-changes.md](reference/sdk-changes.md) | все правки SDK портом DS2 |
| [reference/emulation-gpu-path.md](reference/emulation-gpu-path.md) | эмуляция Xenos (fallback) |
| [reference/switch-platform.md](reference/switch-platform.md) | Switch-слой SDK, профайлер, частоты, память |
| [reference/mesa-nvk.md](reference/mesa-nvk.md) | Mesa NVK и патч |
| [reference/tools.md](reference/tools.md) | `tools/` |

## Журнал по этапам — `handoff/`

Подробные разборы каждого теста в порядке работы (исторические документы: пути и статусы в них — на момент
написания; TOML из них перенесены в `config/toml-history/`).

| Файл | О чём |
|---|---|
| [handoff/README.md](handoff/README.md), [01](handoff/01-status-and-rules.md) | первая передача проекта, исходные правила |
| [02](handoff/02-host-build.md), [03](handoff/03-xex-and-game-files.md) | сборка `rexglue`; XEX и файлы игры |
| [04](handoff/04-codegen-run-1.md), [05](handoff/05-unresolved-analysis.md), [06](handoff/06-tools-source.md), [07](handoff/07-next-steps.md), [08](handoff/08-session-2026-10-19.md) | codegen, 50 ошибок, инструменты, сессия 19.10 |
| [09](handoff/09-first-boot-pc.md) | первый запуск на ПК |
| [10](handoff/10-switch-build.md) | тулчейн Switch, первый NRO |
| [11](handoff/11-switch-level-load-diag.md) + `11-switch-test-plan*.txt` | чёрный экран после ролика, тесты 1–25 |
| [12](handoff/12-switch-first-ingame.md), [13](handoff/13-switch-diag-modes.md) | первый вход в мир; режимы MODE |
| [16](handoff/16-switch-gpu-optimization.md) | оптимизация эмуляции и отладка v30–v52 |
| [17](handoff/17-switch-settings-menu.md), [17a](handoff/17a-cvars-list.md) | меню L+R+→; все cvar |
| [18-native-renderer-study](handoff/18-native-renderer-study.md) | исследование: сторонние рендереры, XenosRecomp на шейдерах DS2 |
| [18-native-renderer](handoff/18-native-renderer.md) | журнал нативного рендерера v51–v56: замеры, ошибки, тесты |
| `handoff/data/` | сводки codegen и трассировок |

## Внешний референс — `nfsmw/`

Документы nfsmw-nx (сборка, платформа Switch, глоссарий, перенос другой игры): [nfsmw/platform-notes.md](nfsmw/platform-notes.md),
[nfsmw/toolchain.md](nfsmw/toolchain.md), [nfsmw/glossary.md](nfsmw/glossary.md), [nfsmw/porting-another-game.md](nfsmw/porting-another-game.md),
[nfsmw/building.md](nfsmw/building.md). Описывают NFSMW; пути и скрипты в них — репозитория nfsmw-nx.

Шаблоны для других ИИ — [../prompts/](../prompts/universal-handoff.md).
