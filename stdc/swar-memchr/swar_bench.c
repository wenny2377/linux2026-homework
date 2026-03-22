#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

/*
 * SWAR (SIMD Within A Register) zero-byte detection.
 *
 * Treat a 64-bit word as 8 independent 8-bit lanes.
 * XOR with a broadcast of the target byte turns the target lane to 0x00,
 * then the standard has-zero-byte trick detects it without branching.
 *
 * Reference: Linux kernel include/linux/word-at-a-time.h
 */
static inline bool has_byte_eq(uint64_t v, uint8_t target)
{
    v ^= (uint64_t)target * 0x0101010101010101ULL;
    return ((v - 0x0101010101010101ULL) & ~v & 0x8080808080808080ULL) != 0;
}

/* SWAR memchr: process 8 bytes per iteration */
static const void *swar_memchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s;
    uint8_t target = (uint8_t)c;

    /* Head: align to 8-byte boundary */
    while (n && ((uintptr_t)p & 7)) {
        if (*p == target) return p;
        p++; n--;
    }

    /* Main loop: 8 bytes at a time */
    while (n >= 8) {
        uint64_t word;
        memcpy(&word, p, 8);
        if (has_byte_eq(word, target)) {
            for (int i = 0; i < 8; i++)
                if (p[i] == target) return p + i;
        }
        p += 8; n -= 8;
    }

    /* Tail: remaining bytes */
    while (n--) {
        if (*p == target) return p;
        p++;
    }
    return NULL;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#define BUF_SIZE (1 << 24)  /* 16 MB */
#define REPEAT   100

int main(void)
{
    char *buf = malloc(BUF_SIZE);
    memset(buf, 'A', BUF_SIZE);
    buf[BUF_SIZE - 1] = '\0'; /* target at end — worst case */

    volatile const void *r;
    double t0, t_std, t_swar;

    t0 = now_sec();
    for (int i = 0; i < REPEAT; i++)
        r = memchr(buf, '\0', BUF_SIZE);
    t_std = now_sec() - t0;

    t0 = now_sec();
    for (int i = 0; i < REPEAT; i++)
        r = swar_memchr(buf, '\0', BUF_SIZE);
    t_swar = now_sec() - t0;

    printf("buffer size : %d bytes\n", BUF_SIZE);
    printf("repeat      : %d\n", REPEAT);
    printf("glibc memchr: %.4f s\n", t_std);
    printf("swar_memchr : %.4f s\n", t_swar);
    printf("speedup     : %.2fx\n", t_std / t_swar);
    printf("(r=%p prevents optimization)\n", r);

    /* Correctness check */
    for (int pos = 0; pos < 100; pos++) {
        buf[pos] = '\x01';
        const void *a = memchr(buf, '\x01', BUF_SIZE);
        const void *b = swar_memchr(buf, '\x01', BUF_SIZE);
        if (a != b) {
            printf("MISMATCH at pos=%d: memchr=%p swar=%p\n", pos, a, b);
            free(buf);
            return 1;
        }
        buf[pos] = 'A';
    }
    printf("correctness : OK\n");

    free(buf);
    return 0;
}