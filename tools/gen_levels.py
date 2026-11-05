#!/usr/bin/env python3
"""Search for Floe Slide puzzles: random floes, solved with floe.py's BFS, kept when the shortest
solution is long enough and uses the world's mechanic. Prints candidates in levels.txt format;
the picks were copied (and some hand-tuned) into tools/levels.txt.

Usage: python3 tools/gen_levels.py <world 1-4> <count> [seed] [minpar]
"""
import random
import sys
import os
from multiprocessing import Pool

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from floe import Floe  # noqa: E402

# per world: sizes (w, h incl. the sea border), fish, holes, blocks, water, cracks, wanted events
WORLDS = {
    1: dict(sizes=[(9, 7), (10, 8), (12, 9), (14, 10)], fish=(1, 4), holes=(0, 0), blocks=(0, 0), water=(0, 0), cracks=(0, 0), need=[]),
    2: dict(sizes=[(10, 8), (12, 9), (14, 10), (16, 11)], fish=(2, 5), holes=(1, 2), blocks=(0, 0), water=(0, 0), cracks=(0, 0), need=['hole']),
    3: dict(sizes=[(10, 8), (12, 9), (14, 10), (16, 11)], fish=(2, 5), holes=(0, 1), blocks=(1, 3), water=(0, 3), cracks=(0, 0), need=['push']),
    4: dict(sizes=[(12, 9), (14, 10), (16, 11), (18, 12)], fish=(2, 6), holes=(0, 2), blocks=(0, 2), water=(0, 2), cracks=(4, 10), need=['crack']),
}


def make(world, rnd):
    cfg = WORLDS[world]
    w, h = rnd.choice(cfg['sizes'])
    g = [['~'] * w for _ in range(h)]
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            g[y][x] = '.'
    # the rim: mostly snow banks, with gaps of open sea to slide out of
    p_snow = rnd.uniform(0.45, 0.85)
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            if x in (1, w - 2) or y in (1, h - 2):
                r = rnd.random()
                g[y][x] = '#' if r < p_snow else ('~' if r < p_snow + 0.08 else '.')
    free = [(x, y) for y in range(2, h - 2) for x in range(2, w - 2)]
    rnd.shuffle(free)
    inner = (w - 4) * (h - 4)
    for _ in range(int(inner * rnd.uniform(0.08, 0.22))):
        x, y = free.pop()
        g[y][x] = rnd.choice('##o')

    def place(ch, n, edge_ok=True):
        cand = [(x, y) for y in range(1, h - 1) for x in range(1, w - 1) if g[y][x] == '.']
        rnd.shuffle(cand)
        for x, y in cand[:n]:
            g[y][x] = ch

    for k in range(rnd.randint(*cfg['holes'])):
        place(str(k + 1), 2)
    place('B', rnd.randint(*cfg['blocks']))
    place('w', rnd.randint(*cfg['water']))
    place('c', rnd.randint(*cfg['cracks']))
    place('f', rnd.randint(*cfg['fish']))
    place('E', 1)
    place('@', 1)
    return [''.join(r) for r in g]


def score(world, rows):
    f = Floe('x', rows)
    sol, states = f.solve(limit=60000)
    if not sol:
        return None
    ok, evs = f.replay(sol)
    events = set(e for _, _, ev in evs for e in ev)
    for n in WORLDS[world]['need']:
        if n not in events:
            return None
    if world == 4:
        # cracks must matter: as plain ice the floe must get easier or change
        alt = Floe('y', [r.replace('c', '.') for r in rows])
        s2, _ = alt.solve(limit=60000)
        if s2 and len(s2) >= len(sol):
            return None
    if world == 2:
        alt_rows = rows
        for k in '123':
            alt_rows = [r.replace(k, '.') for r in alt_rows]
        try:
            alt = Floe('y', alt_rows)
            s2, _ = alt.solve(limit=60000)
            if s2 and len(s2) <= len(sol):
                return None
        except AssertionError:
            pass
    # fail moves along the way make it tense: count how many single moves from the solution path drop you in
    return len(sol), states, sol, sorted(events)


def job(args):
    world, seed, tries, minpar = args
    rnd = random.Random(seed)
    out = []
    for _ in range(tries):
        rows = make(world, rnd)
        try:
            s = score(world, rows)
        except AssertionError:
            continue
        if s and s[0] >= minpar:
            out.append((s, rows))
    return out


if __name__ == '__main__':
    world = int(sys.argv[1])
    count = int(sys.argv[2])
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    minpar = int(sys.argv[4]) if len(sys.argv) > 4 else 6
    with Pool() as p:
        res = p.map(job, [(world, seed * 1000 + i, count, minpar) for i in range(8)])
    allr = [r for rr in res for r in rr]
    allr.sort(key=lambda r: (r[0][0], r[0][1]))
    for (par, states, sol, ev), rows in allr:
        print('- par %d states %d %s %s' % (par, states, sol, ','.join(ev)))
        for r in rows:
            print(r)
        print()
