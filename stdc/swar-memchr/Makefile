CC     = gcc
CFLAGS = -O2 -Wall -Wextra

all: swar_bench

swar_bench: swar_bench.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./swar_bench

bench: all
	perf stat -e cache-references,cache-misses,cycles,instructions ./swar_bench

clean:
	rm -f swar_bench