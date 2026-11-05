"""Floe Slide rules in Python: the level text format, the slide simulation and a BFS solver.

src/play.c implements the same rules in C; keep both in step. make_levels.py uses this to verify
every floe, find its par (fewest moves) and the solution the autopilot plays.

Level text (one char per 16x16 cell):
    ~ sea (fall in = fail)      . ice              # snow bank (stops)    o rock (stops)
    f fish on ice               B ice block (push) c cracked ice          w open water (fail; a block fills it)
    E exit hole (stops while fish remain, then sliding in clears the floe)
    1 2 3 breathing holes: each digit appears twice, slide in one and out of the other
    @ start (ice)
"""
from collections import deque

SEA, ICE, SNOW, ROCK, FISH, BLOCK, CRACK, WATER, EXIT = range(9)
HOLE = 9   # 9..14: pair k = holes 9+2k and 10+2k
CHARS = {'~': SEA, '.': ICE, '#': SNOW, 'o': ROCK, 'f': FISH, 'B': BLOCK, 'c': CRACK, 'w': WATER, 'E': EXIT, '@': ICE}
DIRS = {'U': (0, -1), 'D': (0, 1), 'L': (-1, 0), 'R': (1, 0)}
MAX_STEPS = 200

# move results
FAIL, NOMOVE, MOVED, WIN = 'fail', 'nomove', 'moved', 'win'


def is_hole(c):
    return HOLE <= c < HOLE + 6


def partner(c):
    return c + 1 if (c - HOLE) % 2 == 0 else c - 1


class Floe:
    def __init__(self, name, rows):
        self.name = name
        self.h = len(rows)
        self.w = max(len(r) for r in rows)
        rows = [r.ljust(self.w, '~') for r in rows]
        cells = []
        seen = {}
        self.start = None
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch == '@':
                    assert self.start is None, name
                    self.start = y * self.w + x
                if ch in '123':
                    k = int(ch) - 1
                    n = seen.get(ch, 0)
                    assert n < 2, (name, 'hole', ch)
                    seen[ch] = n + 1
                    cells.append(HOLE + 2 * k + n)
                else:
                    assert ch in CHARS, (name, ch)
                    cells.append(CHARS[ch])
        for ch, n in seen.items():
            assert n == 2, (name, 'hole needs a partner', ch)
        assert self.start is not None, name
        assert cells.count(EXIT) == 1, name
        self.cells = bytes(cells)
        self.rows = rows
        self.fish = cells.count(FISH)
        # partner positions
        self.hole_at = {c: i for i, c in enumerate(cells) if is_hole(c)}

    def cell(self, cells, i, dx, dy):
        x, y = i % self.w + dx, i // self.w + dy
        if not (0 <= x < self.w and 0 <= y < self.h):
            return None, SEA
        j = y * self.w + x
        return j, cells[j]

    def move(self, pos, cells, d, trace=None):
        """-> (result, pos, cells, events). cells is bytes. Same order of checks as play.c"""
        dx, dy = DIRS[d]
        g = bytearray(cells)
        fish = g.count(FISH)
        steps = 0
        events = []
        while True:
            j, c = self.cell(g, pos, dx, dy)
            if c in (SNOW, ROCK) or (c == EXIT and fish):
                break
            if c == BLOCK:
                # push: the seal stops here, the block slides on over ice
                k, m = self.cell(g, j, dx, dy)
                if m not in (ICE, WATER, SEA):
                    break
                g[j] = ICE
                b = j
                while True:
                    k, m = self.cell(g, b, dx, dy)
                    if m == ICE:
                        b = k
                        continue
                    if m == WATER:
                        g[k] = ICE       # fills the hole
                        events.append('fill')
                    elif m == SEA:
                        events.append('sink')
                    else:
                        g[b] = BLOCK
                    break
                events.append('push')
                return MOVED, pos, bytes(g), events
            if c in (SEA, WATER):
                return FAIL, pos, cells, events
            if g[pos] == CRACK:
                g[pos] = WATER
                events.append('crack')
            pos = j
            steps += 1
            if steps > MAX_STEPS:
                return FAIL, pos, cells, events + ['loop']
            if c == FISH:
                g[j] = ICE
                fish -= 1
                events.append('fish')
            elif c == EXIT:
                return WIN, pos, bytes(g), events
            elif is_hole(c):
                pos = self.hole_at[partner(c)]
                events.append('hole')
        if steps == 0:
            return NOMOVE, pos, cells, events
        return MOVED, pos, bytes(g), events

    def solve(self, limit=400000):
        """BFS -> (moves string or None, states explored)"""
        start = (self.start, self.cells)
        prev = {start: None}
        q = deque([start])
        while q:
            st = q.popleft()
            pos, cells = st
            for d in 'URDL':
                r, p2, c2, ev = self.move(pos, cells, d)
                if r in (FAIL, NOMOVE):
                    continue
                if r == WIN:
                    path = [d]
                    s = st
                    while prev[s] is not None:
                        s, dd = prev[s]
                        path.append(dd)
                    return ''.join(reversed(path)), len(prev)
                ns = (p2, c2)
                if ns not in prev:
                    prev[ns] = (st, d)
                    if len(prev) > limit:
                        return None, len(prev)
                    q.append(ns)
        return None, len(prev)

    def replay(self, moves):
        pos, cells = self.start, self.cells
        evs = []
        for d in moves:
            r, pos, cells, ev = self.move(pos, cells, d)
            evs.append((d, r, ev))
            if r == WIN:
                return True, evs
            assert r == MOVED, (self.name, moves, d, r)
        return False, evs


def parse(text):
    """levels.txt -> [(world, Floe)]. '= World name' starts a world, '- Floe name' a floe."""
    floes = []
    world = None
    name = None
    rows = []

    def flush():
        if name is not None:
            floes.append((world, Floe(name, rows)))

    for line in text.splitlines():
        if line.startswith(';') or not line.strip():
            continue
        if line.startswith('= '):
            flush()
            name, rows = None, []
            world = line[2:].strip()
        elif line.startswith('- '):
            flush()
            name, rows = line[2:].strip(), []
        else:
            rows.append(line.rstrip())
    flush()
    return floes
