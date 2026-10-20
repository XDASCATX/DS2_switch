# 16. Оптимизация GPU на Switch от v30

База — v30 (`15-ds2.toml.v30`, NRO diag14): MODE 7 как `render_target_transfer_skip = 1`, ~3–5 кадр/с, правильные
цвета. Каждая итерация — отдельная папка `SW1_ds2/switch/vNN*` и `releases/vNN*`; v30 не меняется.

## v31-diag: измерители (без смены поведения рендеринга)

- `gpu_zone_timing` (cvar SDK, по умолчанию выкл.): timestamp-запросы (`vkCmdWriteTimestamp`, BOTTOM_OF_PIPE) вокруг
  зон — переносы владения + очистки resolve (`PerformTransfersAndResolveClears`), resolve (`Resolve`), загрузка
  текстур (dispatch в `texture_cache.cpp`), вывод (гамма/FXAA) — и вокруг всей отправки. Пул на 8 отправок × 1024
  запросов; результаты читаются без ожидания, когда fence отправки уже сигнален. Тики NVK × 1,627 нс
  (nfsmw-nx `measuring.md`). Зоны могут быть вложенными (очистки внутри resolve) — сумма может превышать итог.
- новые счётчики: render pass (52), барьеры (53), проходы по сэмплам MSAA в переносах (49), host depth store —
  dispatch (50) и пиксели (51); переносы: RT глубины/цвета/MSAA (39–41), draw глубины/битов stencil/цвета/
  пропущенные (35–38), пиксели × реально нарисованные проходы (33–34).
- Отчёт `rex_perfil.log`: строки `transferencias detalle`, `pasadas de render … barreras …`,
  `GPU ms por envio medido: total | transferencias | resolves | carga de texturas | salida | resto`.
- В `DeferredCommandBuffer` добавлен `CmdVkWriteTimestamp`; в таблицу функций устройства — `vkCmdWriteTimestamp`,
  `vkGetQueryPoolResults`.
- ПК: GPU-плагин — отдельная библиотека `librexgpu-xenos.so`, цель `ds2` её не пересобирает: после правок в
  `sdk/src/graphics` собирать и `--target rexgpu-xenos` (иначе несовпадение структур → «Failed to create the 512 MB
  Vulkan buffer»). На Switch плагин статический и пересобирается вместе с NRO.
- Сборка: `releases/v31-diag`, sha256 `3960a1ea…6dc9`; `SW1_ds2/switch/v31_diag/{ds2.nro,ds2.toml}`;
  конфигурация — `15-ds2.toml.v31-diag` (= v30 + `gpu_zone_timing = true`). ПК: run36 без новых ошибок.
