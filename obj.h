// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"
#include "extern.h"

struct obj {
    struct obj*	nobj;
    unsigned	o_id;
    unsigned	o_cnt_id;	// id of container object is in
    unsigned	age;		// creation date
    int8_t	ox;
    int8_t	oy;
    int8_t	odx;
    int8_t	ody;
    uint8_t	otyp;
    uint8_t	owt;
    uint8_t	quan;		// use oextra for tmp gold objects
    int8_t	spe;		// quality of weapon, armor or ring (+/-), number of charges for wand (>= -1), special for amulet %% BAH
    char	olet;
    char	invlet;
    uint8_t	owornmask;
    uint8_t	oinvis:1;	// not yet implemented
    uint8_t	odispl:1;
    uint8_t	known:1;	// exact nature known
    uint8_t	dknown:1;	// color or text known
    uint8_t	cursed:1;
    uint8_t	unpaid:1;	// on some bill
    uint8_t	rustfree:1;
    unsigned	ogold;		// gold value
};

// obj.owornmask values
enum {
    W_ARM	= (1<<0),
    W_ARM2	= (1<<1),
    W_ARMH	= (1<<2),
    W_ARMS	= (1<<3),
    W_ARMG	= (1<<4),
    W_WEP	= (1<<5),
    W_RINGL	= (1<<6),	// make W_RINGL = RING_LEFT (see uprop)
    W_RINGR	= (1<<7),
    W_ARMOR	= W_ARM| W_ARM2| W_ARMH| W_ARMS| W_ARMG,
    W_RING	= W_RINGL| W_RINGR
};

static inline struct obj* newobj (void)
    { return alloc (sizeof(struct obj)); }
