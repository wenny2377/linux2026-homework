#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

/*
 * Method 1: digit-by-digit (same algorithm as Linux kernel lib/int_sqrt.c).
 * Only addition, subtraction, and bit shifts — no division.
 * Iterations are fixed (equal to the number of bits), making WCET predictable.
 */
static uint32_t isqrt_digit(uint32_t x)
{
    if (x == 0) return 0;
    uint32_t op = x, res = 0;
    uint32_t one = 1u << 30; /* highest 2^(2k) <= x */
    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) {
            op  -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

/*
 * Method 2: Newton's method (integer version).
 * Quadratic convergence but requires integer division each iteration
 * and has a variable iteration count — unsuitable for real-time paths.
 */
static uint32_t isqrt_newton(uint32_t n)
{
    if (n == 0) return 0;
    uint32_t x = n, y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n / x) >> 1; }
    return x;
}

/*
 * Method 3: binary search.
 * Fixed iteration count (log2 of range), no division.
 */
static uint32_t isqrt_binary(uint32_t n)
{
    if (n == 0) return 0;
    uint32_t lo = 0, hi = 65536, mid;
    while (lo + 1 < hi) {
        mid = lo + (hi - lo) / 2;
        if ((uint64_t)mid * mid <= n) lo = mid;
        else                          hi = mid;
    }
    return lo;
}

/*
 * Safe version: defend against untrusted input (network packets, etc.).
 * Clamps extreme values and bounds the iteration count.
 */
static uint32_t isqrt_safe(uint32_t n)
{
    if (n == 0)          return 0;
    if (n >= 0xFFFFFFFFu) return 65535;
    uint32_t x = n, y = (x + 1) >> 1;
    int iter = 0;
    while (y < x && iter++ < 64) {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#define ITER 10000000u

int main(void)
{
    /* Correctness check */
    printf("=== Correctness (first 20 values) ===\n");
    printf("%6s  %8s  %8s  %8s  %8s\n",
           "n", "digit", "newton", "binary", "safe");
    for (uint32_t n = 0; n < 20; n++)
        printf("%6u  %8u  %8u  %8u  %8u\n",
               n, isqrt_digit(n), isqrt_newton(n),
               isqrt_binary(n), isqrt_safe(n));

    /* Performance comparison */
    volatile uint32_t sink = 0;
    double t0, elapsed;

    t0 = now_sec();
    for (uint32_t i = 0; i < ITER; i++) sink += isqrt_digit(i);
    elapsed = now_sec() - t0;
    printf("\ndigit-by-digit : %.4f s  (%u iterations)\n", elapsed, ITER);

    t0 = now_sec();
    for (uint32_t i = 0; i < ITER; i++) sink += isqrt_newton(i);
    elapsed = now_sec() - t0;
    printf("newton         : %.4f s\n", elapsed);

    t0 = now_sec();
    for (uint32_t i = 0; i < ITER; i++) sink += isqrt_binary(i);
    elapsed = now_sec() - t0;
    printf("binary search  : %.4f s\n", elapsed);

    printf("(sink=%u)\n", sink);
    return 0;
}