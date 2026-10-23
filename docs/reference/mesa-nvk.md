# Справочник: Mesa NVK для Switch

- Источник: github.com/danfromtico/mesa-switch, база `0ff52079ca9e9e837ed27eb95a8d239f439a1864`.
- Патч DS2: `tools/switch/toolchain/mesa-switch-ds2-perf.patch` (4 коммита, `git format-patch` от базы):
  1. NAK: задержка текстур/памяти 200, `peephole_select` limit 8, новая ревизия компилятора (сброс кэша шейдеров);
  2. хост-сборка (drm-shim): `<dlfcn.h>` в `vk_instance.c`;
  3. Maxwell: дескрипторы UBO наборов из CPU-отображения (без косвенных сегментов GPFIFO) — v34;
  4. sparse binding/residency для буферов (изображения — нет), `NVK_SWITCH_SPARSE_BUFFER=0` выключает — v35.
- Старый патч nfsmw-nx (`mesa-switch-nfsmw.patch`) — для базы `1a8c1a66d6f`, первые сборки (handoff/10); сейчас не
  используется.
- Дерево в этом окружении: `DS2_MESA_SRC=/home/claude/swtc/src/mesa-switch-head` (ветка с 4 коммитами), сборка
  `builddir-switch`, установленный SDK `DS2_MESA_SDK=/home/claude/swtc/mesa-sdk-head-ds2-v35` (v35+ без изменений).
- Пересборка: `tools/build/build-mesa.sh <имя>`; затем `DS2_MESA_SDK` на новый каталог и полная пересборка NRO.
  Rust — строго 1.91. Экспорт патча после коммита в дереве Mesa — команда в конце `build-mesa.sh`.
- Переменные среды на консоли — через `ds2_mesa_env` в TOML (`MESA_SHADER_CACHE_DIR`, `NOUVEAU_HORIZON_BO_CACHE_MB`,
  `NVK_SWITCH_PERF_LOG`, `NVK_SWITCH_SPARSE_BUFFER`).
- Дисковый кэш шейдеров: `sdmc:/switch/ds2/mesa_cache_v33` — первый запуск после новой версии компилирует заново.
