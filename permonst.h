// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"

struct permonst {
    const char* mname;
    char	mlet;
    uint8_t	mlevel;
    uint8_t	mmove;
    int8_t	ac;
    uint8_t	damn;
    uint8_t	damd;
    uint16_t	pxlth;
};

#define PM_ACID_BLOB	&c_Monsters[7]
#define	PM_ZOMBIE	&c_Monsters[13]
#define	PM_PIERCER	&c_Monsters[17]
#define	PM_KILLER_BEE	&c_Monsters[26]
#define	PM_WRAITH	&c_Monsters[33]
#define	PM_VAMPIRE	&c_Monsters[42]
#define	PM_DEMON	&c_Monsters[52]
enum { CMNUM = 53 };			// number of common monsters
#define	PM_MINOTAUR	&c_Monsters[CMNUM]	// last in mons array
#define	PM_SHK		&c_Monsters[CMNUM+1]	// very last
#define	PM_GHOST	&pm_ghost
#define	PM_EEL		&pm_eel
#define	PM_WIZARD	&pm_wizard

extern const struct permonst c_Monsters [CMNUM+2];
