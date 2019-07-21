#include <stdio.h>
#include <stdlib.h>
#include "bmblock.h"

#define MIN 4
#define MAX 131

void print_bm_find_next(struct bmblock_array* bm);

int main(){
	struct bmblock_array* bm = bm_alloc(MIN, MAX);
	
	print_bm_find_next(bm);
	
	bm_set(bm, 4);
	bm_set(bm, 5);
	bm_set(bm, 6);
	print_bm_find_next(bm);
	
	for(int i = MIN; i < MAX; i+= 3){
		bm_set(bm, i);
	}
	print_bm_find_next(bm);
	
	for(int i = MIN + 1; i < MAX; i+= 5){
		bm_clear(bm, i);
	}
	print_bm_find_next(bm);
	
	free(bm);
}

void print_bm_find_next(struct bmblock_array* bm){
	bm_print(bm);
	printf("find_next() = %d\n", bm_find_next(bm));
}
