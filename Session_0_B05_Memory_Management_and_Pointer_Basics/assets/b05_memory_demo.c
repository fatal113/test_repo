#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} int_vector_t;

static void vector_init(int_vector_t *vector)
{
    vector->data = NULL;
    vector->size = 0U;
    vector->capacity = 0U;
}

static void vector_destroy(int_vector_t *vector)
{
    free(vector->data);
    vector->data = NULL;
    vector->size = 0U;
    vector->capacity = 0U;
}

static int vector_reserve(int_vector_t *vector, size_t requested_capacity)
{
    int *replacement;

    if (requested_capacity <= vector->capacity) {
        return 0;
    }
    if (requested_capacity > (SIZE_MAX / sizeof *vector->data)) {
        return -2;
    }

    replacement = realloc(vector->data,
                          requested_capacity * sizeof *vector->data);
    if (replacement == NULL) {
        return -1;
    }

    vector->data = replacement;
    vector->capacity = requested_capacity;
    return 0;
}

static int vector_push(int_vector_t *vector, int value)
{
    if (vector->size == vector->capacity) {
        size_t next_capacity;

        if (vector->capacity == 0U) {
            next_capacity = 4U;
        } else {
            if (vector->capacity > (SIZE_MAX / 2U)) {
                return -2;
            }
            next_capacity = vector->capacity * 2U;
        }
        const int reserve_status = vector_reserve(vector, next_capacity);
        if (reserve_status != 0) {
            return reserve_status;
        }
    }

    vector->data[vector->size] = value;
    vector->size += 1U;
    return 0;
}

static long vector_sum(const int_vector_t *vector)
{
    if (vector->size == 0U) {
        return 0L;
    }

    const int *cursor = vector->data;
    const int *const one_past = vector->data + vector->size;
    long sum = 0L;

    while (cursor != one_past) {
        sum += (long)*cursor;
        ++cursor;
    }
    return sum;
}

static int run_self_test(void)
{
    static const int input[] = {3, -1, 7, 0, 5};
    int_vector_t vector;
    size_t index;

    vector_init(&vector);
    for (index = 0U; index < (sizeof input / sizeof input[0]); ++index) {
        if (vector_push(&vector, input[index]) != 0) {
            vector_destroy(&vector);
            fputs("error: allocation failed\n", stderr);
            return 1;
        }
    }

    printf("size=%zu capacity=%zu sum=%ld first=%d last=%d\n",
           vector.size,
           vector.capacity,
           vector_sum(&vector),
           vector.data[0],
           vector.data[vector.size - 1U]);

    if ((vector.size != 5U) || (vector.capacity != 8U) ||
        (vector_sum(&vector) != 14L)) {
        vector_destroy(&vector);
        fputs("internal error: vector invariant failed\n", stderr);
        return 1;
    }

    vector_destroy(&vector);
    printf("ownership=destroyed data=%s size=%zu capacity=%zu\n",
           (vector.data == NULL) ? "null" : "non-null",
           vector.size,
           vector.capacity);
    puts("self-test=PASS");
    return 0;
}

static int run_negative_test(void)
{
    int_vector_t vector;
    const size_t impossible_capacity = (SIZE_MAX / sizeof *vector.data) + 1U;

    vector_init(&vector);
    if (vector_reserve(&vector, impossible_capacity) != -2) {
        vector_destroy(&vector);
        fputs("internal error: overflow request was not rejected\n", stderr);
        return 1;
    }
    if ((vector.data != NULL) || (vector.size != 0U) ||
        (vector.capacity != 0U)) {
        vector_destroy(&vector);
        fputs("internal error: rejected allocation changed ownership state\n",
              stderr);
        return 1;
    }

    fputs("error: allocation size overflow\n", stderr);
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

    fputs("usage: b05_memory_demo --self-test|--negative\n", stderr);
    return 64;
}
