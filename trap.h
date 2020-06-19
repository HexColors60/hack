// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"

struct trap {
    struct trap *ntrap;
    int8_t tx, ty;
    uint8_t ttyp:5;
    uint8_t tseen:1;
    uint8_t once:1;
};

// various kinds of traps
enum {
    BEAR_TRAP,
    ARROW_TRAP,
    DART_TRAP,
    TRAPDOOR,
    TELEP_TRAP,
    PIT,
    SLP_GAS_TRAP,
    PIERC,
    TRAPNUM	// if not less than 32, change sizeof(ttyp)
};		// see also mtrapseen (bit map)

static inline struct trap* newtrap (void)
    { return alloc (sizeof(struct trap)); }
