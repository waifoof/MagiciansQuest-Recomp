#!/usr/bin/env python3
"""Draws the program's built-in icon: our own art (no game assets), a pointed wizard's hat with a star and a crescent
moon on a night-violet tile with a gold rim, in the colours of the interface. Writes game/res/ffcc.ico (16, 32, 48, 64,
128 and 256 pixels), which game/ffcc.rc puts into the program. Once the player's game is set up, the program swaps in
the game's own icon (game/src/app_icon.hpp). Needs Pillow: python tools/gen_app_icon.py"""
import os

from PIL import Image, ImageDraw, ImageFilter

SIZES = [16, 32, 48, 64, 128, 256]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'game', 'res', 'ffcc.ico')
S = 1024  # drawn at this size, then scaled down


def star(d, cx, cy, r_out, r_in, fill, points=5):
    import math
    pts = []
    for i in range(points * 2):
        a = -math.pi / 2 + i * math.pi / points
        r = r_out if i % 2 == 0 else r_in
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    d.polygon(pts, fill=fill)


def draw():
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # The tile: a rounded square, gold rim around a night gradient.
    d.rounded_rectangle((16, 16, S - 16, S - 16), 190, fill=(214, 178, 96, 255))
    tile = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    grad = Image.linear_gradient('L').resize((S, S))
    top, bottom = (22, 22, 70), (78, 44, 120)
    col = Image.merge('RGB', [grad.point(lambda v, a=a, b=b: int(a + (b - a) * v / 255)) for a, b in zip(top, bottom)])
    mask = Image.new('L', (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((56, 56, S - 56, S - 56), 160, fill=255)
    tile.paste(col, (0, 0), mask)
    img = Image.alpha_composite(img, tile)
    d = ImageDraw.Draw(img)
    # Crescent moon, top right.
    moon = Image.new('L', (S, S), 0)
    md = ImageDraw.Draw(moon)
    md.ellipse((600, 130, 860, 390), fill=255)
    md.ellipse((660, 100, 920, 360), fill=0)
    img.paste((255, 236, 180, 255), (0, 0), moon)
    # The hat: a tall bent cone over a wide brim.
    hat = (92, 62, 200, 255)
    hat_dark = (64, 40, 150, 255)
    d.polygon([(250, 770), (470, 250), (560, 170), (660, 240), (600, 300), (770, 770)], fill=hat)
    d.polygon([(470, 250), (560, 170), (660, 240), (600, 300), (520, 330)], fill=hat_dark)
    d.ellipse((150, 700, 874, 860), fill=hat_dark)
    d.ellipse((170, 690, 854, 830), fill=hat)
    # A gold band and a star on the hat.
    d.polygon([(268, 728), (752, 728), (735, 680), (285, 680)], fill=(214, 178, 96, 255))
    star(d, 470, 520, 105, 44, (255, 226, 120, 255))
    # Small sparkles.
    for cx, cy, r in ((250, 260, 34), (330, 400, 20), (800, 520, 26)):
        star(d, cx, cy, r, r * 0.35, (240, 240, 255, 255), points=4)
    return img


def main():
    big = draw()
    images = [big.resize((n, n), Image.LANCZOS) for n in SIZES]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    images[-1].save(OUT, format='ICO', sizes=[(n, n) for n in SIZES], append_images=images[:-1])
    print('wrote', os.path.normpath(OUT), os.path.getsize(OUT), 'bytes')


if __name__ == '__main__':
    main()
