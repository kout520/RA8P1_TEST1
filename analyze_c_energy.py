#!/usr/bin/env python3
"""分析指令模板的 C1~C12 每帧平方和均值, 判断 speaker_verify 的 <20 阈值是否误拒"""
import sys

NAMES = ["DA_BAI", "DA_KA", "KE_LE", "HONG_BAO_LAI", "NIU_NAI", "XIN_XI"]

def parse(path):
    commands, current = [], None
    with open(path, 'r', encoding='utf-8') as f:
        for raw in f:
            s = raw.strip()
            if not s: continue
            if s.startswith('M:'):
                head, _, body = s.partition(' ')
                n = int(head[2:])
                vals = [float(x) for x in body.split(',') if x.strip() != '']
                current.append((n, vals))
            else:
                current = []; commands.append(current)
    return commands

def main():
    commands = parse('recordings.txt')
    for ci, cmd in enumerate(commands):
        name = NAMES[ci] if ci < len(NAMES) else f"CMD{ci}"
        print(f"\n===== {name} ({len(cmd)}条) =====")
        for ti, (n, vals) in enumerate(cmd):
            # C1~C12 每帧平方和均值
            energy = 0.0
            for f in range(n):
                for k in range(1, 13):
                    v = vals[f*13 + k]
                    energy += v * v
            energy /= n
            flag = "  <<< C1~C12能量<20, 会被 speaker_verify 误拒!" if energy < 20 else ""
            print(f"  [{ti}] 帧={n:3d} C1~C12能量={energy:8.1f}{flag}")

if __name__ == '__main__':
    main()
