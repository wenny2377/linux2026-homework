CC     = gcc
CFLAGS = -O2 -Wall -Wextra

all: isqrt_bench

isqrt_bench: isqrt_bench.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./isqrt_bench

bench: all
	perf stat -e cache-references,cache-misses,cycles,instructions ./isqrt_bench

clean:
	rm -f isqrt_bench