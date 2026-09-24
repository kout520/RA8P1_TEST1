#!/usr/bin/env python3
"""分析 recordings.txt 每条模板的 C0 均值(能量), 判断是真实语音还是噪声/静音。
真人语音 C0 均值应在 -3 ~ -11 之间; 静音/底噪 C0 会 < -11 (能量极低)。"""
import sys

NAMES = ["DA_KA", "KE_LE", "HONG_BAO_LAI", "NIU_NAI", "XIN_XI"]

def parse(path):
    commands = []
    current = None
    with open(path, 'r', encoding='utf-8') as f:
        for raw in f:
            s = raw.strip()
            if not s:
                continue
            if s.startswith('M:'):
                head, _, body = s.partition(' ')
                n = int(head[2:])
                vals = [float(x) for x in body.split(',') if x.strip() != '']
                current.append((n, vals))
            else:
                current = []
                commands.append(current)
    return commands

def main():
    commands = parse('recordings.txt')
    for ci, cmd in enumerate(commands):
        name = NAMES[ci]
        print(f"\n===== {name} ({len(cmd)} 条) =====")
        for ti, (n, vals) in enumerate(cmd):
            c0s = vals[0::13]  # 每帧的第0个系数 = C0
            c0_mean = sum(c0s) / len(c0s)
            c0_min = min(c0s)
            c0_max = max(c0s)
            # 判断: 语音 C0 在 -3~-11, 噪声 < -11 或 > -3
            if c0_mean > -3 or c0_mean < -11:
                flag = "  <<< 异常(疑似噪声/静音)"
            else:
                flag = ""
            who = "第1人" if ti < 7 else "第2人"
            print(f"  [{ti}] {who} 帧={n:3d}  C0均值={c0_mean:8.3f}  min={c0_min:8.3f}  max={c0_max:8.3f}{flag}")

if __name__ == '__main__':
    main()
