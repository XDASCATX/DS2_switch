#!/usr/bin/env python3
"""Writes docs/handoff/17a-cvars-list.md from the JSON of cvars.py.

Usage: python3 tools/analysis/cvars.py . /tmp/cvars.json && python3 tools/analysis/cvars_table.py /tmp/cvars.json
"""
import datetime
import json
import sys

out = json.load(open(sys.argv[1]))
dest = sys.argv[2] if len(sys.argv) > 2 else 'docs/handoff/17a-cvars-list.md'
icon = {'live': '[live]', 'init-only': '[init-only] (в меню серым)', 'restart': '[restart]'}
lines = ["# 17a. Все параметры (cvar) сборки DS2", "",
         "Сгенерировано из исходников (`REXCVAR_DEFINE_*` в `sdk/src`, `sdk/include`, `app`):",
         "`tools/analysis/cvars.py` + `tools/analysis/cvars_table.py` (см. [docs/config.md](../config.md)). "
         f"Обновлено {datetime.date.today().isoformat()}.",
         "Значок — как его показывает меню настроек (L+R+→): «live» — только метка; меняется ли значение на лету, "
         "зависит от кода.", "",
         "| Категория | Имя | Тип | По умолчанию | Значок | Описание |", "|---|---|---|---|---|---|"]
for o in out:
    if o['name'] in ('my_cmd', 'name', 'my_flag', 'my_int', 'my_string'):  # SDK documentation examples
        continue
    d = o['desc'].replace('|', '/').replace('\n', ' ')
    lines.append(f"| {o['cat'] or '-'} | `{o['name']}` | {o['type'].lower()} | `{o['default'][:24]}` | "
                 f"{icon[o['lc']]} | {d} |")
open(dest, 'w').write('\n'.join(lines) + '\n')
print(f"{len(lines) - 8} cvars -> {dest}")
