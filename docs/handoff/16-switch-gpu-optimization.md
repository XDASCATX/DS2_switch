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

## v31-diag на консоли (diag_toml31): куда уходит GPU

GPU 1228,8 МГц (разгон), в мире ~2,7–3,5 кадр/с; в меню 39–47 кадр/с. На кадр (одна отправка) — среднее по 10-с
отчётам в мире:

| | значение |
|---|---|
| GPU всего | 286–364 мс |
| переносы владения (зона) | 105–188 мс |
| resolve (зона, 25–60 шт., 12–16 экранов) | 32–54 мс |
| загрузка текстур | 4,5–6,6 мс |
| вывод | ~0 |
| остальное (draw игры и пр.) | 48–220 мс |
| RT-получатели переносов | глубина 21–25, цвет 13–34, из них MSAA 12–13 |
| transfer-draw | глубина 35–49, цвет 20–49; пропущено проходов по битам stencil 250–360 |
| пиксели переносов × проходы | глубина 21–24 экрана, цвет 14–20 экранов (без множителя сэмплов MSAA) |
| render pass / барьеры | 120–200 / 330–610 на кадр |
| draw игры | 520–860, площадь scissor 230–565 экранов |

Переносы — ~40 экранов пикселей за 105–188 мс (~250 Мпикс/с) — в десятки раз медленнее заливки: каждый
фрагмент переноса считает адрес в EDRAM, читает источник (`texelFetch`) и конвертирует формат; для MSAA-получателей
включён `sampleShadingEnable` (шейдер на каждый сэмпл). Барьеров ~3 на перенесённый RT.

v31b-diag: те же измерения + зоны «transfer-draw в глубину», «transfer-draw в цвет», «барьеры» (GPU-слив, который
они вызывают), «draw игры» (с числом экземпляров и мкс на штуку), счётчик draw в MSAA-RT; 4096 timestamp на
отправку. `releases/v31b-diag` (sha256 `63f71d2a…3d72`), `SW1_ds2/switch/v31b_diag/`. Timestamp на каждый draw
сам добавляет сериализацию — абсолютные цифры будут чуть выше, важны доли.
