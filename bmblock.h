#pragma once

/**
 * @file bmblock.h
 * @brief utility functions for the UNIX v6 filesystem.
 *
 * @author Edouard Bugnion
 * @date summer 2016
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bmblock_array {
    size_t length;
    uint64_t cursor;
    uint64_t min;
    uint64_t max;
    uint64_t bm[1];
};

#define BITS_PER_VECTOR (8*sizeof(((struct bmblock_array*)0)->bm[0]))

/**
 * @brief allocate a new bmblock_array to handle elements indexed
 * between min and may (included, thus (max-min+1) elements).
 * @param min the mininum value supported by our bmblock_array
 * @param max the maxinum value supported by our bmblock_array
 * @return a pointer of the newly created bmblock_array or NULL on failure
 */
struct bmblock_array *bm_alloc(uint64_t min, uint64_t max);

/**
 * @brief free a bmblock_array
 * @param bmblock_array the bmblock_array
 */
void bm_free(struct bmblock_array* bmblock_array);

/**
 * @brief return the bit associated to the given value
 * @param bmblock_array the array containing the value we want to read
 * @param x an integer corresponding to the number of the value we are looking for
 * @return <0 on failure, 0 or 1 on success
 */
int bm_get(struct bmblock_array *bmblock_array, uint64_t x);

/**
 * @brief set to true (or 1) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to set
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_set(struct bmblock_array *bmblock_array, uint64_t x);

/**
 * @brief set to false (or 0) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to clear
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_clear(struct bmblock_array *bmblock_array, uint64_t x);

/**
 * @brief return the next unused bit
 * @param bmblock_array the array we want to search for place
 * @return <0 on failure, the value of the next unused value otherwise
 */
int bm_find_next(struct bmblock_array *bmblock_array);

/**
 * @brief usefull to see (and debug) content of a bmblock_array
 * @param bmblock_array the array we want to see
 */
void bm_print(struct bmblock_array *bmblock_array);

#ifdef __cplusplus
}
#endif
