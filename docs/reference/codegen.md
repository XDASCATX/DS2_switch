# Справочник: рекомпиляция (codegen) и её конфигурация

## 1. Что делает codegen **[код + журналы]**

`sdk/out/linux-amd64/rexglue codegen <манифест>` (исходники — `sdk/src/codegen/`): загружает и расшифровывает XEX,
находит функции (сканер, таблицы переходов, точки входа из данных), проверяет переходы (Validate) и пишет C++:
по функции `DEFINE_REX_FUNC(sub_XXXXXXXX)` (weak-символ `sub_X` → `__imp__sub_X`, чтобы хуки приложения могли
заменить функцию), таблицы регистрации (`ds2_init.cpp`, `ds2_register.cpp`), PCH (`ds2_pch.h` из шаблона
`sdk/resources/templates/codegen/pch_h.inja`), `sources.cmake`. Около 84 тыс. функций в 338 файлах `ds2_recomp.N.cpp`
(конфигурация diag-dataonly).

Запуск: `MANIFEST=app/ds2_manifest.diag-dataonly.toml tools/run_codegen_diag.sh <tag>` — журнал
`out/logs/diag/<tag>.log`, хеши входов `<tag>.inputs.sha256`, не перезаписывает существующий тег. Время — часы;
после него — полная пересборка (~2 ч для Switch).

## 2. Рабочий манифест и overrides

`app/ds2_manifest.diag-dataonly.toml` (выход `app/generated/diag-dataonly`) включает в `[entrypoint].includes`
(именно там — верхнеуровневый `includes` SDK молча игнорирует):

| Файл | Что | Происхождение |
|---|---|---|
| `config/ds2_overrides.proposed.toml` | 50 точек входа (this-adjusting thunks и т.п.) — исправление 50 `UnresolvedCall` первого прогона | разбор по инструкциям, [handoff/05](../handoff/05-unresolved-analysis.md) |
| `config/ds2_overrides.diag.toml` | точки, найденные валидацией прогонов 2–7 | [handoff/08](../handoff/08-session-2026-10-19.md) |
| `config/ds2_overrides.dataonly.toml` | код после терминатора, на который указывают `.rdata/.data` | генерирует `tools/ds2_find_dataonly_entries.py` |
| `config/ds2_overrides.codeptr.toml` | код, адрес которого строится `lis+addi/ori` | тот же скрипт |
| `config/ds2_overrides.gaps.toml` | 370 «дыр» кода между функциями (вызов в дыру = FATAL при косвенном вызове) | `tools/huecos.py`, v49/v50 |

Шапки первых четырёх файлов говорят «not applied» — это устарело: рабочая сборка их включает (манифест).
Исключения: `config/ds2_dataonly_exclude.txt` (адреса, объявление которых разрезало настоящие функции),
`tools/huecos_excluir.txt`. История: `config/ds2_overrides.gaps.v49-663.toml.bak` — список v49 с 293 ложными дырами
(хвосты функций со встроенными таблицами переходов) — сохранён для сравнения. `tools/huecos_functions.toml` — сырой
вывод `huecos.py` («не включать как есть»). `config/unresolved_pairs.txt` — 50 пар первого прогона.
`app/ds2_manifest.toml` и `diag-overrides` — промежуточные конфигурации (см. [app.md](app.md)).

## 3. Правки codegen в SDK (DS2)

| Файл | Правка | Версия |
|---|---|---|
| `sdk/src/codegen/function_scanner.cpp` | таблицы переходов с 16-битными записями (`lhzx` + сдвиг) читались как байтовые — в обоих сканерах (`detect_jump_table` и `detectJumpTable`) запоминается ширина записи | v46 (причина чёрного экрана редактора) |
| `sdk/src/codegen/function_graph.cpp` | в начало каждой функции `REX_FUNC_COUNT(addr)` | v41 |
| `sdk/resources/templates/codegen/pch_h.inja` | `REX_ENTER/LEAVE/CHECK_GLOBAL_LOCK` (области `mfmsr/mtmsrd`): общий рекурсивный мьютекс или счётчик потока (`rex_msr_global_lock`) | v41 (включено по умолчанию в TOML с v56) |

`tools/build/patch-generated.py` дописывает `REX_FUNC_COUNT` в уже сгенерированный код (без повторного codegen).
Проверки: `tools/analysis/check_u16_jump_tables.py` (таблицы `lhzx`), `tools/analysis/gaps.py`, `tools/huecos.py
--comprobar <адрес>`.

## 4. Где искать, если «странное поведение логики»

1. Падение `Call to invalid or unregistered function at guest address X` → дыра кода: `tools/huecos.py --comprobar X`,
   объявить в `gaps.toml` только если это настоящий код и не хвост функции с таблицей переходов (`MAX_LABEL`).
2. Логика игры ведёт себя неправильно (меню не открывается, скрипт ждёт) → подозревать таблицы переходов и
   разрезанные функции, сравнить `codegen.partition.json` и сгенерированный C++ с дизассемблером
   (`tools/ds2_imagedump.py`, `tools/ds2_xexinfo.py`).
3. Любая правка codegen → полная перегенерация и пересборка (часы) — планировать.
