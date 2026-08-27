#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { HASH_CAPACITY = 7, STACK_CAPACITY = 4, QUEUE_CAPACITY = 4 };

typedef struct {
    int key;
    int payload;
} record_t;

typedef struct list_node {
    int value;
    const struct list_node *next;
} list_node_t;

typedef struct tree_node {
    int value;
    const struct tree_node *left;
    const struct tree_node *right;
} tree_node_t;

typedef struct {
    bool used;
    int key;
    int value;
} hash_slot_t;

typedef struct {
    hash_slot_t slots[HASH_CAPACITY];
    size_t size;
} hash_table_t;

typedef struct {
    int values[STACK_CAPACITY];
    size_t size;
} int_stack_t;

typedef struct {
    int values[QUEUE_CAPACITY];
    size_t head;
    size_t size;
} int_queue_t;

static int compare_record_key(const void *left, const void *right)
{
    const record_t *const lhs = left;
    const record_t *const rhs = right;

    return (lhs->key > rhs->key) - (lhs->key < rhs->key);
}

static size_t list_count(const list_node_t *node)
{
    size_t count = 0U;
    while (node != NULL) {
        ++count;
        node = node->next;
    }
    return count;
}

static long list_sum(const list_node_t *node)
{
    long sum = 0L;
    while (node != NULL) {
        sum += (long)node->value;
        node = node->next;
    }
    return sum;
}

static size_t tree_count(const tree_node_t *node)
{
    return (node == NULL)
               ? 0U
               : 1U + tree_count(node->left) + tree_count(node->right);
}

static size_t tree_height(const tree_node_t *node)
{
    size_t left_height;
    size_t right_height;

    if (node == NULL) {
        return 0U;
    }
    left_height = tree_height(node->left);
    right_height = tree_height(node->right);
    return 1U + ((left_height > right_height) ? left_height : right_height);
}

static bool tree_inorder(const tree_node_t *node,
                         int *values,
                         size_t capacity,
                         size_t *used)
{
    if (node == NULL) {
        return true;
    }
    if (!tree_inorder(node->left, values, capacity, used)) {
        return false;
    }
    if (*used == capacity) {
        return false;
    }
    values[*used] = node->value;
    *used += 1U;
    return tree_inorder(node->right, values, capacity, used);
}

static size_t hash_index(int key)
{
    return (size_t)key % (size_t)HASH_CAPACITY;
}

static bool hash_insert(hash_table_t *table, int key, int value)
{
    size_t probe;
    const size_t start = hash_index(key);

    for (probe = 0U; probe < (size_t)HASH_CAPACITY; ++probe) {
        hash_slot_t *const slot =
            &table->slots[(start + probe) % (size_t)HASH_CAPACITY];
        if (!slot->used || (slot->key == key)) {
            if (!slot->used) {
                table->size += 1U;
            }
            slot->used = true;
            slot->key = key;
            slot->value = value;
            return true;
        }
    }
    return false;
}

static const hash_slot_t *hash_find(const hash_table_t *table,
                                    int key,
                                    size_t *probes)
{
    size_t probe;
    const size_t start = hash_index(key);

    for (probe = 0U; probe < (size_t)HASH_CAPACITY; ++probe) {
        const hash_slot_t *const slot =
            &table->slots[(start + probe) % (size_t)HASH_CAPACITY];
        *probes = probe + 1U;
        if (!slot->used) {
            return NULL;
        }
        if (slot->key == key) {
            return slot;
        }
    }
    return NULL;
}

static bool stack_push(int_stack_t *stack, int value)
{
    if (stack->size == (size_t)STACK_CAPACITY) {
        return false;
    }
    stack->values[stack->size++] = value;
    return true;
}

static bool stack_pop(int_stack_t *stack, int *value)
{
    if (stack->size == 0U) {
        return false;
    }
    *value = stack->values[--stack->size];
    return true;
}

static bool queue_push(int_queue_t *queue, int value)
{
    size_t tail;
    if (queue->size == (size_t)QUEUE_CAPACITY) {
        return false;
    }
    tail = (queue->head + queue->size) % (size_t)QUEUE_CAPACITY;
    queue->values[tail] = value;
    queue->size += 1U;
    return true;
}

static bool queue_pop(int_queue_t *queue, int *value)
{
    if (queue->size == 0U) {
        return false;
    }
    *value = queue->values[queue->head];
    queue->head = (queue->head + 1U) % (size_t)QUEUE_CAPACITY;
    queue->size -= 1U;
    return true;
}

static int run_self_test(void)
{
    record_t records[] = {{42, 420}, {5, 50}, {31, 310}, {7, 70}, {19, 190}};
    const size_t record_count = sizeof records / sizeof records[0];
    const record_t search_key = {31, 0};
    const record_t *found;
    const list_node_t list_third = {5, NULL};
    const list_node_t list_second = {1, &list_third};
    const list_node_t list_first = {3, &list_second};
    const tree_node_t tree_left = {2, NULL, NULL};
    const tree_node_t tree_right = {6, NULL, NULL};
    const tree_node_t tree_root = {4, &tree_left, &tree_right};
    hash_table_t table = {{{false, 0, 0}}, 0U};
    int_stack_t stack = {{0}, 0U};
    int_queue_t queue = {{0}, 0U, 0U};
    const hash_slot_t *hash_result;
    size_t probes = 0U;
    int inorder[3] = {0, 0, 0};
    size_t inorder_count = 0U;
    int stack_value = 0;
    int queue_value = 0;

    qsort(records, record_count, sizeof records[0], compare_record_key);
    found = bsearch(&search_key,
                    records,
                    record_count,
                    sizeof records[0],
                    compare_record_key);

    if (!hash_insert(&table, 5, 50) || !hash_insert(&table, 12, 120) ||
        !hash_insert(&table, 19, 190)) {
        fputs("internal error: representative hash insertion failed\n", stderr);
        return 1;
    }
    hash_result = hash_find(&table, 19, &probes);

    if (!stack_push(&stack, 2) || !stack_push(&stack, 4) ||
        !stack_push(&stack, 6) || !stack_pop(&stack, &stack_value) ||
        !queue_push(&queue, 8) || !queue_push(&queue, 13) ||
        !queue_pop(&queue, &queue_value)) {
        fputs("internal error: bounded collection operation failed\n", stderr);
        return 1;
    }

    printf("array.sorted=%d,%d,%d,%d,%d\n",
           records[0].key,
           records[1].key,
           records[2].key,
           records[3].key,
           records[4].key);
    if (found == NULL) {
        fputs("internal error: bsearch did not find key 31\n", stderr);
        return 1;
    }
    printf("search.key=%d payload=%d\n", found->key, found->payload);
    printf("list.count=%zu sum=%ld\n",
           list_count(&list_first),
           list_sum(&list_first));
    if (!tree_inorder(&tree_root,
                      inorder,
                      sizeof inorder / sizeof inorder[0],
                      &inorder_count)) {
        fputs("internal error: tree traversal capacity failed\n", stderr);
        return 1;
    }
    printf("tree.nodes=%zu height=%zu inorder=%d,%d,%d\n",
           tree_count(&tree_root),
           tree_height(&tree_root),
           inorder[0],
           inorder[1],
           inorder[2]);
    if (hash_result == NULL) {
        fputs("internal error: hash lookup did not find key 19\n", stderr);
        return 1;
    }
    printf("hash.key=%d value=%d probes=%zu\n",
           hash_result->key,
           hash_result->value,
           probes);
    printf("stack.pop=%d remaining=%zu\n", stack_value, stack.size);
    printf("queue.pop=%d remaining=%zu\n", queue_value, queue.size);

    if ((records[0].key != 5) || (records[4].key != 42) ||
        (list_count(&list_first) != 3U) || (list_sum(&list_first) != 9L) ||
        (tree_count(&tree_root) != 3U) || (tree_height(&tree_root) != 2U) ||
        (inorder_count != 3U) || (inorder[0] != 2) ||
        (inorder[1] != 4) || (inorder[2] != 6) ||
        (hash_result->value != 190) || (probes != 3U) ||
        (stack_value != 6) || (stack.size != 2U) ||
        (queue_value != 8) || (queue.size != 1U)) {
        fputs("internal error: self-test invariant failed\n", stderr);
        return 1;
    }

    puts("self-test=PASS");
    return 0;
}

static int run_negative_test(void)
{
    hash_table_t table = {{{false, 0, 0}}, 0U};
    int key;

    for (key = 0; key < HASH_CAPACITY; ++key) {
        if (!hash_insert(&table, key, key * 10)) {
            fputs("internal error: table filled too early\n", stderr);
            return 1;
        }
    }
    if (hash_insert(&table, 99, 990)) {
        fputs("internal error: full table accepted key 99\n", stderr);
        return 1;
    }

    fputs("error: hash table full for key 99\n", stderr);
    return 2;
}

int main(int argc, char **argv)
{
    if ((argc == 2) && (strcmp(argv[1], "--self-test") == 0)) {
        return run_self_test();
    }
    if ((argc == 2) && (strcmp(argv[1], "--negative") == 0)) {
        return run_negative_test();
    }

    fputs("usage: b06_dsa_demo --self-test|--negative\n", stderr);
    return 64;
}
