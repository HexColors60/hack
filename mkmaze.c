// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

//----------------------------------------------------------------------

static const struct permonst hell_hound = { "hell hound", 'd', 12, 14, 2, 3, 6, 0 };
static void movept (int *x, int *y, int dir);

//----------------------------------------------------------------------

void makemaz(void)
{
    int x, y;
    int zx, zy;
    struct coord mm;
    bool al = (_u.dlevel >= 30 && !_wflags.made_amulet);

    for (x = 2; x < COLNO - 1; ++x)
	for (y = 2; y < ROWNO - 1; ++y)
	    _level->l[x][y].typ = (x % 2 && y % 2) ? 0 : HWALL;
    if (al) {
	struct monst *mtmp;

	zx = 2 * (COLNO / 4) - 1;
	zy = 2 * (ROWNO / 4) - 1;
	for (x = zx - 2; x < zx + 4; ++x)
	    for (y = zy - 2; y <= zy + 2; ++y) {
		_level->l[x][y].typ = (y == zy - 2 || y == zy + 2 || x == zx - 2 || x == zx + 3) ? POOL : (y == zy - 1 || y == zy + 1 || x == zx - 1 || x == zx + 2) ? HWALL : ROOM;
	    }
	mkobj_at(AMULET_SYM, zx, zy);
	_wflags.made_amulet = 1;
	walkfrom(zx + 4, zy);
	if ((mtmp = makemon(&hell_hound, zx, zy)) != NULL)
	    mtmp->msleep = 1;
	if ((mtmp = makemon(PM_WIZARD, zx + 1, zy)) != NULL) {
	    mtmp->msleep = 1;
	    _wflags.no_of_wizards = 1;
	}
    } else {
	mm = mazexy();
	zx = mm.x;
	zy = mm.y;
	walkfrom(zx, zy);
	mksobj_at(WAN_WISHING, zx, zy);
	mkobj_at(ROCK_SYM, zx, zy);	// put a rock on top of
						// it
    }

    for (x = 2; x < COLNO - 1; ++x)
	for (y = 2; y < ROWNO - 1; ++y) {
	    switch (_level->l[x][y].typ) {
		case HWALL:
		    _level->l[x][y].scrsym = '-';
		    break;
		case ROOM:
		    _level->l[x][y].scrsym = '.';
		    break;
	    }
	}
    for (x = rn1(8, 11); x; --x) {
	mm = mazexy();
	mkobj_at(rn2(2) ? GEM_SYM : 0, mm.x, mm.y);
    }
    for (x = rn1(10, 2); x; --x) {
	mm = mazexy();
	mkobj_at(ROCK_SYM, mm.x, mm.y);
    }
    mm = mazexy();
    makemon(PM_MINOTAUR, mm.x, mm.y);
    for (x = rn1(5, 7); x; --x) {
	mm = mazexy();
	makemon(NULL, mm.x, mm.y);
    }
    for (x = rn1(6, 7); x; --x) {
	mm = mazexy();
	mkgold(0L, mm.x, mm.y);
    }
    for (x = rn1(6, 7); x; --x)
	mktrap(0, 1, NULL);
    mm = mazexy();
    _level->l[(_level->stair.up.x = mm.x)][(_level->stair.up.y = mm.y)].scrsym = '<';
    _level->l[_level->stair.up.x][_level->stair.up.y].typ = STAIRS;
    _level->stair.dn.x = _level->stair.dn.y = 0;
}

void walkfrom(int x, int y)
{
    int q, a, dir;
    int dirs[4];
    _level->l[x][y].typ = ROOM;
    while (1) {
	q = 0;
	for (a = 0; a < 4; ++a)
	    if (okay(x, y, a))
		dirs[q++] = a;
	if (!q)
	    return;
	dir = dirs[rn2(q)];
	movept(&x, &y, dir);
	_level->l[x][y].typ = ROOM;
	movept(&x, &y, dir);
	walkfrom(x, y);
    }
}

static void movept(int *x, int *y, int dir)
{
    switch (dir) {
	case 0:
	    --(*y);
	    break;
	case 1:
	    ++(*x);
	    break;
	case 2:
	    ++(*y);
	    break;
	case 3:
	    --(*x);
	    break;
    }
}

int okay(int x, int y, int dir)
{
    movept(&x, &y, dir);
    movept(&x, &y, dir);
    if (x < 3 || y < 3 || x > COLNO - 3 || y > ROWNO - 3 || _level->l[x][y].typ != 0)
	return 0;
    else
	return 1;
}

struct coord mazexy(void)
{
    struct coord mm;
    mm.x = 3 + 2 * rn2(COLNO / 2 - 2);
    mm.y = 3 + 2 * rn2(ROWNO / 2 - 2);
    return mm;
}
