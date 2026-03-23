# hash-distribution

Compare the bucket distribution of four 32-bit multiplicative hash
constants over keys 0–10000 with 1024 buckets, and verify the
Three-gap theorem prediction for Fibonacci hashing.

## Background

Linux kernel `hash_32()` uses Fibonacci hashing:

```c
/* include/linux/hash.h */
static inline u32 hash_32(u32 val, unsigned int bits)
{
    return (val * GOLDEN_RATIO_32) >> (32 - bits);
}
#define GOLDEN_RATIO_32  0x61C88647
```

The constant `0x61C88647 = ⌊2³² × φ⌋` where `φ = (√5−1)/2 ≈ 0.618`
is the golden ratio. Multiplying by an irrational number's approximation
scatters equal-stride key sequences (like file descriptors or PIDs)
according to the **Three-gap theorem**: the fractional parts of
`{kφ mod 1 : k = 0, 1, …, n}` occupy intervals of at most 3 distinct
lengths, guaranteeing near-uniform distribution.

Taking the **high bits** of the product (not low bits) ensures the hash
is sensitive to all input bits, not just the low-order ones.

## Constants compared

| Constant | Description |
|----------|-------------|
| `0x61C88647` | Golden ratio — Linux `hash_32()` default |
| `0x80000000` | Power of two — pathological case |
| `0x12345678` | Structured constant — periodic distribution |
| `0x54061094` | Arbitrary odd constant — near-golden behaviour |

## Observed results (keys 0–10000, 1024 buckets)

```
Constant              std      max    empty
0x61C88647 (golden)  0.674     11        0   ← near-perfect uniform
0x80000000           220.8   5001     1022   ← degenerate: only 2 buckets used
0x12345678            18.4     45      798   ← periodic gaps
0x54061094             0.873   12        0   ← good, but no theorem guarantee
```

**Why `0x80000000` is catastrophic:**
`k * 0x80000000 = k << 31` — only bit 31 survives. After right-shifting
by 22, the result is either 0 (even k) or 512 (odd k). All 10001 keys
land in exactly 2 buckets out of 1024, degrading O(1) lookup to O(n).

This is why the Linux kernel requires hash table sizes to be powers of
two **and** uses a well-chosen multiplicative constant — the size alone
is not sufficient for good distribution.

## Ring theory perspective

`GOLDEN_RATIO_32 = 0x61C88647` is odd, so `gcd(C, 2³²) = 1`, which
means it is a **unit** in the ring ℤ/2³²ℤ. Multiplication by C is
therefore a bijection on ℤ/2³²ℤ — every input maps to a distinct
output before the right-shift.

However, `hash_32(val, bits) = (val * C) >> (32 - bits)` is **not**
invertible: the right-shift discards `(32 - bits)` bits, so each
output corresponds to `2^(32 - bits)` distinct inputs. Hash functions
require distribution, not invertibility.

## How to run

```bash
python3 hash_dist.py        # print summary table
python3 hash_dist.py --plot # show bucket occupancy histogram (requires matplotlib)
```

Core logic:

```python
def hash32(k, c, bits=10):
    return ((k * c) & 0xFFFF_FFFF) >> (32 - bits)

buckets = [0] * 1024
for k in range(10001):
    buckets[hash32(k, CONSTANT)] += 1
```

## Relation to Linux kernel

```bash
grep -n "GOLDEN_RATIO_32\|hash_32" include/linux/hash.h
git log --oneline -- lib/hashtable.c include/linux/hashtable.h | head -10
```

The same constant is used throughout the kernel for:
- `pid_hash()` — process ID to hash table bucket
- `inet_ehashfn()` — TCP connection lookup
- `d_hash()` — dentry cache lookup

## Test environment

- Python 3.10, matplotlib 3.5 (optional for plots)
- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
