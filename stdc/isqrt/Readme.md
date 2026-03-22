# isqrt

Compare three integer square root implementations in terms of correctness,
performance, and suitability for the Linux kernel environment.

| Method | Division needed | Iterations | WCET predictable |
|--------|----------------|------------|-----------------|
| digit-by-digit | No | Fixed (bit width) | Yes |
| Newton's method | Yes | Variable | No |
| Binary search | No | Fixed (log2 range) | Yes |

`digit-by-digit` matches the algorithm used in `lib/int_sqrt.c` in the
Linux kernel.

## Build and run

```bash
make run     # correctness check + timing
make bench   # timing + perf stat (needs perf_event_paranoid=1)
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X, gcc 11.4.0 -O2