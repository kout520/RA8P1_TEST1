#!/usr/bin/env python3
"""跨人模板一致性检验: 第二人的模板 vs 第一人的同命令模板, 算最小 DTW 距离。
距离小(<0.25)说明两人录的是同一个词; 距离大(>0.30)说明第二人录的内容不对(可能是噪声)。"""
import math, sys

NAMES = ["DA_KA", "KE_LE", "HONG_BAO_LAI", "NIU_NAI", "XIN_XI"]

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
    print("命令        | 第二人模板 vs 第一人模板 的最小DTW距离 (越小越像同一个词)")
    print("            |   [7]      [8]      [9]")
    for ci, cmd in enumerate(commands):
        name = NAMES[ci]
        p1 = cmd[:7]   # 第一人 7 条
        p2 = cmd[7:]   # 第二人 3 条
        # 预归一化第一人的 7 条
        p1n = [norm(v, n) for (n, v) in p1]
        dists = []
        for (n2, v2) in p2:
            v2n = norm(v2, n2)
            best = 1e9
            for v1n in p1n:
                d = dtw_std(v2n, v1n)
                if d < best: best = d
            dists.append(best)
        flag = ""
        if min(dists) > 0.30:
            flag = "  <<< 跨人距离大, 第二人内容可疑(可能录错/录了噪声)"
        print(f"{name:11s} | {dists[0]:.3f}  {dists[1]:.3f}  {dists[2]:.3f}{flag}")

if __name__ == '__main__':
    main()
