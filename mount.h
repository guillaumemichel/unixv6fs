#pragma once

/**
 * @file mount.h
 * @brief accessing the UNIX v6 filesystem -- core of the first set of assignments
 *
 * @author Edouard Bugnion
 * @date summer 2016
 */

#include <stdio.h>
#include "unixv6fs.h"
#include "bmblock.h"

#ifdef __cplusplus
extern "C" {
#endif

struct unix_filesystem {
    FILE *f;
    struct superblock s;           /* copy of the superblock */
    struct bmblock_array *fbm;     /* block bitmmap -- ignore before WEEK 10 */
    struct bmblock_array *ibm;     /* inode bitmap  -- ignore before WEEK 10 */
};

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u);

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u);

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesytem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u);

/*
 * staff only; students will not have to implement
 */
/**
 * @brief create a new filesystem
 * @param num_blocks the total number of blocks (= max size of disk), in sectors
 * @param num_inodes the total number of inodes
 */
int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes);

#ifdef __cplusplus
}
#endif
