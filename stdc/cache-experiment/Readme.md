# cache-experiment

Compare the cache behavior of fast-slow pointer traversal vs. naive
(single-pointer) traversal on a randomly-allocated linked list.

Nodes are allocated individually with `malloc` and then shuffled using
Fisher-Yates, so adjacent nodes in the list are scattered across virtual
memory — this models the realistic case in the Linux kernel where linked
list nodes are not contiguous.

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
and misses respectively.  Run `perf list | grep -i cache` to see all
events available on your CPU.

## Run all list lengths at once

```bash
make bench 2>&1 | tee result.txt
```

## Record call graph for hotspot analysis

```bash
make record
perf report --stdio | head -60
```

## Prerequisites

```bash
sudo apt install linux-tools-common linux-tools-$(uname -r)
sudo sysctl -w kernel.perf_event_paranoid=1
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X (L1d 256 KiB / L2 4 MiB / L3 32 MiB)
- gcc 11.4.0