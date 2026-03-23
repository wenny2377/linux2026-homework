# isqrt

Compare three integer square root implementations in terms of correctness,
performance, and suitability for the Linux kernel environment.

## Algorithms

| Method | Division needed | Iterations | WCET predictable |
|--------|:--------------:|:----------:|:---------------:|
| digit-by-digit | No | Fixed (bit width) | **Yes** |
| Newton's method | Yes (integer div) | Variable | No |
| Binary search | No | Fixed (log₂ range) | **Yes** |

`digit-by-digit` matches the algorithm used in `lib/int_sqrt.c` in the
Linux kernel. Each iteration only requires addition, subtraction, and
bit shifts — no division, no floating point.

## Why digit-by-digit is preferred in the Linux kernel

The Linux kernel cannot use floating-point operations in most contexts
(FPU state is not saved/restored on kernel entry). The `DIV` instruction
on x86-64 takes 20–90 cycles, while shift and add take 1 cycle each.
Newton's method converges in O(log log n) iterations but each iteration
costs an integer division, making it 5× slower in practice and unsuitable
for real-time paths due to variable iteration count.

## Observed results (AMD Ryzen 7 5700X, 10^7 iterations, gcc -O2)

```
digit-by-digit : 0.0575 s   ← 1.00× (baseline)
binary search  : 0.0999 s   ← 1.67×
newton         : 0.3033 s   ← 5.08×
```

```
perf stat ./isqrt_bench:
       501,016  cache-references
       130,638  cache-misses     # 26.07%
 2,140,350,065  cycles
 4,540,207,949  instructions     #  2.12  insn per cycle
```

Cache miss rate is 26% because the entire working set fits comfortably
in L3. The performance difference is purely instruction cost:
Newton's method's integer division dominates.

## Safe version for untrusted input

When input comes from an untrusted source (e.g. network packet parsing),
use the hardened version that guards against division by zero, integer
overflow, and infinite loops:

```c
uint32_t isqrt_safe(uint32_t n)
{
    if (n == 0) return 0;
    if (n >= 0xFFFFFFFFu) return 65535;

    uint32_t x = n, y = (x + 1) >> 1;
    int iter = 0;
    while (y < x && iter++ < 64) {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}
```

## Relation to Linux kernel

```bash
grep -n "int_sqrt\|isqrt" lib/int_sqrt.c
```

`lib/int_sqrt.c` uses digit-by-digit for exactly the reasons shown here:
no division, fixed iteration count, no floating point. The WCET is
`32 × C_iter` for a 32-bit input, making it suitable for real-time
scheduling paths.

## Build and run

```bash
make run     # correctness check + timing
make bench   # timing + perf stat (needs perf_event_paranoid=-1)
```

## Prerequisites

```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X (L1d 256 KiB / L2 4 MiB / L3 32 MiB)
- gcc 11.4.0 -O2
