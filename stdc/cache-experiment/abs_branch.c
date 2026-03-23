/* abs_branch.c - 有分支版本 */
#include <stdio.h>

int abs_branch(int n)
{
    if (n < 0) return -n;
    return n;
}

int main(void)
{
    volatile int x = -5;
    printf("%d\n", abs_branch(x));
    return 0;
}