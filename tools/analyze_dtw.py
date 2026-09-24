#!/usr/bin/env python3
"""DTW 距离分析: 模板内部一致性 + 指令间区分度"""
import sys, re, math

CLASS = ["DA_KA", "KE_LE", "HONG_BAO_LAI", "NIU_NAI", "XIN_XI"]
N_MFCC = 13
T_PER = 7

def parse(path):
    tmpl = [[] for _ in CLASS]
    with open(path) as f:
        text = f.read()
    text = text.replace('\r\n', '\n').replace('\r', '\n')
    i = 0
    for line in text.split('\n'):
        m = re.match(r'^M:(\d+)\s+(.*)', line.strip())
        if not m:
            continue
        nf = int(m.group(1))
        vals = [float(x) for x in m.group(2).replace(',', ' ').split()]
        vals = vals[:nf * N_MFCC]
        if len(vals) < nf * N_MFCC:
            continue
        cls = len([c for c in tmpl if c])  # 按填充顺序分配
        # 实际按顺序分配更可靠
        for c in range(len(CLASS)):
            if len(tmpl[c]) < T_PER:
                tmpl[c].append((nf, vals))
                break
    return tmpl

def norm(seq, nf):
    out = []
    for f in range(nf):
        fr = seq[f*N_MFCC:(f+1)*N_MFCC]
        s = math.sqrt(sum(x*x for x in fr) + 1e-10)
        out.append([x/s for x in fr])
    return out

def dtw(a, b, na, nb):
    # 标准 DTW, 归一化
    buf = [[0.0]*nb for _ in range(2)]
    def d(i, j):
        aa = a[i]; bb = b[j]
        return 1.0 - sum(aa[k]*bb[k] for k in range(N_MFCC))
    buf[0][0] = d(0, 0)
    for j in range(1, nb):
        buf[0][j] = buf[0][j-1] + d(0, j)
    cur, prev = 1, 0
    for i in range(1, na):
        buf[cur][0] = buf[prev][0] + d(i, 0)
        for j in range(1, nb):
            c = d(i, j)
            m = min(buf[prev][j], buf[cur][j-1], buf[prev][j-1])
            buf[cur][j] = c + m
        cur, prev = prev, cur
    return buf[prev][nb-1] / (na + nb)

def main():
    tmpl = parse(sys.argv[1])
    # 内部一致性 (每类 7 条两两)
    print("=== 模板内部一致性 (同类 7 条两两 DTW 距离) ===")
    for c, name in enumerate(CLASS):
        dists = []
        for a in range(T_PER):
            for b in range(a+1, T_PER):
                na, va = tmpl[c][a]; nb, vb = tmpl[c][b]
                na_n = norm(va, na); nb_n = norm(vb, nb)
                dists.append(dtw(na_n, nb_n, na, nb))
        if dists:
            print(f"{name:14s} 平均={sum(dists)/len(dists):.3f}  min={min(dists):.3f}  max={max(dists):.3f}  frames={[t[0] for t in tmpl[c]]}")

    # 指令间区分度 (每类模板 0 vs 其他类模板 0)
    print("\n=== 指令间 DTW 距离 (模板0 vs 模板0) ===")
    for c1 in range(len(CLASS)):
        row = []
        for c2 in range(len(CLASS)):
            if c1 == c2:
                row.append("  --  ")
                continue
            na, va = tmpl[c1][0]; nb, vb = tmpl[c2][0]
            na_n = norm(va, na); nb_n = norm(vb, nb)
            row.append(f"{dtw(na_n, nb_n, na, nb):.3f}")
        print(f"{CLASS[c1]:14s} " + "  ".join(row))
    print("(列顺序: " + " ".join(CLASS) + ")")

main()
