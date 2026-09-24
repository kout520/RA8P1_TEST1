#!/usr/bin/env python3
"""解析 recordings.txt 的 da_bai(唤醒词, 第一个标签) 数据, 生成 code/wake_word.h"""
import sys

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
                if len(vals) != n * 13:
                    print(f"[错误] 帧数{n} float数{len(vals)}", file=sys.stderr)
                    sys.exit(1)
                current.append((n, vals))
            else:
                current = []
                commands.append(current)
    return commands

def gen(tmpls):
    N = len(tmpls)
    L = []
    L.append("// Wake word 大白 templates (auto-generated)")
    L.append("#ifndef WAKE_WORD_H")
    L.append("#define WAKE_WORD_H")
    L.append("")
    L.append("#define WAKE_N_TEMPLATES   %d" % N)
    L.append("#define WAKE_MAX_TMPL_FRAMES 250")
    L.append("")
    L.append("static const int wake_tmpl_frames[WAKE_N_TEMPLATES] = {")
    L.append("    " + ", ".join(str(t[0]) for t in tmpls))
    L.append("};")
    L.append("")
    for t in range(N):
        n, vals = tmpls[t]
        L.append("// 大白 template %d: %d frames" % (t, n))
        L.append("static const float wake_tmpl_%d[WAKE_MAX_TMPL_FRAMES][13] = {" % t)
        for fr in range(n):
            row = ", ".join("%.6ff" % v for v in vals[fr*13:(fr+1)*13])
            L.append("    {" + row + "},")
        L.append("};")
        L.append("")
    L.append("static const float (*wake_templates[WAKE_N_TEMPLATES])[13] = {")
    L.append("    " + ", ".join("wake_tmpl_%d" % t for t in range(N)))
    L.append("};")
    L.append("")
    L.append("#endif")
    L.append("")
    return "\n".join(L)

def main():
    commands = parse('recordings.txt')
    if len(commands) < 1:
        print("[错误] 没有数据", file=sys.stderr)
        sys.exit(1)
    wake = commands[0]  # 第一个标签 da_bai (唤醒词)
    print(f"唤醒词 大白: {len(wake)} 条, 帧数 {[t[0] for t in wake]}")
    with open('code/wake_word.h', 'w', encoding='utf-8') as f:
        f.write(gen(wake))
    print("生成完成 -> code/wake_word.h")

if __name__ == '__main__':
    main()
