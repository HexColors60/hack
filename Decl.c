// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "mkroom.h"

char plname [PL_NSIZ];	// player name

bool in_mklev;

struct level _levels [MAXLEVEL] = { LEVEL_INIT };
struct level* _level = NULL;

struct obj* fcobj = NULL;
struct obj* invent = NULL;
struct obj* uwep = NULL;
struct obj* uarm = NULL;
struct obj* uarm2 = NULL;
struct obj* uarmh = NULL;
struct obj* uarms = NULL;
struct obj* uarmg = NULL;
struct obj* uright = NULL;
struct obj* uleft = NULL;
struct worldflag _wflags = {};
struct you _u = {};
struct monst youmonst;		// dummy; used as return value for boomhit

char* save_cm = NULL;
const char* killer = NULL;
const char* nomovemsg = NULL;

long wailmsg = 0;

int multi = 0;

int8_t curx = 0, cury = 0;
int8_t seelx = 0, seehx = 0, seely = 0, seehy = 0;	// corners of lit room

struct coord bhitpos = {0,0};

const char quitchars[] = " \r\n\033";
