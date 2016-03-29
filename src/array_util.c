#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "array_util.h"
#include "portability.h"
#include "utilasm.h"

// good old bin. search
int32_t binarySearch(const uint16_t *array, int32_t lenarray, uint16_t ikey) {
    int32_t low = 0;
    int32_t high = lenarray - 1;
    while (low <= high) {
        int32_t middleIndex = (low + high) >> 1;
        uint16_t middleValue = array[middleIndex];
        if (middleValue < ikey) {
            low = middleIndex + 1;
        } else if (middleValue > ikey) {
            high = middleIndex - 1;
        } else {
            return middleIndex;
        }
    }
    return -(low + 1);
}

int32_t advanceUntil(const uint16_t *array, int32_t pos, int32_t length,
                     uint16_t min) {
    int32_t lower = pos + 1;

    if ((lower >= length) || (array[lower] >= min)) {
        return lower;
    }

    int32_t spansize = 1;

    while ((lower + spansize < length) && (array[lower + spansize] < min)) {
        spansize <<= 1;
    }
    int32_t upper = (lower + spansize < length) ? lower + spansize : length - 1;

    if (array[upper] == min) {
        return upper;
    }
    if (array[upper] < min) {
        // means
        // array
        // has no
        // item
        // >= min
        // pos = array.length;
        return length;
    }

    // we know that the next-smallest span was too small
    lower += (spansize >> 1);

    int32_t mid = 0;
    while (lower + 1 != upper) {
        mid = (lower + upper) >> 1;
        if (array[mid] == min) {
            return mid;
        } else if (array[mid] < min) {
            lower = mid;
        } else {
            upper = mid;
        }
    }
    return upper;
}

// used by intersect_vector16
static const uint8_t shuffle_mask16[] __attribute__((aligned(0x1000))) = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 2,  3,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,
    2,  3,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 6,  7,  -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 4,  5,  6,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  4,  5,  6,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,
    6,  7,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,
    7,  -1, -1, -1, -1, -1, -1, -1, -1, 8,  9,  -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, 0,  1,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 2,  3,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  2,  3,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,  8,
    9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  -1, -1, -1, -1,
    -1, -1, -1, -1, 6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0,  1,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,
    6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,
    7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  -1, -1, -1,
    -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1,
    -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, 10,
    11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  10, 11,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  10, 11, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  10, 11, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, 4,  5,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 0,  1,  4,  5,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    2,  3,  4,  5,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,
    3,  4,  5,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 6,  7,  10, 11, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  10, 11, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  10, 11, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  10, 11, -1, -1, -1, -1, -1, -1, -1,
    -1, 4,  5,  6,  7,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,
    4,  5,  6,  7,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,
    7,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,
    10, 11, -1, -1, -1, -1, -1, -1, 8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 0,  1,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 2,  3,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  2,  3,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,
    10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  10,
    11, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  10, 11, -1, -1,
    -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  10, 11, -1, -1, -1,
    -1, -1, -1, 6,  7,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  6,  7,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  6,
    7,  8,  9,  10, 11, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,
    8,  9,  10, 11, -1, -1, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  10, 11, -1,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  10, 11, -1, -1,
    -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  8,  9,  10, 11, -1, -1, -1, -1, -1,
    -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, -1, -1, -1, -1, 12, 13,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  12, 13, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  12, 13, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  12, 13, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 4,  5,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 0,  1,  4,  5,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,
    3,  4,  5,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,
    4,  5,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 6,  7,  12, 13, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  12, 13, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  12, 13, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 0,  1,  2,  3,  6,  7,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1,
    4,  5,  6,  7,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,
    5,  6,  7,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,
    12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  12,
    13, -1, -1, -1, -1, -1, -1, 8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 2,  3,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,
    2,  3,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,  12,
    13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  12, 13,
    -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  12, 13, -1, -1, -1,
    -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  12, 13, -1, -1, -1, -1,
    -1, -1, 6,  7,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  6,  7,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,
    8,  9,  12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,
    9,  12, 13, -1, -1, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  12, 13, -1, -1,
    -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  12, 13, -1, -1, -1,
    -1, -1, -1, 2,  3,  4,  5,  6,  7,  8,  9,  12, 13, -1, -1, -1, -1, -1, -1,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  12, 13, -1, -1, -1, -1, 10, 11, 12,
    13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  10, 11, 12, 13,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  10, 11, 12, 13, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  10, 11, 12, 13, -1, -1, -1, -1,
    -1, -1, -1, -1, 4,  5,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0,  1,  4,  5,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,
    4,  5,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,
    5,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 6,  7,  10, 11, 12, 13, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  10, 11, 12, 13, -1, -1, -1,
    -1, -1, -1, -1, -1, 2,  3,  6,  7,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1,
    -1, -1, 0,  1,  2,  3,  6,  7,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 4,
    5,  6,  7,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,
    6,  7,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  10,
    11, 12, 13, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  10, 11,
    12, 13, -1, -1, -1, -1, 8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 0,  1,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1,
    2,  3,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,
    3,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,  10, 11,
    12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  10, 11, 12,
    13, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  10, 11, 12, 13, -1, -1,
    -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  10, 11, 12, 13, -1, -1, -1,
    -1, 6,  7,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,
    6,  7,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  8,
    9,  10, 11, 12, 13, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,  9,
    10, 11, 12, 13, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  10, 11, 12, 13, -1,
    -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, -1, -1,
    -1, -1, 2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, -1, -1, -1, -1, 0,
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, -1, -1, 14, 15, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  14, 15, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  14, 15, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  14, 15, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 4,  5,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  4,  5,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,
    5,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,
    14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 6,  7,  14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  14, 15, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 2,  3,  6,  7,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0,  1,  2,  3,  6,  7,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,
    6,  7,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,
    7,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  14, 15, -1,
    -1, -1, -1, -1, -1, 8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 0,  1,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,
    3,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,
    8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,  14, 15, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  14, 15, -1, -1,
    -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1,
    6,  7,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,
    7,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  8,  9,
    14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,  9,  14,
    15, -1, -1, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  14, 15, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  14, 15, -1, -1, -1, -1, -1,
    -1, 2,  3,  4,  5,  6,  7,  8,  9,  14, 15, -1, -1, -1, -1, -1, -1, 0,  1,
    2,  3,  4,  5,  6,  7,  8,  9,  14, 15, -1, -1, -1, -1, 10, 11, 14, 15, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  10, 11, 14, 15, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  10, 11, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 0,  1,  2,  3,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1,
    -1, -1, 4,  5,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  4,  5,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,
    10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  10,
    11, 14, 15, -1, -1, -1, -1, -1, -1, 6,  7,  10, 11, 14, 15, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  10, 11, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, 2,  3,  6,  7,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  2,  3,  6,  7,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, 4,  5,  6,
    7,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,
    10, 11, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  10, 11, 14,
    15, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  10, 11, 14, 15,
    -1, -1, -1, -1, 8,  9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0,  1,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,
    8,  9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  8,
    9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,  10, 11, 14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  10, 11, 14, 15, -1,
    -1, -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1,
    -1, -1, 0,  1,  2,  3,  4,  5,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1, 6,
    7,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,
    8,  9,  10, 11, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  8,  9,  10,
    11, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,  9,  10, 11,
    14, 15, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  10, 11, 14, 15, -1, -1, -1,
    -1, -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1,
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 14, 15, -1, -1, -1, -1, 0,  1,  2,
    3,  4,  5,  6,  7,  8,  9,  10, 11, 14, 15, -1, -1, 12, 13, 14, 15, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  12, 13, 14, 15, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 2,  3,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  2,  3,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1,
    -1, 4,  5,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,
    4,  5,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  12,
    13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  12, 13,
    14, 15, -1, -1, -1, -1, -1, -1, 6,  7,  12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 0,  1,  6,  7,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1,
    -1, -1, 2,  3,  6,  7,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  2,  3,  6,  7,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 4,  5,  6,  7,
    12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  12,
    13, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  12, 13, 14, 15, -1,
    -1, -1, -1, 8,  9,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 2,  3,  8,
    9,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  8,  9,
    12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 4,  5,  8,  9,  12, 13, 14, 15, -1,
    -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  12, 13, 14, 15, -1, -1,
    -1, -1, -1, -1, 2,  3,  4,  5,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, 0,  1,  2,  3,  4,  5,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1, 6,  7,
    8,  9,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  8,
    9,  12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  6,  7,  8,  9,  12, 13,
    14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,  9,  12, 13, 14,
    15, -1, -1, -1, -1, 4,  5,  6,  7,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1,
    -1, -1, 0,  1,  4,  5,  6,  7,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1, 2,
    3,  4,  5,  6,  7,  8,  9,  12, 13, 14, 15, -1, -1, -1, -1, 0,  1,  2,  3,
    4,  5,  6,  7,  8,  9,  12, 13, 14, 15, -1, -1, 10, 11, 12, 13, 14, 15, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  10, 11, 12, 13, 14, 15, -1, -1,
    -1, -1, -1, -1, -1, -1, 2,  3,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, 0,  1,  2,  3,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1,
    4,  5,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  4,
    5,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  4,  5,  10, 11,
    12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  10, 11, 12,
    13, 14, 15, -1, -1, -1, -1, 6,  7,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1,
    -1, -1, -1, -1, 0,  1,  6,  7,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, 2,  3,  6,  7,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,
    2,  3,  6,  7,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, 4,  5,  6,  7,  10,
    11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  6,  7,  10, 11,
    12, 13, 14, 15, -1, -1, -1, -1, 2,  3,  4,  5,  6,  7,  10, 11, 12, 13, 14,
    15, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  10, 11, 12, 13, 14, 15,
    -1, -1, 8,  9,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, 0,
    1,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 2,  3,  8,  9,
    10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  8,  9,  10,
    11, 12, 13, 14, 15, -1, -1, -1, -1, 4,  5,  8,  9,  10, 11, 12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, 0,  1,  4,  5,  8,  9,  10, 11, 12, 13, 14, 15, -1,
    -1, -1, -1, 2,  3,  4,  5,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1,
    0,  1,  2,  3,  4,  5,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, 6,  7,  8,
    9,  10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, 0,  1,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, -1, -1, -1, -1, 2,  3,  6,  7,  8,  9,  10, 11, 12,
    13, 14, 15, -1, -1, -1, -1, 0,  1,  2,  3,  6,  7,  8,  9,  10, 11, 12, 13,
    14, 15, -1, -1, 4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, -1,
    -1, 0,  1,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, 2,  3,
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, -1, -1, 0,  1,  2,  3,  4,
    5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

/**
 * From Schlegel et al., Fast Sorted-Set Intersection using SIMD Instructions
 * Optimized by D. Lemire on May 3rd 2013
 */
int32_t intersect_vector16(const uint16_t *A, size_t s_a, const uint16_t *B,
                           size_t s_b, uint16_t *C) {
    size_t count = 0;
    size_t i_a = 0, i_b = 0;
    const int vectorlength = sizeof(__m128i) / sizeof(uint16_t);
    const size_t st_a = (s_a / vectorlength) * vectorlength;
    const size_t st_b = (s_b / vectorlength) * vectorlength;
    __m128i v_a, v_b;
    if ((i_a < st_a) && (i_b < st_b)) {
        v_a = _mm_lddqu_si128((__m128i *)&A[i_a]);
        v_b = _mm_lddqu_si128((__m128i *)&B[i_b]);
        while ((A[i_a] == 0) || (B[i_b] == 0)) {
            const __m128i res_v = _mm_cmpestrm(
                v_b, vectorlength, v_a, vectorlength,
                _SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
            const int r = _mm_extract_epi32(res_v, 0);
            __m128i sm16 = _mm_load_si128((const __m128i *)shuffle_mask16 + r);
            __m128i p = _mm_shuffle_epi8(v_a, sm16);
            _mm_storeu_si128((__m128i *)&C[count], p);  // can overflow
            count += _mm_popcnt_u32(r);
            const uint16_t a_max = A[i_a + vectorlength - 1];
            const uint16_t b_max = B[i_b + vectorlength - 1];
            if (a_max <= b_max) {
                i_a += vectorlength;
                if (i_a == st_a) break;
                v_a = _mm_lddqu_si128((__m128i *)&A[i_a]);
            }
            if (b_max <= a_max) {
                i_b += vectorlength;
                if (i_b == st_b) break;
                v_b = _mm_lddqu_si128((__m128i *)&B[i_b]);
            }
        }
        if ((i_a < st_a) && (i_b < st_b))
            while (true) {
                const __m128i res_v = _mm_cmpistrm(
                    v_b, v_a,
                    _SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK);
                const int r = _mm_extract_epi32(res_v, 0);
                __m128i sm16 =
                    _mm_load_si128((const __m128i *)shuffle_mask16 + r);
                __m128i p = _mm_shuffle_epi8(v_a, sm16);
                _mm_storeu_si128((__m128i *)&C[count], p);  // can overflow
                count += _mm_popcnt_u32(r);
                const uint16_t a_max = A[i_a + vectorlength - 1];
                const uint16_t b_max = B[i_b + vectorlength - 1];
                if (a_max <= b_max) {
                    i_a += vectorlength;
                    if (i_a == st_a) break;
                    v_a = _mm_lddqu_si128((__m128i *)&A[i_a]);
                }
                if (b_max <= a_max) {
                    i_b += vectorlength;
                    if (i_b == st_b) break;
                    v_b = _mm_lddqu_si128((__m128i *)&B[i_b]);
                }
            }
    }
    // intersect the tail using scalar intersection
    while (i_a < s_a && i_b < s_b) {
        uint16_t a = A[i_a];
        uint16_t b = B[i_b];
        if (a < b) {
            i_a++;
        } else if (b < a) {
            i_b++;
        } else {
            C[count] = a;  //==b;
            count++;
            i_a++;
            i_b++;
        }
    }
    return count;
}

/* Computes the intersection between one small and one large set of uint16_t.
 * Stores the result into buffer and return the number of elements. */
int32_t intersect_skewed_uint16(const uint16_t *small, size_t size_s,
                                const uint16_t *large, size_t size_l,
                                uint16_t *buffer) {
    size_t pos = 0, idx_l = 0, idx_s = 0;

    if (0 == size_s) {
        return 0;
    }

    uint16_t val_l = large[idx_l], val_s = small[idx_s];

    while (true) {
        if (val_l < val_s) {
            idx_l = advanceUntil(large, idx_l, size_l, val_s);
            if (idx_l == size_l) break;
            val_l = large[idx_l];
        } else if (val_s < val_l) {
            idx_s++;
            if (idx_s == size_s) break;
            val_s = small[idx_s];
        } else {
            buffer[pos++] = val_s;
            idx_s++;
            if (idx_s == size_s) break;
            val_s = small[idx_s];
            idx_l = advanceUntil(large, idx_l, size_l, val_s);
            if (idx_l == size_l) break;
            val_l = large[idx_l];
        }
    }

    return pos;
}

/**
 * Generic intersection function. Passes unit tests.
 */
int32_t intersect_uint16(const uint16_t *A, const size_t lenA,
                         const uint16_t *B, const size_t lenB, uint16_t *out) {
    const uint16_t *initout = out;
    if (lenA == 0 || lenB == 0) return 0;
    const uint16_t *endA = A + lenA;
    const uint16_t *endB = B + lenB;

    while (1) {
        while (*A < *B) {
        SKIP_FIRST_COMPARE:
            if (++A == endA) return (out - initout);
        }
        while (*A > *B) {
            if (++B == endB) return (out - initout);
        }
        if (*A == *B) {
            *out++ = *A;
            if (++A == endA || ++B == endB) return (out - initout);
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    return (out - initout);  // NOTREACHED
}

/**
 * Generic intersection function.
 */
size_t intersect_uint32(const uint32_t *A, const size_t lenA, const uint32_t *B,
                        const size_t lenB, uint32_t *out) {
    const uint32_t *initout = out;
    if (lenA == 0 || lenB == 0) return 0;
    const uint32_t *endA = A + lenA;
    const uint32_t *endB = B + lenB;

    while (1) {
        while (*A < *B) {
        SKIP_FIRST_COMPARE:
            if (++A == endA) return (out - initout);
        }
        while (*A > *B) {
            if (++B == endB) return (out - initout);
        }
        if (*A == *B) {
            *out++ = *A;
            if (++A == endA || ++B == endB) return (out - initout);
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    return (out - initout);  // NOTREACHED
}

size_t intersect_uint32_card(const uint32_t *A, const size_t lenA,
                             const uint32_t *B, const size_t lenB) {
    if (lenA == 0 || lenB == 0) return 0;
    size_t card = 0;
    const uint32_t *endA = A + lenA;
    const uint32_t *endB = B + lenB;

    while (1) {
        while (*A < *B) {
        SKIP_FIRST_COMPARE:
            if (++A == endA) return card;
        }
        while (*A > *B) {
            if (++B == endB) return card;
        }
        if (*A == *B) {
            card++;
            if (++A == endA || ++B == endB) return card;
        } else {
            goto SKIP_FIRST_COMPARE;
        }
    }
    return card;  // NOTREACHED
}

// TODO: can one vectorize the computation of the union?

size_t union_uint16(const uint16_t *set_1, size_t size_1, const uint16_t *set_2,
                    size_t size_2, uint16_t *buffer) {
    size_t pos = 0, idx_1 = 0, idx_2 = 0;

    if (0 == size_2) {
        memcpy(buffer, set_1, size_1 * sizeof(uint16_t));
        return size_1;
    }
    if (0 == size_1) {
        memcpy(buffer, set_2, size_2 * sizeof(uint16_t));
        return size_2;
    }

    uint16_t val_1 = set_1[idx_1], val_2 = set_2[idx_2];

    while (true) {
        if (val_1 < val_2) {
            buffer[pos++] = val_1;
            ++idx_1;
            if (idx_1 >= size_1) break;
            val_1 = set_1[idx_1];
        } else if (val_2 < val_1) {
            buffer[pos++] = val_2;
            ++idx_2;
            if (idx_2 >= size_2) break;
            val_2 = set_2[idx_2];
        } else {
            buffer[pos++] = val_1;
            ++idx_1;
            ++idx_2;
            if (idx_1 >= size_1 || idx_2 >= size_2) break;
            val_1 = set_1[idx_1];
            val_2 = set_2[idx_2];
        }
    }

    if (idx_1 < size_1) {
        const size_t n_elems = size_1 - idx_1;
        memcpy(buffer + pos, set_1 + idx_1, n_elems * sizeof(uint16_t));
        pos += n_elems;
    } else if (idx_2 < size_2) {
        const size_t n_elems = size_2 - idx_2;
        memcpy(buffer + pos, set_2 + idx_2, n_elems * sizeof(uint16_t));
        pos += n_elems;
    }

    return pos;
}

size_t union_uint32(const uint32_t *set_1, size_t size_1, const uint32_t *set_2,
                    size_t size_2, uint32_t *buffer) {
    size_t pos = 0, idx_1 = 0, idx_2 = 0;

    if (0 == size_2) {
        memcpy(buffer, set_1, size_1 * sizeof(uint32_t));
        return size_1;
    }
    if (0 == size_1) {
        memcpy(buffer, set_2, size_2 * sizeof(uint32_t));
        return size_2;
    }

    uint32_t val_1 = set_1[idx_1], val_2 = set_2[idx_2];

    while (true) {
        if (val_1 < val_2) {
            buffer[pos++] = val_1;
            ++idx_1;
            if (idx_1 >= size_1) break;
            val_1 = set_1[idx_1];
        } else if (val_2 < val_1) {
            buffer[pos++] = val_2;
            ++idx_2;
            if (idx_2 >= size_2) break;
            val_2 = set_2[idx_2];
        } else {
            buffer[pos++] = val_1;
            ++idx_1;
            ++idx_2;
            if (idx_1 >= size_1 || idx_2 >= size_2) break;
            val_1 = set_1[idx_1];
            val_2 = set_2[idx_2];
        }
    }

    if (idx_1 < size_1) {
        const size_t n_elems = size_1 - idx_1;
        memcpy(buffer + pos, set_1 + idx_1, n_elems * sizeof(uint32_t));
        pos += n_elems;
    } else if (idx_2 < size_2) {
        const size_t n_elems = size_2 - idx_2;
        memcpy(buffer + pos, set_2 + idx_2, n_elems * sizeof(uint32_t));
        pos += n_elems;
    }

    return pos;
}

size_t union_uint32_card(const uint32_t *set_1, size_t size_1,
                         const uint32_t *set_2, size_t size_2) {
    size_t pos = 0, idx_1 = 0, idx_2 = 0;

    if (0 == size_2) {
        return size_1;
    }
    if (0 == size_1) {
        return size_2;
    }

    uint32_t val_1 = set_1[idx_1], val_2 = set_2[idx_2];

    while (true) {
        if (val_1 < val_2) {
            ++idx_1;
            ++pos;
            if (idx_1 >= size_1) break;
            val_1 = set_1[idx_1];
        } else if (val_2 < val_1) {
            ++idx_2;
            ++pos;
            if (idx_2 >= size_2) break;
            val_2 = set_2[idx_2];
        } else {
            ++idx_1;
            ++idx_2;
            ++pos;
            if (idx_1 >= size_1 || idx_2 >= size_2) break;
            val_1 = set_1[idx_1];
            val_2 = set_2[idx_2];
        }
    }

    if (idx_1 < size_1) {
        const size_t n_elems = size_1 - idx_1;
        pos += n_elems;
    } else if (idx_2 < size_2) {
        const size_t n_elems = size_2 - idx_2;
        pos += n_elems;
    }
    return pos;
}
