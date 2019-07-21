/**
 * @file test-core.c
 * @brief main program to perform tests on disks (mainly for weeks 04 to 06)
 *
 * @author Aurélien Soccard & Jean-Cédric Chappelier
 * @date 15 Oct 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include "mount.h"
#include "error.h"

#define MIN_ARGS 1
#define MAX_ARGS 1
#define USAGE    "test <diskname>"

int test(struct unix_filesystem *u);

void error(const char* message)
{
    fputs(message, stderr);
    putc('\n', stderr);
    fputs("Usage: " USAGE, stderr);
    putc('\n', stderr);
    exit(1);
}

void check_args(int argc)
{
    if (argc < MIN_ARGS) {
        error("too few arguments:");
    }
    if (argc > MAX_ARGS) {
        error("too many arguments:");
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        mountv6_print_superblock(&u);
        error = test(&u);
    }
    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u); /* shall umount even if mount failed,
                   * for instance fopen could have succeeded
                   * in mount (thus fclose required).
                   */

    return error;
}
