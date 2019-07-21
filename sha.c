#include <stdio.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include "sha.h"
#include "error.h"
#include "filev6.h"
#include "sector.h"
#include "inode.h"

/**
 * @brief put the string version of a given SHA into char tab
 * @param SHA a pointer to the array that contain a SHA
 * @param sha_string a pointer to the array that will contain the string version of the SHA
 */
static void sha_to_string(const unsigned char *SHA, char *sha_string)
{
    if ((SHA == NULL) || (sha_string == NULL)) {
        return;
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

/**
 * @brief print the sha of the content
 * @param content the content of which we want to print the sha
 * @param length the length of the content
 */
void print_sha_from_content(const unsigned char *content, size_t length){
	if(content == NULL){
		return;
	}
	
	unsigned char computed_sha[SHA256_DIGEST_LENGTH];
	SHA256(content, length, computed_sha);
	
	char sha_string[2 * SHA256_DIGEST_LENGTH + 1];
	sha_to_string(computed_sha, sha_string);
	printf("%s\n", sha_string);
}

/**
 * @brief print the sha of the content of an inode
 * @param u the filesystem
 * @param inode the inode of which we want to print the content
 * @param inr the inode number
 */
void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr){
	if(u == NULL) return;
	
	if(inode.i_mode & IALLOC){
		printf("SHA inode %d: ", inr);
		if(inode.i_mode & IFDIR){
			printf("no SHA for directories\n");
		}else{
			struct filev6 fv6 = {u, (uint16_t)inr, inode, 0};//since inode already here, no need to filev6_open
			int size = inode_getsize(&inode);
			
			char p[size+1];//char tab to be filled with inode's data 
			strcpy(p, "");
			char tab[SECTOR_SIZE];//char tab to be filled with part of the inode's data (block)
			while(filev6_readblock(&fv6, tab) > 0){//as long as we read data from the inode
				strncat(p, tab, SECTOR_SIZE);//concatenate the current read block to the rest of the inode's data
			}

			//Finally, print the sha of the content of the inode
			print_sha_from_content((unsigned char*) p, strlen(p));
		}
	}
	
}
