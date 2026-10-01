#!/usr/bin/env python3
"""Builds the BREED LAB skin (v0.6) from the design (assets/src/breed.webp, 1672 x 941).

Everything the plugin draws live is removed from the bitmap: preset name, parent icons / names,
children waveforms and stars, lit gene switches, MUTATE / tab / era / knob / meter highlights.
Also cuts the key textures and the category icon strip (from the v0.5 tile art).
Output: assets/lab_bg.jpg, lab_white.png, lab_black.png, lab_icons.png (embedded as BinaryData)."""
import numpy as np
from PIL import Image

SRC, OUT = "assets/src/", "assets/"
img = np.asarray(Image.open(SRC + "breed.webp").convert("RGB")).astype(np.float32)
H, W = img.shape[:2]
yy, xx = np.mgrid[0:H, 0:W]
rng = np.random.default_rng(7)


def inpaint(x0, y0, x1, y1, iters=400):
    """fill a box from its border (diffusion) and add matching grain"""
    pad = 3
    a = img[y0 - pad:y1 + pad, x0 - pad:x1 + pad].copy()
    ring = np.concatenate([a[:pad].reshape(-1, 3), a[-pad:].reshape(-1, 3), a[:, :pad].reshape(-1, 3), a[:, -pad:].reshape(-1, 3)])
    grain = ring.std(0).mean() * 0.35
    small = a[::4, ::4].copy()
    inner = np.zeros(small.shape[:2], bool); inner[1:-1, 1:-1] = True
    small[inner] = ring.mean(0)
    for _ in range(iters):
        avg = (np.roll(small, 1, 0) + np.roll(small, -1, 0) + np.roll(small, 1, 1) + np.roll(small, -1, 1)) / 4
        small[inner] = avg[inner]
    big = np.asarray(Image.fromarray(small.clip(0, 255).astype(np.uint8)).resize((a.shape[1], a.shape[0]), Image.BICUBIC)).astype(np.float32)
    noise = rng.normal(0, grain, big.shape[:2])[..., None]
    img[y0:y1, x0:x1] = (big[pad:-pad, pad:-pad] + noise[pad:-pad, pad:-pad]).clip(0, 255)


def red_mask(a, t=40):
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    return (r - np.maximum(g, b)) > t


def unlight(region, dim=0.85, t=40):
    """turn red glow into the grey 'off' state inside a boolean region (bright captions stay)"""
    m = region & red_mask(img, t) & (np.maximum(img[..., 1], img[..., 2]) < 120)
    lum = (0.30 * img[..., 0] + 0.59 * img[..., 1] + 0.11 * img[..., 2]) * dim
    for c in range(3):
        img[..., c] = np.where(m, lum, img[..., c])


def box(x0, y0, x1, y1):
    m = np.zeros((H, W), bool); m[y0:y1, x0:x1] = True; return m


# header preset name
inpaint(700, 47, 1042, 83)
# parents: icon, name, tags (between the arrows, below the PARENT label)
inpaint(268, 176, 545, 368)
inpaint(360, 140, 545, 178)
inpaint(1128, 176, 1372, 368)
inpaint(1180, 140, 1370, 178)
# children: waveform + stars, selected frame
for x0 in (220, 422, 623, 827, 1030, 1233):
    inpaint(x0 + 12, 414, x0 + 144, 482)
    inpaint(x0 + 36, 481, x0 + 140, 501)
# the selected card 3 frame: copy card 2 over it (keep the CHILD 3 caption)
card = img[378:518, 416:618].copy()
cap = img[394:417, 655:785].copy()
img[378:518, 617:819] = card
img[394:417, 655:785] = cap
unlight(box(612, 378, 824, 518))
unlight(box(650, 392, 790, 418), 0.35, 10)
# gene switches, MUTATE 30 %, era column, SOUND tab, output meter
for cx in (311, 441, 572, 697, 830, 952):
    unlight(box(cx - 42, 556, cx + 42, 590), 0.7)
unlight(box(1138, 551, 1192, 598), 0.5, 15)
unlight(box(1474, 150, 1506, 392), 0.8)
# SOUND tab: take the unlit MOD tab around the caption
unlight(box(255, 612, 480, 660), 0.35, 10)
unlight(box(1330, 712, 1600, 765), 0.7)
# v0.7: the ERA column becomes the WILD rail (drawn by the plugin), the 808 tab becomes ARP
inpaint(1466, 120, 1608, 392)
inpaint(1196, 626, 1290, 650)
# v0.14: the bottom row becomes the module bar (808 SNARE CLAP ROLLS HALF EFFECTOR DIGGA, drawn by the plugin)
for x0, x1 in ((55, 252), (262, 472), (480, 690), (700, 907), (916, 1127)):
    cx = (x0 + x1) // 2
    inpaint(cx - 72, 626, cx + 72, 651)
# knob value arcs
knobs = [((x, 715), 30, 58) for x in (132, 284, 435, 587, 742, 895, 1047, 1202)] + [((1532, 447), 34, 66), ((1490, 552), 21, 40), ((1575, 552), 21, 40)]
ring = np.zeros((H, W), bool)
for (cx, cy), r0, r1 in knobs:
    d = np.hypot(xx - cx, yy - cy)
    ring |= (d >= r0) & (d <= r1)
unlight(ring, 0.8)

Image.fromarray(img.clip(0, 255).astype(np.uint8)).save(OUT + "lab_bg.jpg", quality=90, optimize=True)
src = Image.open(SRC + "breed.webp").convert("RGB")
src.crop((1119, 882, 1139, 920)).save(OUT + "lab_white.png")
src.crop((1033, 820, 1050, 872)).save(OUT + "lab_black.png")

# category icons: the ten v0.5 tile pictures (blood skin), 100 x 104 each
old = Image.open(SRC + "blood.webp").convert("RGB")
tiles = [(43, 127), (133, 231), (237, 330), (336, 433), (440, 539), (546, 649), (654, 751), (757, 856), (862, 960), (966, 1076)]
strip = Image.new("RGBA", (100 * len(tiles), 104))
fy, fx = np.mgrid[0:104, 0:100]
fade = np.clip(1.6 - np.hypot((fx - 50) / 44.0, (fy - 52) / 50.0) * 1.6, 0, 1)   # soft oval: no tile frame edges
for i, (x0, x1) in enumerate(tiles):
    cx = (x0 + x1) // 2
    icon = np.asarray(old.crop((cx - 50, 293, cx + 50, 397))).astype(np.float32)
    lum = icon.mean(2, keepdims=True)
    icon = np.where(red_mask(icon, 30)[..., None], lum, icon)          # no old selection glow
    alpha = (fade * 255)[..., None]
    strip.paste(Image.fromarray(np.concatenate([icon, alpha], 2).clip(0, 255).astype(np.uint8), "RGBA"), (i * 100, 0))
strip.save(OUT + "lab_icons.png")
print("lab skin done")
