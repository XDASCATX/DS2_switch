#!/usr/bin/env python3
"""Hot functions and hot call stacks of one thread in a console rex_perfil.log, symbolized with the release ELF.

rex_perfil.log (sdk/src/ui/switch_perf.cpp) has one block every 10 s ("==== 10.0 s | ... juego N fps ...") and, per
thread, "-- hilo ID "name": CPU x%" followed by "N muestras" and lines "  p%  pc imagen+0x...", "lr ...", "pila a <- b
<- ...". This tool sums the pc samples (weighted by each block's sample count) over a range of blocks and resolves
them with aarch64-none-elf-addr2line.

Usage:
  threads.py <rex_perfil.log> <ds2.elf> [--thread "GPU Commands"] [--blocks 8-20] [--stacks] [--top 25]
  threads.py <rex_perfil.log> --list            # blocks with their FPS and CPU, to choose --blocks

Example (v55): threads.py rex_perfil.log out/app-switch/releases/v55/ds2.elf --thread "Main XThread" --blocks 7-19
"""
import argparse
import collections
import os
import re
import subprocess

A2L = os.path.join(os.environ.get('DEVKITPRO', '/opt/devkitpro'), 'devkitA64', 'bin', 'aarch64-none-elf-addr2line')


def blocks(path):
    cur = None
    for line in open(path, encoding='utf-8', errors='ignore'):
        if line.startswith('==== '):
            if cur:
                yield cur
            cur = {'head': line.strip(), 'lines': []}
        elif cur is not None:
            cur['lines'].append(line.rstrip('\n'))
    if cur:
        yield cur


def symbolize(addrs, elf):
    addrs = sorted(set(addrs))
    out = {}
    if not addrs:
        return out
    res = subprocess.run([A2L, '-f', '-C', '-e', elf] + ['0x' + a for a in addrs], capture_output=True, text=True)
    lines = res.stdout.splitlines()
    for i, a in enumerate(addrs):
        name = lines[2 * i] if 2 * i < len(lines) else '?'
        out[a] = re.sub(r'\(.*', '', name)[:90]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log')
    ap.add_argument('elf', nargs='?')
    ap.add_argument('--thread', default='Main XThread')
    ap.add_argument('--blocks', default='')
    ap.add_argument('--stacks', action='store_true')
    ap.add_argument('--top', type=int, default=25)
    ap.add_argument('--list', action='store_true')
    a = ap.parse_args()
    bl = list(blocks(a.log))
    if a.list:
        for i, b in enumerate(bl):
            m = re.search(r'juego ([\d.]+) fps \| CPU total (\d+)%', b['head'])
            clk = next((l for l in b['lines'] if 'relojes:' in l), '')
            clk = re.sub(r'.*relojes: ', '', clk)[:60]
            print(f"{i:3d}  fps {m.group(1) if m else '?':>6}  cpu {m.group(2) if m else '?':>4}%  {clk}")
        return
    lo, hi = 0, len(bl) - 1
    if a.blocks:
        lo, hi = (int(x) for x in a.blocks.split('-'))
    pcs, stacks, cpu = collections.Counter(), collections.Counter(), []
    for b in bl[lo:hi + 1]:
        inside, ns = False, 0
        for line in b['lines']:
            if line.startswith('-- hilo'):
                inside = a.thread in line
                if inside:
                    m = re.search(r'CPU ([\d.]+)%', line)
                    if m:
                        cpu.append(float(m.group(1)))
                continue
            if not inside:
                continue
            m = re.match(r'^\s+(\d+) muestras', line)
            if m:
                ns = int(m.group(1))
                continue
            m = re.match(r'^\s+([\d.]+)%\s+pc imagen\+0x([0-9a-f]+)', line)
            if m:
                pcs[m.group(2)] += float(m.group(1)) * ns / 100
                continue
            m = re.match(r'^\s+([\d.]+)%\s+pila (.*)', line)
            if m and a.stacks:
                addrs = re.findall(r'imagen\+0x([0-9a-f]+)', m.group(2))[:7]
                stacks[' '.join(addrs)] += float(m.group(1)) * ns / 100
    if not a.elf:
        raise SystemExit('ELF needed (out/app-switch/releases/vNN/ds2.elf of the same build)')
    names = symbolize(list(pcs), a.elf)
    tot = sum(pcs.values()) or 1
    byfn = collections.Counter()
    for addr, w in pcs.items():
        byfn[names.get(addr, '?')] += w
    print(f"thread '{a.thread}', blocks {lo}-{hi}: CPU {sum(cpu) / max(len(cpu), 1):.1f}% average, "
          f"{tot:.0f} weighted samples")
    for fn, w in byfn.most_common(a.top):
        print(f"{100 * w / tot:6.1f}%  {fn}")
    if a.stacks:
        sn = symbolize([x for k in stacks for x in k.split()], a.elf)
        stot = sum(stacks.values()) or 1
        print('\nstacks (innermost first):')
        for k, w in stacks.most_common(a.top):
            print(f"{100 * w / stot:6.1f}%  " + ' < '.join(sn.get(x, '?')[:40] for x in k.split()))


if __name__ == '__main__':
    main()
