#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ITEMS UINT32_C(1000000)

static int parse_count(const char *text, uint32_t *out_count)
{
    char *end = NULL;
    unsigned long parsed = 0UL;

    if (text == NULL || *text == '\0' || out_count == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed == 0UL || parsed > (unsigned long)MAX_ITEMS) {
        return 0;
    }

    *out_count = (uint32_t)parsed;
    return 1;
}

static uint32_t mix_value(uint32_t index)
{
    const uint32_t scaled = index * UINT32_C(17) + UINT32_C(3);
    const uint32_t shared = (scaled ^ UINT32_C(0xA5A5A5A5)) + (scaled >> 3U);

    return shared ^ (shared >> 16U);
}

static uint32_t compute_checksum(uint32_t count)
{
    uint32_t checksum = UINT32_C(2166136261);

    for (uint32_t index = 0U; index < count; ++index) {
        checksum ^= mix_value(index);
        checksum *= UINT32_C(16777619);
    }

    return checksum;
}

int main(int argc, char **argv)
{
    uint32_t count = 0U;
    uint32_t checksum = 0U;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <item-count>\n", argv[0]);
        return 64;
    }
    if (!parse_count(argv[1], &count)) {
        fputs("ERROR item-count must be 1..1000000\n", stderr);
        return 2;
    }

    checksum = compute_checksum(count);
    printf("OK n=%" PRIu32 " checksum=%08" PRIX32 "\n", count, checksum);
    return 0;
}
