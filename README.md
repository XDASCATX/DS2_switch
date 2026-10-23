# DARK SOULS II — Nintendo Switch Port

<p align="center">
  <img src="https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/335300/header.jpg" alt="Dark Souls II: Scholar of the First Sin" width="800">
</p>

<p align="center">
  <strong>Experimental graphics port · Nintendo Switch · Native rendering research</strong>
</p>

<p align="center">
  <a href="#-русский">Русский</a> ·
  <a href="#-english">English</a> ·
  <a href="https://www.bandainamcoent.com/games/dark-souls-ii">Original Game</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/STATUS-EXPERIMENTAL-orange?style=flat-square" alt="Experimental">
  <img src="https://img.shields.io/badge/PLATFORM-Nintendo%20Switch-red?style=flat-square" alt="Nintendo Switch">
  <img src="https://img.shields.io/badge/FOCUS-Graphics%20%26%20Performance-6f42c1?style=flat-square" alt="Graphics and performance">
  <img src="https://img.shields.io/badge/DOCUMENTATION-In%20Progress-blue?style=flat-square" alt="Documentation in progress">
</p>

---

## 🇷🇺 Русский

### О проекте

Экспериментальный проект по запуску **Dark Souls II: Scholar of the First Sin на Nintendo Switch** с исследованием и развитием альтернативного графического пути.

Основной технический фокус — native rendering, совместимость графических ресурсов, корректность изображения и оптимизация производительности на ограниченном аппаратном бюджете консоли.

Проект находится в активной разработке. Полная совместимость и стабильная производительность пока не заявляются.

### Технические направления

| Подсистема | Направление работы |
|---|---|
| Graphics backend | Исследование и развитие native rendering path |
| Rendering | Совместимость графических операций и корректность кадра |
| GPU performance | Снижение стоимости рендеринга и накладных расходов |
| Resource handling | Текстуры, mipmaps и загрузка графических ресурсов |
| Frame pacing | Снижение зависаний, рывков и нестабильности кадров |
| Diagnostics | Логирование, профилирование GPU/CPU и анализ ошибок |
| Configuration | Контролируемые TOML-конфигурации для воспроизводимых тестов |

### Текущий статус

- Развитие альтернативного графического пути.
- Исследование производительности на реальном оборудовании.
- Анализ корректности освещения, ресурсов и отображения объектов.
- Итеративная оптимизация с использованием диагностических данных.
- Подготовка воспроизводимого процесса сборки и тестирования.

**Цель:** добиться максимально корректного изображения и стабильной производительности на Nintendo Switch, постепенно приближаясь к целевым 30 FPS там, где это технически достижимо.

### Принципы разработки

- Измерения важнее предположений.
- Результаты ПК не считаются подтверждением работоспособности на консоли.
- Изменения проверяются по логам и воспроизводимым сценариям.
- Конфигурации и результаты экспериментов должны сохранять трассируемость.
- Известные ограничения документируются вместе с результатами.
- Основной путь разработки не должен распыляться на ненужные альтернативные конфигурации.

### Инструменты и технологии

- **Target:** Nintendo Switch.
- **Graphics:** native rendering path; графическая инфраструктура существующего проекта.
- **Configuration:** TOML.
- **Diagnostics:** CPU/GPU timing, runtime logs, profiling.
- **Build & deployment:** существующая инфраструктура сборки проекта и упаковка `.nro`.

Конкретные версии инструментов, зависимости, команды сборки и подтверждённые детали реализации должны фиксироваться в технической документации репозитория.

### Документация

- [Project overview](README.md)
- [Current state and handoff](HANDOFF.md)
- [Architecture and source reference](docs/)
- [Build and testing instructions](docs/)

*Ссылки на документы будут уточнены в соответствии с фактической структурой репозитория.*

---

## 🇬🇧 English

### Overview

An experimental project focused on running **Dark Souls II: Scholar of the First Sin on Nintendo Switch**, with an emphasis on alternative graphics rendering, compatibility and performance engineering.

The primary technical objectives are native rendering research, graphics-resource compatibility, visual correctness and reducing rendering overhead within the console's hardware constraints.

The project is under active development. Full compatibility and stable performance are not yet claimed.

### Technical scope

| Subsystem | Focus |
|---|---|
| Graphics backend | Research and development of a native rendering path |
| Rendering | Graphics operation compatibility and frame correctness |
| GPU performance | Reducing rendering cost and submission overhead |
| Resource handling | Textures, mipmaps and graphics-resource loading |
| Frame pacing | Reducing stalls, stutter and frame-time instability |
| Diagnostics | Logging, CPU/GPU profiling and failure analysis |
| Configuration | Controlled TOML configurations for reproducible testing |

### Development status

- Alternative graphics-path development.
- Performance investigation on physical hardware.
- Analysis of lighting, resource handling and object visibility.
- Iterative optimization guided by diagnostic data.
- Preparation of a reproducible build and testing workflow.

**Goal:** achieve visually correct rendering and the most stable performance possible on Nintendo Switch, progressively approaching a 30 FPS target where technically feasible.

### Engineering principles

- Measurements over assumptions.
- PC results do not establish console compatibility.
- Changes should be evaluated using logs and reproducible scenarios.
- Configurations and experiments should remain traceable.
- Known limitations must be documented alongside results.
- Development should focus on the primary rendering path rather than unnecessary configuration branches.

### Tooling and technologies

- **Target platform:** Nintendo Switch.
- **Graphics:** native rendering path within the existing project infrastructure.
- **Configuration:** TOML.
- **Diagnostics:** CPU/GPU timing, runtime logs and profiling.
- **Build & deployment:** existing build pipeline and `.nro` packaging.

Exact toolchain versions, dependencies, build commands and implementation details belong in the repository's technical documentation.

### Documentation

- [Project overview](README.md)
- [Current state and handoff](HANDOFF.md)
- [Architecture and source reference](docs/)
- [Build and testing instructions](docs/)

*Documentation links should be aligned with the actual repository structure.*

---

## Project philosophy

<p align="center">
  <em>Understand the pipeline. Measure the bottlenecks. Preserve the evidence. Improve one verified step at a time.</em>
</p>

## Disclaimer

This is an independent, experimental, non-commercial engineering project. It is not affiliated with, endorsed by, or an official port from FromSoftware or Bandai Namco Entertainment. DARK SOULS and related assets are the property of their respective owners.

