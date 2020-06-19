// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"

// definition of a class of c_Objects
struct objclass {
    const char*	oc_name;	// actual name
    const char*	oc_descr;	// description when name unknown
    bool	oc_merge;	// merge otherwise equal objects
    char	oc_olet;
    int8_t	oc_prob;	// probability for mkobj()
    int8_t	oc_delay;	// delay when using such an object
    uint8_t	oc_weight;
    union {
	int8_t	oc_oc1;
	int8_t	a_ac;		// for armors - only used in ARM_BONUS
	int8_t	bits;		// for wands and rings
	int8_t	wldam;		// for weapons and PICK_AXE
    };
    union {
	int8_t	oc_oc2;
	int8_t	a_can;		// for armors
	int8_t	wsdam;		// for weapons and PICK_AXE
    };
    union {
	int	oc_oi;
	int	nutrition;	// for foods
	int	g_val;		// for gems: value on exit
    };
};

// Some values of oc_oi
enum {
    // wands
    NODIR	= (1<<0),
    IMMEDIATE	= (1<<1),
    RAY		= (1<<2),
    // rings
    SPEC	= 1	// +n is meaningful
};

extern const struct objclass c_Objects[];

// definitions of all object-symbols
enum {
    ILLOBJ_SYM	= '\\',
    AMULET_SYM	= '"',
    FOOD_SYM	= '%',
    WEAPON_SYM	= ')',
    TOOL_SYM	= '(',
    ROCK_SYM	= '`',
    ARMOR_SYM	= '[',
    POTION_SYM	= '!',
    SCROLL_SYM	= '?',
    WAND_SYM	= '/',
    RING_SYM	= '=',
    GEM_SYM	= '*'
};

#define ARM_BONUS(obj)	((10 - c_Objects[obj->otyp].a_ac) + obj->spe)

// Other places with explicit knowledge of object symbols:
// ....shk.c:   char shtypes[] = "=/)%?![";
// mklev.c:     "=/)%?![<>"
// hack.mkobj.c:        char mkobjstr[] = "))[[!!!!????%%%%/=**";
// hack.apply.c:   otmp = getobj("0#%", "put in");
// hack.eat.c:     otmp = getobj("%", "eat");
// hack.invent.c:          if(index("!%?[)=*(0/\"", sym)){
// hack.invent.c:    || index("%?!*",otmp->olet))){
