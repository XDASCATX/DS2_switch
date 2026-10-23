# Тестирование: ПК и консоль

Главное правило: **«работает/исправлено» — только после проверки на настоящем Switch** (её делает владелец).
ПК-прогон подтверждает корректность картинки и отсутствие падений, но не скорость.

## 1. ПК (облачный контейнер, без экрана)

Что есть: копия файлов игры `out/run/game_root`, DLC `out/run/dlc/…`, сохранение `out/run/user/` (загрузка
продолжения ставит персонажа в Междумирье у костра), lavapipe (программный Vulkan).

```sh
rm -rf out/run/frames/<tag>; mkdir -p out/run/frames/<tag>
tools/run/run-pc.sh <tag> 600 --log_level=info --ds2_native_renderer=true \
  --ds2_native_gpu_profile=true --ds2_native_log_every=120 \
  --ds2_capture_dir=$PWD/out/run/frames/<tag> --ds2_capture_every_ms=60000 --ds2_capture_max=40 \
  --ds2_input_script=60:start,75:a,90:a,105:a,120:a,135:a,150:a,165:a,180:a
rm -f /dev/shm/xenia_memory_*        # после прогона (иначе следующий упадёт по памяти)
```
- Скрипт ввода: «секунда:кнопка». Этот проходит титул → «Load game» → загрузку → в игре к ~9-й минуте.
  Если параллельно идёт сборка, загрузка медленнее и `60:start` попадает до титула — прогон застрянет на титуле.
- Логи: `out/run/logs/<tag>.log` (основной), `<tag>.console`; кадры — PPM в `out/run/frames/<tag>/`
  (`python3 -c "from PIL import Image; Image.open('frame_0008.ppm').save('x.png')"`).
- Дамп всех RT и resolve-текстур кадра с трассой draw: `--ds2_native_dump_dir=<dir> --ds2_native_dump_swap=N`
  (N-й кадр с >200 вызовами).
- Скорость ПК-прогона (~0,5–1 кадр/с в игре) и время GPU по lavapipe **ничего не говорят о консоли**.
- Типовые аргументы эмуляционного пути — `tools/run/pc-args-example.txt` (исторически, эмуляция).

Что проверять на ПК после правки рендерера: титул и меню без артефактов; кадр в игре как эталонный
(Междумирье: колонны, небо с лунным светом, персонаж, HUD) — сравнить с кадром предыдущей версии;
`grep -ci "error\|fatal"` в логе (ожидаемы только `ResolvePath(cache:\)` и `KeyConfigParam.param`).

Тесты инструментов: `python3 -m unittest discover -s tests` (5 тестов `ds2_xexinfo.py`, проходят).

## 2. Консоль (владелец)

Протокол (договорённость с владельцем, с v55):
1. Один `ds2.toml` на версию (несколько вариантов — только если нужно A/B-сравнение и это явно сказано).
2. Владелец играет обычным образом (Маджула у моря и вид на город, бой, туманные стены, пролом в стене) и пишет
   субъективные наблюдения: FPS по счётчику, рывки, дефекты картинки.
3. Логи — на Mac в `SW1_ds2/ds2_logs/diag_tomlNN/`: `ds2_001.log`, `rex/rex_perfil.log`, `rex/rex_stderr.log`,
   `ds2_wd_last.txt`, при снимке — `capture_NN/`.
4. Снимок кадра на консоли: зажать **L3+R3 ~1 с** — следующий кадр в игре трассируется в журнал и пишутся
   изображения в `sdmc:/switch/ds2/native_dump/capture_NN` (несколько секунд заморозки, ~20–50 МБ).

Длинные автоматические прохождения на консоли не делаются; ПК — только короткие прогоны.

## 3. Разбор логов консоли

| Что | Как |
|---|---|
| FPS/CPU по 10-секундным блокам | `python3 tools/analysis/threads.py rex_perfil.log --list` |
| Горячие функции потока | `python3 tools/analysis/threads.py rex_perfil.log out/app-switch/releases/vNN/ds2.elf --thread "GPU Commands" --blocks A-B [--stacks]` |
| Один адрес | `aarch64-none-elf-addr2line -f -C -e out/app-switch/releases/vNN/ds2.elf 0x<offset>` |
| Нативный рендерер: на кадр | `grep "\[native\] per frame\|cpu ms per frame\|early-z" ds2_001.log` |
| Время GPU по проходам / по вызовам | `grep -A17 "GPU ms per frame" ds2_001.log`, `grep -A31 "native gpu draws" ds2_001.log` |
| Рывки | `grep "\[native\] hitch" ds2_001.log` |
| Частоты | `rex_stderr.log` (`[relojes] fijos del TOML`) и строки `relojes:` в rex_perfil.log |
| Эмуляционный путь (до v51) | `tools/analysis/perf.py rex_perfil.log` |
| Снимок L3+R3 | PPM в `capture_NN/` (`res_<адрес>_g<формат>` — resolve-текстуры, `rt_<база>_<c/d>_f<формат>` — RT, `shader_*.txt` — дизассемблер микрокода), трасса `[native trace]` в ds2_001.log |

Важные особенности чтения:
- `CPU x%` потока в rex_perfil.log — точное время потока по ядру; доля сэмплов в `svc*` — это ожидание.
- Время GPU в `[native gpu]` с v55 уже в реальных мс (на Switch тики 614,4 МГц пересчитываются в коде).
- `[native] cpu ms per frame`: `waiting for the GPU` — поток команд ждёт fence кадра (предел — GPU), большое
  «outside the command processor» в `hitch` — поток ждёт команд от игры (предел — игра/CPU).

## 4. Где хранятся результаты

Таблица версий с итогами консольных тестов — [experiments.md](experiments.md); подробные разборы —
[handoff/16](handoff/16-switch-gpu-optimization.md) (до v50) и [handoff/18-native-renderer.md](handoff/18-native-renderer.md)
(нативный рендерер). Логи консоли в git не хранятся (они у владельца), в документы переносятся выдержки.
