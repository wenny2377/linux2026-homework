#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Print IEEE-754 single-precision fields of f */
static void print_float_bits(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);
    printf("  sign     = %u\n",  bits >> 31);
    printf("  exponent = %u  (biased, actual = %d)\n",
           (bits >> 23) & 0xFF,
           (int)((bits >> 23) & 0xFF) - 127);
    printf("  mantissa = 0x%06X  (implicit leading 1)\n",
           bits & 0x7FFFFF);
    printf("  hex      = 0x%08X\n", bits);
}

/* Print IEEE-754 double-precision fields of d */
static void print_double_bits(double d)
{
    uint64_t bits;
    memcpy(&bits, &d, sizeof bits);
    printf("  sign     = %llu\n",
           (unsigned long long)(bits >> 63));
    printf("  exponent = %llu  (biased, actual = %lld)\n",
           (unsigned long long)((bits >> 52) & 0x7FF),
           (long long)((bits >> 52) & 0x7FF) - 1023);
    printf("  mantissa = 0x%013llX\n",
           (unsigned long long)(bits & 0x000FFFFFFFFFFFFFULL));
}

int main(void)
{
    printf("=== float 0.1f ===\n");
    print_float_bits(0.1f);

    printf("\n=== double 0.1 ===\n");
    print_double_bits(0.1);

    printf("\n=== 0.1 + 0.2 == 0.3 ? ===\n");
    printf("  double: %s  (0.1+0.2 = %.20f)\n",
           (0.1 + 0.2 == 0.3) ? "true" : "false",
           0.1 + 0.2);
    printf("  float:  %s  (0.1f+0.2f = %.10f)\n",
           (0.1f + 0.2f == 0.3f) ? "true" : "false",
           (double)(0.1f + 0.2f));

    printf("\n=== Associativity check: (a+b)+c vs a+(b+c) ===\n");
    double a = 1e15, b = -1e15, c = 1.0;
    printf("  (a+b)+c = %.1f\n", (a + b) + c);
    printf("  a+(b+c) = %.1f\n",  a + (b + c));

    return 0;
}