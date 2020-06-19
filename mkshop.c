// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "mkroom.h"
#include "eshk.h"
#define	ESHK	((struct eshk *)(&(shk->mextra[0])))

void mkshop(void)
{
    struct mkroom *sroom;
    int sh, sx, sy, i = -1;
    char let;
    int roomno;
    struct monst *shk;
    for (sroom = &_level->rooms[0], roomno = 0;; ++sroom, ++roomno) {
	if (sroom->hx < 0)
	    return;
	if (sroom - _level->rooms >= nroom) {
	    pline("rooms not closed by -1?");
	    return;
	}
	if (sroom->rtype)
	    continue;
	if (!sroom->rlit || has_dnstairs(sroom) || has_upstairs(sroom))
	    continue;
	if (sroom->doorct == 1)
	    break;
    }

    if (i < 0) {	// shoptype not yet determined
	static const uint8_t c_ShopTypeProb[] = { 3, 3, 5, 5, 10, 10, 14, 50 };
	i = 0;
	for (int j = rn2(100); (j -= c_ShopTypeProb[i]) >= 0; ++i)
	    if (!shtypes[i])
		break;	       // superfluous
	if (isbig(sroom) && i + SHOPBASE == WANDSHOP)
	    i = GENERAL - SHOPBASE;
    }
    sroom->rtype = i + SHOPBASE;
    let = shtypes[i];
    sh = sroom->fdoor;
    sx = _level->doors[sh].x;
    sy = _level->doors[sh].y;
    if (sx == sroom->lx - 1)
	++sx;
    else if (sx == sroom->hx + 1)
	--sx;
    else if (sy == sroom->ly - 1)
	++sy;
    else if (sy == sroom->hy + 1)
	--sy;
    else
	return;
    if (!(shk = makemon(PM_SHK, sx, sy)))
	return;
    shk->isshk = shk->mpeaceful = 1;
    shk->msleep = 0;
    shk->mtrapseen = ~0;       // we know all the traps already
    ESHK->shoproom = roomno;
    ESHK->shoplevel = _u.dlevel;
    ESHK->shd = _level->doors[sh];
    ESHK->shk.x = sx;
    ESHK->shk.y = sy;
    ESHK->robbed = 0;
    ESHK->visitct = 0;
    ESHK->following = 0;
    shk->mgold = 1000 + 30 * rnd(100);	// initial capital
    ESHK->billct = 0;
    findname(ESHK->shknam, let);
    for (sx = sroom->lx; sx <= sroom->hx; ++sx)
	for (sy = sroom->ly; sy <= sroom->hy; ++sy) {
	    if ((sx == sroom->lx && _level->doors[sh].x == sx - 1)
		    || (sx == sroom->hx && _level->doors[sh].x == sx + 1)
		    || (sy == sroom->ly && _level->doors[sh].y == sy - 1)
		    || (sy == sroom->hy && _level->doors[sh].y == sy + 1))
		continue;
	    mkobj_at(let, sx, sy);
	}
}

void mkzoo(int type)
{
    struct mkroom *sroom;
    struct monst *mon;
    int sh, sx, sy, i;
    int goldlim = 500 * _u.dlevel;
    int moct = 0;

    i = nroom;
    for (sroom = &_level->rooms[rn2(nroom)];; ++sroom) {
	if (sroom == &_level->rooms[nroom])
	    sroom = &_level->rooms[0];
	if (!i-- || sroom->hx < 0)
	    return;
	if (sroom->rtype)
	    continue;
	if (type == MORGUE && sroom->rlit)
	    continue;
	if (has_upstairs(sroom) || (has_dnstairs(sroom) && rn2(3)))
	    continue;
	if (sroom->doorct == 1 || !rn2(5))
	    break;
    }
    sroom->rtype = type;
    sh = sroom->fdoor;
    for (sx = sroom->lx; sx <= sroom->hx; ++sx)
	for (sy = sroom->ly; sy <= sroom->hy; ++sy) {
	    if ((sx == sroom->lx && _level->doors[sh].x == sx - 1)
		    || (sx == sroom->hx && _level->doors[sh].x == sx + 1)
		    || (sy == sroom->ly && _level->doors[sh].y == sy - 1)
		    || (sy == sroom->hy && _level->doors[sh].y == sy + 1))
		continue;
	    mon = makemon((type == MORGUE) ? morguemon() : (type == BEEHIVE) ? PM_KILLER_BEE : NULL, sx, sy);
	    if (mon)
		mon->msleep = 1;
	    switch (type) {
		case ZOO:
		    i = sq(dist2(sx, sy, _level->doors[sh].x, _level->doors[sh].y));
		    if (i >= goldlim)
			i = 5 * _u.dlevel;
		    goldlim -= i;
		    mkgold((long) (10 + rn2(i)), sx, sy);
		    break;
		case MORGUE:
		    // Usually there is one dead body in the morgue
		    if (!moct && rn2(3)) {
			mksobj_at(CORPSE, sx, sy);
			++moct;
		    }
		    break;
		case BEEHIVE:
		    if (!rn2(3))
			mksobj_at(LUMP_OF_ROYAL_JELLY, sx, sy);
		    break;
	    }
	}
}

const struct permonst *morguemon(void)
{
    int i = rn2(100), hd = rn2(_u.dlevel);
    if (hd > 10 && i < 10)
	return PM_DEMON;
    if (hd > 8 && i > 85)
	return PM_VAMPIRE;
    return (i < 40) ? PM_GHOST : (i < 60) ? PM_WRAITH : PM_ZOMBIE;
}

void mkswamp(void)
{			       // Michiel Huisjes & Fred de Wilde
    struct mkroom *sroom;
    int sx, sy, i, eelct = 0;

    for (i = 0; i < 5; ++i) {  // 5 tries
	sroom = &_level->rooms[rn2(nroom)];
	if (sroom->hx < 0 || sroom->rtype || has_upstairs(sroom) || has_dnstairs(sroom))
	    continue;

	// satisfied; make a swamp
	sroom->rtype = SWAMP;
	for (sx = sroom->lx; sx <= sroom->hx; ++sx)
	    for (sy = sroom->ly; sy <= sroom->hy; ++sy)
		if ((sx + sy) % 2 && !o_at(sx, sy) && !t_at(sx, sy)
		    && !m_at(sx, sy) && !nexttodoor(sx, sy)) {
		    _level->l[sx][sy].typ = POOL;
		    _level->l[sx][sy].scrsym = POOL_SYM;
		    if (!eelct || !rn2(4)) {
			makemon(PM_EEL, sx, sy);
			++eelct;
		    }
		}
    }
}

int nexttodoor(int sx, int sy)
{
    int dx, dy;
    struct rm *lev;
    for (dx = -1; dx <= 1; ++dx)
	for (dy = -1; dy <= 1; ++dy)
	    if ((lev = &_level->l[sx + dx][sy + dy])->typ == DOOR || lev->typ == SDOOR || lev->typ == LDOOR)
		return 1;
    return 0;
}

int has_dnstairs(struct mkroom *sroom)
{
    return sroom->lx <= _level->stair.dn.x
	&& _level->stair.dn.x <= sroom->hx
	&& sroom->ly <= _level->stair.dn.y
	&& _level->stair.dn.y <= sroom->hy;
}

int has_upstairs(struct mkroom *sroom)
{
    return sroom->lx <= _level->stair.up.x
	&& _level->stair.up.x <= sroom->hx
	&& sroom->ly <= _level->stair.up.y
	&& _level->stair.up.y <= sroom->hy;
}

int isbig(struct mkroom *sroom)
{
    int area = (sroom->hx - sroom->lx) * (sroom->hy - sroom->ly);
    return area > 20;
}

int dist2(int x0, int y0, int x1, int y1)
{
    return (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1);
}

int sq(int a)
{
    return a * a;
}
