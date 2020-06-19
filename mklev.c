// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

#include "mkroom.h"
enum { XLIM = 4, YLIM = 3 };	// define minimum required space around a room
bool secret;			// true while making a vault: increase [XY]LIM
int smeq[MAXNROFROOMS + 1];
int doorindex;
int8_t nxcor;
bool goldseen;
int nroom;

// Definitions used by makerooms() and addrs()
enum { MAXRS = 50 };	       // max lth of temp rectangle table - arbitrary
struct rectangle {
    int8_t rlx, rly, rhx, rhy;
} rs[MAXRS + 1];
int rscnt, rsmax;		// 0..rscnt-1: currently under consideration
// rscnt..rsmax: discarded

#define somex() (nrand(croom->hx-croom->lx+1)+croom->lx)
#define somey() (nrand(croom->hy-croom->ly+1)+croom->ly)

void makelevel (void)
{
    struct mkroom *croom, *troom;
    unsigned tryct;
    int x, y;

    nroom = 0;
    doorindex = 0;
    _level->rooms[0].hx = -1;	// in case we are in a maze
    memset (_level->l, 0, sizeof(_level->l));

    if (_u.dlevel >= rn1(3, 26)) {	// there might be several mazes
	makemaz();
	return;
    }
    // construct the rooms
    nroom = 0;
    secret = false;
    makerooms();

    // construct stairs (up and down in different rooms if possible)
    croom = &_level->rooms[rn2(nroom)];
    _level->stair.dn.x = somex();
    _level->stair.dn.y = somey();
    _level->l[_level->stair.dn.x][_level->stair.dn.y].scrsym = '>';
    _level->l[_level->stair.dn.x][_level->stair.dn.y].typ = STAIRS;
    if (nroom > 1) {
	troom = croom;
	croom = &_level->rooms[rn2(nroom - 1)];
	if (croom >= troom)
	    ++croom;
    }
    _level->stair.up.x = somex();	       // %% < and > might be in the same place
    _level->stair.up.y = somey();
    _level->l[_level->stair.up.x][_level->stair.up.y].scrsym = '<';
    _level->l[_level->stair.up.x][_level->stair.up.y].typ = STAIRS;

    // for each room: put things inside
    for (croom = _level->rooms; croom->hx > 0; ++croom) {
	// put a sleeping monster inside
	//
	// Note: monster may be on the stairs. This cannot be
	// avoided: maybe the player fell through a trapdoor while a
	// monster was on the stairs. Conclusion: we have to check
	// for monsters on the stairs anyway.
	if (!rn2(3))
	    makemon (NULL, somex(), somey());

	// put traps inside
	goldseen = false;
	while (!rn2(8 - (_u.dlevel / 6)))
	    mktrap(0, 0, croom);
	if (!goldseen && !rn2(3))
	    mkgold(0L, somex(), somey());
	if (!rn2(3)) {
	    mkobj_at(0, somex(), somey());
	    tryct = 0;
	    while (!rn2(5)) {
		if (++tryct > 100) {
		    printf("tryct overflow4\n");
		    break;
		}
		mkobj_at(0, somex(), somey());
	    }
	}
    }

    qsort (_level->rooms, nroom, sizeof(struct mkroom), comp);
    makecorridors();
    make_niches();

    // make a secret treasure vault, not connected to the rest
    if (nroom <= (2 * MAXNROFROOMS / 3)) {
	if (rn2(3)) {
	    troom = &_level->rooms[nroom];
	    secret = true;
	    if (makerooms()) {
		troom->rtype = VAULT;	// treasure vault
		for (x = troom->lx; x <= troom->hx; ++x)
		    for (y = troom->ly; y <= troom->hy; ++y)
			mkgold((long) (rnd(_u.dlevel * 100) + 50), x, y);
		if (!rn2(3))
		    makevtele();
	    }
	}
    }
    if (_u.dlevel > 1 && _u.dlevel < 20 && rn2(_u.dlevel) < 3)
	mkshop();
    else if (_u.dlevel > 6 && !rn2(7))
	mkzoo(ZOO);
    else if (_u.dlevel > 9 && !rn2(5))
	mkzoo(BEEHIVE);
    else if (_u.dlevel > 11 && !rn2(6))
	mkzoo(MORGUE);
    else if (_u.dlevel > 18 && !rn2(6))
	mkswamp();
}

int makerooms(void)
{
    struct rectangle *rsp;
    int lx, ly, hx, hy, lowx, lowy, hix, hiy, dx, dy;
    int tryct = 0, xlim, ylim;

    // init
    xlim = XLIM + secret;
    ylim = YLIM + secret;
    if (nroom == 0) {
	rsp = rs;
	rsp->rlx = rsp->rly = 0;
	rsp->rhx = COLNO - 1;
	rsp->rhy = ROWNO - 1;
	rsmax = 1;
    }
    rscnt = rsmax;

    // make rooms until satisfied
    while (rscnt > 0 && nroom < MAXNROFROOMS - 1) {
	if (!secret && nroom > (MAXNROFROOMS / 3) && !rn2((MAXNROFROOMS - nroom) * (MAXNROFROOMS - nroom)))
	    return 0;

	// pick a rectangle
	rsp = &rs[rn2(rscnt)];
	hx = rsp->rhx;
	hy = rsp->rhy;
	lx = rsp->rlx;
	ly = rsp->rly;

	// find size of room
	if (secret)
	    dx = dy = 1;
	else {
	    dx = 2 + rn2((hx - lx - 8 > 20) ? 12 : 8);
	    dy = 2 + rn2(4);
	    if (dx * dy > 50)
		dy = 50 / dx;
	}

	// look whether our room will fit
	if (hx - lx < dx + dx / 2 + 2 * xlim || hy - ly < dy + dy / 3 + 2 * ylim) {
	    // no, too small; maybe we throw this area out
	    if (secret || !rn2(MAXNROFROOMS + 1 - nroom - tryct)) {
		--rscnt;
		rs[rsmax] = *rsp;
		*rsp = rs[rscnt];
		rs[rscnt] = rs[rsmax];
		tryct = 0;
	    } else
		++tryct;
	    continue;
	}
	lowx = lx + xlim + rn2(hx - lx - dx - 2 * xlim + 1);
	lowy = ly + ylim + rn2(hy - ly - dy - 2 * ylim + 1);
	hix = lowx + dx;
	hiy = lowy + dy;

	if (maker(lowx, dx, lowy, dy)) {
	    if (secret)
		return 1;
	    addrs(lowx - 1, lowy - 1, hix + 1, hiy + 1);
	    tryct = 0;
	} else if (tryct++ > 100)
	    break;
    }
    return 0;		       // failed to make vault - very strange
}

void addrs(int lowx, int lowy, int hix, int hiy)
{
    struct rectangle *rsp;
    int lx, ly, hx, hy, xlim, ylim;
    bool discarded;

    xlim = XLIM + secret;
    ylim = YLIM + secret;

    // walk down since rscnt and rsmax change
    for (rsp = &rs[rsmax - 1]; rsp >= rs; --rsp) {

	if ((lx = rsp->rlx) > hix || (ly = rsp->rly) > hiy || (hx = rsp->rhx) < lowx || (hy = rsp->rhy) < lowy)
	    continue;
	if ((discarded = (rsp >= &rs[rscnt]))) {
	    *rsp = rs[--rsmax];
	} else {
	    --rsmax;
	    --rscnt;
	    *rsp = rs[rscnt];
	    if (rscnt != rsmax)
		rs[rscnt] = rs[rsmax];
	}
	if (lowy - ly > 2 * ylim + 4)
	    addrsx(lx, ly, hx, lowy - 2, discarded);
	if (lowx - lx > 2 * xlim + 4)
	    addrsx(lx, ly, lowx - 2, hy, discarded);
	if (hy - hiy > 2 * ylim + 4)
	    addrsx(lx, hiy + 2, hx, hy, discarded);
	if (hx - hix > 2 * xlim + 4)
	    addrsx(hix + 2, ly, hx, hy, discarded);
    }
}

// piece of a discarded area
void addrsx(int lx, int ly, int hx, int hy, bool discarded)
{
    struct rectangle *rsp;

    // check inclusions
    for (rsp = rs; rsp < &rs[rsmax]; ++rsp) {
	if (lx >= rsp->rlx && hx <= rsp->rhx && ly >= rsp->rly && hy <= rsp->rhy)
	    return;
    }

    // make a new entry
    if (rsmax >= MAXRS)
	return;
    ++rsmax;
    if (!discarded) {
	*rsp = rs[rscnt];
	rsp = &rs[rscnt];
	++rscnt;
    }
    rsp->rlx = lx;
    rsp->rly = ly;
    rsp->rhx = hx;
    rsp->rhy = hy;
}

int comp(const void *vx, const void *vy)
{
    const struct mkroom *x = vx, *y = vy;
    if (x->lx < y->lx)
	return -1;
    return x->lx > y->lx;
}

static struct coord finddpos(unsigned xl, unsigned yl, unsigned xh, unsigned yh)
{
    struct coord ff;
    unsigned x = (xl == xh) ? xl : (xl + rn2(xh - xl + 1));
    unsigned y = (yl == yh) ? yl : (yl + rn2(yh - yl + 1));
    if (okdoor(x, y))
	goto gotit;

    for (x = xl; x <= xh; ++x)
	for (y = yl; y <= yh; ++y)
	    if (okdoor(x, y))
		goto gotit;

    for (x = xl; x <= xh; ++x)
	for (y = yl; y <= yh; ++y)
	    if (_level->l[x][y].typ == DOOR || _level->l[x][y].typ == SDOOR)
		goto gotit;
    // cannot find something reasonable -- strange
    x = xl;
    y = yh;
gotit:
    ff.x = x;
    ff.y = y;
    return ff;
}

// see whether it is allowable to create a door at [x,y]
int okdoor(int x, int y)
{
    if (_level->l[x - 1][y].typ == DOOR || _level->l[x + 1][y].typ == DOOR || _level->l[x][y + 1].typ == DOOR || _level->l[x][y - 1].typ == DOOR || _level->l[x - 1][y].typ == SDOOR || _level->l[x + 1][y].typ == SDOOR || _level->l[x][y - 1].typ == SDOOR || _level->l[x][y + 1].typ == SDOOR
	|| (_level->l[x][y].typ != HWALL && _level->l[x][y].typ != VWALL) || doorindex >= DOORMAX)
	return 0;
    return 1;
}

void dodoor(int x, int y, struct mkroom *aroom)
{
    if (doorindex >= DOORMAX) {
	impossible("DOORMAX exceeded?");
	return;
    }
    if (!okdoor(x, y) && nxcor)
	return;
    dosdoor(x, y, aroom, rn2(8) ? DOOR : SDOOR);
}

void dosdoor(int x, int y, struct mkroom *aroom, int type)
{
    struct mkroom *broom;
    int tmp;

    if (!IS_WALL(_level->l[x][y].typ))	// avoid SDOORs with '+' as scrsym
	type = DOOR;
    _level->l[x][y].typ = type;
    if (type == DOOR)
	_level->l[x][y].scrsym = '+';
    ++aroom->doorct;
    broom = aroom + 1;
    if (broom->hx < 0)
	tmp = doorindex;
    else
	for (tmp = doorindex; tmp > broom->fdoor; --tmp)
	    _level->doors[tmp] = _level->doors[tmp - 1];
    ++doorindex;
    _level->doors[tmp].x = x;
    _level->doors[tmp].y = y;
    for (; broom->hx >= 0; ++broom)
	++broom->fdoor;
}

// Only called from makerooms()
int maker(int lowx, int ddx, int lowy, int ddy)
{
    struct mkroom *croom;
    int x, y, hix = lowx + ddx, hiy = lowy + ddy;
    int xlim = XLIM + secret, ylim = YLIM + secret;

    if (nroom >= MAXNROFROOMS)
	return 0;
    if (lowx < XLIM)
	lowx = XLIM;
    if (lowy < YLIM)
	lowy = YLIM;
    if (hix > COLNO - XLIM - 1)
	hix = COLNO - XLIM - 1;
    if (hiy > ROWNO - YLIM - 1)
	hiy = ROWNO - YLIM - 1;
  chk:
    if (hix <= lowx || hiy <= lowy)
	return 0;

    // check area around room (and make room smaller if necessary)
    for (x = lowx - xlim; x <= hix + xlim; ++x) {
	for (y = lowy - ylim; y <= hiy + ylim; ++y) {
	    if (_level->l[x][y].typ) {
		if (!rn2(3))
		    return 0;
		if (x < lowx)
		    lowx = x + xlim + 1;
		else
		    hix = x - xlim - 1;
		if (y < lowy)
		    lowy = y + ylim + 1;
		else
		    hiy = y - ylim - 1;
		goto chk;
	    }
	}
    }

    croom = &_level->rooms[nroom];

    // on low levels the room is lit (usually)
    // secret vaults are always lit
    if ((rnd(_u.dlevel) < 10 && rn2(77)) || (ddx == 1 && ddy == 1)) {
	for (x = lowx - 1; x <= hix + 1; ++x)
	    for (y = lowy - 1; y <= hiy + 1; ++y)
		_level->l[x][y].lit = 1;
	croom->rlit = 1;
    } else
	croom->rlit = 0;
    croom->lx = lowx;
    croom->hx = hix;
    croom->ly = lowy;
    croom->hy = hiy;
    croom->rtype = croom->doorct = croom->fdoor = 0;

    for (x = lowx - 1; x <= hix + 1; ++x)
	for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
	    _level->l[x][y].scrsym = '-';
	    _level->l[x][y].typ = HWALL;
	}
    for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
	for (y = lowy; y <= hiy; ++y) {
	    _level->l[x][y].scrsym = '|';
	    _level->l[x][y].typ = VWALL;
	}
    for (x = lowx; x <= hix; ++x)
	for (y = lowy; y <= hiy; ++y) {
	    _level->l[x][y].scrsym = '.';
	    _level->l[x][y].typ = ROOM;
	}

    smeq[nroom] = nroom;
    ++croom;
    croom->hx = -1;
    ++nroom;
    return 1;
}

void makecorridors(void)
{
    int a, b;

    nxcor = 0;
    for (a = 0; a < nroom - 1; ++a)
	join(a, a + 1);
    for (a = 0; a < nroom - 2; ++a)
	if (smeq[a] != smeq[a + 2])
	    join(a, a + 2);
    for (a = 0; a < nroom; ++a)
	for (b = 0; b < nroom; ++b)
	    if (smeq[a] != smeq[b])
		join(a, b);
    if (nroom > 2)
	for (nxcor = rn2(nroom) + 4; nxcor; --nxcor) {
	    a = rn2(nroom);
	    b = rn2(nroom - 2);
	    if (b >= a)
		b += 2;
	    join(a, b);
	}
}

void join(int a, int b)
{
    struct coord cc, tt;
    int tx, ty, xx, yy;
    struct rm *crm;
    struct mkroom *croom, *troom;
    int dx, dy, dix, diy, cct;

    croom = &_level->rooms[a];
    troom = &_level->rooms[b];

    // find positions cc and tt for doors in croom and troom and
    // direction for a corridor between them

    if (troom->hx < 0 || croom->hx < 0 || doorindex >= DOORMAX)
	return;
    if (troom->lx > croom->hx) {
	dx = 1;
	dy = 0;
	xx = croom->hx + 1;
	tx = troom->lx - 1;
	cc = finddpos(xx, croom->ly, xx, croom->hy);
	tt = finddpos(tx, troom->ly, tx, troom->hy);
    } else if (troom->hy < croom->ly) {
	dy = -1;
	dx = 0;
	yy = croom->ly - 1;
	cc = finddpos(croom->lx, yy, croom->hx, yy);
	ty = troom->hy + 1;
	tt = finddpos(troom->lx, ty, troom->hx, ty);
    } else if (troom->hx < croom->lx) {
	dx = -1;
	dy = 0;
	xx = croom->lx - 1;
	tx = troom->hx + 1;
	cc = finddpos(xx, croom->ly, xx, croom->hy);
	tt = finddpos(tx, troom->ly, tx, troom->hy);
    } else {
	dy = 1;
	dx = 0;
	yy = croom->hy + 1;
	ty = troom->ly - 1;
	cc = finddpos(croom->lx, yy, croom->hx, yy);
	tt = finddpos(troom->lx, ty, troom->hx, ty);
    }
    xx = cc.x;
    yy = cc.y;
    tx = tt.x - dx;
    ty = tt.y - dy;
    if (nxcor && _level->l[xx + dx][yy + dy].typ)
	return;
    dodoor(xx, yy, croom);

    cct = 0;
    while (xx != tx || yy != ty) {
	xx += dx;
	yy += dy;

	// loop: dig corridor at [xx,yy] and find new [xx,yy]
	if (cct++ > 500 || (nxcor && !rn2(35)))
	    return;

	if (xx == COLNO - 1 || xx == 0 || yy == 0 || yy == ROWNO - 1)
	    return;	       // impossible

	crm = &_level->l[xx][yy];
	if (!(crm->typ)) {
	    if (rn2(100)) {
		crm->typ = CORR;
		crm->scrsym = CORR_SYM;
		if (nxcor && !rn2(50))
		    mkobj_at(ROCK_SYM, xx, yy);
	    } else {
		crm->typ = SCORR;
		crm->scrsym = ' ';
	    }
	} else if (crm->typ != CORR && crm->typ != SCORR) {
	    // strange ...
	    return;
	}
	// find next corridor position
	dix = abs(xx - tx);
	diy = abs(yy - ty);

	// do we have to change direction ?
	if (dy && dix > diy) {
	    int ddx = (xx > tx) ? -1 : 1;

	    crm = &_level->l[xx + ddx][yy];
	    if (!crm->typ || crm->typ == CORR || crm->typ == SCORR) {
		dx = ddx;
		dy = 0;
		continue;
	    }
	} else if (dx && diy > dix) {
	    int ddy = (yy > ty) ? -1 : 1;

	    crm = &_level->l[xx][yy + ddy];
	    if (!crm->typ || crm->typ == CORR || crm->typ == SCORR) {
		dy = ddy;
		dx = 0;
		continue;
	    }
	}
	// continue straight on?
	crm = &_level->l[xx + dx][yy + dy];
	if (!crm->typ || crm->typ == CORR || crm->typ == SCORR)
	    continue;

	// no, what must we do now??
	if (dx) {
	    dx = 0;
	    dy = (ty < yy) ? -1 : 1;
	    crm = &_level->l[xx + dx][yy + dy];
	    if (!crm->typ || crm->typ == CORR || crm->typ == SCORR)
		continue;
	    dy = -dy;
	    continue;
	} else {
	    dy = 0;
	    dx = (tx < xx) ? -1 : 1;
	    crm = &_level->l[xx + dx][yy + dy];
	    if (!crm->typ || crm->typ == CORR || crm->typ == SCORR)
		continue;
	    dx = -dx;
	    continue;
	}
    }

    // we succeeded in digging the corridor
    dodoor(tt.x, tt.y, troom);

    if (smeq[a] < smeq[b])
	smeq[b] = smeq[a];
    else
	smeq[a] = smeq[b];
}

void make_niches(void)
{
    int ct = rnd(nroom / 2 + 1);
    while (ct--)
	makeniche(false);
}

void makevtele(void)
{
    makeniche(true);
}

void makeniche (bool with_trap)
{
    struct mkroom *aroom;
    struct rm *rm;
    int vct = 8;
    struct coord dd;
    int dy, xx, yy;
    struct trap *ttmp;

    if (doorindex < DOORMAX)
	while (vct--) {
	    aroom = &_level->rooms[rn2(nroom - 1)];
	    if (aroom->rtype != 0)
		continue;      // not an ordinary room
	    if (aroom->doorct == 1 && rn2(5))
		continue;
	    if (rn2(2)) {
		dy = 1;
		dd = finddpos(aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1);
	    } else {
		dy = -1;
		dd = finddpos(aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1);
	    }
	    xx = dd.x;
	    yy = dd.y;
	    if ((rm = &_level->l[xx][yy + dy])->typ)
		continue;
	    if (with_trap || !rn2(4)) {
		rm->typ = SCORR;
		rm->scrsym = ' ';
		if (with_trap) {
		    ttmp = maketrap(xx, yy + dy, TELEP_TRAP);
		    ttmp->once = 1;
		    make_engr_at(xx, yy - dy, "ad ae?ar um");
		}
		dosdoor(xx, yy, aroom, SDOOR);
	    } else {
		rm->typ = CORR;
		rm->scrsym = CORR_SYM;
		if (rn2(7))
		    dosdoor(xx, yy, aroom, rn2(5) ? SDOOR : DOOR);
		else {
		    mksobj_at(SCR_TELEPORTATION, xx, yy + dy);
		    if (!rn2(3))
			mkobj_at(0, xx, yy + dy);
		}
	    }
	    return;
	}
}

// make a trap somewhere (in croom if mazeflag = 0)
void mktrap(int num, int mazeflag, struct mkroom *croom)
{
    struct trap *ttmp;
    int kind, nopierc, tryct = 0;
    int8_t mx, my;

    if (!num || num >= TRAPNUM) {
	nopierc = (_u.dlevel < 4) ? 1 : 0;
	kind = rn2(TRAPNUM - nopierc - 1);
	// note: PIERC = 7, TRAPNUM = 9
    } else
	kind = num;
    do {
	if (++tryct > 200)
	    return;
	if (mazeflag) {
	    struct coord mm;
	    mm = mazexy();
	    mx = mm.x;
	    my = mm.y;
	} else {
	    mx = somex();
	    my = somey();
	}
    } while (t_at(mx, my) || _level->l[mx][my].typ == STAIRS);
    ttmp = maketrap(mx, my, kind);
    if (mazeflag && !rn2(10) && ttmp->ttyp < PIERC)
	ttmp->tseen = 1;
}
