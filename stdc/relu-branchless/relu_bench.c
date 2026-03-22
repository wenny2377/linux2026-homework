#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define COUNT 100000000

/* Method 1: branch-based ReLU */
static float relu_branch(float x)
{
    return x > 0.0f ? x : 0.0f;
}

/*
 * Method 2: branchless ReLU using type punning via memcpy (portable, C11).
 *
 * The sign bit of the IEEE-754 representation is propagated to all bits
 * by an arithmetic right shift, producing a mask of all 0s (positive)
 * or all 1s (negative).  ANDing the original bits with ~mask zeroes the
 * value when negative.
 *
 * Note: right-shifting a signed integer is implementation-defined in C11
 * (§6.5.7 p5), but all major architectures (x86, ARM, RISC-V) use an
 * arithmetic shift, so this works in practice.
 */
static float relu_branchless(float x)
{
    uint32_t bits;
    memcpy(&bits, &x, sizeof bits);
    int32_t sign = (int32_t)bits >> 31;   /* 0x00000000 or 0xFFFFFFFF */
    bits &= ~(uint32_t)sign;
    memcpy(&x, &bits, sizeof bits);
    return x;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
    /* Uniform input in [-1, +1] — branch predictor cannot predict well */
    float *data = malloc(COUNT * sizeof *data);
    srand(42);
    for (int i = 0; i < COUNT; i++)
        data[i] = (float)(rand() - RAND_MAX / 2) / (RAND_MAX / 2);

    volatile float sink = 0;
    double t0, t_branch, t_branchless;

    t0 = now_sec();
    for (int i = 0; i < COUNT; i++)
        sink += relu_branch(data[i]);
    t_branch = now_sec() - t0;

    t0 = now_sec();
    for (int i = 0; i < COUNT; i++)
        sink += relu_branchless(data[i]);
    t_branchless = now_sec() - t0;

    printf("count        = %d\n", COUNT);
    printf("branch       = %.4f s\n", t_branch);
    printf("branchless   = %.4f s\n", t_branchless);
    printf("speedup      = %.2fx\n", t_branch / t_branchless);
    printf("(sink=%.1f prevents dead-code elimination)\n", (double)sink);

    free(data);
    return 0;
}