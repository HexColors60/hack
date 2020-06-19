// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"
#include "extern.h"

enum { MTSZ = 4 };		// monst.mtrack size

struct monst {
    struct monst*	nmon;
    const struct permonst* data;
    struct obj*		minvent;
    unsigned		m_id;
    struct coord	mtrack [MTSZ];	// monster track
    int8_t		mx;
    int8_t		my;
    int8_t		mdx;
    int8_t		mdy;		// if mdispl then pos where last displayed
    int8_t		mhp;
    int8_t		mhpmax;
    char		mappearance;	// nonzero for undetected 'M's and for '1's
    uint8_t		mspeed:2;
    uint8_t		mdispl:1;	// mdx,mdy valid
    uint8_t		minvis:1;	// invisible
    uint8_t		mhide:1;	// hides beneath objects
    uint8_t		mundetected:1;	// not seen in present hiding place
    uint8_t		msleep:1;
    uint8_t		mfroz:1;
    uint8_t		mcan:1;		// has been cancelled
    uint8_t		mconf:1;
    uint8_t		mpeaceful:1;	// does not attack unprovoked
    uint8_t		isshk:1;	// is shopkeeper
    uint8_t		mcansee:1;	// cansee 1, temp.blinded 0, blind 0
    uint8_t		mtrapped:1;	// trapped in a pit or bear trap
    uint8_t		isgd:1;		// is guard
    uint8_t		mflee:1;	// fleeing
    uint8_t		mfleetim;	// timeout for mflee
    uint8_t		mblinded;	// cansee 0, temp.blinded n, blind 0
    uint8_t		mnamelth;	// length of name (following mxlth)
    unsigned		mtrapseen;	// bitmap of traps we've been trapped in
    unsigned		mlstmv;		// prevent two moves at once
    unsigned		mgold;
    unsigned		mxlth;		// length of following data
    // in order to prevent alignment problems mextra should
    // be (or follow) a long int
    long		mextra[1];	// monster dependent info
};

static inline struct monst* newmonst (size_t xl)
    { return alloc (sizeof(struct monst) + xl); }

extern struct monst *fallen_down;

// these are in mspeed
enum {
    MSLOW = 1,	// slow monster
    MFAST	// speeded monster
};

#define	NAME(mtmp)	(((char *) mtmp->mextra) + mtmp->mxlth)
#define	MREGEN		"TVi1"
#define	UNDEAD		"ZVW "
