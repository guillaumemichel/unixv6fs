#include <stdio.h>
#include <string.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "inode.h"
#include "error.h"
#include "sha.h"

void print_inode(struct unix_filesystem *u, int inode_number, struct filev6 *fs);

int test(struct unix_filesystem *u){
	struct filev6 fs;
	memset(&fs, 255, sizeof(fs));

	putchar('\n');
	print_inode(u, 3, &fs);
	putchar('\n');
	print_inode(u, 5, &fs);
	putchar('\n');
	
	printf("Listing inodes SHA:\n");
	int i = ROOT_INUMBER;
	while(filev6_open(u, i, &fs) == 0){
		inode_read(u, i, &fs.i_node);
		print_sha_inode(u, fs.i_node, i);
		++i;
	}

	return 0;
}

void print_inode(struct unix_filesystem *u, int inode_number, struct filev6 *fs){	
	int err = filev6_open(u, inode_number, fs);
	if(err != 0){
		printf("filev6_open failed for inode #%d\n", inode_number);
		return;
	}
	
	printf("Printing inode #%d:\n", inode_number);
	inode_print(&fs->i_node);
		
	if(fs->i_node.i_mode & IFDIR){
		 printf("which is a directory.\n");
	}else{
		printf("the first sector of data of which contains:\n");
		char tab[SECTOR_SIZE + 1];
		tab[SECTOR_SIZE] = '\0';
		filev6_readblock(fs, tab);
		printf("%s\n----\n", tab);
	}
}
