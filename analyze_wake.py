#!/usr/bin/env python3
"""分析唤醒词"大白"模板: C0 均值 + 模板自匹配 DTW 距离, 判断 WAKE_THRESHOLD 是否合适"""
import math, sys

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

def dot(a, b): return sum(x*y for x, y in zip(a, b))

def norm(seq, n):
    out = []
    for f in range(n):
        fr = seq[f*13:(f+1)*13]
        s = sum(x*x for x in fr)
        inv = 1.0 / math.sqrt(s + 1e-10)
        out.append([x*inv for x in fr])
    return out

def dtw_std(qn, tn):
    q_len, t_len = len(qn), len(tn)
    INF = 1e9
    dp = [[INF]*t_len for _ in range(q_len)]
    dp[0][0] = 1.0 - dot(qn[0], tn[0])
    for j in range(1, t_len): dp[0][j] = dp[0][j-1] + (1.0 - dot(qn[0], tn[j]))
    for i in range(1, q_len):
        dp[i][0] = dp[i-1][0] + (1.0 - dot(qn[i], tn[0]))
        for j in range(1, t_len):
            c = 1.0 - dot(qn[i], tn[j])
            dp[i][j] = c + min(dp[i-1][j], dp[i][j-1], dp[i-1][j-1])
    return dp[q_len-1][t_len-1] / (q_len + t_len)

def main():
    commands = parse('recordings.txt')
    wake = commands[0]
    print("=== 大白模板 C0 均值 ===")
    for i, (n, vals) in enumerate(wake):
        c0s = vals[0::13]
        c0_mean = sum(c0s)/len(c0s)
        flag = "  <<< C0异常(会被能量检查拒)" if (c0_mean > -3 or c0_mean < -11) else ""
        print(f"  [{i}] 帧={n:3d} C0均值={c0_mean:8.3f}{flag}")
    print("=== 大白模板 两两自匹配 DTW 距离 ===")
    norms = [norm(v, n) for (n, v) in wake]
    N = len(wake)
    dmin, dmax = 1e9, 0
    for i in range(N):
        for j in range(i+1, N):
            d = dtw_std(norms[i], norms[j])
            dmin = min(dmin, d); dmax = max(dmax, d)
            print(f"  [{i}]-[{j}] d={d:.3f}")
    print(f"  自匹配距离范围: {dmin:.3f} ~ {dmax:.3f}")
    print(f"  建议 WAKE_THRESHOLD > {dmax:.3f} (否则自匹配都唤醒不了)")

if __name__ == '__main__':
    main()
