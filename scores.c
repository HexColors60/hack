// Copyright (c) 2016 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT license.

#include "common.h"
#include <sys/file.h>
#include <sys/uio.h>

struct ScorefileHeader {
    char	magic [6];
    uint16_t	sum;
};

bool read_score_file (const char* filename, const char* magic, void* scores, size_t scoresSize)
{
    int fd = open (filename, O_RDONLY);
    if (fd < 0)
	return false;
    struct ScorefileHeader header;
    struct iovec iov[2] = {
	{ &header, sizeof(header)},
	{ scores, scoresSize}
    };
    while (0 != flock (fd, LOCK_SH))
	sleep (1);
    size_t br = readv (fd, iov, ArraySize(iov));
    close (fd);
    // Check that score list appears valid
    return br == sizeof(header)+scoresSize
	    && memcmp (header.magic, magic, sizeof(header.magic)) == 0
	    && header.sum == bsdsum (scores, scoresSize, 0);
}

void write_score_file (const char* filename, const char* magic, const void* scores, size_t scoresSize)
{
    int fd = open (filename, O_WRONLY);
    if (fd < 0)
	return;
    struct ScorefileHeader header;
    memcpy (header.magic, magic, sizeof(header.magic));
    header.sum = bsdsum (scores, scoresSize, 0);
    #pragma GCC diagnostic ignored "-Wcast-qual"
    const struct iovec iov[2] = {
	{ &header, sizeof(header)},
	{ (void*) scores, scoresSize}
    };
    while (0 != flock (fd, LOCK_EX))
	sleep (1);
    writev (fd, iov, ArraySize(iov));
    ftruncate (fd, sizeof(header)+scoresSize);
    close (fd);
}
