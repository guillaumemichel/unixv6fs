#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "mount.h"
#include "error.h"
#include "sector.h"
#include "bmblock.h"
#include "inode.h"

void fill_ibm(struct unix_filesystem *u);
void fill_fbm(struct unix_filesystem *u);

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u){
	//Check if given pointers are non-null
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(u);
	
	memset(u, 0, sizeof(*u));

	u->f = fopen(filename, "r+");
	
	if(u->f == NULL){
		return ERR_IO;
	}
	
	//Create a buffer to get sector data
	uint8_t buffer[SECTOR_SIZE];
	
	//Read bootblock sector
	int err_bootsector = sector_read(u->f, BOOTBLOCK_SECTOR, buffer);
	
	if(err_bootsector != 0) return err_bootsector;
	if(buffer[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM) return ERR_BADBOOTSECTOR;
	
	int err_superblock = sector_read(u->f, SUPERBLOCK_SECTOR, buffer);
	if(err_superblock != 0) return err_superblock;
	
	//Write the superblock
	memcpy(&u->s, buffer, SECTOR_SIZE);
	
	u->fbm = bm_alloc(u->s.s_block_start + 1, u->s.s_fsize - 1);
	u->ibm = bm_alloc(u->s.s_inode_start, u->s.s_isize * INODES_PER_SECTOR - 1);
	fill_ibm(u);
	fill_fbm(u);
	
	return 0;
	
}

/**
 * @brief fill the ibm struct of the filesystem
 * @param u the filesystem to fill
 */
void fill_ibm(struct unix_filesystem *u){
	if(u != NULL){
		struct inode inode_tab[INODES_PER_SECTOR];
		
		for(int i = 0; i < u->s.s_isize; ++i){
			int err = sector_read(u->f, u->s.s_inode_start+i, inode_tab);
			for(unsigned int j = 0; j < INODES_PER_SECTOR; ++j){
				if(inode_tab[j].i_mode & IALLOC || err < 0)
					bm_set(u->ibm, i * INODES_PER_SECTOR + j);
			}
		} 
	}
}

/**
 * @brief fill the fbm struct of the filesystem
 * @param u the filesystem to fill
 */
void fill_fbm(struct unix_filesystem *u){
	if(u != NULL){
		struct inode inode_tab[INODES_PER_SECTOR];
		int sector;
		int32_t offset;
		int32_t addr;
		int32_t inode_size;
		
		for(int i = 0; i < u->s.s_isize; ++i){
			int err = sector_read(u->f, u->s.s_inode_start+i, inode_tab);
			if(err == 0){
				for(unsigned int j = 0; j < INODES_PER_SECTOR; ++j){
					offset = 0;
					inode_size = inode_getsize(&inode_tab[j]);
					
					while((sector = inode_findsector(u, &inode_tab[j], offset)) > 0){
						
						//If size is too big, then it uses indirect sectors
						if(inode_size > ADDR_SMALL_LENGTH * SECTOR_SIZE){
							addr = offset / ADDRESSES_PER_SECTOR;
							
							//Handle indirect sectors
							if(addr >= 0 && addr < ADDR_SMALL_LENGTH)
								bm_set(u->fbm, inode_tab[j].i_addr[addr]);

						}
						
						bm_set(u->fbm, sector);
						++offset;
					}
				}
			}
		} 
	}
}

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u){
	if (u!=NULL){
		printf("**********FS SUPERBLOCK START**********\n");
		printf("%-19s : %" PRIu16 "\n", "s_isize", u->s.s_isize);
		printf("%-19s : %" PRIu16 "\n", "s_fsize", u->s.s_fsize);
		printf("%-19s : %" PRIu16 "\n", "s_fbmsize", u->s.s_fbmsize);
		printf("%-19s : %" PRIu16 "\n", "s_ibmsize", u->s.s_ibmsize);
		printf("%-19s : %" PRIu16 "\n", "s_inode_start", u->s.s_inode_start);
		printf("%-19s : %" PRIu16 "\n", "s_block_start", u->s.s_block_start);
		printf("%-19s : %" PRIu16 "\n", "s_fbm_start", u->s.s_fbm_start);
		printf("%-19s : %" PRIu16 "\n", "s_ibm_start", u->s.s_ibm_start);
		printf("%-19s : %" PRIu8 "\n", "s_flock", u->s.s_flock);
		printf("%-19s : %" PRIu8 "\n", "s_ilock", u->s.s_ilock);
		printf("%-19s : %" PRIu8 "\n", "s_fmod", u->s.s_fmod);
		printf("%-19s : %" PRIu8 "\n", "s_ronly", u->s.s_ronly);
		printf("%-19s : [0] %" PRIu16 "\n", "s_time", u->s.s_time[0]);
		printf("**********FS SUPERBLOCK END**********\n");
	}
	else debug_print("Null filesystem pointer\n");
}

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesytem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u){
	M_REQUIRE_NON_NULL(u);
	bm_free(u->ibm);
	bm_free(u->fbm);
	if(fclose(u->f) != 0){
		debug_print("Cannot unmount the file system\n");
		return ERR_IO;
	}
	return 0;
}

/**
 * @brief create a new filesystem
 * @param num_blocks the total number of blocks (= max size of disk), in sectors
 * @param num_inodes the total number of inodes
 */
int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes){
	M_REQUIRE_NON_NULL(filename);
	
	//Creation of the superblock
	struct superblock s;
	memset(&s, 0, SECTOR_SIZE);
	
	s.s_isize = num_inodes / INODES_PER_SECTOR;
	//Make sure that there is enough sectors for 'num_inodes' inodes
	if(num_inodes % INODES_PER_SECTOR != 0)
		++s.s_isize;
		
	if(num_blocks >= s.s_isize + num_inodes)
		s.s_fsize = num_blocks;
	else
		return ERR_NOT_ENOUGH_BLOCS;
		
	s.s_inode_start = SUPERBLOCK_SECTOR + 1;
	s.s_block_start = s.s_inode_start + s.s_isize;
	
	//Create the new file
	FILE* file = fopen(filename, "wb");
	if(file == NULL) return ERR_IO;
	
	//Create and write the bootblock sector
	uint8_t bootblock[SECTOR_SIZE];
	memset(bootblock, 0, SECTOR_SIZE);
	bootblock[BOOTBLOCK_MAGIC_NUM_OFFSET] = BOOTBLOCK_MAGIC_NUM;
	int err = sector_write(file, BOOTBLOCK_SECTOR, bootblock);
	if(err < 0){
		fclose(file);
		return err;
	}
	
	//Write the superblock
	err = sector_write(file, SUPERBLOCK_SECTOR, &s);
	if(err < 0){
		fclose(file);
		return err;
	}
	
	struct inode inode_tab[INODES_PER_SECTOR];
	memset(inode_tab, 0, SECTOR_SIZE);
	
	//Create and write root inode
	struct inode root;
	memset(&root, 0, sizeof(struct inode));
	root.i_mode = IFDIR | IALLOC;
	inode_tab[ROOT_INUMBER] = root;
	err = sector_write(file, s.s_inode_start, inode_tab);
	
	//Reset memory to have empty inodes
	memset(inode_tab, 0, SECTOR_SIZE);
	
	for(int i = s.s_inode_start + 1; i < s.s_block_start; ++i){
		err = sector_write(file, i, inode_tab);
		if(err < 0){
			fclose(file);
			return err;
		}
	}	
	
	if(fclose(file) != 0) return ERR_IO;
	
	return 0;
}
