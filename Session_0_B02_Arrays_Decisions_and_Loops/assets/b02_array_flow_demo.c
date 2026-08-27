/*
 * CASE-B02-01 — Bounded array analysis with explicit decisions and loops.
 * Covers OUT-B02-02..07, OUT-B02-09..12, and OUT-B02-14..17.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_VALUES = 8 };

typedef enum {
    MODE_SUM,
    MODE_MAX,
    MODE_FIRST_POSITIVE
} AnalysisMode;

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

static bool parse_mode(const char *text, AnalysisMode *out)
{
    if ((text == NULL) || (out == NULL)) {
        return false;
    }
    if (strcmp(text, "sum") == 0) {
        *out = MODE_SUM;
    } else if (strcmp(text, "max") == 0) {
        *out = MODE_MAX;
    } else if (strcmp(text, "first-positive") == 0) {
        *out = MODE_FIRST_POSITIVE;
    } else {
        return false;
    }
    return true;
}

static const char *mode_name(AnalysisMode mode)
{
    switch (mode) {
    case MODE_SUM:
        return "SUM";
    case MODE_MAX:
        return "MAX";
    case MODE_FIRST_POSITIVE:
        return "FIRST_POSITIVE";
    default:
        return "UNKNOWN";
    }
}

static bool analyze(const int32_t values[], size_t count, AnalysisMode mode,
                    int64_t *result)
{
    size_t i;

    if ((values == NULL) || (result == NULL) || (count == 0U)) {
        return false;
    }
    switch (mode) {
    case MODE_SUM:
        *result = 0;
        for (i = 0U; i < count; ++i) {
            *result += values[i];
        }
        return true;
    case MODE_MAX:
        *result = values[0];
        for (i = 1U; i < count; ++i) {
            if (values[i] > *result) {
                *result = values[i];
            }
        }
        return true;
    case MODE_FIRST_POSITIVE:
        for (i = 0U; i < count; ++i) {
            if (values[i] <= 0) {
                continue;
            }
            *result = values[i];
            break;
        }
        return i < count;
    default:
        return false;
    }
}

static unsigned decimal_digits(uint32_t value)
{
    unsigned digits = 0U;

    do {
        digits++;
        value /= 10U;
    } while (value != 0U);
    return digits;
}

static int run_analysis(int argc, char **argv)
{
    int32_t values[MAX_VALUES] = {0};
    AnalysisMode mode;
    const size_t count = (size_t)(argc - 3);
    size_t i;
    int64_t result;
    const char *band;

    if (!parse_mode(argv[2], &mode)) {
        fprintf(stderr,
                "error: mode must be sum, max, or first-positive\n");
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
    if (!analyze(values, count, mode, &result)) {
        fprintf(stderr, "error: no value satisfies mode %s\n",
                mode_name(mode));
        return 3;
    }
    band = (result >= 20) ? "HIGH" : "NORMAL";
    printf("count=%zu accepted=%zu rejected=0\n", count, count);
    printf("mode=%s result=%" PRId64 " band=%s\n",
           mode_name(mode), result, band);
    return 0;
}

static int self_test(void)
{
    const int32_t row_major[2][3] = {{1, 2, 3}, {4, 5, 6}};
    const int32_t values[] = {-2, 0, 7, 11};
    int64_t result = 0;
    unsigned checks = 0U;
    size_t row = 0U;
    int32_t matrix_sum = 0;

    while (row < 2U) {
        size_t column;
        for (column = 0U; column < 3U; ++column) {
            matrix_sum += row_major[row][column];
        }
        row++;
    }
    if (matrix_sum == 21) {
        checks++;
    }
    if (analyze(values, 4U, MODE_SUM, &result) && (result == 16)) {
        checks++;
    }
    if (analyze(values, 4U, MODE_MAX, &result) && (result == 11)) {
        checks++;
    }
    if (analyze(values, 4U, MODE_FIRST_POSITIVE, &result) && (result == 7)) {
        checks++;
    }
    if (!analyze(values, 0U, MODE_SUM, &result)) {
        checks++;
    }
    if (decimal_digits(0U) == 1U) {
        checks++;
    }
    if (decimal_digits(2026U) == 4U) {
        checks++;
    }
    if ((sizeof row_major / sizeof row_major[0]) == 2U) {
        checks++;
    }
    if (checks != 8U) {
        fprintf(stderr, "B02 SELF-TEST FAIL checks=%u/8\n", checks);
        return 1;
    }
    printf("B02 SELF-TEST PASS checks=8\n");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc == 2) && (strcmp(argv[1], "--self-test") == 0)) {
        return self_test();
    }
    if ((argc >= 4) && (strcmp(argv[1], "--analyze") == 0)) {
        return run_analysis(argc, argv);
    }
    fprintf(stderr,
            "usage: %s --self-test | --analyze MODE VALUE [VALUE ...]\n",
            argv[0]);
    return 2;
}
