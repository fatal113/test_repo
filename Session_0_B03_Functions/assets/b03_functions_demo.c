/*
 * CASE-B03-01 — Portable function contracts.
 * Covers OUT-B03-02..04, 06..09, 11..17, and 19..20.
 * ISO C17 does not permit nested function definitions. The file-scope callback
 * plus explicit context below is the portable alternative.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    MAX_VALUES = 16,
    MAX_RECURSION_DEPTH = 16,
    MAX_BENCHMARK_ITERATIONS = 50000000
};

typedef bool (*VisitFn)(int32_t value, void *context);

typedef struct {
    int32_t limit;
    int64_t sum;
    size_t accepted;
} SumContext;

static inline int32_t clamp_upper(int32_t value, int32_t limit)
{
    return (value > limit) ? limit : value;
}

static bool parse_i32(const char *text, int32_t *out)
{
    char *end = NULL;
    intmax_t value;

    if ((text == NULL) || (out == NULL) || (*text == '\0')) {
        return false;
    }
    errno = 0;
    value = strtoimax(text, &end, 10);
    if ((errno == ERANGE) || (end == text) || (*end != '\0') ||
        (value < INT32_MIN) || (value > INT32_MAX)) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static bool find_range(const int32_t values[], size_t count,
                       int32_t *minimum, int32_t *maximum)
{
    size_t i;
    int32_t candidate_minimum;
    int32_t candidate_maximum;

    if ((values == NULL) || (minimum == NULL) || (maximum == NULL) ||
        (minimum == maximum) || (count == 0U)) {
        return false;
    }
    candidate_minimum = values[0];
    candidate_maximum = values[0];
    for (i = 1U; i < count; ++i) {
        if (values[i] < candidate_minimum) {
            candidate_minimum = values[i];
        }
        if (values[i] > candidate_maximum) {
            candidate_maximum = values[i];
        }
    }
    *minimum = candidate_minimum;
    *maximum = candidate_maximum;
    return true;
}

static bool checked_sum_varargs(size_t count, int64_t *out, ...)
{
    va_list arguments;
    size_t i;
    int64_t sum = 0;

    if (out == NULL) {
        return false;
    }
    va_start(arguments, out);
    for (i = 0U; i < count; ++i) {
        sum += va_arg(arguments, int);
    }
    va_end(arguments);
    *out = sum;
    return true;
}

static bool visit_values(const int32_t values[], size_t count,
                         VisitFn visit, void *context)
{
    size_t i;

    if ((values == NULL) || (visit == NULL)) {
        return false;
    }
    for (i = 0U; i < count; ++i) {
        if (!visit(values[i], context)) {
            return false;
        }
    }
    return true;
}

static bool add_clamped(int32_t value, void *context)
{
    SumContext *state = context;

    if (state == NULL) {
        return false;
    }
    state->sum += clamp_upper(value, state->limit);
    state->accepted++;
    return true;
}

static bool recursive_sum_impl(const int32_t values[], size_t count,
                               size_t depth, int64_t *out)
{
    int64_t tail;

    if ((values == NULL) || (out == NULL) ||
        (depth > MAX_RECURSION_DEPTH)) {
        return false;
    }
    if (count == 0U) {
        *out = 0;
        return true;
    }
    if (!recursive_sum_impl(values + 1, count - 1U, depth + 1U, &tail)) {
        return false;
    }
    *out = values[0] + tail;
    return true;
}

static bool recursive_sum(const int32_t values[], size_t count, int64_t *out)
{
    if (count > MAX_RECURSION_DEPTH) {
        return false;
    }
    return recursive_sum_impl(values, count, 0U, out);
}

static int run_benchmark(const char *text)
{
    int32_t iterations;
    uint32_t state = UINT32_C(0x12345678);
    int64_t checksum = 0;
    int32_t iteration;

    if (!parse_i32(text, &iterations) || (iterations <= 0) ||
        (iterations > MAX_BENCHMARK_ITERATIONS)) {
        fprintf(stderr, "error: iterations must be in range 1..%d\n",
                MAX_BENCHMARK_ITERATIONS);
        return 2;
    }
    for (iteration = 0; iteration < iterations; ++iteration) {
        int32_t values[3];
        int32_t minimum;
        int32_t maximum;
        int64_t total;
        SumContext context = {.limit = 1000, .sum = 0, .accepted = 0U};
        size_t i;

        for (i = 0U; i < 3U; ++i) {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            values[i] = (int32_t)((state >> 16) & UINT32_C(0xffff)) -
                        INT32_C(32768);
        }
        if (!find_range(values, 3U, &minimum, &maximum) ||
            !visit_values(values, 3U, add_clamped, &context) ||
            !recursive_sum(values, 3U, &total)) {
            fprintf(stderr, "error: benchmark contract failed\n");
            return 3;
        }
        checksum += minimum;
        checksum += maximum;
        checksum += context.sum;
        checksum += total;
    }
    printf("BENCH iterations=%" PRId32 " checksum=%" PRId64 "\n",
           iterations, checksum);
    return 0;
}

static int run_summary(int argc, char **argv)
{
    int32_t values[MAX_VALUES] = {0};
    int32_t limit;
    int32_t minimum;
    int32_t maximum;
    int64_t recursive_total;
    SumContext context = {0};
    const size_t count = (size_t)(argc - 3);
    size_t i;

    if (!parse_i32(argv[2], &limit)) {
        fprintf(stderr, "error: limit is not an int32 value: %s\n", argv[2]);
        return 2;
    }
    if ((count == 0U) || (count > MAX_VALUES)) {
        fprintf(stderr, "error: expected 1..%d values, got %zu\n",
                MAX_VALUES, count);
        return 2;
    }
    for (i = 0U; i < count; ++i) {
        if (!parse_i32(argv[i + 3U], &values[i])) {
            fprintf(stderr, "error: invalid integer at position %zu: %s\n",
                    i + 1U, argv[i + 3U]);
            return 2;
        }
    }
    context.limit = limit;
    if (!find_range(values, count, &minimum, &maximum) ||
        !visit_values(values, count, add_clamped, &context) ||
        !recursive_sum(values, count, &recursive_total)) {
        fprintf(stderr, "error: function contract rejected the request\n");
        return 3;
    }
    printf("count=%zu sum=%" PRId64 " min=%" PRId32
           " max=%" PRId32 " clipped_sum=%" PRId64 "\n",
           count, recursive_total, minimum, maximum, context.sum);
    printf("recursive_sum=%" PRId64 "\n", recursive_total);
    return 0;
}

static int self_test(void)
{
    const int32_t values[] = {3, 7, 11};
    int32_t minimum = 0;
    int32_t maximum = 0;
    int32_t aliased_output = 1234;
    int64_t total = 0;
    SumContext context = {.limit = 10, .sum = 0, .accepted = 0U};
    unsigned checks = 0U;

    if (find_range(values, 3U, &minimum, &maximum) &&
        (minimum == 3) && (maximum == 11)) {
        checks++;
    }
    if (!find_range(values, 0U, &minimum, &maximum)) {
        checks++;
    }
    if (!find_range(values, 3U, &aliased_output, &aliased_output) &&
        (aliased_output == 1234)) {
        checks++;
    }
    if (checked_sum_varargs(3U, &total, 3, 7, 11) && (total == 21)) {
        checks++;
    }
    if (visit_values(values, 3U, add_clamped, &context) &&
        (context.sum == 20) && (context.accepted == 3U)) {
        checks++;
    }
    if (recursive_sum(values, 3U, &total) && (total == 21)) {
        checks++;
    }
    if (!recursive_sum(values, MAX_RECURSION_DEPTH + 1U, &total)) {
        checks++;
    }
    if (clamp_upper(11, 10) == 10) {
        checks++;
    }
    if (!visit_values(values, 3U, NULL, &context)) {
        checks++;
    }
    if (checks != 9U) {
        fprintf(stderr, "B03 SELF-TEST FAIL checks=%u/9\n", checks);
        return 1;
    }
    printf("B03 SELF-TEST PASS checks=9\n");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc == 2) && (strcmp(argv[1], "--self-test") == 0)) {
        return self_test();
    }
    if ((argc >= 4) && (strcmp(argv[1], "--summarize") == 0)) {
        return run_summary(argc, argv);
    }
    if ((argc == 3) && (strcmp(argv[1], "--benchmark") == 0)) {
        return run_benchmark(argv[2]);
    }
    fprintf(stderr,
            "usage: %s --self-test | --summarize LIMIT VALUE [VALUE ...] | "
            "--benchmark ITERATIONS\n",
            argv[0]);
    return 2;
}
