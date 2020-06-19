// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "mkroom.h"

enum { FCSIZ = ROWNO+COLNO };
struct fakecorridor {
    int8_t fx;
    int8_t fy;
    int8_t ftyp;
};

struct egd {
    int fcbeg;
    int fcend;		// fcend: first unused pos
    int8_t gdx;
    int8_t gdy;		// goal of guard's walk
    bool gddone:1;
    struct fakecorridor fakecorr [FCSIZ];
};

static const struct permonst pm_guard = { "guard", '@', 12, 12, -1, 4, 10, sizeof(struct egd) };

static struct monst* _guard = NULL;
static int _gdlevel = 0;

static inline struct egd *GetEGD(void)
{
    union UEGD {
	struct egd *pegd;
	long *l;
    } cst;
    cst.l = &(_guard->mextra[0]);
    return cst.pegd;
}

#define	EGD	GetEGD()

static void restfakecorr(void);
static int goldincorridor(void);

static void restfakecorr(void)
{
    int fcx, fcy, fcbeg;
    struct rm *crm;

    while ((fcbeg = EGD->fcbeg) < EGD->fcend) {
	fcx = EGD->fakecorr[fcbeg].fx;
	fcy = EGD->fakecorr[fcbeg].fy;
	if ((_u.ux == fcx && _u.uy == fcy) || cansee(fcx, fcy) || m_at(fcx, fcy))
	    return;
	crm = &_level->l[fcx][fcy];
	crm->typ = EGD->fakecorr[fcbeg].ftyp;
	if (!crm->typ)
	    crm->seen = 0;
	newsym(fcx, fcy);
	++EGD->fcbeg;
    }
    // it seems he left the corridor - let the guard disappear
    mondead(_guard);
    _guard = NULL;
}

static int goldincorridor(void)
{
    for (int fci = EGD->fcbeg; fci < EGD->fcend; ++fci)
	if (g_at(EGD->fakecorr[fci].fx, EGD->fakecorr[fci].fy))
	    return 1;
    return 0;
}

void setgd (void)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon) {
	if (m->isgd) {
	    _guard = m;
	    _gdlevel = _u.dlevel;
	    return;
	}
    }
    _guard = NULL;
}

void invault(void)
{
    int tmp = inroom(_u.ux, _u.uy);
    if (tmp < 0 || _level->rooms[tmp].rtype != VAULT) {
	_u.uinvault = 0;
	return;
    }
    _u.uinvault = (_u.uinvault+1) % 50;
    if (!_u.uinvault && (!_guard || _gdlevel != _u.dlevel)) {
	char buf[BUFSZ];
	int x, y, dd, gx, gy;

	// first find the goal for the guard
	for (dd = 1; (dd < ROWNO || dd < COLNO); ++dd) {
	    for (y = _u.uy - dd; y <= _u.uy + dd; ++y) {
		if (y < 0 || y > ROWNO - 1)
		    continue;
		for (x = _u.ux - dd; x <= _u.ux + dd; ++x) {
		    if (y != _u.uy - dd && y != _u.uy + dd && x != _u.ux - dd)
			x = _u.ux + dd;
		    if (x < 0 || x > COLNO - 1)
			continue;
		    if (_level->l[x][y].typ == CORR)
			goto fnd;
		}
	    }
	}
	impossible("Not a single corridor on this level??");
	tele();
	return;
      fnd:
	gx = x;
	gy = y;

	// next find a good place for a door in the wall
	x = _u.ux;
	y = _u.uy;
	while (_level->l[x][y].typ == ROOM) {
	    int dx, dy;

	    dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
	    dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
	    if (abs(gx - x) >= abs(gy - y))
		x += dx;
	    else
		y += dy;
	}

	// make something interesting happen
	if (!(_guard = makemon(&pm_guard, x, y)))
	    return;
	_guard->isgd = _guard->mpeaceful = 1;
	EGD->gddone = 0;
	_gdlevel = _u.dlevel;
	if (!cansee(_guard->mx, _guard->my)) {
	    mondead(_guard);
	    _guard = 0;
	    return;
	}
	pline("Suddenly one of the Vault's guards enters!");
	pmon(_guard);
	do {
	    pline("\"Hello stranger, who are you?\" - ");
	    getlin(buf);
	} while (!letter(buf[0]));

	if (!strcmp(buf, "Croesus") || !strcmp(buf, "Kroisos")) {
	    pline("\"Oh, yes - of course. Sorry to have disturbed you.\"");
	    mondead(_guard);
	    _guard = 0;
	    return;
	}
	clrlin();
	pline("\"I don't know you.\"");
	if (!_u.ugold)
	    pline("\"Please follow me.\"");
	else {
	    pline("\"Most likely all that gold was stolen from this vault.\"");
	    pline("\"Please drop your gold (say d$ ) and follow me.\"");
	}
	EGD->gdx = gx;
	EGD->gdy = gy;
	EGD->fcbeg = 0;
	EGD->fakecorr[0].fx = x;
	EGD->fakecorr[0].fy = y;
	EGD->fakecorr[0].ftyp = _level->l[x][y].typ;
	_level->l[x][y].typ = DOOR;
	EGD->fcend = 1;
    }
}

int gd_move(void)
{
    int x, y, dx, dy, gx, gy, nx, ny, typ;
    struct fakecorridor *fcp;
    struct rm *crm;
    if (!_guard || _gdlevel != _u.dlevel) {
	impossible("Where is the guard?");
	return 2;	       // died
    }
    if (_u.ugold || goldincorridor())
	return 0;	       // didnt move
    if (dist(_guard->mx, _guard->my) > 1 || EGD->gddone) {
	restfakecorr();
	return 0;	       // didnt move
    }
    x = _guard->mx;
    y = _guard->my;
    // look around (hor & vert only) for accessible places
    for (nx = x - 1; nx <= x + 1; ++nx)
	for (ny = y - 1; ny <= y + 1; ++ny) {
	    if (nx == x || ny == y)
		if (nx != x || ny != y)
		    if (isok(nx, ny))
			if (!IS_WALL(typ = (crm = &_level->l[nx][ny])->typ) && typ != POOL) {
			    int i;
			    for (i = EGD->fcbeg; i < EGD->fcend; ++i)
				if (EGD->fakecorr[i].fx == nx && EGD->fakecorr[i].fy == ny)
				    goto nextnxy;
			    if ((i = inroom(nx, ny)) >= 0 && _level->rooms[i].rtype == VAULT)
				goto nextnxy;
			    // seems we found a good place to leave him alone
			    EGD->gddone = 1;
			    if (ACCESSIBLE(typ))
				goto newpos;
			    crm->typ = (typ == SCORR) ? CORR : DOOR;
			    goto proceed;
			}
	  nextnxy:;
	}
    nx = x;
    ny = y;
    gx = EGD->gdx;
    gy = EGD->gdy;
    dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
    dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
    if (abs(gx - x) >= abs(gy - y))
	nx += dx;
    else
	ny += dy;

    while ((typ = (crm = &_level->l[nx][ny])->typ) != 0) {
	// in view of the above we must have IS_WALL(typ) or typ == POOL must be a wall here
	if (isok(nx + nx - x, ny + ny - y) && typ != POOL && ZAP_POS(_level->l[nx + nx - x][ny + ny - y].typ)) {
	    crm->typ = DOOR;
	    goto proceed;
	}
	if (dy && nx != x) {
	    nx = x;
	    ny = y + dy;
	    continue;
	}
	if (dx && ny != y) {
	    ny = y;
	    nx = x + dx;
	    dy = 0;
	    continue;
	}
	// I don't like this, but ...
	crm->typ = DOOR;
	goto proceed;
    }
    crm->typ = CORR;
  proceed:
    if (cansee(nx, ny)) {
	mnewsym(nx, ny);
	prl(nx, ny);
    }
    fcp = &(EGD->fakecorr[EGD->fcend]);
    if (EGD->fcend++ == FCSIZ)
	panic("fakecorr overflow");
    fcp->fx = nx;
    fcp->fy = ny;
    fcp->ftyp = typ;
  newpos:
    if (EGD->gddone)
	nx = ny = 0;
    _guard->mx = nx;
    _guard->my = ny;
    pmon(_guard);
    restfakecorr();
    return 1;
}

void gddead (void)
{
    _guard = NULL;
}

void replgd(struct monst *mtmp, struct monst *mtmp2)
{
    if (mtmp == _guard)
	_guard = mtmp2;
}
