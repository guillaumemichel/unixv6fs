#pragma once

/**
 * @file unixv6fs.h
 * @brief This is the header file from Unix Version 6 that describes the superblock of
 * the file system.
 *
 * This file is a revision from Edouard Bugnion of a file created by
 * Mendel Rosenblum (Stanford), who derived it himself from the actual
 * UNIX v6 source code.
 *
 * It has been converted to use stdint.h to work on 32- and 64-bit systems.
 * Free blocks and inodes are handled as bitmasks rather than linked
 * lists (like modern filesystems do).
 *
 * It specifies the ondisk layout of the superblock, the inode, and
 * the directories
 *
 * @author original UNIX v6 team + Mendel Rosenblum (Stanford) + Edouard Bugnion
 */

/*
 * The disk is organized in 512-byte size units called sectors.
 *
 * Each disk can have one filesystem (i.e. there are no partitions);
 * the disk is split into non-overlapping regions. The exact start and
 * size of each region is specified in the superblock.
 *
 * Region                | BEGIN          | END
 * --------------------- | -------------- | -----------------------
 * Boot sector           | 0              | 0
 * Superblock sector     | 1              | 1
 * Data block bitmap     | s_fbm_start    | s_fbm_start+s_fbm_size-1
 * inode bitmap          | s_ibm_start    | s_ibm_start+s_ibm_size-1
 * inodes                | s_inode_start  | s_inode_start+s_isize-1
 * data sectors          | s_block_start  | s_fsize
 * -----------------------------------------------------------------
 */

/*
                                              FILESYSTEM
                                     Small boxes represent inodes.
                            (this is best viewed using 96 or more columns)
+-----------------------------------------------------------------------------------------------+
|                                                                                               |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | 7 |               |  |s_fbm_start s_fsize|  |                   |  |                   |  |
|   +---+               |  |i_fbm_start s_isize|  |                   |  |                   |  |
|   |                   |  |s_inode_start      |  |                   |  |                   |  |
|   |                   |  |s_block_start      |  |                   |  |                   |  |
|   |                   |  |s_fbmsize          |  |                   |  |                   |  |
|   |                   |  |s_ibmsize          |  |                   |  |                   |  |
|   |                   |  |                   |  |                   |  |                   |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|         BOOT SECTOR           SUPERBLOCK               BLOCK 2                 BLOCK 3        |
|                                                                                               |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|                                                                                               |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|    BLOCK s_fbm_start      BLOCK s_fbm_start+1    BLOCK s_fbm_start+2    BLOCK s_fbm_start+3   |
|                                                                                               |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|                                                                                               |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|    BLOCK i_fbm_start      BLOCK i_fbm_start+1    BLOCK i_fbm_start+2    BLOCK i_fbm_start+3   |
|                                                                                               |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|                                                                                               |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | 00 | 01 | 02 | 03 |  | 16 |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | 04 | 05 | 06 | 07 |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | 08 | 09 | 10 | 11 |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | 12 | 13 | 14 | 15 |  |    |    |    | 31 |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   BLOCK s_inode_start    BLOCK s_inode_start+1  BLOCK s_inode_start+2 BLOCK s_inode_start+3   |
|                                                                                               |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|                                                                                               |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   | DD |    |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    | DD |    |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    | DD |    |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   |    |    |    | DD |  |    |    |    |    |  |    |    |    |    |  |    |    |    |    |  |
|   +-------------------+  +-------------------+  +-------------------+  +-------------------+  |
|   BLOCK s_block_start    BLOCK s_block_start+1  BLOCK s_block_start+2  BLOCK s_block_start+3  |
|                                                                                               |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|              .                     .                      .                      .            |
|                                                                                               |
+-----------------------------------------------------------------------------------------------+
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHORT_DIR_NAME "DIR"
#define SHORT_FIL_NAME "FIL"
#define PATH_TOKEN '/'
#define ROOTDIR_NAME "/"

// Max. number of sector locations held in an inode
#define ADDR_SMALL_LENGTH 8

#define SECTOR_SIZE 512 /* bytes */

#define BOOTBLOCK_SECTOR   0
#define SUPERBLOCK_SECTOR  1

#define ADDRESS_SIZE 2 /* bytes */
#define ADDRESSES_PER_SECTOR (SECTOR_SIZE / ADDRESS_SIZE)

/*
 * Definition of the boot block
 *   On a real bootable device, this contains bootstrap code.
 *   Here, the only thing is the first byte, which must be the "magic" number
 */

#define BOOTBLOCK_MAGIC_NUM_OFFSET 0
#define BOOTBLOCK_MAGIC_NUM ((uint8_t)0407)

/*
 * Definition of the unix super block.
 * 1 sector in size (not all entries are used)
 */
struct superblock {

    uint16_t	s_isize;    	/* size in sectors of the inodes */
    uint16_t	s_fsize;	    /* size in sectors of entire volume */
    uint16_t    s_fbmsize;      /* size in sectors of the freelist bitmap */
    uint16_t    s_ibmsize;      /* size in sectors of the inode bitmap */
    uint16_t    s_inode_start;  /* first sector with inodes */
    uint16_t    s_block_start;  /* first sector with data */
    uint16_t    s_fbm_start;    /* first sector with the freebitmap (==2) */
    uint16_t    s_ibm_start;    /* first sector with the inode bitmap */

    uint8_t	    s_flock;	    /* lock during free list manipulation */
    uint8_t	    s_ilock;	    /* lock during I list manipulation */
    uint8_t	    s_fmod;		    /* super block modified flag */
    uint8_t	    s_ronly;	    /* mounted read-only flag */
    uint16_t	s_time[2];	    /* current date of last update */
    uint16_t	pad[244];       /* unused entries:
                                 * padding to ensure sizeof(superblock) == SECTOR_SIZE */
};

/*
 * Definition of the on-disk inode.
 * 32 bytes in size-
 * The original Unix 6 inode assumed 16-bit integers;
 * the structure is here adapted for 32-bit and 64-bit compilers.
 * The maximum file size is 16MB (24 bits).
 */
struct inode {
	uint16_t i_mode;
	uint8_t i_nlink;
	uint8_t i_uid;
	uint8_t i_gid;
	uint8_t i_size0;
	uint16_t i_size1;
	uint16_t i_addr[ADDR_SMALL_LENGTH];
	uint16_t i_atime[2];
	uint16_t i_mtime[2];
};

#define INODES_PER_SECTOR (SECTOR_SIZE / sizeof(struct inode))

/* Modes */
#define	IALLOC	0100000		/* file is used */
#define	IFMT	060000		/* type of file */
#define	IFDIR	040000   	/* directory */
#define	IFCHR	020000	    /* character special */
#define	IFBLK	060000	    /* block special, 0 is regular */
#define	ILARG	010000		/* large addressing algorithm */
#define	ISUID	04000		/* set user  id on execution */
#define	ISGID	02000		/* set group id on execution */
#define ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read    permission */
#define	IWRITE	0200        /* write   permission */
#define	IEXEC	0100        /* execute permission */

/*
 * The root directory (/) is at inode 1; inode 0 is never used.
 */

#define ROOT_INUMBER    1

/*
 * Directory layer:
 *   Each filename is up to 14 characters.
 *   It is NOT null-terminated for names that are exactly
 *   14 characters long.
 */

#define DIRENT_MAXLEN 14

struct direntv6 {
    uint16_t d_inumber;
    char   d_name[DIRENT_MAXLEN];     /* NOT null terminated when
                                       * length(filename) == DIRENT_MAXLEN  */
};

#define DIRENTRIES_PER_SECTOR ((int)(SECTOR_SIZE/sizeof(struct direntv6)))

/*
 * Static checks below -- always very useful:
 * this code will only compile if BUILD_BUG_ON condition expands to
 * zero each time.
 */

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

static inline void unixfs_static_checks(void)
{
    BUILD_BUG_ON(sizeof(int) < 4); // if ints have less than 32 bits, we're quite in trouble
    BUILD_BUG_ON(sizeof(struct superblock) != SECTOR_SIZE);
    BUILD_BUG_ON(sizeof(struct inode) != 32);
    BUILD_BUG_ON(sizeof(struct inode) * INODES_PER_SECTOR != SECTOR_SIZE);
    BUILD_BUG_ON(sizeof(struct direntv6) * DIRENTRIES_PER_SECTOR != SECTOR_SIZE);
    BUILD_BUG_ON(sizeof(1ULL) != 8); // in bmblock.c we make us of 1ULL has 64 bits
}

#ifdef __cplusplus
}
#endif
