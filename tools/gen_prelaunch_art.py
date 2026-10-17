#!/usr/bin/env python3
"""Draws the backdrop of the start screen: our own art (no game assets), a starry night over a magic school's
rooftops, with a large crescent moon and a trail of sparkles on the right, leaving the left for the menu. Writes
game/ui/res/prelaunch-bg.png (1920x1080). Needs numpy and Pillow; a few seconds.
Run from anywhere: python tools/gen_prelaunch_art.py"""
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

W, H = 1920, 1080
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'game', 'ui', 'res', 'prelaunch-bg.png')
rng = np.random.default_rng(7)

yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)

# Sky: deep indigo at the top to a violet haze near the horizon.
t = (yy / H) ** 1.2
top = np.array([10, 14, 46], np.float32)
bottom = np.array([58, 34, 96], np.float32)
img = top[None, None, :] * (1 - t[..., None]) + bottom[None, None, :] * t[..., None]

# A soft glow around the moon.
MOON = (1370.0, 360.0, 190.0)  # centre x, y, radius
d = np.hypot(xx - MOON[0], yy - MOON[1])
glow = np.exp(-((d / 520.0) ** 2))
img += glow[..., None] * np.array([70, 70, 120], np.float32)

# Stars: many faint, some bright, fewer to the left (behind the menu).
stars = Image.new('L', (W, H), 0)
sd = ImageDraw.Draw(stars)
for _ in range(1400):
    x, y = rng.uniform(0, W), rng.uniform(0, H * 0.82)
    if rng.uniform() > 0.25 + 0.75 * x / W:
        continue
    r = rng.choice([0.6, 0.9, 1.3, 1.8], p=[0.55, 0.28, 0.12, 0.05])
    b = int(rng.uniform(90, 255))
    sd.ellipse((x - r, y - r, x + r, y + r), fill=b)
stars = np.asarray(stars.filter(ImageFilter.GaussianBlur(0.6)), np.float32) / 255.0
img = img * (1 - stars[..., None]) + np.array([235, 235, 255], np.float32) * stars[..., None]


def sparkle(layer, x, y, size, alpha):
    """A four-pointed sparkle with a soft core."""
    d = ImageDraw.Draw(layer)
    s, w = size, max(1.0, size * 0.12)
    d.polygon([(x, y - s), (x + w, y), (x, y + s), (x - w, y)], fill=alpha)
    d.polygon([(x - s, y), (x, y - w), (x + s, y), (x, y + w)], fill=alpha)
    d.ellipse((x - w * 1.8, y - w * 1.8, x + w * 1.8, y + w * 1.8), fill=alpha)


# The crescent moon: a disc with an offset disc taken away, lightly shaded.
moon = Image.new('L', (W, H), 0)
md = ImageDraw.Draw(moon)
cx, cy, r = MOON
md.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
md.ellipse((cx - r * 0.55, cy - r * 1.12, cx + r * 1.35, cy + r * 0.78), fill=0)
moon = np.asarray(moon.filter(ImageFilter.GaussianBlur(1.2)), np.float32) / 255.0
shade = 0.82 + 0.18 * np.clip((MOON[1] + r - yy) / (2 * r), 0, 1)
moon_col = np.stack([255 * shade, 238 * shade, 190 * shade], -1)
halo = np.asarray(Image.fromarray((moon * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(28)), np.float32) / 255.0
img += halo[..., None] * np.array([90, 80, 50], np.float32)
img = img * (1 - moon[..., None]) + moon_col * moon[..., None]

# A trail of sparkles sweeping down from the moon, like a wand's.
spark = Image.new('L', (W, H), 0)
for i in range(70):
    u = i / 69.0
    x = 1150 - 380 * u + 60 * math.sin(u * 7.0)
    y = 520 + 330 * u * u + rng.normal(0, 18)
    sparkle(spark, x + rng.normal(0, 25), y, rng.uniform(4, 16) * (1.2 - 0.6 * u), int(230 * (1 - 0.5 * u)))
for _ in range(14):
    sparkle(spark, rng.uniform(950, 1880), rng.uniform(80, 760), rng.uniform(8, 22), int(rng.uniform(150, 255)))
spark_a = np.asarray(spark.filter(ImageFilter.GaussianBlur(0.8)), np.float32) / 255.0
spark_glow = np.asarray(spark.filter(ImageFilter.GaussianBlur(9)), np.float32) / 255.0
img += spark_glow[..., None] * np.array([120, 110, 200], np.float32)
img = img * (1 - spark_a[..., None]) + np.array([255, 246, 210], np.float32) * spark_a[..., None]

# Rooftops: a skyline of steep roofs and a tower, in silhouette along the bottom right, with a few lit windows.
sil = Image.new('L', (W, H), 0)
sdw = ImageDraw.Draw(sil)
base = 1080
roofs = [  # (left, right, wall top, roof peak)
    (520, 640, 1000, 950), (620, 780, 960, 890), (760, 960, 900, 820), (930, 1120, 870, 760), (1100, 1260, 910, 830), (1240, 1400, 800, 640),
    (1380, 1560, 880, 780), (1540, 1720, 850, 720), (1700, 1920, 900, 800),
]
for l, rr, wall, peak in roofs:
    sdw.rectangle((l, wall, rr, base), fill=255)
    sdw.polygon([(l - 14, wall), ((l + rr) / 2, peak), (rr + 14, wall)], fill=255)
# The tower, with a pointed spire and a flag.
sdw.rectangle((1290, 560, 1350, base), fill=255)
sdw.polygon([(1278, 560), (1320, 430), (1362, 560)], fill=255)
sdw.line((1320, 430, 1320, 395), fill=255, width=3)
sdw.polygon([(1321, 396), (1352, 404), (1321, 412)], fill=255)
sil = np.asarray(sil.filter(ImageFilter.GaussianBlur(0.8)), np.float32) / 255.0
sil_col = np.array([14, 10, 30], np.float32)
img = img * (1 - sil[..., None]) + sil_col * sil[..., None]
win = Image.new('L', (W, H), 0)
wd = ImageDraw.Draw(win)
for l, rr, wall, peak in roofs:
    for _ in range(3):
        x = rng.uniform(l + 20, rr - 30)
        y = rng.uniform(wall + 25, wall + 120)
        if rng.uniform() < 0.6:
            wd.rounded_rectangle((x, y, x + 12, y + 20), 4, fill=255)
wd.rounded_rectangle((1312, 600, 1328, 626), 6, fill=255)
win_a = np.asarray(win, np.float32) / 255.0
win_glow = np.asarray(win.filter(ImageFilter.GaussianBlur(10)), np.float32) / 255.0
img += win_glow[..., None] * np.array([120, 80, 20], np.float32)
img = img * (1 - win_a[..., None]) + np.array([255, 200, 110], np.float32) * win_a[..., None]

# Ordered dither before quantising, so the gradients do not band.
bayer = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32) / 16.0 - 0.5
img += np.tile(bayer, (H // 4, W // 4))[..., None]
Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), 'RGB').save(OUT, optimize=True)
print('wrote', os.path.normpath(OUT))
