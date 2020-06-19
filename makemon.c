// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

//
// called with [x,y] = coordinates;
//      [0,0] means anyplace
//      [_u.ux,_u.uy] means: call mnexto (if !in_mklev)
//
//      In case we make an Orc or killer bee, we make an entire horde (swarm);
//      note that in this case we return only one of them (the one at [x,y]).
struct monst* makemon (const struct permonst* ptr, int x, int y)
{
    if (x != 0 || y != 0)
	if (m_at(x, y))
	    return NULL;
    bool anything = false;
    if (!ptr) {
	anything = true;
	unsigned mi = rn2 (CMNUM*_u.dlevel/24+7);
	if (mi+4 < _u.dlevel)
	    mi = rn2 (CMNUM*_u.dlevel/24+12);
	if (mi >= CMNUM)
	    mi = rn1 (CMNUM-CMNUM/2, CMNUM/2);
	while (mi < CMNUM && is_monster_genocided (c_Monsters[mi].mlet))
	    ++mi;
	if (mi >= CMNUM)
	    return NULL;	// all monsters genocided
	ptr = &c_Monsters[mi];
    } else if (is_monster_genocided (ptr->mlet))
	return NULL;
    struct monst* mtmp = newmonst(ptr->pxlth);
    mtmp->nmon = _level->monsters;
    _level->monsters = mtmp;
    mtmp->m_id = _wflags.ident++;
    mtmp->data = ptr;
    mtmp->mxlth = ptr->pxlth;
    if (ptr->mlet == 'D')
	mtmp->mhpmax = mtmp->mhp = 80;
    else if (!ptr->mlevel)
	mtmp->mhpmax = mtmp->mhp = rnd(4);
    else
	mtmp->mhpmax = mtmp->mhp = d(ptr->mlevel, 8);
    mtmp->mx = x;
    mtmp->my = y;
    mtmp->mcansee = 1;
    if (!in_mklev) {
	if (x == _u.ux && y == _u.uy && ptr->mlet != ' ')
	    mnexto(mtmp);
	if (x == 0 && y == 0)
	    rloc(mtmp);
    }
    if (ptr->mlet == 's' || ptr->mlet == 'S') {
	mtmp->mhide = mtmp->mundetected = 1;
	if (in_mklev)
	    if (mtmp->mx && mtmp->my)
		mkobj_at(0, mtmp->mx, mtmp->my);
    }
    if (ptr->mlet == 'I' || ptr->mlet == ';')
	mtmp->minvis = 1;
    if (ptr->mlet == 'L' || ptr->mlet == 'N' || (in_mklev && strchr("&w;", ptr->mlet) && rn2(5))
	)
	mtmp->msleep = 1;

    if (anything)
	if (ptr->mlet == 'O' || ptr->mlet == 'k') {
	    struct coord mm;
	    int cnt = rnd(10);
	    mm.x = x;
	    mm.y = y;
	    while (cnt--) {
		mm = enexto(mm.x, mm.y);
		makemon(ptr, mm.x, mm.y);
	    }
	}
    return mtmp;
}

struct coord enexto(int xx, int yy)
{
    int8_t x, y;
    struct coord foo[15], *tfoo;
    int range;

    tfoo = foo;
    range = 1;
    do {		       // full kludge action.
	for (x = xx - range; x <= xx + range; ++x)
	    if (goodpos(x, yy - range)) {
		tfoo->x = x;
		tfoo++->y = yy - range;
		if (tfoo == &foo[15])
		    goto foofull;
	    }
	for (x = xx - range; x <= xx + range; ++x)
	    if (goodpos(x, yy + range)) {
		tfoo->x = x;
		tfoo++->y = yy + range;
		if (tfoo == &foo[15])
		    goto foofull;
	    }
	for (y = yy + 1 - range; y < yy + range; ++y)
	    if (goodpos(xx - range, y)) {
		tfoo->x = xx - range;
		tfoo++->y = y;
		if (tfoo == &foo[15])
		    goto foofull;
	    }
	for (y = yy + 1 - range; y < yy + range; ++y)
	    if (goodpos(xx + range, y)) {
		tfoo->x = xx + range;
		tfoo++->y = y;
		if (tfoo == &foo[15])
		    goto foofull;
	    }
	++range;
    } while (tfoo == foo);
  foofull:
    return foo[rn2(tfoo - foo)];
}

int goodpos(int x, int y)
{			       // used only in mnexto and rloc
    return !(x < 1 || x > COLNO - 2 || y < 1 || y > ROWNO - 2 || m_at(x, y) || !ACCESSIBLE(_level->l[x][y].typ)
	      || (x == _u.ux && y == _u.uy)
	      || sobj_at(ENORMOUS_ROCK, x, y)
	    );
}

void rloc(struct monst *mtmp)
{
    int tx, ty;
    do {
	tx = rn1(COLNO - 3, 2);
	ty = rn2(ROWNO);
    } while (!goodpos(tx, ty));
    mtmp->mx = tx;
    mtmp->my = ty;
    pmon(mtmp);
}

struct monst* mkmon_at(int let, int x, int y)
{
    for (unsigned ct = 0; ct < CMNUM; ++ct) {
	const struct permonst* ptr = &c_Monsters[ct];
	if (ptr->mlet == let)
	    return makemon (ptr, x, y);
    }
    return 0;
}
