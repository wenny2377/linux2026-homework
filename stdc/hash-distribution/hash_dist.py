"""
hash_dist.py  --  Compare bucket distribution of four multiplicative hash constants.

Usage:
    python3 hash_dist.py          # save hash_dist.png
    python3 hash_dist.py --show   # also open a display window
"""

import sys
import numpy as np
import matplotlib.pyplot as plt

CONSTANTS = {
    'GOLDEN_RATIO\n0x61C88647': 0x61C88647,
    '0x80000000':               0x80000000,
    '0x12345678':               0x12345678,
    '0x54061094':               0x54061094,
}

KEYS    = range(10001)   # 0 ~ 10000
BUCKETS = 1024           # 2^10
BITS    = 10             # log2(BUCKETS)


def hash32(k, c):
    """Multiplicative hash: take the top BITS bits of k*c (mod 2^32)."""
    return ((k * c) & 0xFFFF_FFFF) >> (32 - BITS)


def compute_dist(c):
    counts = [0] * BUCKETS
    for k in KEYS:
        counts[hash32(k, c)] += 1
    return counts


def main():
    show = '--show' in sys.argv

    fig, axes = plt.subplots(2, 2, figsize=(14, 8))
    fig.suptitle('Hash Distribution  |  keys 0–10000  |  buckets=1024',
                 fontsize=13)

    stats = {}
    for ax, (name, c) in zip(axes.flat, CONSTANTS.items()):
        counts = compute_dist(c)
        arr   = np.array(counts)
        std   = arr.std()
        maxi  = arr.max()
        empty = int((arr == 0).sum())
        stats[name.replace('\n', ' ')] = {
            'std': round(float(std), 3),
            'max': int(maxi),
            'empty_buckets': empty,
        }

        ax.bar(range(BUCKETS), counts, width=1, color='steelblue', alpha=0.7)
        ax.set_title(f'{name}\nstd={std:.3f}  max={maxi}  empty={empty}',
                     fontsize=9)
        ax.set_xlabel('bucket index')
        ax.set_ylabel('occupancy')

    plt.tight_layout()
    plt.savefig('hash_dist.png', dpi=150)
    print('Saved: hash_dist.png')

    print('\n=== Summary ===')
    print(f'{"constant":30s}  {"std":>7}  {"max":>5}  {"empty":>6}')
    for name, s in stats.items():
        print(f'{name:30s}  {s["std"]:7.3f}  {s["max"]:5d}  {s["empty_buckets"]:6d}')

    if show:
        plt.show()


if __name__ == '__main__':
    main()