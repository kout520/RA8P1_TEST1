#!/usr/bin/env python3
"""
DTW Template Generator — converts serial MFCC output to templates_dtw.h

Usage:
  1. In collect mode, say each command 3 times.
  2. Save ALL serial output to a file, e.g. recordings.txt
  3. Run:  python gen_templates.py recordings.txt > code/templates_dtw.h

Serial format expected (one template per capture):
    M:<nframes> <c0>,<c1>,...,<c12>, <c0>,<c1>,...,<c12>, ...

    Example:
    M:193 -6.067126,1.584945,0.351509,... -6.544822,1.640043,...

Each template block starts with "M:<nframes>" followed by the CSV data.
Multiple templates can appear sequentially in the file.
"""

import sys
import re
import os

# ── Config ──────────────────────────────────────────────────────────

CLASS_NAMES = ["DA_KA", "KE_LE", "HONG_BAO_LAI", "NIU_NAI", "XIN_XI"]
TEMPLATES_PER_CLASS = 7
N_MFCC = 13
MAX_FRAMES = 250  # DTW_MAX_TMPL_FRAMES

# ── Parse ───────────────────────────────────────────────────────────

def parse_recordings(text):
    """Extract all (nframes, [float...]) tuples from the serial output."""
    templates = []
    # Match "M:<nframes>" followed by CSV floats
    # Normalize: strip \r, split on \n (handle serial \r\n endings)
    text = text.replace('\r\n', '\n').replace('\r', '\n')
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        stripped = lines[i].strip()
        m = re.match(r'^M:(\d+)\s+(.*)', stripped)
        if m:
            nf = int(m.group(1))
            data_str = m.group(2)
            # data may continue on next lines if wrapped
            while not data_str.strip().endswith(',') and data_str.count(',') < (nf * N_MFCC - 1):
                floats_found = len(data_str.split(','))
                if floats_found >= nf * N_MFCC:
                    break
                # Try joining next line if this one is incomplete
                if nf * N_MFCC - floats_found > 20:  # probably wrapped
                    i += 1
                    if i < len(lines):
                        data_str += ' ' + lines[i].strip()
                    else:
                        break
                else:
                    break
            floats = [float(x) for x in data_str.replace(',', ' ').split()]
            expected = nf * N_MFCC
            if len(floats) >= expected:
                templates.append((nf, floats[:expected]))
            else:
                print(f"// WARNING: template expected {expected} floats, got {len(floats)}, skipping",
                      file=sys.stderr)
        i += 1
    return templates

# ── Generate Header ─────────────────────────────────────────────────

def generate_header(templates):
    """Generate the full templates_dtw.h content."""
    n_total = len(templates)
    n_expected = len(CLASS_NAMES) * TEMPLATES_PER_CLASS

    if n_total == 0:
        print("// ERROR: no templates found in input!", file=sys.stderr)
        sys.exit(1)

    if n_total < n_expected:
        print(f"// WARNING: found {n_total} templates, expected {n_expected}",
              file=sys.stderr)

    # Organize by class
    class_templates = [[] for _ in CLASS_NAMES]
    for idx, (nf, data) in enumerate(templates):
        cls = idx // TEMPLATES_PER_CLASS
        if cls >= len(CLASS_NAMES):
            cls = len(CLASS_NAMES) - 1
        class_templates[cls].append((nf, data))

    lines = []
    lines.append("// DTW Templates from MCU-computed MFCC (auto-generated)")
    lines.append(f"// Commands: {', '.join(CLASS_NAMES)}")
    lines.append(f"// {n_total} templates recorded")
    lines.append("")
    lines.append("#ifndef TEMPLATES_DTW_H")
    lines.append("#define TEMPLATES_DTW_H")
    lines.append("")
    lines.append(f"#define DTW_N_CLASSES    {len(CLASS_NAMES)}")
    lines.append(f"#define DTW_N_TEMPLATES  {TEMPLATES_PER_CLASS}")
    lines.append(f"#define DTW_N_MFCC       {N_MFCC}")
    lines.append(f"#define DTW_THRESHOLD    0.180000f")
    lines.append(f"#define DTW_MAX_TMPL_FRAMES {MAX_FRAMES}")
    lines.append("")
    lines.append("static const char *g_dtw_names[DTW_N_CLASSES] = {")
    lines.append("    " + ", ".join(f'"{n}"' for n in CLASS_NAMES))
    lines.append("};")
    lines.append("")

    # Frame counts
    lines.append("static const int dtw_tmpl_frames[DTW_N_CLASSES][DTW_N_TEMPLATES] = {")
    for cls_idx, tpls in enumerate(class_templates):
        counts = []
        for nf, _ in tpls:
            counts.append(str(nf))
        while len(counts) < TEMPLATES_PER_CLASS:
            counts.append("0")
        lines.append(f"    {{{', '.join(counts)}}},  // {CLASS_NAMES[cls_idx]}")
    lines.append("};")
    lines.append("")

    # Template data
    tpl_idx = 0
    for cls_idx, tpls in enumerate(class_templates):
        for t_idx, (nf, data) in enumerate(tpls):
            name = f"dtw_tmpl_{cls_idx}_{t_idx}"
            lines.append(f"// {CLASS_NAMES[cls_idx]} template {t_idx}: {nf} frames")
            lines.append(f"static const float {name}[DTW_MAX_TMPL_FRAMES][DTW_N_MFCC] = {{")
            for f in range(nf):
                coeffs = ', '.join(f"{data[f * N_MFCC + k]:.6f}f" for k in range(N_MFCC))
                lines.append(f"    {{{coeffs}}},")
            lines.append("};")
            lines.append("")
            tpl_idx += 1

    # Template pointer array
    lines.append("static const float (*dtw_templates[DTW_N_CLASSES][DTW_N_TEMPLATES])[DTW_N_MFCC] = {")
    for cls_idx in range(len(CLASS_NAMES)):
        ptrs = []
        for t_idx in range(TEMPLATES_PER_CLASS):
            ptrs.append(f"dtw_tmpl_{cls_idx}_{t_idx}")
        lines.append(f"    {{{', '.join(ptrs)}}},")
    lines.append("};")
    lines.append("")
    lines.append("#endif")

    return '\n'.join(lines)

# ── Main ────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {os.path.basename(sys.argv[0])} <recordings.txt> [> code/templates_dtw.h]")
        print("")
        print("  recordings.txt: copy-paste serial output from template collection mode")
        print("  Output goes to stdout — redirect to templates_dtw.h")
        sys.exit(1)

    with open(sys.argv[1], 'r', encoding='utf-8') as f:
        text = f.read()

    templates = parse_recordings(text)
    header = generate_header(templates)
    print(header)

if __name__ == '__main__':
    main()
