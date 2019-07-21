#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "unixv6fs.h"
#include "inode.h"
#include "error.h"
#include "sector.h"


/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u){
	//Check if given pointer are non-null
	M_REQUIRE_NON_NULL(u);
	
	//Contains "FIL" or "DIR" depending on the type of the inode to be printed
	char fileOrDir[strlen(SHORT_DIR_NAME)+1];
	
	//Array of 16 inodes representing a sector
	struct inode inode_tab[INODES_PER_SECTOR];
	
	//Read all the sectors containing the inodes
	for (size_t i = 0;i < (u->s.s_isize); ++i){
		//Read a sector and put it in the inodes tab
		int err_read = sector_read(u->f, (uint32_t)u->s.s_inode_start+i, inode_tab);
		if(err_read < 0) return err_read;
		
		//Print the inodes of the current sector, with respect to the format asked
		for(unsigned int j = 0; j < INODES_PER_SECTOR; ++j){
			if (inode_tab[j].i_mode & IALLOC){
				
				if (inode_tab[j].i_mode & IFDIR) strncpy(fileOrDir,SHORT_DIR_NAME,strlen(SHORT_DIR_NAME));
				else strncpy(fileOrDir, SHORT_FIL_NAME,strlen(SHORT_FIL_NAME));
				fileOrDir[strlen(SHORT_DIR_NAME)]='\0';
				
				printf("inode %3ld (%s) len %4d\n", i * INODES_PER_SECTOR + j, fileOrDir, inode_getsize(&inode_tab[j]));
			}			
		}
	}
	return 0;
}

/**
 * @brief prints the content of an inode structure
 * @param inode the inode structure to be displayed
 */
void inode_print(const struct inode *inode){
	printf("**********FS INODE START**********\n");
	if(inode == NULL){
		printf("NULL ptr\n");
	}else{
		printf("i_mode: %" PRIu16 "\n", inode->i_mode);
		printf("i_nlink: %" PRIu8 "\n", inode->i_nlink);
		printf("i_uid: %" PRIu8 "\n", inode->i_uid);
		printf("i_gid: %" PRIu8 "\n", inode->i_gid);
		printf("i_size0: %" PRIu8 "\n", inode->i_size0);
		printf("i_size1: %" PRIu16 "\n", inode->i_size1);
		printf("size: %d\n", inode_getsize(inode));
	}
	printf("**********FS INODE END**********\n");

}
	
/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode){
	M_REQUIRE_NON_NULL(u);
	
	//Check if file is mounted
	if (u->f == NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	struct inode inode_tab[INODES_PER_SECTOR];
	//Check if inode is in range
	uint16_t inode_number = u->s.s_isize * INODES_PER_SECTOR;
	if (inr >= inode_number || inr < (uint16_t)ROOT_INUMBER) return ERR_INODE_OUTOF_RANGE;
	
	//Read the sector and return the error if there is a error in sector_read
	int r = sector_read(u->f, (uint32_t)((u->s.s_inode_start) + inr/INODES_PER_SECTOR), inode_tab);
	if (r < 0) return r;
	
	//Check if the inode is allocated
	if (!(inode_tab[inr % INODES_PER_SECTOR].i_mode & IALLOC)) return ERR_UNALLOCATED_INODE;
	
	*inode = inode_tab[inr % INODES_PER_SECTOR];
				
	return 0;
}

/**
 * @brief write the content of an inode to disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (IN)
 * @return 0 on success; <0 on error
 */
int inode_write(struct unix_filesystem *u, uint16_t inr, struct inode *inode){
	M_REQUIRE_NON_NULL(u);
	
	//Check if file is mounted
	if (u->f == NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	struct inode inode_tab[INODES_PER_SECTOR];
	//Check if inode is in range
	uint16_t inode_number = u->s.s_isize * INODES_PER_SECTOR;
	if (inr >= inode_number) return ERR_INODE_OUTOF_RANGE;

	//Read the sector and return the error if there is a error in sector_read
	int err = sector_read(u->f, ((u->s.s_inode_start) + inr/INODES_PER_SECTOR), inode_tab);
	if (err < 0) return err;

	inode_tab[inr % INODES_PER_SECTOR] = *inode;
	err = sector_write(u->f, ((u->s.s_inode_start) + inr/INODES_PER_SECTOR), inode_tab);
	if (err < 0) return err;
		
	return 0;
}

/**
 * @brief identify the sector that corresponds to a given portion of a file
 * @param u the filesystem (IN)
 * @param inode the inode (IN)
 * @param file_sec_off the offset within the file (in sector-size units)
 * @return >0: the sector on disk; <0 error
 */
int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(i);
	
	//Check if file is mounted
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	//Check if inode is allocated
	if(!(i->i_mode & IALLOC)) return ERR_UNALLOCATED_INODE;
	
	//Get the size of the file
	int32_t filesize = inode_getsize(i);
	
	//Check if the file is too large (that is 7*256*sectors)
	if(filesize > (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE) return ERR_FILE_TOO_LARGE;
	
	//Check if file_sec_off is in the right range
	if((file_sec_off < 0) || (file_sec_off * SECTOR_SIZE >= filesize)) return ERR_OFFSET_OUT_OF_RANGE;
	
	if(filesize <= ADDR_SMALL_LENGTH * SECTOR_SIZE){
		return i->i_addr[file_sec_off];
	}else{
		uint16_t buffer[ADDRESSES_PER_SECTOR];
		
		int err = sector_read(u->f, i->i_addr[file_sec_off/ ADDRESSES_PER_SECTOR], buffer);
		if(err != 0){
			return err;
		}else{
			return buffer[file_sec_off % ADDRESSES_PER_SECTOR];
		}
	}
}

/**
 * @brief alloc a new inode (returns its inr if possible)
 * @param u the filesystem (IN)
 * @return the inode number of the new inode or error code on error
 */
int inode_alloc(struct unix_filesystem *u){
	int inr = bm_find_next(u->ibm);
	
	if(inr < 0){
		return ERR_NOMEM;
	}else{
		bm_set(u->ibm, (uint64_t)inr);
		return inr;
	}
}

/** 
 * @brief set the size of a given inode to the given size
 * @param inode the inode
 * @param new_size the new size
 * @return 0 on success; <0 on error
 */
int inode_setsize(struct inode *inode, int new_size){
	M_REQUIRE_NON_NULL(inode);
	if (new_size < 0) return ERR_NOMEM;
	inode->i_size1 = new_size & 0xFFFF;
	inode->i_size0 = (new_size >> 16) & 0xFF;
	return 0;
}
