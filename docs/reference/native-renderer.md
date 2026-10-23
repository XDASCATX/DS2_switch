# Справочник: нативный рендерер (`sdk/src/graphics/native/`)

Файлы: `native_renderer.h` (2 функции наружу), `native_renderer.cpp` (~4 тыс. строк, всё остальное). Подключение —
`sdk/src/graphics/plugin_main.cpp` (если `ds2_native_renderer`, плагин «xenos» создаёт `NativeGraphicsSystem`
вместо `VulkanGraphicsSystem`), сборка — строка в `sdk/src/graphics/CMakeLists.txt`. Журнал разработки, замеры и
найденные ошибки — [handoff/18-native-renderer.md](../handoff/18-native-renderer.md). Ключи TOML —
[config.md](../config.md). Всё ниже — **[код]**, если не помечено.

## 1. Классы и поток управления

| Сущность | Роль |
|---|---|
| `NativeGraphicsSystem` (конец файла) | наследник `GraphicsSystem` SDK: создаёт Vulkan-провайдер и `NativeCommandProcessor`; кольцо PM4, прерывания, vblank — базовые |
| `NativeCommandProcessor` | наследник `CommandProcessor` SDK; переопределяет `SetupContext/ShutdownContext`, `LoadShader`, `IssueDraw`, `IssueCopy`, `IssueSwap`, `ExecutePacketType3_EVENT_WRITE_ZPD`, `WriteRegister`, `WriteRegistersFromMem` |
| `NativeImage` | VkImage + память (dedicated) + виды (полный, depth, stencil), размер, число мипов |
| `FrameSlot` (×3, `kFrameSlots`) | командный буфер, fence, буфер загрузки 48 МБ (+8 КБ запаса), пул дескрипторов, набор «shared memory», набор констант (dynamic UBO), кэш наборов текстур кадра, отложенные уничтожения, пулы запросов (профиль, occlusion) |
| `PipelineKey` | всё, что выбирает VkPipeline (хеши/модификации шейдеров, render pass, топология, cull, глубина, трафарет, блендинг, маски, число целей, раскладка); хешируется как байты |
| `ConvertMode` | переинтерпретации EDRAM: `kConvertDepthToRgba8`, `kConvertFloat7e3ToUnorm10`, `kConvertUnorm10ToFloat7e3` (GLSL в строках файла, компиляция glslang при старте) |

Поток `GPU Commands` (базовый `CommandProcessor`) читает кольцо; для каждого рисования — `IssueDraw`, для resolve
(`RB_MODECONTROL.edram_mode == kCopy`) — `IssueCopy`, на `XE_SWAP` — `IssueSwap`. Кадр = один командный буфер
слота; отправка — в `IssueSwap` (или при переполнении буфера загрузки — `Upload` → `SubmitFrame`).

## 2. Кадр: слоты и синхронизация

- `BeginFrame`: ждёт fence слота (3 кадра назад), собирает результаты профиля/occlusion, уничтожает отложенные
  объекты, сбрасывает пул команд и дескрипторов, выделяет наборы «shared memory» (все привязки = буфер загрузки) и
  констант (5 dynamic UBO с диапазонами `kConstantRanges`), сбрасывает пулы запросов.
- `SubmitFrame`: закрывает проход, последняя метка профиля, `vkQueueSubmit` с fence слота, `++frame_`, сразу
  `BeginFrame` следующего слота.
- `IssueSwap`: одна отправка на кадр (с v54; до этого — лишняя пустая).
- `Barrier()` — глобальный барьер ALL→ALL (на NVK = wait-for-idle + сброс кэшей). Ставится до/после копий,
  очисток, конверсий, загрузок текстур; **между проходами — нет** (v55, `ds2_native_pass_barriers`).

## 3. Render target (`GetRenderTarget`, `AdoptRenderTarget`)

- Ключ: база EDRAM (тайлы) | шаг **в сэмплах** (`surface_pitch × 2` для 4x по X) | формат хранения
  (`GetStorageColorFormat`; `_GAMMA` = обычный 8888) | глубина/цвет. Высота — по высоте ножниц, выровнена до 8,
  ≤ 2048 (если цель выросла — пересоздаётся, содержимое теряется, предупреждение в журнале).
- MSAA в пространстве сэмплов: 4x 640×360 = изображение 1280×720 (сэмпл s пикселя (x,y) → (2x + s&1, 2y + s>>1)),
  совпадает с раскладкой Xenia. Поэтому 1x- и 4x-виды одной области EDRAM — одно изображение.
- Форматы: цвет — `ColorRtFormat` (k_2_10_10_10_FLOAT → RGBA16F); глубина D24S8 → `D24_UNORM_S8`, D24FS8 → `D32_SFLOAT_S8`.
- `edram_owner_` (база+шаг → последний ключ): смена владельца того же места с другим форматом запускает
  `Convert` только для трёх известных переинтерпретаций DS2; иначе новое изображение начинается с нуля.

## 4. Рисование (`IssueDraw`)

Порядок: копия-resolve? → вершинный шейдер, растеризация → пиксельный шейдер (если `kColorDepth`) → топология
(rect/point — режим переводчика «полоса из 4 вершин», quad — индексы на CPU) → модификации шейдеров
(интерполяторы, `kEarlyHint` при `ds2_native_early_z` и условии как в эмуляции) → перевод SPIR-V при первом
использовании → ножницы/вьюпорт в пространстве сэмплов → RT цвета (по маске записи) и глубины (если z/stencil) →
текстуры (`GetTextureView`, может записать загрузку — поэтому до начала прохода) → раскладка конвейера →
`PipelineKey` → `GetPipeline` (асинхронно, полноэкранные rect/≤6 индексов ждут) → вершины (копия диапазонов
fetch-констант в буфер кадра, адрес в fetch-константе переписывается; дедуп по (адрес, размер) в кадре) → индексы
(копия гостевых, порядок байт меняет вершинный шейдер) → системные константы (`SystemConstants` переводчика:
флаги, вьюпорт `GetHostViewportInfo`, альфа-тест, exp bias, знаки текстур) → 5 буферов констант в буфер загрузки
(смещения → dynamic offsets) → наборы текстур (кэш по содержимому в кадре) → проход (`framebuffers_` по видам,
`vkCmdBeginRenderPass` при смене) → динамические состояния (вьюпорт, ножницы, depth bias, blend constants,
трафарет) → `vkCmdDraw(Indexed)`.

Пропуски (`draws_skipped`): нет VS; растеризация невозможна / шаг 0; нет ни цвета, ни глубины («no target»,
~170–300 на кадр в игре — такие draw ничего не пишут).

## 5. Текстуры (`GetTextureView`, `GetSampler`)

- Resolve-текстура по адресу (`resolved_`) — берётся изображение напрямую (вид со swizzle: глубина — RRRR,
  `copy_dest_swap` — R↔B).
- Иначе — гостевая текстура: ключ = XXH3(fetch dword 0,1,2,5 + число уровней) ^ endian. Раздвижка тайлов на CPU
  (`TiledOffset2D`, копия блоками 1/2/4/8/16 байт), уровни 1..mip_max из `mip_address` по
  `texture_util::GetGuestTextureLayout` (упакованный хвост мипов), смена порядка байт, `vkCmdCopyBufferToImage`.
  Проверка содержимого (XXH3 базы и мипов) раз в 30 кадров.
- Форматы — `GetTextureFormat` (8, 8_8, 8888, 2_10_10_10, 16/16_16/16x4 (float и unorm), 32_FLOAT, DXT1/2_3/4_5,
  DXN, DXT5A); прочие → белая 1×1. Кубы и 3D — белая заглушка.
- Сэмплер: фильтры из инструкции/fetch, mip-фильтр, `minLod = mip_min_level`, `maxLod` (base map → +0,25),
  анизотропия ≤ `ds2_native_max_anisotropy`. LOD bias — в шейдере (переводчик).
- `EvictTextures` (раз в 60 кадров): текстуры без обращений > `ds2_native_texture_keep_frames`, resolve-цели > 4×.

## 6. Resolve (`IssueCopy`)

`draw_util::GetResolveInfo` → прямоугольник (вершины rect-а) в пространстве сэмплов → источник (RT цвета по
`copy_src_select` или глубина) → изображение назначения в `resolved_[адрес]` (размер ≥ шаг×высота назначения в
сэмплах; формат = формат источника; глубина остаётся глубиной) → `vkCmdCopyImage`. Sub-resolve: если адреса нет,
ищется текстура-владелец, в которую адрес попадает тайлом 32×32 (атлас теней DS2 1600×1600 из 4 каскадов);
результат поиска кэшируется до изменения набора resolve-целей (v56). Затем очистки (`ClearRect` —
`vkCmdClearAttachments` по прямоугольнику).

## 7. Показ (`IssueSwap`)

Resolve-изображение по адресу fetch-константы 0 (или frontbuffer) → проход презентера с гамма-рампой игры (256
записей в буфере загрузки, `ds2_native_gamma_ramp`) → `RefreshGuestOutput` → `SubmitFrame`. Затем статистика,
освобождение текстур, дамп/снимок, опрос L3+R3.

## 8. Конвейеры и шейдеры

- `LoadShader`: `VulkanShader` по XXH3 микрокода; трансляции кэшируются по модификации (`GetOrCreateTranslation`).
- `SpirvShaderTranslator` создан с `edram_fragment_shader_interlock = false`; v56: `SetPixelShaderContraction`
  (пиксельные шейдеры без `NoContraction` при `ds2_native_fast_pixel_math`).
- `GetPipeline`: кэш по XXH3(`PipelineKey`); новый — в очередь потока `PipelineThreadMain` (pthread, стек 4 МБ);
  пока не готов — draw пропускается (`draws_pipeline_pending`), полноэкранные ждут. `CreatePipelineObject` —
  чистая функция ключа (без регистров), поэтому безопасна в другом потоке.

## 9. Occlusion queries (`ExecutePacketType3_EVENT_WRITE_ZPD`)

По умолчанию выключено → базовая реализация SDK (всегда `query_occlusion_fake_sample_count`). При включении:
`VK_QUERY_TYPE_OCCLUSION` между событиями begin/end, ответ игре — последний измеренный по адресу результата
(запаздывание до 3 кадров).

## 10. Наблюдаемость

`LogStats` (`[native] …`), `Mark/MarkPass/CollectProfile` (метки времени, `[native gpu]`, `[native gpu draws]`),
журнал рывков в `IssueSwap`, `Tracing()`/`DumpImages`/`ReadPixel` (трасса и дамп; на Switch — по L3+R3 через
`RexSwitchStickClicksHeld` из `sdk/src/ui/switch_perf.cpp`, каталог `DumpDir()`).

## 11. Перед изменением

- Любая правка IssueDraw/текстур/resolve → ПК-прогон до кадра в игре и сравнение с эталоном ([testing.md](../testing.md)).
- Таймер GPU на Switch — 614,4 МГц (`SetupContext`).
- Пиксельный шейдер решает early-Z: не добавлять в шейдеры побочные эффекты без проверки `IsExecutionModeEarlyFragmentTests`.
- Диапазоны `kConstantRanges` должны покрывать `sizeof(SystemConstants)` (static_assert), 256×16 float, 160 байт bool/loop, 768 байт fetch.
- Пул дескрипторов на кадр: `kMaxSetsPerFrame = 12288` наборов; кэш наборов текстур очищается в `BeginFrame`.
