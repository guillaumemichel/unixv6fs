#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filev6.h"
#include "direntv6.h"
#include "inode.h"
#include "error.h"

int test(struct unix_filesystem *u){
	struct filev6 fv6;

	int err = filev6_open(u, ROOT_INUMBER, &fv6);
	if(err != 0){
		printf("filev6_open failed for root inode \n");
		return err;
	}
	
	direntv6_print_tree(u, fv6.i_number, "");
	return 0;
}
