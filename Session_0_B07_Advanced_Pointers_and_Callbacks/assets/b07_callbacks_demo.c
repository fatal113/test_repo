#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_VALUES 16U
#define SUMMARY_CAPACITY 128U

typedef int (*value_predicate)(int value, const void *context);

struct threshold_context {
    int minimum;
};

static int contains_whitespace(const char *text)
{
    if (text == NULL) {
        return 0;
    }
    while (*text != '\0') {
        if (isspace((unsigned char)*text) != 0) {
            return 1;
        }
        ++text;
    }
    return 0;
}

static int parse_int(const char *text, int *out_value)
{
    char *end = NULL;
    long parsed = 0L;

    if (text == NULL || *text == '\0' || out_value == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *out_value = (int)parsed;
    return 1;
}

static int parse_list(
    const char *text,
    int values[MAX_VALUES],
    size_t *out_count)
{
    const char *cursor = text;
    size_t count = 0U;

    if (text == NULL || *text == '\0' || values == NULL || out_count == NULL) {
        return 0;
    }
    *out_count = 0U;

    for (;;) {
        char *end = NULL;
        long parsed = 0L;

        if (count == MAX_VALUES) {
            return 0;
        }

        errno = 0;
        parsed = strtol(cursor, &end, 10);
        if (errno == ERANGE || end == cursor || parsed < INT_MIN || parsed > INT_MAX) {
            return 0;
        }

        values[count] = (int)parsed;
        ++count;

        if (*end == '\0') {
            break;
        }
        if (*end != ',' || end[1] == '\0') {
            return 0;
        }
        cursor = end + 1;
    }

    *out_count = count;
    return 1;
}

static int at_least_threshold(int value, const void *context)
{
    const struct threshold_context *threshold = context;

    return threshold != NULL && value >= threshold->minimum;
}

static int select_values(
    const int *values,
    size_t count,
    value_predicate predicate,
    const void *context,
    size_t *out_selected,
    int *out_sum,
    int *out_first)
{
    size_t selected = 0U;
    int sum = 0;
    int first = 0;

    if (values == NULL || count == 0U || predicate == NULL || context == NULL ||
        out_selected == NULL || out_sum == NULL || out_first == NULL) {
        return 0;
    }

    for (size_t index = 0U; index < count; ++index) {
        if (predicate(values[index], context) != 0) {
            if (selected == 0U) {
                first = values[index];
            }
            if ((values[index] > 0 && sum > INT_MAX - values[index]) ||
                (values[index] < 0 && sum < INT_MIN - values[index])) {
                return 0;
            }
            sum += values[index];
            ++selected;
        }
    }

    *out_selected = selected;
    *out_sum = sum;
    *out_first = first;
    return 1;
}

static int build_summary(
    int threshold,
    size_t selected,
    int sum,
    int first,
    char **out_summary)
{
    char *summary = NULL;
    int written = 0;

    if (out_summary == NULL) {
        return 0;
    }
    *out_summary = NULL;

    summary = malloc(SUMMARY_CAPACITY);
    if (summary == NULL) {
        return 0;
    }

    if (selected == 0U) {
        written = snprintf(
            summary,
            SUMMARY_CAPACITY,
            "OK threshold=%d selected=0 sum=0 first=NA",
            threshold);
    } else {
        written = snprintf(
            summary,
            SUMMARY_CAPACITY,
            "OK threshold=%d selected=%zu sum=%d first=%d",
            threshold,
            selected,
            sum,
            first);
    }

    if (written < 0 || (size_t)written >= SUMMARY_CAPACITY) {
        free(summary);
        return 0;
    }

    *out_summary = summary;
    return 1;
}

int main(int argc, char **argv)
{
    int threshold = 0;
    int values[MAX_VALUES] = {0};
    size_t value_count = 0U;
    size_t selected = 0U;
    int sum = 0;
    int first = 0;
    char *summary = NULL;
    struct threshold_context context = {0};

    if (argc != 3) {
        fprintf(stderr, "USAGE: %s <threshold> <comma-separated-integers>\n", argv[0]);
        return 64;
    }
    if (contains_whitespace(argv[1]) || contains_whitespace(argv[2])) {
        fputs("ERROR whitespace is not allowed\n", stderr);
        return 2;
    }
    if (!parse_int(argv[1], &threshold)) {
        fputs("ERROR invalid threshold\n", stderr);
        return 2;
    }
    if (!parse_list(argv[2], values, &value_count)) {
        fputs("ERROR invalid integer list\n", stderr);
        return 2;
    }

    context.minimum = threshold;
    if (!select_values(
            values,
            value_count,
            at_least_threshold,
            &context,
            &selected,
            &sum,
            &first)) {
        fputs("ERROR selection failed\n", stderr);
        return 3;
    }
    if (!build_summary(threshold, selected, sum, first, &summary)) {
        fputs("ERROR allocation failure\n", stderr);
        return 3;
    }

    puts(summary);
    free(summary);
    return 0;
}
