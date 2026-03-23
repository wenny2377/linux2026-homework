#!/usr/bin/env bash
# run_all_perf.sh
#
# Build and run all stdc experiments, collecting perf stat output.
#
# Events used (confirmed available on AMD Ryzen 7 5700X):
#   cache-references      -- LLC references (AMD hardware mapping)
#   cache-misses          -- LLC misses     (AMD hardware mapping)
#   L1-dcache-loads       -- L1 data cache loads
#   L1-dcache-load-misses -- L1 data cache load misses
#   cycles, instructions  -- basic pipeline counters
#
# Note: LLC-load-misses and mem-loads are Intel PEBS-only events,
#       not available on AMD Ryzen. Use cache-misses instead.
#
# Usage: bash run_all_perf.sh 2>&1 | tee all_results.txt

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# ── Check / fix perf_event_paranoid ──────────────────────────────────
PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid)
if [ "$PARANOID" -gt 0 ]; then
    echo "WARNING: perf_event_paranoid=$PARANOID — hardware events may be restricted."
    echo "         Run: sudo sysctl -w kernel.perf_event_paranoid=-1"
    echo "         Continuing anyway, some counters may show <not counted>."
    echo ""
fi

CACHE_EVENTS="cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions"

# ── Environment ──────────────────────────────────────────────────────
echo "========================================"
echo " Environment"
echo "========================================"
echo "Kernel : $(uname -r)"
echo "OS     : $(grep '^VERSION=' /etc/os-release | cut -d= -f2 | tr -d '"')"
echo "CPU    : $(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | xargs)"
lscpu | grep -i 'l[123]. cache'
echo ""

# ── 1. cache-experiment ──────────────────────────────────────────────
echo "========================================"
echo " 1. cache-experiment  (fast-slow vs naive vs prefetch)"
echo "========================================"
cd "$ROOT/cache-experiment"
make -s all

for N in 10000 100000 1000000 10000000 100000000; do
    echo ""
    echo "--- fast_slow      N=$N ---"
    perf stat -e "$CACHE_EVENTS" \
        ./fast_slow --length "$N" 2>&1 | \
        grep -E "cache|L1-dcache|cycles|instructions|seconds|steps"

    echo "--- fast_slow+prefetch N=$N ---"
    perf stat -e "$CACHE_EVENTS" \
        ./fast_slow --length "$N" --prefetch 2>&1 | \
        grep -E "cache|L1-dcache|cycles|instructions|seconds|steps"

    echo "--- naive          N=$N ---"
    perf stat -e "$CACHE_EVENTS" \
        ./naive_detect --length "$N" 2>&1 | \
        grep -E "cache|L1-dcache|cycles|instructions|seconds|steps"
done

# ── 2. float-bits ────────────────────────────────────────────────────
echo ""
echo "========================================"
echo " 2. float-bits  (IEEE 754 representation of 0.1)"
echo "========================================"
cd "$ROOT/float-bits"
make -s all
./float_bits

# ── 3. relu-branchless ───────────────────────────────────────────────
echo ""
echo "========================================"
echo " 3. relu-branchless  (branch vs branchless ReLU)"
echo "========================================"
cd "$ROOT/relu-branchless"
make -s all
./relu_bench
echo ""
perf stat -e branch-misses,branches,cycles \
    ./relu_bench 2>&1 | grep -E "branch|cycles|seconds"

# ── 4. isqrt ─────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo " 4. isqrt  (digit-by-digit vs Newton vs binary search)"
echo "========================================"
cd "$ROOT/isqrt"
make -s all
./isqrt_bench
echo ""
perf stat -e cycles,instructions \
    ./isqrt_bench 2>&1 | grep -E "cycles|instructions|seconds"

# ── 5. swar-memchr ───────────────────────────────────────────────────
echo ""
echo "========================================"
echo " 5. swar-memchr  (SWAR vs glibc memchr)"
echo "========================================"
cd "$ROOT/swar-memchr"
make -s all
./swar_bench
echo ""
perf stat -e "$CACHE_EVENTS" \
    ./swar_bench 2>&1 | grep -E "cache|cycles|instructions|seconds"

# ── 6. hash-distribution ─────────────────────────────────────────────
echo ""
echo "========================================"
echo " 6. hash-distribution  (Fibonacci hashing vs others)"
echo "========================================"
cd "$ROOT/hash-distribution"
if command -v python3 &>/dev/null; then
    python3 hash_dist.py
else
    echo "SKIP: python3 not found"
fi

# ── Done ─────────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo " Done"
echo "========================================"