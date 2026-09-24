#!/usr/bin/env python3
"""解析 recordings.txt, 生成 code/templates_dtw.h (混合第1人7条+第2人3条/命令)"""
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
                if len(vals) != n * 13:
                    print(f"[错误] 帧数{n} 但 float 数 {len(vals)}", file=sys.stderr)
                    sys.exit(1)
                current.append((n, vals))
            else:
                # 命令标签行 (Da_kai / Ke_le / ...)
                current = []
                commands.append(current)
    return commands

def gen(commands):
    N = len(commands[0])
    L = []
    L.append("// DTW Templates from MCU-computed MFCC (auto-generated)")
    L.append("// Commands: DA_KA, KE_LE, HONG_BAO_LAI, NIU_NAI, XIN_XI")
    L.append("// %d templates recorded (%d per command)" % (5 * N, N))
    L.append("")
    L.append("#ifndef TEMPLATES_DTW_H")
    L.append("#define TEMPLATES_DTW_H")
    L.append("")
    L.append("#define DTW_N_CLASSES    5")
    L.append("#define DTW_N_TEMPLATES  %d" % N)
    L.append("#define DTW_N_MFCC       13")
    L.append("#define DTW_THRESHOLD    0.180000f")
    L.append("#define DTW_MAX_TMPL_FRAMES 250")
    L.append("")
    L.append("static const char *g_dtw_names[DTW_N_CLASSES] = {")
    L.append("    " + ", ".join('"%s"' % n for n in NAMES))
    L.append("};")
    L.append("")
    L.append("static const int dtw_tmpl_frames[DTW_N_CLASSES][DTW_N_TEMPLATES] = {")
    for c in range(5):
        frames = ", ".join(str(commands[c][t][0]) for t in range(N))
        L.append("    {%s},  // %s" % (frames, NAMES[c]))
    L.append("};")
    L.append("")
    for c in range(5):
        for t in range(N):
            n, vals = commands[c][t]
            L.append("// %s template %d: %d frames" % (NAMES[c], t, n))
            L.append("static const float dtw_tmpl_%d_%d[DTW_MAX_TMPL_FRAMES][DTW_N_MFCC] = {" % (c, t))
            for fr in range(n):
                row = ", ".join("%.6ff" % v for v in vals[fr*13:(fr+1)*13])
                L.append("    {" + row + "},")
            L.append("};")
            L.append("")
    L.append("static const float (*dtw_templates[DTW_N_CLASSES][DTW_N_TEMPLATES])[DTW_N_MFCC] = {")
    for c in range(5):
        ptrs = ", ".join("dtw_tmpl_%d_%d" % (c, t) for t in range(N))
        L.append("    {" + ptrs + "},")
    L.append("};")
    L.append("")
    L.append("#endif")
    L.append("")
    return "\n".join(L)

def main():
    commands = parse('recordings.txt')
    # 跳过第一个标签 da_bai(唤醒词), 只取 5 个指令
    commands = commands[1:]
    if len(commands) != 5:
        print(f"[错误] 命令数 {len(commands)}, 应为 5", file=sys.stderr)
        sys.exit(1)
    # 只取每命令前 7 条 (第一个人), 第二个人的 3 条暂不加入
    commands = [c[:7] for c in commands]
    for i, cmd in enumerate(commands):
        if len(cmd) != 7:
            print(f"[错误] 命令 {NAMES[i]} 模板数 {len(cmd)}, 应为 7", file=sys.stderr)
            sys.exit(1)
    # 打印每命令帧数概览
    for i, cmd in enumerate(commands):
        frames = [c[0] for c in cmd]
        print(f"{NAMES[i]}: {len(cmd)} 条, 帧数 {frames}")
    header = gen(commands)
    with open('code/templates_dtw.h', 'w', encoding='utf-8') as f:
        f.write(header)
    print("生成完成 -> code/templates_dtw.h")

if __name__ == '__main__':
    main()
