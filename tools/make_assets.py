#!/usr/bin/env python3
"""Generate the Floe Slide tileset (assets/tileset.gif) and its tile index header (src/gfx.h).

Top-down art, all text pixel art so it stays diffable. One 256-tile, 8-bit tileset:
    0         empty (transparent on layer 1)
    1..31     layer 1: things on the ice (snow banks, rocks, fish, blocks, cracks, holes, exit)
    32..95    font at the ASCII codes (HUD rows are layer 1 text)
    96..127   layer 0: sea, the floe itself (16 edge variants) and the ice cliff face
    128..     sprites: the seal (drawn facing up, rotated for right, flipped for down/left), effects
The seal is one 16x16 sprite: the puzzle grid is 16 px, so a 16x32 sprite would cover two cells.
Palette entries 1-3 (sea waves) and 4-5 (exit glow) are cycled by the game.
Run: python3 tools/make_assets.py   (from the game folder, needs Pillow)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import Tileset  # noqa: E402

PAL = [
    ('0', (0, 0, 0)),         # 0 transparent
    ('1', (40, 104, 168)),    # 1-3 sea wave colors (cycled)
    ('2', (58, 130, 192)),
    ('3', (92, 162, 214)),
    ('4', (255, 220, 90)),    # 4-5 exit glow (cycled)
    ('5', (255, 160, 60)),
    ('A', (24, 70, 132)),     # sea base
    ('a', (14, 44, 92)),      # deep / HUD band
    ('k', (12, 20, 36)),      # outline
    ('w', (255, 255, 255)),
    # ice floe
    ('i', (228, 242, 250)),
    ('j', (204, 228, 244)),
    ('l', (164, 204, 230)),   # ice rim / shade
    ('m', (120, 170, 210)),   # cliff face
    ('M', (84, 136, 186)),    # cliff face dark
    # snow
    ('s', (250, 252, 255)),
    ('S', (214, 226, 240)),   # snow shade
    ('n', (170, 190, 216)),   # snow deep shade
    # rock
    ('r', (120, 118, 128)),
    ('R', (86, 84, 96)),
    ('q', (54, 52, 64)),
    # water hole
    ('h', (20, 56, 110)),
    ('H', (10, 32, 70)),
    # thin ice
    ('t', (186, 214, 234)),
    ('T', (90, 130, 170)),    # crack lines
    # block (clear ice)
    ('b', (150, 220, 246)),
    ('B', (96, 184, 228)),
    ('c', (210, 244, 255)),
    # fish
    ('F', (255, 160, 48)),
    ('f', (206, 100, 28)),
    ('e', (255, 222, 140)),
    # seal (harbor seal grey with spots)
    ('G', (128, 136, 152)),
    ('g', (92, 98, 116)),
    ('L', (176, 184, 198)),
    ('N', (40, 40, 52)),      # nose / eyes
    # hole pair rims
    ('x', (232, 64, 72)),     # red
    ('y', (250, 200, 40)),    # yellow
    ('z', (80, 210, 110)),    # green
    ('u', (150, 100, 220)),   # unused purple
    ('o', (60, 200, 255)),    # sparkle
]
ts = Tileset(PAL)
grid, put = ts.grid, ts.put


def rot_cw(px):
    """rotate 90 degrees clockwise (facing up -> facing right)"""
    n = len(px)
    return [[px[n - 1 - x][y] for x in range(n)] for y in range(n)]


def flip_y(px):
    return list(reversed([r[:] for r in px]))


def disc(r_out, ch_in, ch_rim, rim_w=2, cx=7.5, cy=7.5, ch_edge='k', squash=1.0):
    """filled disc with a rim and an outline, transparent outside"""
    rows = []
    for y in range(16):
        r = ''
        for x in range(16):
            d = (((x - cx)) ** 2 + ((y - cy) * squash) ** 2) ** 0.5
            if d <= r_out - rim_w - 0.5:
                r += ch_in
            elif d <= r_out - 0.5:
                r += ch_rim
            elif d <= r_out + 0.5:
                r += ch_edge
            else:
                r += '.'
        rows.append(r)
    return rows


def paint(rows, spots):
    rows = [list(r) for r in rows]
    for (x, y, ch) in spots:
        rows[y][x] = ch
    return [''.join(r) for r in rows]


# ---------------------------------------------------------------- layer 1: on the ice
put(0, 'TILE_EMPTY', ts.blank())
SNOW = [
    "................",
    "....kkkk........",
    "..kksssskk.kk...",
    ".kssssssssksskk.",
    ".ksssssssssssssk",
    "ksssssssssssssSk",
    "ksswssssssssssSk",
    "kssssssssssssSSk",
    "ksssssssssssSSSk",
    "kSsssssssssSSSnk",
    "kSSssssssSSSSSnk",
    "kSSSSSSSSSSSSnnk",
    ".knSSSSSSSSSnnk.",
    "..knnnnnnnnnnk..",
    "...kkkkkkkkkk...",
    "................",
]
put(1, 'TILE_SNOW', grid(SNOW))
SNOW2 = [
    "................",
    "..........kkk...",
    ".kkk.....kssskk.",
    "ksssk..kksssssk.",
    "ksssskksssssssSk",
    "kssssssssssssSSk",
    "ksswsssssssssSSk",
    "kSssssssssssSSnk",
    "kSSsssssssSSSSnk",
    "kSSSsssssSSSSnnk",
    "kSSSSSSSSSSSSnnk",
    ".knSSSSSSSSSnnk.",
    ".knnnnSSSSnnnnk.",
    "..kkknnnnnnnkk..",
    ".....kkkkkkk....",
    "................",
]
put(2, 'TILE_SNOW2', grid(SNOW2))
ROCK = [
    "................",
    "................",
    ".....kkkkk......",
    "....krrrrRkk....",
    "...krrrrrrRRk...",
    "..krrwrrrrrRRk..",
    "..krrrrrrrrRRqk.",
    ".krrrrrrrrRRRqk.",
    ".kRrrrrrrRRRqqk.",
    ".kRRrrrrRRRRqqk.",
    ".kqRRRRRRRRqqqk.",
    "..kqqRRRRRqqqk..",
    "...kkqqqqqqkk...",
    ".....kkkkkk.....",
    "................",
    "................",
]
put(3, 'TILE_ROCK', grid(ROCK))
FISH = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "...kkkkk....kk..",
    "..kFFFeFkk.kFk..",
    ".kFkFFFFFFkFFk..",
    ".kFFFFFFFFFFk...",
    ".kffFFFFFfkFFk..",
    "..kfffffkk.kfk..",
    "...kkkkk....kk..",
    "................",
    "................",
    "................",
    "................",
]
put(4, 'TILE_FISH', grid(FISH))
BLOCK = [
    "kkkkkkkkkkkkkkkk",
    "kccccccccccccccB",
    "kcwwbbbbbbbbbbcB",
    "kcwbbbbbbbbbbbBB",
    "kcbbbbbbbbbbbbBB",
    "kcbbbcbbbbbbbbBB",
    "kcbbbbcbbbbbbbBB",
    "kcbbbbbcbbbbbbBB",
    "kcbbbbbbbbbbbbBB",
    "kcbbbbbbbbcbbbBB",
    "kcbbbbbbbbbcbbBB",
    "kcbbbbbbbbbbbbBB",
    "kcbbbbbbbbbbbbBB",
    "kcBBBBBBBBBBBBBB",
    "kBBBBBBBBBBBBBBk",
    "kkkkkkkkkkkkkkkk",
]
put(5, 'TILE_BLOCK', grid(BLOCK))
CRACK = [
    "tttttttttttttttt",
    "ttttttTttttttttt",
    "tttttTtttttttTtt",
    "ttTtTttttttttTtt",
    "tttTtttttttTtttt",
    "ttTttTTtttTttttt",
    "tTttttttTTtttttt",
    "ttttttttTttttttt",
    "tttttttTtTTttttt",
    "ttttttTtttttTTtt",
    "ttttTTttttttttTt",
    "tttTtttttttttttt",
    "ttTttttttTtttttt",
    "tttttttttTtttttt",
    "ttttttttTttttttt",
    "tttttttttttttttt",
]
put(6, 'TILE_CRACK', grid(CRACK))
WATER = disc(7.2, 'h', 'H', rim_w=1.5, ch_edge='l')
WATER = paint(WATER, [(5, 5, '3'), (6, 5, '3'), (9, 9, '2'), (10, 9, '2')])
put(7, 'TILE_WATER', grid(WATER))
EXIT_SHUT = disc(7.2, 'S', 'l', rim_w=2, ch_edge='k')
EXIT_SHUT = paint(EXIT_SHUT, [(7, 4, 'T'), (7, 5, 'T'), (7, 6, 'T'), (7, 7, 'T'), (7, 8, 'T'), (7, 9, 'T'), (7, 10, 'T'),
                              (5, 5, 'T'), (6, 6, 'T'), (8, 8, 'T'), (9, 9, 'T'), (9, 5, 'T'), (8, 6, 'T'),
                              (6, 8, 'T'), (5, 9, 'T'), (4, 7, 'T'), (5, 7, 'T'), (9, 7, 'T'), (10, 7, 'T')])
put(8, 'TILE_EXIT_SHUT', grid(EXIT_SHUT))
EXIT_OPEN = disc(7.4, 'h', '4', rim_w=2, ch_edge='5')
EXIT_OPEN = paint(EXIT_OPEN, [(6, 6, 'H'), (7, 6, 'H'), (8, 7, 'H'), (7, 8, 'H'), (5, 9, '3'), (10, 5, '3')])
put(9, 'TILE_EXIT_OPEN', grid(EXIT_OPEN))
for i, (nm, rim) in enumerate((('TILE_HOLE_A', 'x'), ('TILE_HOLE_B', 'y'), ('TILE_HOLE_C', 'z'))):
    h = disc(7.2, 'h', rim, rim_w=2, ch_edge='k')
    h = paint(h, [(7, 7, rim), (8, 7, rim), (7, 8, rim), (8, 8, rim), (5, 5, '3'), (10, 10, '2')])
    put(10 + i, nm, grid(h))
SLUSH = [
    "................",
    "................",
    "....kk.kkk......",
    "...klljjjlkk....",
    "..kljjiijjjlk...",
    "..kjjiwiijjjlk..",
    ".kljiiiiiijjlk..",
    ".kljjiiiijjjlk..",
    "..kljjjjjjjllk..",
    "..klljjjjjllk...",
    "...kkllllllk....",
    ".....kkkkkk.....",
    "................",
    "................",
    "................",
    "................",
]
put(13, 'TILE_SLUSH', grid(SLUSH))
# little fish icon for the HUD
put(14, 'TILE_FISH_ICON', grid(FISH))

ts.font()

# ---------------------------------------------------------------- layer 0: sea and the floe


def sea(seed):
    """calm sea: short wave dashes scattered on a hash, colors 1-3 are cycled by the game"""
    rows = [['A'] * 16 for _ in range(16)]
    for k in range(7):
        h = (seed * 131 + k * 977 + 17) * 2654435761 & 0xFFFFFFFF
        x, y = h % 16, (h >> 8) % 16
        c = '123'[(h >> 16) % 3]
        for dx in range(3 + (h >> 20) % 3):
            rows[y][(x + dx) % 16] = c
        if (h >> 24) % 2:
            rows[(y + 1) % 16][(x + 1) % 16] = '1'
    return [''.join(r) for r in rows]


put(96, 'BG_SEA', grid(sea(0)))
put(97, 'BG_SEA2', grid(sea(3)))
put(98, 'BG_BAND', ts.fill('a'))


def ice(mask):
    """floe tile; mask bits: 1 = sea to the north, 2 east, 4 south, 8 west"""
    px = []
    for y in range(16):
        r = ''
        for x in range(16):
            v = (x * 3 + y * 7 + (x * y) % 5) % 29
            r += 'j' if v in (0, 9) else 'i'
        px.append(list(r))
    for y in range(16):
        for x in range(16):
            if mask & 1 and y < 2:
                px[y][x] = 'k' if y == 0 else 'l'
            if mask & 4 and y > 13:
                px[y][x] = 'k' if y == 15 else 'l'
            if mask & 8 and x < 2:
                px[y][x] = 'k' if x == 0 else 'l'
            if mask & 2 and x > 13:
                px[y][x] = 'k' if x == 15 else 'l'
    # round the outer corners
    for (bit_a, bit_b, cx, cy) in ((1, 8, 0, 0), (1, 2, 15, 0), (4, 8, 0, 15), (4, 2, 15, 15)):
        if mask & bit_a and mask & bit_b:
            px[cy][cx] = '1'
            px[cy if cy == 0 else 14][1 if cx == 0 else 14] = 'k'
    return [[ts.ci[c] for c in r] for r in px]


for m in range(16):
    put(100 + m, 'BG_ICE' if m == 0 else None, ice(m))
FACE = []
for y in range(16):
    if y < 5:
        FACE.append(('M' if y >= 3 else 'm') * 16)
    elif y == 5:
        FACE.append('k' * 16)
    else:
        FACE.append(sea(1)[y])
FACE = paint(FACE, [(3, 1, 'l'), (4, 1, 'l'), (10, 2, 'l'), (11, 2, 'l'), (12, 2, 'l'), (7, 3, 'm')])
put(116, 'BG_FACE', grid(FACE))

# ---------------------------------------------------------------- sprites
ts.start_sprites(128)
SEAL_UP = [
    "......kkkk......",
    ".....kgNNgk.....",
    "....kGGGGGGk....",
    "....kNGGGGNk....",
    "....kGGLLGGk....",
    "...kkGGLLGgkk...",
    "..kGkGGLLGGkGk..",
    ".kGgkGgLLGGkgGk.",
    ".kkk.kGLLgGk.kkk",
    "....kGGLLGGk....",
    "....kgGLLGGk....",
    ".....kGGGgk.....",
    ".....kGGGGk.....",
    "......kggk......",
    ".....kGkkGk.....",
    ".....kk..kk.....",
]
SEAL_UP2 = [   # flippers forward: the idle wiggle
    "......kkkk......",
    ".....kgNNgk.....",
    "....kGGGGGGk....",
    "....kNGGGGNk....",
    "....kGGLLGGk....",
    "..kkkGGLLGgkkk..",
    ".kGGkGGLLGGkGGk.",
    ".kkkkGgLLGGkkkk.",
    ".....kGLLgGk....",
    "....kGGLLGGk....",
    "....kgGLLGGk....",
    ".....kGGGgk.....",
    ".....kGGGGk.....",
    "......kggk......",
    "......kGGk......",
    ".....kk..kk.....",
]
SEAL_SLIDE = [   # flippers tucked, stretched out
    "......kkkk......",
    ".....kgNNgk.....",
    "....kGGGGGGk....",
    "....kNGGGGNk....",
    "....kGGLLGGk....",
    "....kGGLLGgk....",
    "...kGkGLLGkGk...",
    "...kGkgLLGkgk...",
    "...kGkGLLgkGk...",
    "....kkGLLGkk....",
    "....kgGLLGGk....",
    ".....kGGGgk.....",
    ".....kGGGGk.....",
    "......kggk......",
    "......kGGk......",
    "......kkkk......",
]
FRAMES = [('UP', SEAL_UP), ('UP2', SEAL_UP2), ('SLIDE', SEAL_SLIDE)]
for nm, art in FRAMES:
    ts.spr('SPR_SEAL_' + nm, grid(art))           # facing up / down (flip Y)
for nm, art in FRAMES:
    ts.spr('SPR_SEAL_SIDE_' + nm, rot_cw(grid(art)))   # facing right / left (flip X)
DIVE = [
    "................",
    "................",
    "................",
    "....kkkkkkk.....",
    "...k3333333k....",
    "..k33kkkk333k...",
    "..k3kgNNgk33k...",
    "..k3kGGGGGk3k...",
    "..k3kNGGGNk3k...",
    "..k33kGGGk33k...",
    "...k33kkk333k...",
    "....k333333k....",
    ".....kkkkkk.....",
    "................",
    "................",
    "................",
]
ts.spr('SPR_DIVE1', grid(DIVE))
RIPPLE = [
    "................",
    "................",
    "....3.....3.....",
    "......333.......",
    "...3.3...3.3....",
    "....3.....3.....",
    "...3..www..3....",
    "..3..w...w..3...",
    "...3..www..3....",
    "....3.....3.....",
    "...3.3...3.3....",
    "......333.......",
    "....3.....3.....",
    "................",
    "................",
    "................",
]
ts.spr('SPR_DIVE2', grid(RIPPLE))
SPLASH1 = [
    "................",
    "......w...w.....",
    "...w...w.w...w..",
    ".....w.www.w....",
    "..w..wwcwcww..w.",
    "....wcc3c3ccw...",
    "...wc3333333cw..",
    "..wc332222233cw.",
    "..wc322111223cw.",
    "..wc332222233cw.",
    "...wc3333333cw..",
    "....wcc333ccw...",
    ".....wwwwwww....",
    "................",
    "................",
    "................",
]
ts.spr('SPR_SPLASH1', grid(SPLASH1))
SPLASH2 = [
    "................",
    "................",
    "................",
    "................",
    "......w...w.....",
    "....w..3.3..w...",
    "...3..33333..3..",
    "....3332223333..",
    "...33221112233..",
    "....3332223333..",
    "...3..33333..3..",
    "....w..3.3..w...",
    "......w...w.....",
    "................",
    "................",
    "................",
]
ts.spr('SPR_SPLASH2', grid(SPLASH2))
STAR1 = [
    "................",
    "................",
    "................",
    ".......w........",
    ".......w........",
    ".......o........",
    "......ooo.......",
    "..wwooowooww....",
    "......ooo.......",
    ".......o........",
    ".......w........",
    ".......w........",
    "................",
    "................",
    "................",
    "................",
]
ts.spr('SPR_STAR1', grid(STAR1))
STAR2 = [
    "................",
    "................",
    "................",
    "................",
    "....w.....w.....",
    ".....o...o......",
    "......owo.......",
    ".......w........",
    "......owo.......",
    ".....o...o......",
    "....w.....w.....",
    "................",
    "................",
    "................",
    "................",
    "................",
]
ts.spr('SPR_STAR2', grid(STAR2))
# arrows for the floe picker on the title screen
ARROW = [
    "................",
    "................",
    "................",
    "......kk........",
    "......kwk.......",
    "......kwwk......",
    "......kwwwk.....",
    "......kwwwwk....",
    "......kwwwk.....",
    "......kwwk......",
    "......kwk.......",
    "......kk........",
    "................",
    "................",
    "................",
    "................",
]
ts.spr('SPR_ARROW', grid(ARROW))
used = ts.save(ROOT)
print('tiles used:', used)

# Shared SNES art, owned locally so the game remains standalone.
from snes_glyphs import install as install_snes_glyphs
install_snes_glyphs(ROOT, used)
