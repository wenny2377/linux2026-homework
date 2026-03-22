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
# Usage: bash run_all_perf.sh 2>&1 | tee all_results.txt

set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# ── Check perf_event_paranoid ────────────────────────────────────────
PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid)
if [ "$PARANOID" -gt 1 ]; then
    echo "ERROR: perf_event_paranoid=$PARANOID"
    echo "Fix:   sudo sysctl -w kernel.perf_event_paranoid=1"
    exit 1
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
echo " 1. cache-experiment"
echo "========================================"
cd "$ROOT/cache-experiment"
make -s all
for N in 10000 100000 1000000 10000000 100000000; do
    echo ""
    echo "--- fast_slow  N=$N ---"
    perf stat -e "$CACHE_EVENTS" \
        ./fast_slow --length "$N" 2>&1 | \
        grep -E "cache|L1-dcache|cycles|instructions|seconds"
    echo "--- naive      N=$N ---"
    perf stat -e "$CACHE_EVENTS" \
        ./naive_detect --length "$N" 2>&1 | \
        grep -E "cache|L1-dcache|cycles|instructions|seconds"
done

# ── 2. relu-branchless ───────────────────────────────────────────────
echo ""
echo "========================================"
echo " 2. relu-branchless"
echo "========================================"
cd "$ROOT/relu-branchless"
make -s all
./relu_bench
echo ""
perf stat -e branch-misses,branches,cycles \
    ./relu_bench 2>&1 | grep -E "branch|cycles|seconds"

# ── 3. isqrt ─────────────────────────────────────────────────────────
echo ""
echo "========================================"
echo " 3. isqrt"
echo "========================================"
cd "$ROOT/isqrt"
make -s all
./isqrt_bench

# ── 4. swar-memchr ───────────────────────────────────────────────────
echo ""
echo "========================================"
echo " 4. swar-memchr"
echo "========================================"
cd "$ROOT/swar-memchr"
make -s all
./swar_bench

echo ""
echo "========================================"
echo " Done"
echo "========================================"