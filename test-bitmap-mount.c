#include <stdio.h>
#include "bmblock.h"
#include "mount.h"
#include "error.h"

int test(struct unix_filesystem *u){
	bm_print(u->fbm);
	bm_print(u->ibm);
	return 0;
}
