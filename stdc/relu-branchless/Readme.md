# relu-branchless

Benchmark branch-based ReLU against a branchless bit-manipulation
implementation over 10⁸ uniformly distributed inputs in [-1, +1].

## How the branchless version works

ReLU clamps negative values to zero. The branchless version propagates
the IEEE-754 sign bit with an arithmetic right shift to build a mask,
then uses the mask to zero out negative inputs:

```c
/* branch version */
float relu_branch(float x)    { return x > 0.0f ? x : 0.0f; }

/* branchless version — sign bit mask */
float relu_branchless(float x)
{
    uint32_t bits;
    memcpy(&bits, &x, sizeof bits);          /* type-safe type punning */
    uint32_t mask = ~((uint32_t)((int32_t)bits >> 31)); /* 0 if negative, ~0 if positive */
    bits &= mask;
    memcpy(&x, &bits, sizeof bits);
    return x;
}
```

`memcpy`-based type punning is used instead of a `union` cast because
it is safe under strict aliasing rules and GCC -O2 compiles it to a
single `movd` instruction.

## Observed results (AMD Ryzen 7 5700X, 10⁸ inputs, gcc -O2)

```
branch       = 0.2596 s
branchless   = 0.2596 s
speedup      = 1.00×
```

```
perf stat ./relu_bench:
    14,641,037  branch-misses   # 0.72% of all branches
 2,021,907,797  branches
 5,590,593,326  cycles
     1.205165   seconds time elapsed
```

## Why both versions have identical performance

Branch miss rate is only **0.72%** despite uniformly distributed
random inputs (theoretical 50% misprediction). GCC -O2 automatically
compiles the ternary `x > 0.0f ? x : 0.0f` to `MAXSS` or `CMOVSS` —
a conditional move instruction that does not involve the branch
predictor at all. Both the "branch" and "branchless" versions generate
identical branch-free machine code.

This demonstrates an important principle: **the compiler eliminates
branches when it recognises the pattern**. Manual branchless code has
value primarily where the compiler cannot optimise automatically —
for example, complex conditional logic, or security-sensitive paths
where predictable timing is required regardless of compiler version.

## Assembly comparison (gcc -O2, x86-64)

```asm
# relu_branch — no branch instruction generated
relu_branch:
    xorps  %xmm1, %xmm1
    maxss  %xmm1, %xmm0    # MAXSS: branchless max of two floats
    ret

# relu_branchless — explicit bitmask path
relu_branchless:
    movd   %xmm0, %eax
    sarl   $31, %eax        # arithmetic shift: mask = 0 or 0xFFFFFFFF
    notl   %eax
    andl   %eax, ...
    ...
    ret
```

## SIMD advantage of branchless design

A branchless structure vectorises naturally. With AVX2, 8 floats can be
processed simultaneously with a single `VMAXPS` instruction:

```c
#include <immintrin.h>
__m256 relu_avx(__m256 x) {
    return _mm256_max_ps(x, _mm256_setzero_ps());  /* 8 floats at once */
}
```

On RISC-V with the V extension, the same pattern applies with `vfmax.vv`.
A branchy implementation cannot be auto-vectorised because the branch
creates a data-dependent control flow that the vectoriser cannot flatten.

## Relation to Linux kernel

`include/linux/minmax.h` implements `min()` and `max()` using the
ternary operator, which GCC optimises to `CMOV` — the same technique
observed here. The `(void)(&_min1 == &_min2)` trick enforces same-type
comparison at compile time.

For kernel AI inference modules, the preferred implementation is
`memcpy`-based type punning (not `union`) because Linux builds with
`-fno-strict-aliasing`, but `memcpy` is portable across all
optimisation levels and compilers without relying on that flag.

## Build and run

```bash
make run     # timing only
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
