#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201710L)
#error "b04_macro_bits_demo.c requires ISO C17 or newer"
#endif

#if UINT32_MAX != UINT32_C(0xFFFFFFFF)
#error "b04_macro_bits_demo.c requires an exact 32-bit uint32_t"
#endif

#ifndef B04_VARIANT
#define B04_VARIANT portable_c17
#endif

#define STRINGIFY_INNER(token) #token
#define STRINGIFY(token) STRINGIFY_INNER(token)
#define DECLARE_FLAG(name, bit_index) \
    static const uint32_t name##_MASK = (UINT32_C(1) << (bit_index))

#define MODE_SHIFT UINT32_C(4)
#define MODE_MASK (UINT32_C(7) << MODE_SHIFT)

DECLARE_FLAG(ENABLE, 0);
DECLARE_FLAG(READY, 2);

typedef struct {
    uint32_t value;
} register_image_t;

static void flags_set(register_image_t *image, uint32_t mask)
{
    image->value |= mask;
}

static void flags_clear(register_image_t *image, uint32_t mask)
{
    image->value &= ~mask;
}

static bool flags_test(const register_image_t *image, uint32_t mask)
{
    return (image->value & mask) == mask;
}

static bool field_write(register_image_t *image,
                        uint32_t mask,
                        uint32_t shift,
                        uint32_t field_value)
{
    uint32_t field_max;

    if ((mask == UINT32_C(0)) || (shift >= UINT32_C(32))) {
        return false;
    }

    field_max = mask >> shift;
    if ((field_max == UINT32_C(0)) || (field_value > field_max)) {
        return false;
    }

    image->value = (image->value & ~mask) | ((field_value << shift) & mask);
    return true;
}

static uint32_t field_read(const register_image_t *image,
                           uint32_t mask,
                           uint32_t shift)
{
    return (image->value & mask) >> shift;
}

static int run_self_test(void)
{
    register_image_t image = {UINT32_C(0)};

    printf("config=%s\n", STRINGIFY(B04_VARIANT));
    printf("initial=0x%08" PRIX32 "\n", image.value);

    flags_set(&image, ENABLE_MASK | READY_MASK);
    printf("after_set=0x%08" PRIX32 "\n", image.value);

    if (!field_write(&image, MODE_MASK, MODE_SHIFT, UINT32_C(5))) {
        fputs("internal error: valid mode rejected\n", stderr);
        return 1;
    }
    printf("after_mode=0x%08" PRIX32 " mode=%" PRIu32 "\n",
           image.value,
           field_read(&image, MODE_MASK, MODE_SHIFT));

    flags_clear(&image, READY_MASK);
    printf("after_clear=0x%08" PRIX32 " enabled=%u\n",
           image.value,
           flags_test(&image, ENABLE_MASK) ? 1U : 0U);

    if ((image.value != UINT32_C(0x51)) ||
        (field_read(&image, MODE_MASK, MODE_SHIFT) != UINT32_C(5)) ||
        !flags_test(&image, ENABLE_MASK) ||
        flags_test(&image, READY_MASK)) {
        fputs("internal error: self-test invariant failed\n", stderr);
        return 1;
    }

    puts("self-test=PASS");
    return 0;
}

static int run_negative_test(void)
{
    register_image_t image = {UINT32_C(0xA5)};
    const uint32_t before = image.value;

    if (field_write(&image, MODE_MASK, MODE_SHIFT, UINT32_C(8))) {
        fputs("internal error: invalid mode accepted\n", stderr);
        return 1;
    }
    if (image.value != before) {
        fputs("internal error: rejected write changed image\n", stderr);
        return 1;
    }

    fputs("error: mode value 8 does not fit mask 0x00000070\n", stderr);
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

    fputs("usage: b04_macro_bits_demo --self-test|--negative\n", stderr);
    return 64;
}
