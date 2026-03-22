CC     = gcc
CFLAGS = -O2 -Wall -Wextra

all: relu_bench

relu_bench: relu_bench.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./relu_bench

# Requires: sudo sysctl -w kernel.perf_event_paranoid=1
bench: all
	@echo "=== relu: branch vs branchless ==="
	perf stat -e branch-misses,branches,cycles ./relu_bench

clean:
	rm -f relu_bench