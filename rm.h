// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"

// Level location types
enum {
    HWALL = 1,
    VWALL,
    SDOOR,
    SCORR,
    LDOOR,
    POOL,	// not yet fully implemented, this should in fact be a bit like lit
    DOOR,
    CORR,
    ROOM,
    STAIRS
};

// Avoid using the level types in inequalities:
//  these types are subject to change.
// Instead, use one of the macros below.
#define	IS_WALL(typ)	((typ) <= VWALL)
#define IS_ROCK(typ)	((typ) < POOL)	// absolutely nonaccessible
#define	ACCESSIBLE(typ)	((typ) >= DOOR)	// good position
#define	IS_ROOM(typ)	((typ) >= ROOM)	// ROOM or STAIRS
#define	ZAP_POS(typ)	((typ) > DOOR)

// A few of the associated symbols are not hardwired.
enum {
    CORR_SYM	= '#',
    POOL_SYM	= '}',
    ERRCHAR	= '{'
};

// The structure describing a coordinate position.
// Before adding fields, remember that this will significantly affect
// the size of temporary files and save files.
struct rm {
    char scrsym;
    unsigned typ:5;
    unsigned new:1;
    unsigned seen:1;
    unsigned lit:1;
};
