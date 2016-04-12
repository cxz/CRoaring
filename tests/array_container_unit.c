/*
 * array_container_unit.c
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "containers/array.h"
#include "misc/configreport.h"

#include "test.h"

void printf_test() {
    array_container_t* B = array_container_create();
    assert_non_null(B);

    array_container_add(B, 1U);
    array_container_add(B, 2U);
    array_container_add(B, 3U);
    array_container_add(B, 10U);
    array_container_add(B, 10000U);

    array_container_printf(B);
    printf("\n");

    array_container_free(B);
}

void add_contains_test() {
    array_container_t* B = array_container_create();
    assert_non_null(B);

    int expected_card = 0;

    for (size_t x = 0; x < 1 << 16; x += 3) {
        assert_true(array_container_add(B, x));
        assert_true(array_container_contains(B, x));
        assert_int_equal(B->cardinality, ++expected_card);
        assert_false(B->cardinality > B->capacity);
    }

    for (size_t x = 0; x < 1 << 16; x++) {
        assert_int_equal(array_container_contains(B, x), (x / 3 * 3 == x));
    }

    assert_int_equal(array_container_cardinality(B), (1 << 16) / 3 + 1);

    for (size_t x = 0; x < 1 << 16; x += 3) {
        assert_true(array_container_contains(B, x));
        assert_true(array_container_remove(B, x));
        assert_int_equal(B->cardinality, --expected_card);
        assert_false(array_container_contains(B, x));
    }

    assert_int_equal(array_container_cardinality(B), 0);

    for (int x = 65535; x >= 0; x -= 3) {
        assert_true(array_container_add(B, x));
        assert_true(array_container_contains(B, x));
        assert_int_equal(B->cardinality, ++expected_card);
        assert_false(B->cardinality > B->capacity);
    }

    assert_int_equal(array_container_cardinality(B), expected_card);

    for (size_t x = 0; x < 1 << 16; x++) {
        assert_int_equal(array_container_contains(B, x), (x / 3 * 3 == x));
    }

    for (size_t x = 0; x < 1 << 16; x += 3) {
        assert_true(array_container_contains(B, x));
        assert_true(array_container_remove(B, x));
        assert_int_equal(B->cardinality, --expected_card);
        assert_false(array_container_contains(B, x));
    }

    array_container_free(B);
}

void and_or_test() {
    DESCRIBE_TEST;

    array_container_t* B1 = array_container_create();
    array_container_t* B2 = array_container_create();
    array_container_t* BI = array_container_create();
    array_container_t* BO = array_container_create();
    array_container_t* TMP = array_container_create();

    assert_non_null(B1);
    assert_non_null(B2);
    assert_non_null(BI);
    assert_non_null(BO);
    assert_non_null(TMP);

    for (size_t x = 0; x < (1 << 16); x += 3) {
        array_container_add(B1, x);
        array_container_add(BI, x);
    }

    // important: 62 is not divisible by 3
    for (size_t x = 0; x < (1 << 16); x += 62) {
        array_container_add(B2, x);
        array_container_add(BI, x);
    }

    for (size_t x = 0; x < (1 << 16); x += 62 * 3) {
        array_container_add(BO, x);
    }

    const int card_inter = array_container_cardinality(BO);
    const int card_union = array_container_cardinality(BI);

    array_container_intersection(B1, B2, TMP);
    assert_int_equal(card_inter, array_container_cardinality(TMP));
    assert_true(array_container_equals(BO, TMP));

    array_container_union(B1, B2, TMP);
    assert_int_equal(card_union, array_container_cardinality(TMP));
    assert_true(array_container_equals(BI, TMP));

    array_container_free(B1);
    array_container_free(B2);
    array_container_free(BI);
    array_container_free(BO);
    array_container_free(TMP);
}

void to_uint32_array_test() {
    for (size_t offset = 1; offset < 128; offset *= 2) {
        array_container_t* B = array_container_create();
        assert_non_null(B);

        for (size_t k = 0; k < (1 << 16); k += offset) {
            assert_true(array_container_add(B, k));
        }

        int card = array_container_cardinality(B);
        uint32_t* out = malloc(sizeof(uint32_t) * card);
        assert_non_null(out);
        int nc = array_container_to_uint32_array(out, B, 0);

        assert_int_equal(card, nc);

        for (int k = 1; k < nc; ++k) {
            assert_int_equal(out[k], offset + out[k - 1]);
        }

        free(out);
        array_container_free(B);
    }
}

int main() {

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(printf_test),
        cmocka_unit_test(add_contains_test),
        cmocka_unit_test(and_or_test),
        cmocka_unit_test(to_uint32_array_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
