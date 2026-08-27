#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VALUES 8U
#define LABEL_CAPACITY 16U
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

static int parse_count(const char *text, size_t *out_count)
{
    char *end = NULL;
    unsigned long parsed = 0UL;

    if (text == NULL || *text == '\0' || out_count == NULL) {
        return 0;
    }
    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed == 0UL || parsed > MAX_VALUES) {
        return 0;
    }
    *out_count = (size_t)parsed;
    return 1;
}

static int copy_label(const char *source, char destination[LABEL_CAPACITY])
{
    size_t length = 0U;

    if (source == NULL || destination == NULL) {
        return 0;
    }
    length = strlen(source);
    if (length == 0U || length >= LABEL_CAPACITY) {
        return 0;
    }
    for (size_t index = 0U; index < length; ++index) {
        const unsigned char character = (unsigned char)source[index];
        if (isalnum(character) == 0 && character != (unsigned char)'-') {
            return 0;
        }
    }
    memcpy(destination, source, length + 1U);
    return 1;
}

static int checked_add_u32(uint32_t left, uint32_t right, uint32_t *out_sum)
{
    if (out_sum == NULL || UINT32_MAX - left < right) {
        return 0;
    }
    *out_sum = left + right;
    return 1;
}

static int run_overflow_test(void)
{
    uint32_t unchanged = UINT32_C(123);

    if (checked_add_u32(UINT32_MAX, UINT32_C(1), &unchanged) != 0 ||
        unchanged != UINT32_C(123)) {
        fputs("ERROR overflow was not rejected transactionally\n", stderr);
        return 3;
    }
    printf(
        "OK overflow-rejected left=%" PRIu32 " right=1 output-unchanged=%" PRIu32 "\n",
        UINT32_MAX,
        unchanged);
    return 0;
}

static uint16_t decode_u16_le(const unsigned char bytes[2])
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
}

int main(int argc, char **argv)
{
    static const unsigned char encoded_word[] = {0x34U, 0x12U};
    const uint32_t flags = UINT32_C(0x03);
    size_t count = 0U;
    char label[LABEL_CAPACITY] = {0};
    uint32_t *values = NULL;
    uint32_t total = 0U;
    int status = 3;

    if (argc == 2 && strcmp(argv[1], "--overflow-test") == 0) {
        return run_overflow_test();
    }

    if (argc != 3) {
        fprintf(
            stderr,
            "USAGE: %s <count-1..8> <label> | --overflow-test\n",
            argv[0]);
        return 64;
    }
    if (!parse_count(argv[1], &count)) {
        fputs("ERROR count must be 1..8\n", stderr);
        return 2;
    }
    if (!copy_label(argv[2], label)) {
        fputs("ERROR label must be 1..15 alnum-or-dash characters\n", stderr);
        return 2;
    }
    if (count > SIZE_MAX / sizeof(*values)) {
        fputs("ERROR allocation size overflow\n", stderr);
        return 3;
    }

    values = malloc(count * sizeof(*values));
    if (values == NULL) {
        fputs("ERROR allocation failure\n", stderr);
        return 3;
    }

    for (size_t index = 0U; index < count; ++index) {
        values[index] = (uint32_t)(index + 1U) * UINT32_C(10);
    }

    {
        const uint32_t * const read_only_view = values;
        for (size_t index = 0U; index < count; ++index) {
            if (!checked_add_u32(total, read_only_view[index], &total)) {
                fputs("ERROR sum overflow\n", stderr);
                goto cleanup;
            }
        }
    }

    if (ARRAY_COUNT(encoded_word) != 2U || (flags & UINT32_C(0x01)) == 0U) {
        fputs("ERROR internal contract\n", stderr);
        goto cleanup;
    }

    printf(
        "OK count=%zu sum=%" PRIu32 " label=%s word=%" PRIu16 " mask=%" PRIu32 "\n",
        count,
        total,
        label,
        decode_u16_le(encoded_word),
        flags);
    status = 0;

cleanup:
    free(values);
    values = NULL;
    return status;
}
