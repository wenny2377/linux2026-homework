# float-bits

Inspect the IEEE-754 bit layout (sign, exponent, mantissa) of `0.1` in
single and double precision, and demonstrate that `0.1 + 0.2 != 0.3`
and that floating-point addition is not associative.

## Build and run

```bash
make run
```

## Expected output (excerpt)

```
=== float 0.1f ===
  sign     = 0
  exponent = 123  (biased, actual = -4)
  mantissa = 0x4CCCCD
  hex      = 0x3DCCCCCD

=== 0.1 + 0.2 == 0.3 ? ===
  double: false  (0.1+0.2 = 0.30000000000000004441...)

=== Associativity check: (a+b)+c vs a+(b+c) ===
  (a+b)+c = 1.0
  a+(b+c) = 0.0
```

## Test environment

- Linux 6.8.0-106-generic, Ubuntu 22.04.5 LTS
- AMD Ryzen 7 5700X, gcc 11.4.0