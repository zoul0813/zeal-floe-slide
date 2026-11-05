#!/usr/bin/env python3
"""Pick 8 floes per world from gen_levels.py candidates along a par ramp and write tools/levels.txt.
Floe 1-1 is hand made. Run once; levels.txt is then the source (hand edits welcome),
make_levels.py turns it into src/levels.c.
"""
import os
import sys
import random
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gen_levels import job  # noqa: E402

NAMES = {1: 'FAST ICE', 2: 'BREATHING HOLES', 3: 'ICE BLOCKS', 4: 'THIN ICE'}
# wanted par per floe, and extra events wanted for some
RAMP = {
    1: [None, 4, 5, 6, 8, 9, 11, 13],
    2: [4, 6, 7, 9, 10, 12, 14, 16],
    3: [4, 6, 8, 10, 11, 13, 15, 18],
    4: [6, 8, 10, 12, 14, 16, 19, 22],
}
WANT = {3: {4: 'fill', 6: 'fill', 7: 'fill'}, 4: {5: 'hole', 6: 'push', 7: 'hole'}}
FIRST = """\
~~~~~~~~~~
~########~
~#@....f#~
~#......#~
~#......~~
~#E.....#~
~########~
~~~~~~~~~~"""


def main():
    out = ['; Floe Slide levels. Made by tools/pick_levels.py, then edited by hand.',
           '; make_levels.py checks every floe with the solver and writes src/levels.c.', '']
    for w in (1, 2, 3, 4):
        with Pool() as p:
            res = p.map(job, [(w, 7000 + w * 100 + i, 4000, 3) for i in range(8)])
        cand = [r for rr in res for r in rr]
        used = set()
        out.append('= ' + NAMES[w])
        out.append('')
        for i, par in enumerate(RAMP[w]):
            if par is None:
                out.append('- 1')
                out += FIRST.splitlines()
                out.append('')
                continue
            want = WANT.get(w, {}).get(i)
            best = None
            for (s, rows) in cand:
                p_, states, sol, ev = s
                key = tuple(rows)
                if key in used or not (par <= p_ <= par + 3):
                    continue
                if want and want not in ev:
                    continue
                area = len(rows) * len(rows[0])
                # early floes small, later ones roomy; then most branching
                size_pen = abs(area - (60 + i * 25))
                sc = (-(p_ - par), -size_pen, states)
                if best is None or sc > best[0]:
                    best = (sc, rows, s)
            assert best, (w, i, par)
            used.add(tuple(best[1]))
            out.append('- %d' % (i + 1))
            out += best[1]
            out.append('')
            print(w, i + 1, 'par', best[2][0], best[2][3], file=sys.stderr)
    with open(os.path.join(HERE, 'levels.txt'), 'w') as f:
        f.write('\n'.join(out))


if __name__ == '__main__':
    main()
