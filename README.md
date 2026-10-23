 <p align="center">
  <img src="https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/335300/logo.png" alt="DARK SOULS II: Scholar of the First Sin" width="620">
</p>

<h1 align="center">DARK SOULS II: Scholar of the First Sin
 Nintendo Switch</h1>

<p align="center">
  <strong>Experimental
   Xbox 360
   Native rendering
  </strong>
</p>

<p align="center">
  <a href="#русский">Русский</a> · <a href="#english">English</a> · <a href="docs/">Documentation</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/STATUS-EXPERIMENTAL-C58B36?style=flat-square" alt="Experimental status">
  <img src="https://img.shields.io/badge/TARGET-NINTENDO%20SWITCH-8B1E24?style=flat-square" alt="Nintendo Switch">
  <img src="https://img.shields.io/badge/FOCUS-RENDERING%20%7C%20PERFORMANCE-343434?style=flat-square" alt="Rendering and performance">
</p>

---

## Русский

Экспериментальный порт **Dark Souls II: Scholar of the First Sin** с Xbox 360 на Nintendo Switch. Проект исследует альтернативный графический конвейер, native rendering и оптимизацию выполнения игры на аппаратуре Switch.

Главная цель — добиться стабильных **30+ FPS**, корректного отображения игровых сцен и полноценного прохождения пути от редактора персонажа до открытого мира.

### Технические направления

| Компонент | Назначение |
|---|---|
| Game execution | Запуск и совместимость кода оригинальной Xbox 360-версии |
| Native renderer | Альтернативный путь отрисовки игровых кадров |
| Graphics resources | Обработка текстур, mipmaps и графических ресурсов |
| Lighting | Корректность освещения, теней и видимости объектов |
| Performance | Снижение CPU/GPU frame time и накладных расходов |
| Diagnostics | Логирование, профилирование и анализ зависаний |
| Configuration | TOML-конфигурации для воспроизводимых экспериментов |

### Текущее состояние

- Развивается отдельный native rendering path.
- Проверяется производительность на физической Nintendo Switch.
- Исследуются проблемы освещения, загрузки ресурсов и видимости объектов.
- Оптимизация ведётся итеративно на основе диагностических данных.
- Полная совместимость и стабильные 30 FPS пока являются целями, а не гарантированными результатами.

### Инструменты

`C++` · `ReXGlue` · `TOML` · `NRO` · CPU/GPU profiling · runtime diagnostics

Конкретные версии инструментов, зависимости и команды сборки следует смотреть в исходниках и технической документации проекта.

### Документация

- [Исходный код и структура проекта](.)
- [Техническая документация](docs/)
- [История экспериментов и диагностика](docs/)

---

## English

An experimental port of **Dark Souls II: Scholar of the First Sin** from Xbox 360 to Nintendo Switch, focused on alternative graphics execution, native rendering research and performance optimization.

The primary goal is stable **30+ FPS**, correct scene rendering and a functional path from character creation into the open world.

### Technical scope

| Component | Focus |
|---|---|
| Game execution | Compatibility with the original Xbox 360 game code |
| Native renderer | Alternative rendering path for game frames |
| Graphics resources | Texture handling, mipmaps and resource compatibility |
| Lighting | Correct lighting, shadows and object visibility |
| Performance | Reducing CPU/GPU frame time and submission overhead |
| Diagnostics | Runtime logs, profiling and stall analysis |
| Configuration | TOML-based reproducible experiments |

### Current status

- A separate native rendering path is under development.
- Performance is evaluated on physical Nintendo Switch hardware.
- Lighting, resource-loading and visibility issues are under investigation.
- Optimization is driven by diagnostic data and iterative testing.
- Full compatibility and stable 30 FPS remain development goals, not guaranteed results.

### Tooling

`C++` · `ReXGlue` · `TOML` · `NRO` · CPU/GPU profiling · runtime diagnostics

Refer to the source tree and technical documentation for verified toolchain versions, dependencies and build instructions.

### Documentation

- [Source code and project structure](.)
- [Technical documentation](docs/)
- [Experiment history and diagnostics](docs/)

---

## Screenshots

<p align="center">
  <img src="https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/2038278/movie_full.jpg" alt="Dark Souls II: Scholar of the First Sin gameplay" width="800">
</p>

*Original game reference image. Replace or supplement with verified Nintendo Switch captures as testing progresses.*

---

## Disclaimer

This is an independent, experimental project and is not affiliated with or endorsed by FromSoftware, Bandai Namco Entertainment or Nintendo. DARK SOULS and related assets are the property of their respective owners.

