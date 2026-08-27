/*
 * CASE-B01-01 — Portable C17 diagnostic-record model.
 * Covers OUT-B01-01 through OUT-B01-10.
 */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    READING_TEMP_C = 1,
    READING_RPM = 2
} ReadingKind;

typedef union {
    int32_t milli_celsius;
    uint32_t rpm;
} ReadingValue;

typedef struct {
    uint32_t id;
    ReadingKind kind;
    ReadingValue value;
} DiagnosticRecord;

static uint32_t records_processed = 0U;

static bool parse_i64(const char *text, int64_t *out)
{
    char *end = NULL;
    intmax_t value;

    if ((text == NULL) || (out == NULL) || (*text == '\0')) {
        return false;
    }
    errno = 0;
    value = strtoimax(text, &end, 10);
    if ((errno == ERANGE) || (end == text) || (*end != '\0')) {
        return false;
    }
    if ((value < INT64_MIN) || (value > INT64_MAX)) {
        return false;
    }
    *out = (int64_t)value;
    return true;
}

static bool checked_u32(int64_t value, uint32_t *out)
{
    if ((out == NULL) || (value < 0) || ((uint64_t)value > UINT32_MAX)) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

static bool checked_i32(int64_t value, int32_t *out)
{
    if ((out == NULL) || (value < INT32_MIN) || (value > INT32_MAX)) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static const char *kind_name(ReadingKind kind)
{
    switch (kind) {
    case READING_TEMP_C:
        return "TEMP_C";
    case READING_RPM:
        return "RPM";
    default:
        return "UNKNOWN";
    }
}

static bool make_record(uint32_t id, const char *kind_text, int64_t raw,
                        DiagnosticRecord *out)
{
    DiagnosticRecord candidate = {0};

    if ((kind_text == NULL) || (out == NULL)) {
        return false;
    }
    candidate.id = id;
    if (strcmp(kind_text, "temp") == 0) {
        candidate.kind = READING_TEMP_C;
        if (!checked_i32(raw, &candidate.value.milli_celsius)) {
            return false;
        }
    } else if (strcmp(kind_text, "rpm") == 0) {
        candidate.kind = READING_RPM;
        if (!checked_u32(raw, &candidate.value.rpm)) {
            return false;
        }
    } else {
        return false;
    }
    *out = candidate;
    return true;
}

static void print_record(const DiagnosticRecord *record)
{
    if (record->kind == READING_TEMP_C) {
        const int32_t raw = record->value.milli_celsius;
        printf("record id=%" PRIu32 " kind=%s raw=%" PRId32
               " whole=%" PRId32 " processed=%" PRIu32 "\n",
               record->id, kind_name(record->kind), raw, raw / 1000,
               records_processed);
    } else {
        printf("record id=%" PRIu32 " kind=%s raw=%" PRIu32
               " processed=%" PRIu32 "\n",
               record->id, kind_name(record->kind), record->value.rpm,
               records_processed);
    }
}

static int run_record(const char *id_text, const char *kind_text,
                      const char *value_text)
{
    int64_t parsed_id;
    int64_t parsed_value;
    uint32_t id;
    DiagnosticRecord record;

    if (!parse_i64(id_text, &parsed_id) || !checked_u32(parsed_id, &id)) {
        fprintf(stderr, "error: id '%s' is outside uint32 range\n", id_text);
        return 2;
    }
    if (!parse_i64(value_text, &parsed_value)) {
        fprintf(stderr, "error: value '%s' is not a base-10 integer\n",
                value_text);
        return 2;
    }
    if (!make_record(id, kind_text, parsed_value, &record)) {
        if (strcmp(kind_text, "rpm") == 0) {
            fprintf(stderr,
                    "error: value '%s' is outside uint32 range for RPM\n",
                    value_text);
        } else if (strcmp(kind_text, "temp") == 0) {
            fprintf(stderr,
                    "error: value '%s' is outside int32 range for TEMP_C\n",
                    value_text);
        } else {
            fprintf(stderr, "error: kind must be 'temp' or 'rpm'\n");
        }
        return 2;
    }
    records_processed++;
    print_record(&record);
    return 0;
}

static int self_test(void)
{
    DiagnosticRecord temp;
    DiagnosticRecord rpm;
    uint32_t converted = 0U;
    unsigned checks = 0U;

    _Static_assert(sizeof(uint32_t) * CHAR_BIT == 32U,
                   "uint32_t must have exactly 32 bits");
    if (make_record(17U, "temp", 25375, &temp) &&
        (temp.value.milli_celsius == INT32_C(25375))) {
        checks++;
    }
    if (make_record(18U, "rpm", 3200, &rpm) &&
        (rpm.value.rpm == UINT32_C(3200))) {
        checks++;
    }
    if (!make_record(19U, "rpm", -1, &rpm)) {
        checks++;
    }
    if (!make_record(20U, "temp", INT64_C(2147483648), &temp)) {
        checks++;
    }
    if (checked_u32(INT64_C(4294967295), &converted) &&
        (converted == UINT32_MAX)) {
        checks++;
    }
    if (!checked_u32(INT64_C(4294967296), &converted)) {
        checks++;
    }
    if (checks != 6U) {
        fprintf(stderr, "B01 SELF-TEST FAIL checks=%u/6\n", checks);
        return 1;
    }
    printf("B01 SELF-TEST PASS checks=6\n");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc == 2) && (strcmp(argv[1], "--self-test") == 0)) {
        return self_test();
    }
    if ((argc == 5) && (strcmp(argv[1], "--record") == 0)) {
        return run_record(argv[2], argv[3], argv[4]);
    }
    fprintf(stderr,
            "usage: %s --self-test | --record ID {temp|rpm} VALUE\n",
            argv[0]);
    return 2;
}
