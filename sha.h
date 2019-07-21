#pragma once

/**
 * @file sha.h
 * @brief helpers for the project -- SHA-related part
 *
 * @author Aurélien Soccard
 * @date 11 Oct 2016
 */

#include "mount.h"
#include "unixv6fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief print the sha of the content
 * @param content the content of which we want to print the sha
 * @param length the length of the content
 */
void print_sha_from_content(const unsigned char *content, size_t length);

/**
 * @brief print the sha of the content of an inode
 * @param u the filesystem
 * @param inode the inocde of which we want to print the content
 * @param inr the inode number
 */
void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr);

#ifdef __cplusplus
}
#endif
