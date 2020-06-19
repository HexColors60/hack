// This file is free software, distributed under the BSD license.

#pragma once

struct mkroom {
    int8_t lx, hx, ly, hy;	// usually int8_t, but hx may be -1
    int8_t rtype, rlit, doorct, fdoor;
};

enum {
    MAXNROFROOMS = 15,
    DOORMAX = 100
};

// various values of rtype
// 0: ordinary room; 8-15: various shops
// Note: some code assumes that >= 8 means shop, so be careful when adding
// new roomtypes
enum {
    SWAMP = 3,
    VAULT,
    BEEHIVE,
    MORGUE,
    ZOO,
    SHOPBASE,
    WANDSHOP,
    GENERAL = 15
};
