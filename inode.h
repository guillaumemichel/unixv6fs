#pragma once

/**
 * @file filev6.h
 * @brief accessing the UNIX v6 filesystem -- core of the first set of assignments
 *
 * @author Edouard Bugnion
 * @date summer 2016
 */

#include "unixv6fs.h"
#include "mount.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the size of a file associated to a given inode.
 *
 *        To save space, the UNIX v6 filesystem stores the maximal
 *        file size (which is 24 bits) in two fields or 8 bits and 16
 *        bits each.  This saves a byte in the inode.
 *        This function helps conver the file size into a more meaningful number.
 *
 * @param inode the inode
 * @return the size of the file
 */
static inline int32_t inode_getsize(const struct inode *inode)
{
    return ((inode->i_size0 << 16) | inode->i_size1);
}

/**
 * @brief Return the size of a given inode rounded up to SECTOR_SIZE and
 *        plus 1 for null-terminating it; i.e. :
 *           if inode_getsize() is ... --> returns ...
 *                                   0 --> 1
 *                                   1 --> SECTOR_SIZE + 1
 *                     SECTOR_SIZE - 1 --> SECTOR_SIZE + 1
 *                         SECTOR_SIZE --> SECTOR_SIZE + 1
 *                     SECTOR_SIZE + 1 --> 2 * SECTOR_SIZE + 1
 *                                     etc,
 *
 * @param inode the inode
 * @return the size to store sector-read data plus one extra null char.
 */
static inline int32_t inode_getsectorsize(const struct inode *inode)
{
    const int32_t i_size = inode_getsize(inode);
    return (i_size ? ((i_size - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE + 1 : 1);
}

/**
 * @brief set the size of a given inode to the given size
 * @param inode the inode
 * @param new_size the new size
 * @return 0 on success; <0 on error
 */
int inode_setsize(struct inode *inode, int new_size);

/**
 * @brief prints the content of an inode structure
 * @param inode the inode structure to be displayed
 */
void inode_print(const struct inode *inode);

/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u);

/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode);

/**
 * @brief identify the sector that corresponds to a given portion of a file
 * @param u the filesystem (IN)
 * @param inode the inode (IN)
 * @param file_sec_off the offset within the file (in sector-size units)
 * @return >0: the sector on disk; <0 error
 */
int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off);

/**
 * @brief alloc a new inode (returns its inr if possible)
 * @param u the filesystem (IN)
 * @return the inode number of the new inode or error code on error
 */
int inode_alloc(struct unix_filesystem *u);

/**
 * @brief write the content of an inode to disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (IN)
 * @return 0 on success; <0 on error
 */
int inode_write(struct unix_filesystem *u, uint16_t inr, struct inode *inode);

#ifdef __cplusplus
}
#endif
