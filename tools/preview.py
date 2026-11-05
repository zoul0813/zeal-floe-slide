#!/usr/bin/env python3
"""Render floes from levels.txt with the tileset, the way the game draws them (layer 0 floe + sea,
layer 1 objects). Usage: python3 tools/preview.py <out.png> [floe numbers 1..32]"""
import os
import sys
import re
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import floe as F  # noqa: E402

GRID_W, GRID_H, GRID_Y = 20, 12, 1
names = {}
for line in open(os.path.join(ROOT, 'src', 'gfx.h')):
    m = re.match(r'#define (\w+)\s+(\d+)', line)
    if m:
        names[m.group(1)] = int(m.group(2))
ts = Image.open(os.path.join(ROOT, 'assets', 'tileset.gif'))
pal = ts.getpalette()


def tile(i):
    x, y = (i % 16) * 16, (i // 16) * 16
    return ts.crop((x, y, x + 16, y + 16))


def l1_tile(c, fish_left):
    T = names
    return {F.SNOW: T['TILE_SNOW'], F.ROCK: T['TILE_ROCK'], F.FISH: T['TILE_FISH'], F.BLOCK: T['TILE_BLOCK'],
            F.CRACK: T['TILE_CRACK'], F.WATER: T['TILE_WATER'],
            F.EXIT: T['TILE_EXIT_SHUT'] if fish_left else T['TILE_EXIT_OPEN']}.get(
        c, T['TILE_HOLE_A'] + (c - F.HOLE) // 2 if F.is_hole(c) else 0)


def render(fl):
    img = Image.new('RGB', (320, 240))
    ox, oy = (GRID_W - fl.w) // 2, (GRID_H - fl.h) // 2
    floe = [[False] * GRID_W for _ in range(15)]
    for i, c in enumerate(fl.cells):
        if c != F.SEA:
            floe[oy + GRID_Y + i // fl.w][ox + i % fl.w] = True
    for y in range(15):
        for x in range(20):
            if y in (0, 14):
                t = names['BG_BAND']
            elif floe[y][x]:
                m = 0
                if not floe[y - 1][x]: m |= 1
                if x == 19 or not floe[y][x + 1]: m |= 2
                if not floe[y + 1][x]: m |= 4
                if x == 0 or not floe[y][x - 1]: m |= 8
                t = names['BG_ICE'] + m
            elif floe[y - 1][x]:
                t = names['BG_FACE']
            else:
                t = names['BG_SEA'] + ((x + y) & 1)
            img.paste(tile(t).convert('RGB'), (x * 16, y * 16))
    for i, c in enumerate(fl.cells):
        t = l1_tile(c, fl.fish)
        if t:
            tl = tile(t)
            mask = Image.frombytes('L', (16, 16), bytes(255 if v else 0 for v in tl.tobytes()))
            img.paste(tl.convert('RGB'), ((ox + i % fl.w) * 16, (oy + GRID_Y + i // fl.w) * 16), mask)
    s = tile(names['SPR_SEAL_UP'])
    mask = Image.frombytes('L', (16, 16), bytes(255 if v else 0 for v in s.tobytes()))
    img.paste(s.convert('RGB'), ((ox + fl.start % fl.w) * 16, (oy + GRID_Y + fl.start // fl.w) * 16), mask)
    return img


if __name__ == '__main__':
    floes = F.parse(open(os.path.join(HERE, 'levels.txt')).read())
    pick = [int(a) for a in sys.argv[2:]] or [1]
    imgs = [render(floes[n - 1][1]) for n in pick]
    out = Image.new('RGB', (320 * min(4, len(imgs)), 240 * ((len(imgs) + 3) // 4)))
    for k, im in enumerate(imgs):
        out.paste(im, ((k % 4) * 320, (k // 4) * 240))
    out = out.resize((out.width * 2, out.height * 2), Image.NEAREST)
    out.save(sys.argv[1])
