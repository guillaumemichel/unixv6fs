#include <stdio.h>
#include <stdlib.h>
#include "inode.h"
#include "sector.h"
#include "error.h"

int test(struct unix_filesystem *u){
	inode_scan_print(u);
	
	struct inode inode_tab[INODES_PER_SECTOR];
	sector_read(u->f,u->s.s_inode_start, inode_tab);


	uint16_t t = 5;
	inode_print(&inode_tab[t]);
	printf("--------using inode_read----------\n");
	struct inode* i = calloc(1, sizeof(struct inode));
	int r = inode_read(u, t, i);
	if (r!=0) printf("%d\n",r);
	else inode_print(i);
	
	printf("%d\n", inode_findsector(u, i, 8));
	free(i);
	return 0;
}
