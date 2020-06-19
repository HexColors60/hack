// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"

enum { BILLSZ = 200 };

struct bill_x {
    unsigned bo_id;
    unsigned useup:1;
    unsigned bquan:7;
    unsigned price;		// price per unit
};

struct eshk {
    long robbed;		// amount stolen by most recent customer
    bool following;		// following customer since he owes us sth
    int8_t shoproom;		// index in _level->rooms; set by inshop()
    struct coord shk;		// usual position shopkeeper
    struct coord shd;		// position shop door
    int shoplevel;		// level of his shop
    int billct;
    struct bill_x bill[BILLSZ];
    int visitct;		// nr of visits by most recent customer
    char customer[PL_NSIZ];	// most recent customer
    char shknam[PL_NSIZ];
};
