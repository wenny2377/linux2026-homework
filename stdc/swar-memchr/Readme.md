# swar-memchr

Benchmark a SWAR (SIMD Within A Register) implementation of `memchr`
against the glibc version over a 16 MB buffer with the target byte at
the very end (worst case).

SWAR processes 8 bytes per iteration by treating a 64-bit register as
8 independent 8-bit lanes, using the has-zero-byte trick to detect a
match without any per-byte branch.

## Build and run

```bash
make run     # timing + correctness check
make bench   # timing + perf stat (needs perf_event_paranoid=1)
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X, gcc 11.4.0 -O2