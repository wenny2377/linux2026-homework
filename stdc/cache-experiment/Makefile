CC      = gcc
CFLAGS  = -O2 -Wall -Wextra

all: fast_slow naive_detect

fast_slow: fast_slow.c
	$(CC) $(CFLAGS) -o $@ $<

naive_detect: naive_detect.c
	$(CC) $(CFLAGS) -o $@ $<

# Hardware events confirmed available on AMD Ryzen 7 5700X.
# cache-references / cache-misses map to LLC on AMD.
# L1-dcache-loads / L1-dcache-load-misses cover L1 data cache.
#
# Prerequisites:
#   sudo apt install linux-tools-common linux-tools-$(uname -r)
#   sudo sysctl -w kernel.perf_event_paranoid=1
PERF_EVENTS = cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions

bench: fast_slow naive_detect
	@echo "=== Environment ==="
	@uname -r
	@grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | xargs
	@lscpu | grep -i "l[123]. cache"
	@for N in 10000 100000 1000000 10000000 100000000; do \
	    echo ""; \
	    echo "=== N=$$N  fast_slow ==="; \
	    perf stat -e $(PERF_EVENTS) \
	        ./fast_slow --length $$N 2>&1; \
	    echo "=== N=$$N  naive_detect ==="; \
	    perf stat -e $(PERF_EVENTS) \
	        ./naive_detect --length $$N 2>&1; \
	done

# Record call graph and show top hotspots
record: fast_slow
	perf record -e cache-misses,L1-dcache-load-misses -g \
	    ./fast_slow --length 1000000
	perf report --stdio | head -60

clean:
	rm -f fast_slow naive_detect perf.data perf.data.old