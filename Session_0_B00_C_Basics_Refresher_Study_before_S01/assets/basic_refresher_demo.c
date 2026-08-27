#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SAMPLES 8U
#define MIN_SAMPLE (-50L)
#define MAX_SAMPLE 150L
#define EXIT_INVALID_INPUT 2

static int parse_samples(const char *text,
                         int *values,
                         size_t capacity,
                         size_t *out_count)
{
    const char *cursor = text;
    size_t count = 0U;

    if (text == NULL || values == NULL || capacity == 0U || out_count == NULL) {
        return 0;
    }
    *out_count = 0U;

    while (*cursor != '\0') {
        char *end = NULL;
        long parsed;

        if (count == capacity || *cursor == ' ' || *cursor == '\t' ||
            *cursor == '\n' || *cursor == '\r' || *cursor == '\f' ||
            *cursor == '\v') {
            return 0;
        }

        errno = 0;
        parsed = strtol(cursor, &end, 10);
        if (errno == ERANGE || end == cursor ||
            parsed < MIN_SAMPLE || parsed > MAX_SAMPLE) {
            return 0;
        }

        values[count] = (int)parsed;
        count += 1U;

        if (*end == '\0') {
            cursor = end;
        } else if (*end == ',') {
            cursor = end + 1;
            if (*cursor == '\0') {
                return 0;
            }
        } else {
            return 0;
        }
    }

    if (count == 0U) {
        return 0;
    }
    *out_count = count;
    return 1;
}

static int summarize_samples(const int *values,
                             size_t count,
                             int *out_minimum,
                             int *out_maximum,
                             int *out_total)
{
    int minimum;
    int maximum;
    int total = 0;
    size_t index;

    if (values == NULL || count == 0U || out_minimum == NULL ||
        out_maximum == NULL || out_total == NULL) {
        return 0;
    }

    minimum = values[0];
    maximum = values[0];

    for (index = 0U; index < count; ++index) {
        const int current = values[index];

        total += current;
        if (current < minimum) {
            minimum = current;
        }
        if (current > maximum) {
            maximum = current;
        }
    }

    *out_minimum = minimum;
    *out_maximum = maximum;
    *out_total = total;
    return 1;
}

static double mean_of(int total, size_t count)
{
    return (double)total / (double)count;
}

int main(int argc, char **argv)
{
    int samples[MAX_SAMPLES];
    size_t sample_count = 0U;
    int minimum = 0;
    int maximum = 0;
    int total = 0;

    if (argc != 2) {
        fputs("USAGE: basic_refresher_demo <comma-separated-samples>\n", stderr);
        return EXIT_FAILURE;
    }

    if (!parse_samples(argv[1], samples, MAX_SAMPLES, &sample_count) ||
        !summarize_samples(samples, sample_count,
                           &minimum, &maximum, &total)) {
        fputs("ERROR invalid sample list\n", stderr);
        return EXIT_INVALID_INPUT;
    }

    printf("OK count=%zu min=%d max=%d mean=%.2f\n",
           sample_count,
           minimum,
           maximum,
           mean_of(total, sample_count));
    return EXIT_SUCCESS;
}
