#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "filev6.h"
#include "inode.h"
#include "error.h"
#include "unixv6fs.h"
#include "sector.h"

/**
 * @brief open up a file corresponding to a given inode; set offset to zero
 * @param u the filesystem (IN)
 * @param inr he inode number (IN)
 * @param fv6 the complete filve6 data structure (OUT)
 * @return 0 on success; <0 on errror
 */
int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(fv6);
	
	//Check if file is mounted
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	int err = inode_read(u, inr, &(fv6->i_node));
	if(err != 0) return err;
	fv6->u = u;
	fv6->i_number = inr;
	fv6->offset = 0;
	
	return 0;
}

/**
 * @brief read at most SECTOR_SIZE from the file at the current cursor
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param buf points to SECTOR_SIZE bytes of available memory (OUT)
 * @return >0: the number of bytes of the file read; 0: end of file; <0 error
 */
int filev6_readblock(struct filev6 *fv6, void *buf){
	M_REQUIRE_NON_NULL(fv6);
	M_REQUIRE_NON_NULL(fv6->u);
	M_REQUIRE_NON_NULL(buf);
	
	//Check if file is mounted
	if (fv6->u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
		
	//get the size of the inode
	int size = inode_getsize(&(fv6->i_node));

	if (fv6->offset+1 >= size) return 0;
	
	int sector = inode_findsector(fv6->u,&(fv6->i_node),fv6->offset/SECTOR_SIZE);
	if (sector<0) return sector; //sector < 0 iff an error is returned from inode_findsector
	
	int err = sector_read(fv6->u->f, (uint32_t)sector, buf);
	if (err!=0) return err;
	
	int readBytes = SECTOR_SIZE;
	
	//if we are reading the last block, set the readBytes to the size used by the filev6
	if (fv6->offset + SECTOR_SIZE > size){
		readBytes = size % SECTOR_SIZE;
	}
	fv6->offset += readBytes;
	
	return readBytes;
}

/**
 * @brief change the current offset of the given file to the one specified
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param off the new offset of the file
 * @return 0 on success; <0 on errror
 */
int filev6_lseek(struct filev6 *fv6, int32_t offset){
	if (offset < 0 || offset >= inode_getsize(&fv6->i_node)) return ERR_OFFSET_OUT_OF_RANGE;
	fv6->offset = offset;
	return 0;
}

/**
 * @brief create a new filev6
 * @param u the filesystem (IN)
 * @param mode the mode of the file
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @return 0 on success; <0 on errror
 */
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(fv6);
	
	struct inode in;
	memset(&in, 0, sizeof(struct inode));
	in.i_mode = IALLOC | mode;
	
	int err = inode_write(u,fv6->i_number, &in);
	if (err < 0) return err;
	
	fv6->i_node = in;
	fv6->u = u;
	fv6->offset = 0;
	return 0;
}

/**
 * @brief write a sector of data 
 * @param u the filesystem (IN)
 * @param buf the data we want to write (IN)
 * @param remaining_len number of bytes we still have to write on disk
 * @param sector_number the number of the sector we have to write
 * @param offset the offset of the sector
 * @return the number of bytes written on the sector
 */
 int filev6_writesector(struct unix_filesystem* u, void* buf, int remaining_len, uint16_t sector_number, int32_t offset){
	int err;
	int size = remaining_len + offset < SECTOR_SIZE ? remaining_len : SECTOR_SIZE - offset;
	
	uint8_t buffer[SECTOR_SIZE];
	
	//if offset is not null, we will write on an already-written sector, so we have to read it first
	if(offset != 0){
		err = sector_read(u->f, sector_number, buffer);
		if(err < 0) return err;
	}
	
	//add the data to fill the sector
	memcpy(buffer + offset, buf, size);
	
	//effectively write the filled sector
	err = sector_write(u->f, sector_number, buffer);
	if(err < 0) return err;
	
	return size;
}

/**
 * @brief pass from a small file to a big file with indirect sectors
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @return 0 on success, <0 on error
 */
int smallfile_to_bigfile(struct unix_filesystem *u, struct filev6 *fv6, uint16_t current_sector){
	//find the next unused sector
	int sector = bm_find_next(u->fbm);
	if (sector<0) return ERR_BITMAP_FULL;
	
	//write the sector numbers that were in i_addr to the new sector
	uint16_t buffer[ADDRESSES_PER_SECTOR];
	memset(buffer,0,ADDRESSES_PER_SECTOR);
	memcpy(buffer,fv6->i_node.i_addr,ADDR_SMALL_LENGTH*sizeof(uint16_t));
	
	int err = sector_write(u->f,sector,buffer);
	if (err<0) return err;
	
	//set all sectors pointed by i_addr to 0 except the first that is set to 'sector'
	fv6->i_node.i_addr[0]=sector;
	for (int i=1;i<ADDR_SMALL_LENGTH;++i)
		fv6->i_node.i_addr[i]=0;
		
	bm_set(u->fbm,sector);
	return 0;
}

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on error
 */
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, void *buf, int len){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(fv6);
	M_REQUIRE_NON_NULL(buf);
	
	int bytes_written = 0;
	int err;
	int32_t inode_size = inode_getsize(&fv6->i_node);
	
	//if inode is already too big OR will be too big, return ERR_FILE_TOO_LARGE
	if(inode_size > (ADDR_SMALL_LENGTH -1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE || 
		inode_size + len > (ADDR_SMALL_LENGTH -1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE) return ERR_FILE_TOO_LARGE;
	
	//Values to handle sectors for small files
	int32_t sector_number = inode_size / SECTOR_SIZE;
	int32_t sector_offset = inode_size % SECTOR_SIZE;
	
	//Values to handle indirect sectors for big files
	int32_t indirect_sector_number = sector_number / ADDRESSES_PER_SECTOR;
	int32_t indirect_sector_offset = sector_number % ADDRESSES_PER_SECTOR;
	
	//First we try to write a sector entirely
	if(sector_offset != 0){
		uint16_t sec_num;
		//Get the sector number that need to be completed
		if(inode_size <= ADDR_SMALL_LENGTH * SECTOR_SIZE){//Small files
			sec_num = fv6->i_node.i_addr[sector_number];
			
			//We completed the sector, now go to next one
			++sector_number;
		}else{//Big Files
			uint16_t tab[ADDRESSES_PER_SECTOR];
			err = sector_read(u->f, fv6->i_node.i_addr[indirect_sector_number], tab);
			if(err != 0) return err;

			sec_num = tab[indirect_sector_offset];
			
			//We completed the sector, now go to next one, in this case increment offset of indirect sector
			++indirect_sector_offset;
		}
		
		bytes_written = filev6_writesector(u, buf, len, sec_num, sector_offset);
		if(bytes_written < 0) return bytes_written;

		//Shift buf by number of written data
		buf += bytes_written;
	}
	
	int nb_bytes;
	//If there is still data to write, we are sure that we need to write in new sectors
	while(bytes_written < len){
		
		//If at some point the file reach 4Ko, we need to pass to indirect sectors
		if(inode_size + bytes_written == ADDR_SMALL_LENGTH * SECTOR_SIZE){
			err = smallfile_to_bigfile(u, fv6, sector_number);
			if (err<0) return err;
			
			//We went to indirected sectors, so the value of the current indirect sector is 0 and 
			//since there are 8 addresses already, we adjust the offset properly
			indirect_sector_number = 0;
			indirect_sector_offset = ADDR_SMALL_LENGTH + 1;
		}
		
		//Find a new sector to write the data
		int sector = bm_find_next(u->fbm);
		if(sector < 0) return ERR_BITMAP_FULL;
		
		if(inode_size + bytes_written <= ADDR_SMALL_LENGTH * SECTOR_SIZE){//Small files
			fv6->i_node.i_addr[sector_number] = sector;
			
			//Write data on the new sector
			nb_bytes = filev6_writesector(u, buf, len - bytes_written, sector, 0);
			if(nb_bytes < 0) return nb_bytes;
			
			//update the sector number
			++sector_number;
		}else{//Big files
			//if we have completed a sector with indirections, we need to create a new one
			if(indirect_sector_offset >= ADDRESSES_PER_SECTOR){
				++indirect_sector_number;//there ise a new indirection sector
				indirect_sector_offset = 0;//offset back to zero
				
				//Find a new indirection sector
				int new_indirection_sector = bm_find_next(u->fbm);
				if(new_indirection_sector < 0) return new_indirection_sector;
				
				//Update the value in i_addr
				fv6->i_node.i_addr[indirect_sector_number] = new_indirection_sector;
				
				bm_set(u->fbm, new_indirection_sector);
			}

			//we read the proper indirect sector
			uint16_t tab[ADDRESSES_PER_SECTOR];
			err = sector_read(u->f, fv6->i_node.i_addr[indirect_sector_number], tab);
			if(err < 0) return err;
			
			//Update at the right offset to indicate the new sector that will contain data
			tab[indirect_sector_offset] = sector;
			
			//Write the updated indirection
			err = sector_write(u->f, fv6->i_node.i_addr[indirect_sector_number], tab);
			if(err < 0) return err;
			
			//Write data on the new sector
			nb_bytes = filev6_writesector(u, buf, len - bytes_written, sector, 0);
			if(nb_bytes < 0) return nb_bytes;
			
			//Update the offset by one
			++indirect_sector_offset;
		}
		
		//Shift buffer and update values
		bytes_written += nb_bytes;
		buf += nb_bytes;
		
		//Update the i_addr and set the sector in the bitmap vector
		bm_set(u->fbm, sector);
	}
	
	//We set the new size of the inode
	err = inode_setsize(&fv6->i_node, inode_size + len);
	if(err < 0) return err;
	
	//Write the new inode to disk
	err = inode_write(u, fv6->i_number, &fv6->i_node);
	if(err < 0) return err;
	
	return 0;
}
