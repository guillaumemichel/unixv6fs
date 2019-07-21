#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include "error.h"
#include <errno.h>

/**
 * @brief allocate a new bmblock_array to handle elements indexed
 * between min and may (included, thus (max-min+1) elements).
 * @param min the mininum value supported by our bmblock_array
 * @param max the maxinum value supported by our bmblock_array
 * @return a pointer of the newly created bmblock_array or NULL on failure
 */
struct bmblock_array *bm_alloc(uint64_t min, uint64_t max){
	if(min > max) return NULL;

	size_t size = (max - min) / BITS_PER_VECTOR + 1;
	struct bmblock_array* bm = calloc(1, sizeof(struct bmblock_array) + (size-1)*sizeof(uint64_t));

	if(bm != NULL){
		bm->length = size;
		bm->cursor = 0;
		bm->min = min;
		bm->max = max;
	}
	return bm;
}

/**
 * @brief free a bmblock_array
 * @param bmblock_array the bmblock_array
 */
void bm_free(struct bmblock_array* bmblock_array){
	free(bmblock_array);
}

/**
 * @brief check if x is valid is between min and max and return its position in the array
 * @param bmblock_array the array containing the value we want to read
 * @param x an integer corresponding to the number of the value we are looking for
 * @return the position in the bmblock_array 
 */
int check_and_get_pos(struct bmblock_array *bmblock_array, uint64_t x){
	if(x < bmblock_array->min || x > bmblock_array->max) return ERR_BAD_PARAMETER;
	return (x - bmblock_array->min) / BITS_PER_VECTOR;
}
/**
 * @brief return the bit associated to the given value
 * @param bmblock_array the array containing the value we want to read
 * @param x an integer corresponding to the number of the value we are looking for
 * @return <0 on failure, 0 or 1 on success
 */
int bm_get(struct bmblock_array *bmblock_array, uint64_t x){
	M_REQUIRE_NON_NULL(bmblock_array);
	
	int pos_in_bm = check_and_get_pos(bmblock_array, x);
	if(pos_in_bm < 0) return pos_in_bm;
	
	uint64_t elem = bmblock_array->bm[pos_in_bm];
	uint64_t mask = UINT64_C(1);
	int offset = (x % BITS_PER_VECTOR) - bmblock_array->min;
	
	int bit = (elem >> offset) & mask;
	return bit;
}

/**
 * @brief set to true (or 1) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to set
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_set(struct bmblock_array *bmblock_array, uint64_t x){
	if(bmblock_array != NULL){
		int pos_in_bm = check_and_get_pos(bmblock_array, x);
		if(pos_in_bm >= 0){
			int offset = (x % BITS_PER_VECTOR) - bmblock_array->min;
			uint64_t mask = UINT64_C(1) << offset;
			bmblock_array->bm[pos_in_bm] |= mask;
		}
	}
}

/**
 * @brief set to false (or 0) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to clear
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_clear(struct bmblock_array *bmblock_array, uint64_t x){
	if(bmblock_array != NULL){
		int pos_in_bm = check_and_get_pos(bmblock_array, x);
		if(pos_in_bm < 0) return;
		
		int offset = (x % BITS_PER_VECTOR) - bmblock_array->min;
		uint64_t mask = ~(UINT64_C(1) << offset);
		bmblock_array->bm[pos_in_bm] &= mask;
		
		//Update the cursor
		if((unsigned int) pos_in_bm < bmblock_array->cursor)
			bmblock_array->cursor = pos_in_bm;
	}
}

/**
 * @brief return the next unused bit
 * @param bmblock_array the array we want to search for place
 * @return <0 on failure, the value of the next unused value otherwise
 */
int bm_find_next(struct bmblock_array *bmblock_array){
	M_REQUIRE_NON_NULL(bmblock_array);
	
	uint64_t i;
	for(i = bmblock_array->cursor; bmblock_array->bm[i] == UINT64_C(-1) && i < bmblock_array->length; ++i);

	if(i == bmblock_array->length){
		bmblock_array->cursor = i;
		 return ERR_BITMAP_FULL;
	}else{
		uint64_t mask = UINT64_C(1);
		uint64_t elem = bmblock_array->bm[i];
		//Find first ununsed bit and set the cursor
		unsigned int j;
		for(j = 0; ((elem & mask) != 0) && (j < BITS_PER_VECTOR); ++j){
		 elem >>= 1;
		}
		int next = i * BITS_PER_VECTOR + j + bmblock_array->min;	
		bmblock_array->cursor = i;
		return next;
	}
}

/**
 * @brief print the bits value of the given uint64_t value
 * @param to_print the uint64_t value to print
 */
void print_uint64(uint64_t to_print){
	uint64_t mask = UINT64_C(1);
	int value;
	for(unsigned int i = 0; i < BITS_PER_VECTOR; ++i){
		value = to_print & mask;
		to_print >>= 1;
		printf("%d", value);
		
		//Add a space every 8 bits
		if((i + 1) % 8 == 0 && i != BITS_PER_VECTOR - 1) putchar(' ');
	}
	putchar('\n');
}

/**
 * @brief usefull to see (and debug) content of a bmblock_array
 * @param bmblock_array the array we want to see
 */
void bm_print(struct bmblock_array *bmblock_array){
	if(bmblock_array != NULL){
		printf("**********BitMap Block START**********\n");
		printf("length: %" PRIu64 "\n", bmblock_array->length);
		printf("min: %" PRIu64 "\n", bmblock_array->min);
		printf("max: %" PRIu64 "\n", bmblock_array->max);
		printf("cursor: %" PRIu64 "\n", bmblock_array->cursor);
		printf("content:\n");
		for(unsigned int i = 0; i < bmblock_array->length; ++i){
			printf("%d: ", i);
			print_uint64(bmblock_array->bm[i]);
		}
		printf("**********BitMap Block END************\n");
	}
}
