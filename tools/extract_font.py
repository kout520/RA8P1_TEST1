#!/usr/bin/env python3
"""从优化.md 提取 16x16 中文字模, 按出现顺序生成 font16.h (列行式, 每字32字节)"""
import re

with open('优化.md', encoding='utf-8') as f:
    text = f.read()

# 匹配: 两行 16 字节 + 注释 /*"字",索引*/ (忽略索引, 按出现顺序)
pattern = re.compile(r'\{([^}]+)\},\s*\n\s*\{([^}]+)\},/\*"([^"]+)",(\d+)\*/')

chars = []
for m in pattern.finditer(text):
    b1 = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(1))]
    b2 = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(2))]
    ch = m.group(3)
    if len(b1) == 16 and len(b2) == 16:
        chars.append((ch, b1 + b2))

print(f"提取到 {len(chars)} 个中文字模")

lines = []
lines.append("/* 16x16 中文字模 (列行式, 每字32字节: 前16=上半, 后16=下半) */")
lines.append("/* 由 tools/extract_font.py 从 优化.md 自动生成, 顺序索引 */")
lines.append("#ifndef FONT16_H")
lines.append("#define FONT16_H")
lines.append("")
lines.append("#include <stdint.h>")
lines.append("")
lines.append("/* 索引顺序对应字: " + " ".join(ch for ch, _ in chars) + " */")
lines.append("static const uint8_t font16[][32] = {")
for i, (ch, data) in enumerate(chars):
    hex_str = ",".join(f"0x{b:02X}" for b in data)
    lines.append(f"    {{{hex_str}}}, /* {i}:{ch} */")
lines.append("};")
lines.append("")
lines.append(f"#define FONT16_NUM  {len(chars)}")
lines.append("")
lines.append("#endif")

with open('code/font16.h', 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')

# 输出索引清单 (方便手动建立中文提示映射)
print("=== 索引清单 ===")
for i, (ch, _) in enumerate(chars):
    print(f"{i}:{ch}", end=" ")
print()
