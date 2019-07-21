/**
 * @file error.c
 * @brief filesystem error messages
 */

const char * const ERR_MESSAGES[] = {
    "", // no error
    "Not enough memory",
    "IO error",
    "bad boot sector",
    "inode out of range",
    "filename too long",
    "invalid directory inode",
    "unallocated inode",
    "filename already exists",
    "bitmap full",
    "file too large",
    "offset out of range",
    "bad parameter",
    "not enough sectors for inodes"
};
