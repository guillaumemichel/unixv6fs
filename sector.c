#include <stdio.h>
#include "error.h"
#include "unixv6fs.h"

/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (OUT)
 * @return 0 on success; <0 on error
 */
int sector_read(FILE *f, uint32_t sector, void *data){
	//Check if given pointers are non-null
	M_REQUIRE_NON_NULL(f);
	M_REQUIRE_NON_NULL(data);
	
	//Place the cursor on the right position in the sector
	int err = fseek(f, SECTOR_SIZE*sector, SEEK_SET);
	if(err != 0){
		debug_print("Erreur: impossible de se mettre au bon endroit\n");
		return ERR_IO;
	}
	
	//If no error, read the 512 bytes of the sector
	int read_bytes = fread(data, SECTOR_SIZE, 1, f);
	if(read_bytes == 1){
		return 0;
	}else{
		debug_print("Erreur: impossible de lire le secteur\n");
		return ERR_IO;
	}
}

/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (IN)
 * @return 0 on success; <0 on error
 */
int sector_write(FILE *f, uint32_t sector, void  *data){
	//Check if given pointers are non-null
	M_REQUIRE_NON_NULL(f);
	M_REQUIRE_NON_NULL(data);

	//Place the cursor on the right position in the sector	
	int err = fseek(f, SECTOR_SIZE * sector, SEEK_SET);
	if(err != 0){
		debug_print("Erreur: impossible de se mettre au bon endroit\n");
		return ERR_IO;
	}
	
	int written_bytes = fwrite(data, SECTOR_SIZE, 1, f);
	if(written_bytes == 1){
		return 0;
	}else{
		debug_print("Erreur: impossible d'écrire dans le secteur\n");
		return ERR_IO;
	}
}
