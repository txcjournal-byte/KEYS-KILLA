#!/usr/bin/env python3
"""Builds the skin bitmaps from the design mockups (assets/src/*.webp).

Removes every highlight that is baked into the mockup (selected tile, era, lit knob arcs, XY dot,
CHAOS thumb, CHORD / EXCLUSIVE glow, meter, preset name) so the plugin can draw the live state,
and cuts key textures for the keyboard. Output goes to assets/*.jpg|png (embedded as BinaryData)."""
import numpy as np
from PIL import Image

SRC, OUT = "assets/src/", "assets/"

SKINS = {
    "blood": dict(
        accent="red",
        rects=[(128, 285, 236, 432), (540, 455, 600, 500), (845, 452, 1072, 502), (1100, 648, 1240, 698),
               (1450, 648, 1510, 782), (1130, 450, 1272, 480), (1318, 298, 1508, 442)],
        knobs=[((x, 650), 50, 82) for x in (166, 330, 494, 655, 816, 977)] + [((x, 540), 27, 46) for x in (1206, 1335, 1461)],
        patches=[((1370, 357), (1450, 357), 30), ((1165, 465), (1206, 465), 11)],
        name_box=(775, 118, 1085, 158),
        white=(244, 884, 262, 930), black=(175, 805, 192, 880)),
    "chrome": dict(
        accent="blue",
        rects=[(128, 290, 230, 442), (170, 455, 235, 512), (825, 458, 1050, 514), (1105, 660, 1228, 714),
               (1450, 652, 1515, 778), (1105, 462, 1262, 492), (1310, 298, 1510, 445)],
        knobs=[((x, 657), 49, 80) for x in (158, 319, 480, 639, 797, 958)] + [((x, 547), 33, 56) for x in (1190, 1329, 1472)],
        patches=[((1360, 367), (1436, 367), 30), ((1150, 478), (1201, 478), 11)],
        name_box=(792, 110, 1068, 154),
        white=(197, 865, 220, 915), black=(185, 798, 200, 872)),
}

def highlight_mask(a, accent):
    r, g, b = a[..., 0].astype(int), a[..., 1].astype(int), a[..., 2].astype(int)
    if accent == "red":
        return (r - np.maximum(g, b)) > 45
    return (b - r) > 28

def desaturate(a, mask):
    lum = (0.30 * a[..., 0] + 0.59 * a[..., 1] + 0.11 * a[..., 2])
    for c in range(3):
        a[..., c] = np.where(mask, lum, a[..., c])

for name, s in SKINS.items():
    img = np.asarray(Image.open(SRC + name + ".webp").convert("RGB")).astype(np.float32)
    H, W = img.shape[:2]
    yy, xx = np.mgrid[0:H, 0:W]
    region = np.zeros((H, W), bool)
    for (x0, y0, x1, y1) in s["rects"]:
        region[y0:y1, x0:x1] = True
    for (cx, cy), r0, r1 in s["knobs"]:
        d = np.hypot(xx - cx, yy - cy)
        region |= (d >= r0) & (d <= r1)
    desaturate(img, region & highlight_mask(img, s["accent"]))
    # soften what is left of the glow so it reads as an unlit track
    for (src, dst, h) in s["patches"]:   # feathered copy of a clean neighbour area
        sx, sy = src; dx, dy = dst
        py, px = np.mgrid[-h:h, -h:h]
        w = np.clip(1.5 - np.hypot(px, py) / h * 1.5 + 0.5, 0, 1)[..., None]
        img[dy - h:dy + h, dx - h:dx + h] = img[sy - h:sy + h, sx - h:sx + h] * w + img[dy - h:dy + h, dx - h:dx + h] * (1 - w)
    x0, y0, x1, y1 = s["name_box"]
    band = np.concatenate([img[y0:y0 + 3, x0:x1], img[y1 - 3:y1, x0:x1]]).reshape(-1, 3).mean(0)
    grad = np.linspace(0.96, 1.04, y1 - y0)[:, None, None]
    img[y0:y1, x0:x1] = np.clip(band[None, None, :] * grad, 0, 255)
    out = Image.fromarray(img.clip(0, 255).astype(np.uint8))
    out.save(OUT + name + "_bg.jpg", quality=90, optimize=True)
    src = Image.open(SRC + name + ".webp").convert("RGB")
    src.crop(s["white"]).save(OUT + name + "_white.png")
    src.crop(s["black"]).save(OUT + name + "_black.png")
    print(name, "done")
