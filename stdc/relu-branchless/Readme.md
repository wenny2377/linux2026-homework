# relu-branchless

Benchmark branch-based ReLU against a branchless bit-manipulation
implementation over 10^8 uniformly distributed inputs in [-1, +1].

The branchless version propagates the IEEE-754 sign bit with an arithmetic
right shift to build a mask, then clears the value when the input is
negative — no conditional branch is needed.

## Build and run

```bash
make run       # timing only
make bench     # timing + perf stat (needs perf_event_paranoid=1)
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X, gcc 11.4.0 -O2