CC     = gcc
CFLAGS = -O2 -Wall -Wextra

all: float_bits

float_bits: float_bits.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./float_bits

clean:
	rm -f float_bits