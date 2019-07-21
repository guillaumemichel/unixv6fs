#include <stdio.h>
#include "unixv6fs.h"
#include "direntv6.h"
#include "filev6.h"
#include "mount.h"
#include "error.h"
#include "inode.h"
#include "sha.h"

#define CMD_NUMBER 13
#define CMD_MAX_CHARS 255

typedef int (*shell_fct)(const char** args);

struct shell_map {
	const char* name;    // nom de la commande
	shell_fct fct;      // fonction réalisant la commande
	const char* help;   // description de la commande
	size_t argc;        // nombre d'arguments de la commande 
	const char* args;   // description des arguments de la commande
};

int do_exit(const char** args);
int do_help(const char** args);
int do_mount(const char** args);
int do_lsall(const char** args);
int do_psb(const char** args);
int do_cat(const char** args);
int do_sha(const char** args);
int do_inode(const char** args);
int do_istat(const char** args);
int do_mkfs(const char** args);
int do_mkdir(const char** args);
int do_add(const char** args);

//the array with all commands
struct shell_map shell_cmds[CMD_NUMBER] = {
	{"help", do_help, "display this help", 0, ""},
	{"exit", do_exit, "exit shell", 0, ""},
	{"quit", do_exit, "exit shell", 0,  ""},
	{"mkfs", do_mkfs, "create a new filesystem", 3, "<diskname> <#inodes> <blocks>"},
	{"mount", do_mount, "mount the provided filesystem", 1, "<diskname>"},
	{"mkdir", do_mkdir, "create a new directory", 1, "<dirname>"},
	{"lsall", do_lsall, "list all directories and files contained in the currently mounted filesystem", 0, ""},
	{"add", do_add, "add a new file", 2, "<src-fullpath> <dst>"},
	{"cat", do_cat, "display the content of a file", 1, "<pathname>"},
	{"istat", do_istat, "display information about the provided inode", 1, "<inode_nr>"},
	{"inode", do_inode, "display the inode number of a file", 1, "<pathname>"},
	{"sha", do_sha, "display the SHA of a file", 1, "<pathname>"},
	{"psb", do_psb, "Print SuperBlock of the currently mounted filesystem", 0, ""}
};

//Global variable, to hold the current unix filesystem
struct unix_filesystem u;

/**
 * @brief execute the "exit" function of the shell
 * @param args not needed for this function
 * @return 0
 */
int do_exit(const char** args){
	if(u.f != NULL) return umountv6(&u);
	return 0;
}

/**
 * @brief execute the "help" function of the shell, basically print info about the possible commands
 * @param args not needed for this function
 * @return 0
 */
int do_help(const char** args){
	for(int i = 0; i < CMD_NUMBER; ++i)
		printf("- %s %s: %s.\n", shell_cmds[i].name, shell_cmds[i].args, shell_cmds[i].help);
		
	return 0;
}

/**
 * @brief execute the "mount" function of the shell, basically mount a unix filesystem, given its name
 * @param args an array containing the name of the file to mount the unix filesystem
 * @return 0 on success; <0 otherwise
 */
int do_mount(const char** args){
	if(u.f != NULL){
		int err = umountv6(&u);
		if(err < 0) return err;
	}
	return mountv6(args[0], &u);
}

/**
 * @brief execute the "lsall" function of the shell, basically print the content of a unix filesystem
 * @param args not needed for this function
 * @return 0 on success; < 0 otherwise
 */
int do_lsall(const char** args){
	return direntv6_print_tree(&u, ROOT_INUMBER, "");
}

/**
 * @brief execute the "psb" function of the shell, basically print the superblock of a unix filesystem
 * @param args not needed for this function
 * @return 0
 */
int do_psb(const char** args){
	mountv6_print_superblock(&u);
	return 0;
}

/**
 * @brief execute the "istat" function of the shell, basically print info about an inode, given its number in a unix filesystem
 * @param args an array containing the number of the inode of a unix filesystem
 * @return 0 on success; < 0 otherwise
 */
int do_istat(const char** args){
	int given = atoi(args[0]);
	if(given < 0){
		return ERR_INODE_OUTOF_RANGE;
	}
	
	struct inode i[sizeof(struct inode)];
	int err = inode_read(&u, (uint16_t)given, i);
	if(err != 0) return err;
	
	inode_print(i);
	return 0;
}

/**
 * @brief execute the "inode" function of the shell, basically print an inode number associated to the given filename of a unix filesystem
 * @param args an array containing the name of the file/directory of a unix filesystem
 * @return the number of the inode if it exists; < 0 otherwise
 */
int do_inode(const char** args){
	int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]);
	if(inr >= 0){
		printf("inode: %d\n", inr);
		return 0;
	}else{
		return inr;
	}
}

/**
 * @brief execute the "cat" function of the shell, basically print the content of a file given its name
 * @param args an array containing the name of the file of a unix filesystem
 * @return 0 on success; < 0 otherwise
 */
int do_cat(const char** args){
	int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]);
	if(inr < 0) return inr;

	struct filev6 fv6;
	int err = filev6_open(&u, (uint16_t)inr, &fv6);
	if(err < 0) return err;
	
	if(fv6.i_node.i_mode & IFDIR){
			printf("ERROR SHELL: cat on a directory is not defined\n");
	}else{
		if (!(fv6.i_node.i_mode & IALLOC)) return ERR_UNALLOCATED_INODE;
		int size = inode_getsectorsize(&fv6.i_node);
		char p[size];
		strcpy(p, "");
		char tab[SECTOR_SIZE];
		int readBytes;
		while((readBytes=filev6_readblock(&fv6, tab)) > 0){
			strncat(p, tab, SECTOR_SIZE);
		}
		if (readBytes<0) return readBytes;
		printf("%s\n", p);
	}
	return 0;
}

/**
 * @brief execute the "sha" function of the shell, basically print the SHA of a file given its name
 * @param args an array containing the name of the file of a unix filesystem
 * @return 0 on success; < 0 otherwise
 */
int do_sha(const char** args){
	int inr = direntv6_dirlookup(&u, ROOT_INUMBER, args[0]);
	if(inr >= ROOT_INUMBER){
		struct inode inode;
		int err = inode_read(&u, (uint16_t)inr, &inode);
		if(err < 0) return err;
		print_sha_inode(&u, inode, inr);
		return 0;
	}else{
		return inr;
	}
}

/**
 * @brief create a new filesystem
 * @param args an array containing the name of the filesystem, the number of inodes and the number of blocks of this system
 * @return 0 on success; <0 on error
 */
int do_mkfs(const char** args){
	const char* filename = args[0];
	const uint16_t num_inodes = atoi(args[1]);
	const uint16_t num_blocks = atoi(args[2]);
	return mountv6_mkfs(filename, num_blocks, num_inodes);
}

/**
 * @brief create a new directory
 * @param args an array containing the name this new directory with the path to it
 * @return 0 on success; <0 on error
 */
int do_mkdir(const char** args){ 
	const char* dirname = args[0];
	return direntv6_create(&u, dirname, IFDIR);
}

/**
 * @brief add a new file to the filesystem
 * @param args an array containing the path to the source file and the path to the dest file on the filesystem
 * @return 0 on success; <0 on error
 */
int do_add(const char** args){
	const char* src = args[0];
	const char* dst = args[1];
	
	int inr = direntv6_create(&u, dst, 0);
	if (inr < 0) return inr;
	
	FILE* f = fopen(src,"rb");
	if (f == NULL) return ERR_IO;
	
	fseek(f, 0L, SEEK_END);
	int sz = (int)ftell(f);
	uint8_t buffer[sz];
	rewind(f);
	fread(buffer, sz, sizeof(uint8_t), f);
	
	struct filev6 fv6;
	int err = filev6_open(&u, (uint16_t)inr, &fv6);
	if(err < 0) return err;
	err = filev6_writebytes(&u, &fv6, buffer, sz);
	if(err < 0) return err;
	
	if(fclose(f) != 0) return ERR_IO;
	
	return 0;
}

/**
 * @brief given a command name, and an array consisting of a command name and arguments, return the corresponding function
 * @param cmd the name of the command to execute
 * @param args the array hosting a command name and arguments to this command
 * @return a pointer to the correct function; NULL if any errors
 */
shell_fct get_func(const char* cmd, const char** args){
	unsigned int size = 0;
	while(args[size] != NULL){
		++size;
	}
	--size;//Value of size corresponds to the number of arguments
	
	int i = 0;
	while(i < CMD_NUMBER && strcmp(cmd, shell_cmds[i].name) != 0){
		++i;
	}
	
	if(i == CMD_NUMBER){
		printf("ERROR SHELL: invalid command\n");
	}else if(shell_cmds[i].argc != size){
		printf("ERROR SHELL: wrong number of arguments\n");
	}else if((i > 5 && i < 13) && u.f == NULL){
		printf("ERROR SHELL: mount the FS before the operation\n");
	}else{
		return shell_cmds[i].fct;
	}
	return NULL;
}


/**
 * @brief parse a given string to an array of words
 * @param c - the string to parse
 * @param args - the array that will contain the parsed words
 */
void tokenize_input(char* c, char** args){
	const char space[2] = " ";
	char* token = strtok(c, space);
	
	if (!token) args[0]="\0";
	else {
		for (int count = 0; token != NULL; ++count){
			if (strlen(token) > 0) args[count] = token;
			token = strtok(NULL, space);
		}
	}
}

/**
 * @brief run the custom shell
 * @return 0
 */
int main(void){
	char input[CMD_MAX_CHARS];
	char* cmd_args[CMD_MAX_CHARS*sizeof(char*)];
	shell_fct fct;
	int err;
	
	while(!feof(stdin) && !ferror(stdin) && fct != do_exit){
		printf(">>> ");
		
		//Get input of the user
		fgets(input, CMD_MAX_CHARS, stdin);
		input[strlen(input)-1] = '\0';
		
		//Tokenize the input of the user
		tokenize_input(input, cmd_args);
		
		if (cmd_args[0][0]!='\0'){
			//Get the corresponding function if user input is ok
			fct = get_func(input, (const char**)cmd_args);
			
			//If everything is valid, execute the chosen function
			if(fct != NULL)
				err = fct((const char**)&cmd_args[1]);
				
			//If any errror occurs while executing the function, return it
			if (err < 0) printf("ERROR FS: %s\n", ERR_MESSAGES[err - ERR_FIRST]);
		}
		
		//Clean the args array between two commands
		memset(cmd_args, 0, CMD_MAX_CHARS);
	}
	return 0;
}
