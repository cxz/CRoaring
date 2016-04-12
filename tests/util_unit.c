/*
 * util_unit.c
 *
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitset_util.h"

#include "test.h"

void setandextract_uint16() {
    const unsigned int bitset_size = 1 << 16;
    const unsigned int bitset_size_in_words =
        bitset_size / (sizeof(uint64_t) * 8);

    for (unsigned int offset = 1; offset < bitset_size; offset++) {
        const unsigned int valsize = bitset_size / offset;
        uint16_t* vals = malloc(valsize * sizeof(uint16_t));
        uint64_t* bitset = calloc(bitset_size_in_words, sizeof(uint64_t));
        for (unsigned int k = 0; k < valsize; ++k) {
            vals[k] = (uint16_t)(k * offset);
        }

        bitset_set_list(bitset, vals, valsize);
        uint16_t* newvals = malloc(valsize * sizeof(uint16_t));
        bitset_extract_setbits_uint16(bitset, bitset_size_in_words, newvals, 0);

        for (unsigned int k = 0; k < valsize; ++k) {
            assert_int_equal(newvals[k], vals[k]);
        }

        free(vals);
        free(newvals);
        free(bitset);
    }
}

void setandextract_sse_uint16() {
    const unsigned int bitset_size = 1 << 16;
    const unsigned int bitset_size_in_words =
        bitset_size / (sizeof(uint64_t) * 8);

    for (unsigned int offset = 1; offset < bitset_size; offset++) {
        const unsigned int valsize = bitset_size / offset;
        uint16_t* vals = malloc(valsize * sizeof(uint16_t));
        uint64_t* bitset = calloc(bitset_size_in_words, sizeof(uint64_t));
        for (unsigned int k = 0; k < valsize; ++k) {
            vals[k] = (uint16_t)(k * offset);
        }

        bitset_set_list(bitset, vals, valsize);
        uint16_t* newvals = malloc(valsize * sizeof(uint16_t) + 64);
        bitset_extract_setbits_sse_uint16(bitset, bitset_size_in_words, newvals,
                                          valsize, 0);

        for (unsigned int k = 0; k < valsize; ++k) {
            assert_int_equal(newvals[k], vals[k]);

        }

        free(vals);
        free(newvals);
        free(bitset);
    }
}

void setandextract_uint32() {
    const unsigned int bitset_size = 1 << 16;
    const unsigned int bitset_size_in_words =
        bitset_size / (sizeof(uint64_t) * 8);

    for (unsigned int offset = 1; offset < bitset_size; offset++) {
        const unsigned int valsize = bitset_size / offset;
        uint16_t* vals = malloc(valsize * sizeof(uint16_t));
        uint64_t* bitset = calloc(bitset_size_in_words, sizeof(uint64_t));

        for (unsigned int k = 0; k < valsize; ++k) {
            vals[k] = (uint16_t)(k * offset);
        }

        bitset_set_list(bitset, vals, valsize);
        uint32_t* newvals = malloc(valsize * sizeof(uint32_t));
        bitset_extract_setbits(bitset, bitset_size_in_words, newvals, 0);

        for (unsigned int k = 0; k < valsize; ++k) {
            assert_int_equal(newvals[k], vals[k]);
        }

        free(vals);
        free(newvals);
        free(bitset);
    }
}

// returns 1 when ok
void setandextract_avx2_uint32() {
    const unsigned int bitset_size = 1 << 16;
    const unsigned int bitset_size_in_words =
        bitset_size / (sizeof(uint64_t) * 8);

    for (unsigned int offset = 1; offset < bitset_size; offset++) {
        const unsigned int valsize = bitset_size / offset;
        uint16_t* vals = malloc(valsize * sizeof(uint16_t));
        uint64_t* bitset = calloc(bitset_size_in_words, sizeof(uint64_t));

        for (unsigned int k = 0; k < valsize; ++k) {
            vals[k] = (uint16_t)(k * offset);
        }

        bitset_set_list(bitset, vals, valsize);
        uint32_t* newvals = malloc(valsize * sizeof(uint32_t) + 64);
        bitset_extract_setbits_avx2(bitset, bitset_size_in_words, newvals,
                                    valsize, 0);

        for (unsigned int k = 0; k < valsize; ++k) {
            assert_int_equal(newvals[k], vals[k]);
        }

        free(vals);
        free(newvals);
        free(bitset);
    }
}

int main() {

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(setandextract_uint16),
        cmocka_unit_test(setandextract_sse_uint16),
        cmocka_unit_test(setandextract_uint32),
        cmocka_unit_test(setandextract_avx2_uint32),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
