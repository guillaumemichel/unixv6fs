#include <stdio.h>
#include <string.h>
#include "direntv6.h"
#include "inode.h"
#include "error.h"

#define MAXPATHLEN_UV6 1024

/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(d);
	
	//Check if file is mounted
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO; 
	}
	
	//Create filev6 to be filled
	struct filev6 fv6;
	
	int err = filev6_open(u, inr, &fv6);
	if(err != 0) return err;
	//Set values for the directory reader
	d->fv6 = fv6;
	d->curr = 0;
	d->last = 0;
	
	//Check if inode is dir
	if((fv6.i_node.i_mode & IFMT) != IFDIR) return ERR_INVALID_DIRECTORY_INODE;
	
	return 0;
}

/**
 * @brief check if a directory reader is non-empty
 * @param d the directory reader
 * @return 0 is the directory ready is empty; 1 otherwise
 */
int direntv6_nonempty(struct directory_reader *d) {
  return inode_getsize(&(d->fv6.i_node)) ? 1 : 0;
}

/**
 * @brief return the next directory entry.
 * @param d the directory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes. 
          Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success; 0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr){
	M_REQUIRE_NON_NULL(d);
	M_REQUIRE_NON_NULL(d->fv6.u);
	M_REQUIRE_NON_NULL(name);
	M_REQUIRE_NON_NULL(child_inr);
	
	//if cursor is at the beginning of a sector, read it
	if (d->curr==0){
		struct direntv6 buf[DIRENTRIES_PER_SECTOR];
		int readBytes = filev6_readblock(&d->fv6,buf);
		if (readBytes<0){
			return readBytes;
		}
		memcpy(&d->dirs,buf,readBytes);
		d->last = readBytes/sizeof(struct direntv6);
		if (readBytes == 0)
		  debug_print("Error: empty dir!\n");
		//place the number of the last child of the sector in d->last
	}
	
	if (d->curr > DIRENTRIES_PER_SECTOR) return ERR_BAD_PARAMETER;
	//write the inode number of the next file to read
	*child_inr = d->dirs[d->curr].d_inumber; 
	
	//read the name and add the '\0' at the end
	//(if the name has DIRENT_MAXLEN chars there is not '\0' at the end of the name)
	int l;
	char c;
	for (l=0;l<DIRENT_MAXLEN && (c=d->dirs[d->curr].d_name[l])!='\0';++l){
		name[l]=c;
	}
	name[l]='\0';
	++d->curr;
	if (d->curr==d->last){
		//if last element is the last of the sector, read next sector
		if (d->last==DIRENTRIES_PER_SECTOR) d->curr = 0;
		else return 0;
	}
	return 1;
}

/**
 * @brief debugging routine; print a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(prefix);
	
	//Check if file is mounted
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	struct directory_reader d;
		
	//open the directory
	int is_dir = direntv6_opendir(u, inr, &d);
	//if is_dir==ERR_INVALID_DIRECTORY_INODE then the dirent is a FIL -> not an error
	if (is_dir != 0 && is_dir != ERR_INVALID_DIRECTORY_INODE) return is_dir;
	
	char next_name[DIRENT_MAXLEN+1];

	if(is_dir == 0){
		int r=direntv6_nonempty(&d);
		uint16_t child_inr;
		char to_print[MAXPATHLEN_UV6+1];
		printf("%s %s%c\n", SHORT_DIR_NAME, prefix, PATH_TOKEN);
		while(r){
			r=direntv6_readdir(&d, next_name, &child_inr);
			if (r<0) return r;
			//create the to_print by concatenating the prefix, the PATH_TOKEN and the next_name
			snprintf(to_print, MAXPATHLEN_UV6+1, "%s%c%s", prefix, PATH_TOKEN, next_name);
			//call recursively print_tree with all children of the node
			direntv6_print_tree(u, child_inr, to_print);
		}
	}else{
		//the dirent is a FIL, simply print it
		strncpy(next_name, prefix, strlen(prefix)+1);
		printf("%s %s\n", SHORT_FIL_NAME, next_name);
	}
	return 0;
}

/**
 * @brief utility function for direntv6_dirlookup, used to handle recursion easily by also passing the size of the entry
 * @param u a mounted filesystem
 * @param inr the current of the subtree
 * @param entry the prefix to the subtree
 * @param size the size of the string entry
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, const char *entry, size_t size){
	int i = 0;
	while(entry[i] == PATH_TOKEN){
		 ++i;
		 --size;
	 }//first, get rid of leading '/'
		
	if(size == 0){
		return inr;
	}
	const char* clean_entry = &entry[i];		
	char* ret = strchr(clean_entry, PATH_TOKEN);//ret points on the first '/' in the "cleaned" entry
	
	char next_entry[MAXPATHLEN_UV6+1];
	
	if(ret != NULL){
		strncpy(next_entry, ret, MAXPATHLEN_UV6+1);
		ret[0] = '\0';//now, "clean_entry" just contain the name of the next thing to search
	}else{
		strcpy(next_entry, "");
	}
	
	struct directory_reader d;
	char name[DIRENT_MAXLEN+1];
	int err = direntv6_opendir(u, inr, &d);//Open the directory
	if (err<0) return err;
	
	int success;
	do{
		success = direntv6_readdir(&d, name, &d.fv6.i_number);
	}while(strcmp(name, clean_entry) != 0 && success == 1);//We check if the next file/dir in the entry exist
	if (success<0) return success;
	
	if(strcmp(name, clean_entry) != 0)
		return ERR_INODE_OUTOF_RANGE;//file with this name doesn't exist
	else{
		return direntv6_dirlookup_core(u, d.fv6.i_number, next_entry, strlen(next_entry));//we found, the next file/dir, recurse !
	}	
}
/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the current of the subtree
 * @param entry the prefix to the subtree
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(entry);
	
	//Check if file is mounted
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	
	return direntv6_dirlookup_core(u, inr, entry, strlen(entry));
}

/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(entry);
	if (u->f ==  NULL) {
		debug_print("File system not mounted");
		return ERR_IO;
	}
	unsigned int l = strlen(entry);
	//if the given name doesn't begin with a '/' add it at the beginning of realPath
	short unsigned int startByToken=1;
	if (entry[0]!=PATH_TOKEN){
		++l;
		startByToken=0;
	}
	unsigned int lastToken = 0;
	short unsigned int isToken = 0;
	char realPath[l + 1];
	unsigned int size = 0;
	if (!startByToken){
		realPath[0]=PATH_TOKEN;
		++size;
	}
	//copy the entry ignoring multiple '/' in realPath
	for (unsigned int i = 0; i < l && size < MAXPATHLEN_UV6; ++i){
		if (!isToken || entry[i] != PATH_TOKEN){
			if (entry[i] != PATH_TOKEN) isToken = 0;
			else if (entry[i] == PATH_TOKEN){
				isToken = 1;
				lastToken = size;
			}
			realPath[size] = entry[i];
			size++;
		}
	}
	realPath[size] = '\0';
	int nameSize = size - lastToken;
	if (size-1==lastToken){//the name ends with a '/'
		return ERR_BAD_PARAMETER;
	}
	
	if (nameSize - 1 > DIRENT_MAXLEN || size > MAXPATHLEN_UV6){//name or path too long
		return ERR_FILENAME_TOO_LONG;
	}
	
	//getting the parent name
	char parent[lastToken+1];
	strncpy(parent, realPath, lastToken+1);
	parent[lastToken]='\0';
	
	//getting the file name
	char name[DIRENT_MAXLEN];
	strncpy(name, realPath + lastToken + 1, DIRENT_MAXLEN);
	
	//tests if a file with the same name already exists
	if(direntv6_dirlookup(u, ROOT_INUMBER, realPath) >= 0)
		return ERR_FILENAME_ALREADY_EXISTS;
	
	//check if the parent directory exists
	int parent_inr;
	if ((parent_inr = direntv6_dirlookup(u, ROOT_INUMBER, parent)) < 0){
		return ERR_BAD_PARAMETER;
	}
	
	int inr = inode_alloc(u);
	if (inr<0) return inr;
	
	int err;
	
	//We use filev6_create for the inode write
	struct filev6 fv6;
	fv6.i_number = (uint16_t)inr;
	err = filev6_create(u, mode, &fv6);
	if(err < 0) return err;
	
	//open the fv6 of the parent
	struct filev6 fv6_parent;
	err = filev6_open(u, (uint16_t)parent_inr, &fv6_parent);
	if(err < 0) return err;
	
	nameSize = nameSize > DIRENT_MAXLEN ? DIRENT_MAXLEN : nameSize;
	
	//create the corresponding direntv6
	struct direntv6 d;
	d.d_inumber = (uint16_t)inr;
	strncpy(d.d_name, name, nameSize);
	
	//write the direntv6 to the parent
	err = filev6_writebytes(u, &fv6_parent, &d, (int)sizeof(struct direntv6));
	if(err < 0) return err;
	return inr;
}
