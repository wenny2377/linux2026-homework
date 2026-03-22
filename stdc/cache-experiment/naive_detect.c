#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct node {
    int val;
    struct node *next;
} node_t;

static void shuffle(node_t **a, size_t n)
{
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        node_t *t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static node_t *build_list(size_t n)
{
    node_t **pool = malloc(n * sizeof *pool);
    for (size_t i = 0; i < n; i++) {
        pool[i] = malloc(sizeof **pool);
        pool[i]->val = (int)i;
    }
    shuffle(pool, n);
    for (size_t i = 0; i < n - 1; i++)
        pool[i]->next = pool[i + 1];
    pool[n - 1]->next = NULL;
    node_t *head = pool[0];
    free(pool);
    return head;
}

/* Naive traversal: walk the list from head to tail */
static long naive_traverse(node_t *head)
{
    long count = 0;
    for (node_t *cur = head; cur; cur = cur->next)
        count++;
    return count;
}

static void free_list(node_t *head)
{
    while (head) {
        node_t *next = head->next;
        free(head);
        head = next;
    }
}

int main(int argc, char *argv[])
{
    size_t n = 1000000;
    if (argc > 2 && strcmp(argv[1], "--length") == 0)
        n = (size_t)atol(argv[2]);

    srand((unsigned)time(NULL));
    node_t *head = build_list(n);
    long result = naive_traverse(head);
    printf("naive traverse steps: %ld (n=%zu)\n", result, n);
    free_list(head);
    return 0;
}