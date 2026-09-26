#!/usr/bin/env python3
"""Loudness-match factory presets: runs KeysKillaTests -cal and writes the output gain into Source/Presets.cpp."""
import re, subprocess, sys
exe, target = sys.argv[1], -15.0
out = subprocess.run([exe, "-cal"], capture_output=True, text=True).stdout
src = open("Source/Presets.cpp").read()
for line in out.strip().splitlines():
    name, g, win = line.split("|"); g, win = float(g), float(win)
    new = max(-24.0, min(12.0, round(g + (target - win), 1)))
    m = re.search(r'[PB] \("' + re.escape(name) + '"', src)
    end = src.index("});", m.start())
    block = src[m.start():end]
    if re.search(r"\{ gain, [-0-9.]+f? \}", block):
        block = re.sub(r"\{ gain, [-0-9.]+f? \}", "{ gain, %.1ff }" % new, block)
    else:
        block = block.rstrip() + (", { gain, %.1ff } " % new)
    src = src[:m.start()] + block + src[end:]
    print(f"{name:28s} win {win:6.1f} dB  gain {g:+.1f} -> {new:+.1f}")
open("Source/Presets.cpp", "w").write(src)
