// This file is free software, distributed under the BSD license.

#pragma once

enum {
    ALLOW_TRAPS	= 0777,
    ALLOW_U	= 01000,
    ALLOW_M	= 02000,
    ALLOW_TM	= 04000,
    ALLOW_ALL	= ALLOW_U| ALLOW_M| ALLOW_TM| ALLOW_TRAPS,
    ALLOW_SSM	= 010000,
    ALLOW_ROCK	= 020000,
    NOTONL	= 040000,
    NOGARLIC	= 0100000
};
