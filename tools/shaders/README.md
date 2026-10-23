# Шейдеры DS2 через XenosRecomp (исследование, не часть сборки)

- `drv.cpp` — драйвер: обходит папку `.fpo/.vpo` (распакованные `shader/*.bnd`, см. `tools/analysis/bnd4.py`) и пишет HLSL
  через XenosRecomp (hedge-dev или форк nfsmw-nx `NFSMW_RECOMP`). Сборка: `g++ -std=c++20 -I<XenosRecomp>/XenosRecomp
  -Istub drv.cpp <XenosRecomp>/XenosRecomp/shader_recompiler.cpp -o drv`.
- `compile_all.sh <file.hlsl>` — HLSL → SPIR-V через DXC (Linux-бинарь из renderbag/dxc-bin).
- `stub/` — заглушки заголовков, чтобы собрать только переводчик без зависимостей.
- Результаты (2271/2275 HLSL, 2166 SPIR-V) — `docs/handoff/18-native-renderer-study.md`. Сами шейдеры — данные игры, не коммитить.
