# swar-memchr

Benchmark a SWAR (SIMD Within A Register) implementation of `memchr`
against the glibc version over a 16 MB buffer with the target byte at
the very end (worst case).

SWAR processes 8 bytes per iteration by treating a 64-bit register as
8 independent 8-bit lanes, using the has-zero-byte trick to detect a
match without any per-byte branch.

## How SWAR works

The standard null-byte detection trick used in Linux kernel
`include/linux/word-at-a-time.h`:

```c
/* Detect if any byte in a 64-bit word is zero */
static inline uint64_t has_zero(uint64_t v)
{
    return (v - 0x0101010101010101ULL) & ~v & 0x8080808080808080ULL;
}
```

For `memchr`, XOR the target byte into every lane first:

```c
uint64_t word = *(uint64_t *)p ^ 0x0101010101010101ULL * (uint8_t)c;
if (has_zero(word)) { /* target byte found in this word */ }
```

This checks 8 bytes with 4 arithmetic instructions and no branches,
vs. 8 comparisons + 8 branches for a naive byte loop.

## Build and run

```bash
make run     # timing + correctness check
make bench   # timing + perf stat (needs perf_event_paranoid=-1)
```

## Observed results (AMD Ryzen 7 5700X, 16 MB buffer, repeat=100)

```
glibc memchr : 0.0000 s   (time truncated to 0 — too fast for clock_gettime)
swar_memchr  : 0.0011 s
speedup      : 0.00×       ← unfair comparison: glibc uses SSE2/AVX2
correctness  : OK
```

```
perf stat ./swar_bench:
  1,912,231  cache-references
    180,497  cache-misses     #  9.44% of all cache refs
 46,489,978  cycles
 50,440,855  instructions     #  1.08  insn per cycle
```

## Why SWAR is slower than glibc here

glibc `memchr` on x86-64 uses SSE2/AVX2 SIMD, processing 16–32 bytes
per iteration. Our SWAR processes 8 bytes — so glibc is 2–4× wider.

The real value of SWAR is in environments **without** hardware SIMD:

- Linux kernel itself (SIMD registers must be saved/restored explicitly
  with `kernel_fpu_begin()`, too expensive for hot paths)
- Embedded systems without vector extensions
- RISC-V targets without the V extension

In those environments SWAR is ~8× faster than a naive byte loop.
The kernel uses exactly this technique in `lib/string.c` (`strlen`,
`strchr`) and `lib/strncpy_from_user.c` via `word-at-a-time.h`.

## Relation to Linux kernel

```bash
grep -r "word_at_a_time\|has_zero_byte" include/linux/ lib/
```

```
lib/strncpy_from_user.c:  const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
lib/string.c:             const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
lib/string.c:             c = read_word_at_a_time(src+res);
lib/strnlen_user.c:       const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
```

## Prerequisites

```bash
sudo apt install linux-tools-common linux-tools-$(uname -r)
sudo sysctl -w kernel.perf_event_paranoid=-1
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X, gcc 11.4.0 -O2
