# cache-experiment

Compare the cache behavior of fast-slow pointer traversal vs. naive
(single-pointer) traversal on a randomly-allocated linked list.

Nodes are allocated individually with `malloc` and then shuffled using
Fisher-Yates, so adjacent nodes in the list are scattered across virtual
memory — this models the realistic case in the Linux kernel where linked
list nodes are not contiguous.

## Files

| File | Description |
|------|-------------|
| `fast_slow.c` | Fast-slow pointer traversal; supports `--prefetch` flag to enable `__builtin_prefetch` two steps ahead |
| `naive_detect.c` | Single-pointer (naive) traversal for baseline comparison |
| `abs_branch.c` | Absolute value via `if (n < 0) return -n` — compiled to `CMOVS` by GCC -O2 |
| `abs_branchless.c` | Absolute value via bitmask `(mask ^ n) - mask` — explicit branchless |

## Build

```bash
make
```

## Run a single measurement

```bash
# Events confirmed available on AMD Ryzen 7 5700X
perf stat -e cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions \
    ./fast_slow --length 1000000

perf stat -e cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions \
    ./naive_detect --length 1000000
```

On AMD, `cache-references` and `cache-misses` map to LLC (L3) references
and misses respectively. Run `perf list | grep -i cache` to see all
events available on your CPU.

Note: `LLC-load-misses` and `mem-loads` are Intel PEBS-only events and
are not supported on AMD Ryzen. Use `L1-dcache-load-misses` instead.

## Run prefetch vs. original comparison

```bash
gcc -O2 -o fast_slow fast_slow.c

# original (no prefetch)
perf stat -e cache-references,cache-misses,cycles,instructions \
    ./fast_slow --length 1000000

# with __builtin_prefetch two steps ahead
perf stat -e cache-references,cache-misses,cycles,instructions \
    ./fast_slow --length 1000000 --prefetch
```

Observed result on AMD Ryzen 7 5700X (N=10^6):

| Version | cycles | cache-miss rate | time (s) |
|---------|-------:|:--------------:|--------:|
| original | 657,627,586 | 36.63% | 0.1441 |
| prefetch | 651,919,885 | 36.55% | 0.1429 |
| speedup | — | — | **1.01×** |

The improvement is only ~1% because `__builtin_prefetch(fast->next->next)`
prefetches only two steps ahead (~30 cycles), far less than the
150–200 cycle L3 miss penalty on this CPU. Cache-miss rates are nearly
identical, meaning the bottleneck is DRAM bandwidth, not instruction
latency. A larger prefetch distance (8–16 steps) would be needed to
hide the full miss penalty, but requires careful NULL-boundary handling.

## Run all list lengths at once

```bash
make bench 2>&1 | tee result.txt
```

## Record call graph for hotspot analysis

```bash
make record
perf report --stdio | head -60
```

Observed hotspot (N=10^6, event: L1-dcache-load-misses, 594 samples):

```
64.20%  62.44%  fast_slow  fast_slow   [.] main          ← traversal loop
17.21%   8.57%  fast_slow  libc.so.6   [.] _int_malloc   ← build_list
```

62% of L1 misses are inside the traversal loop — each `fast->next->next`
jump lands on a random cache line. The kernel symbols (`0xffffffff...`)
cannot be resolved without root access to `/proc/kallsyms` but account
for less than 1% of misses.

## abs branchless assembly comparison

```bash
gcc -O2 -S abs_branch.c     -o abs_branch.s
gcc -O2 -S abs_branchless.c -o abs_branchless.s
diff abs_branch.s abs_branchless.s
```

Key finding: GCC -O2 compiles `if (n < 0) return -n` to `CMOVS`
(conditional move), which is already branchless. The manual bitmask
version uses `SARL+XORL+SUBL` instead — different instructions, same
branch-free property, similar performance on x86-64.

```asm
# abs_branch (gcc -O2) — no branch instruction generated
abs_branch:
    movl  %edi, %eax
    negl  %eax
    cmovs %edi, %eax   # conditional move if sign (negative)
    ret

# abs_branchless (gcc -O2)
abs_branchless:
    movl  %edi, %edx
    sarl  $31, %edx    # arithmetic shift: mask = 0 or 0xFFFFFFFF
    xorl  %edx, %edi
    movl  %edi, %eax
    subl  %edx, %eax
    ret
```

## Prerequisites

```bash
sudo apt install linux-tools-common linux-tools-$(uname -r)
sudo sysctl -w kernel.perf_event_paranoid=-1
echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X (L1d 256 KiB / L2 4 MiB / L3 32 MiB)
- gcc 11.4.0, gcc 13.3.0 (assembly output)
