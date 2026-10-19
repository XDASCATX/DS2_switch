# 04. Первый прогон `rexglue codegen`

## Манифест (`app/ds2_manifest.toml`)

Только подтверждённые поля, без overrides. Копия — `config/ds2_manifest.toml`.

```toml
# Dark Souls II: Scholar of the First Sin (Xbox 360), title 465307E4.
# Only fields confirmed from the XEX header / required by the SDK. No overrides yet: the first codegen run decides
# what (switch tables, gaps, chunks) has to be declared.

[project]
name = "ds2"
sdk_version = "0.10.0"
game_root = "../assets/game_root"

[entrypoint]
file_path = "../assets/game_root/Default.xex"   # exact case: the host filesystem is case-sensitive
out_directory_path = "generated/default"
```

Требования (по `sdk/src/codegen/config.cpp` и `manifest.cpp`):
- `[project].name` — валидный идентификатор; `game_root` необязателен (по умолчанию — папка XEX).
- `[entrypoint].file_path` обязателен; при пустых `file_path`/`out_directory_path` выдаётся предупреждение.
- XEX должен лежать внутри `game_root`; он монтируется как `\Device\Harddisk0\Partition1` и читается через VFS-путь `game:\…`.
- Пути относительно файла манифеста. У NFSMW `file_path` написан как `default.xex` строчными — для DS2 нужен точный регистр.

XEX в `assets/game_root/` — **копия**, хэш сверен с оригиналом; `assets/` в `.gitignore`.

## Запуск

`tools/run_codegen_monitored.sh` (см. `06`):
```bash
/usr/bin/time -v -o out/logs/codegen_time.txt \
  sdk/out/linux-amd64/rexglue --log-file out/logs/codegen.log codegen app/ds2_manifest.toml
```

## Результат

| Параметр | Значение |
|---|---|
| Загрузка XEX | Успешно: title `465307E4`, media `458B3E06`, версия `0.0.0.22`, filetime 2014-11-26 11:32:08 UTC |
| Фазы | Register → Scan → Discover → GapFill → Merge → Validate |
| Итог | `Validation failed: 50 unresolved calls (after 815.4s)`, `Analysis failed for 'Default'`, exit 1 |
| Время | 13 мин 36 с (1 ядро, ~99% CPU; узкое место — CPU) |
| Пик памяти | ~390 МБ RSS — память не проблема |
| Записано | только `app/generated/rexglue.cmake`; C++ не генерируется до успешной валидации |

Сообщения в логе по ходу: guest arena смонтирована (virtual base 0x100000000, physical 0x200000000), `game_root` смонтирован read-only, предупреждение «Too few processor cores».

## 50 ошибок `UnresolvedCall`

Все 50 — безусловный `b` (не `bl`), 50 разных целей; формат строки лога: `0xTARGET from 0xSOURCE: b 0xTARGET from 0xSOURCE - target not in any function`. 41 переход назад, 9 вперёд. **Ни один источник и ни одна цель не лежат внутри функции из PDATA** — все в «промежутках» между записями PDATA (47 разных промежутков, 9 480 байт).

| # | Цель | Источник | # | Цель | Источник |
|---|---|---|---|---|---|
| 1 | 822844D0 | 82677E44 | 26 | 8256A7D0 | 8256C458 |
| 2 | 823A4748 | 823A98A4 | 27 | 82646FF0 | 82647078 |
| 3 | 823A4750 | 823A98AC | 28 | 826473A0 | 82647428 |
| 4 | 823A4758 | 823A98B4 | 29 | 82647758 | 826477E0 |
| 5 | 823A4760 | 823A98BC | 30 | 82647E60 | 82647EE8 |
| 6 | 823A4790 | 823A98DC | 31 | 82648240 | 826482C8 |
| 7 | 823A4798 | 823A98E4 | 32 | 826EBEE8 | 8271F178 |
| 8 | 823A47A0 | 823A98EC | 33 | 826EBF18 | 8271F188 |
| 9 | 823A47A8 | 823A98F4 | 34 | 826EBF60 | 8271F220 |
| 10 | 823A4850 | 823A9994 | 35 | 826EC020 | 8271F2C4 |
| 11 | 823A4BB8 | 823A9A44 | 36 | 82717150 | 826EBFFC |
| 12 | 823A4BC0 | 823A9A4C | 37 | 8284B85C | 8284B86C |
| 13 | 823A4BC8 | 823A9A54 | 38 | 828C7B10 | 828E9974 |
| 14 | 823A4BD0 | 823A9A5C | 39 | 828C7B88 | 828D4D04 |
| 15 | 823A4BD8 | 823A9A64 | 40 | 828C7C38 | 828553C8 |
| 16 | 823A4BE0 | 823A9A74 | 41 | 828C7C48 | 828553C0 |
| 17 | 823A4BF0 | 823A9A84 | 42 | 828C7C58 | 828553B8 |
| 18 | 823A4D38 | 823A9BA4 | 43 | 828C7C68 | 828553B0 |
| 19 | 823A4D90 | 823A9C5C | 44 | 828D3080 | 828D4B4C |
| 20 | 823A9978 | 82373D34 | 45 | 828D88F4 | 828D8904 |
| 21 | 823DEDD8 | 823E17E0 | 46 | 828D8A0C | 828D8A1C |
| 22 | 823DEDE0 | 823E1848 | 47 | 82CFF064 | 82CFF094 |
| 23 | 82406940 | 823E4F60 | 48 | 82F66FF0 | 826366F4 |
| 24 | 824813E0 | 8268C6C8 | 49 | 82FBBA00 | 830A6B88 |
| 25 | 82481408 | 8268CFC4 | 50 | 8332E2C8 | 8332B9D4 |

Тот же список в машиночитаемом виде — `config/unresolved_pairs.txt`. Из него `07` восстанавливает `runner.out` для анализаторов. Проверено 19.10.2026: во всех 50 источниках расшифрованного образа действительно стоит `b` на указанную цель (50/50) — дамп совпадает с тем, что видел codegen.
