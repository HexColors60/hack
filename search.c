// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

int findit(void)
{			       // returns number of things found
    int num;
    int8_t zx, zy;
    struct trap *ttmp;
    int8_t lx, hx, ly, hy;

    for (lx = _u.ux; (num = _level->l[lx - 1][_u.uy].typ) && num != CORR; --lx) {}
    for (hx = _u.ux; (num = _level->l[hx + 1][_u.uy].typ) && num != CORR; ++hx) {}
    for (ly = _u.uy; (num = _level->l[_u.ux][ly - 1].typ) && num != CORR; --ly) {}
    for (hy = _u.uy; (num = _level->l[_u.ux][hy + 1].typ) && num != CORR; ++hy) {}
    num = 0;
    for (zy = ly; zy <= hy; ++zy) {
	for (zx = lx; zx <= hx; ++zx) {
	    if (_level->l[zx][zy].typ == SDOOR) {
		_level->l[zx][zy].typ = DOOR;
		atl(zx, zy, '+');
		++num;
	    } else if (_level->l[zx][zy].typ == SCORR) {
		_level->l[zx][zy].typ = CORR;
		atl(zx, zy, CORR_SYM);
		++num;
	    } else if ((ttmp = t_at(zx, zy)) != NULL) {
		if (ttmp->ttyp == PIERC) {
		    makemon(PM_PIERCER, zx, zy);
		    ++num;
		    deltrap(ttmp);
		} else if (!ttmp->tseen) {
		    ttmp->tseen = 1;
		    if (!vism_at(zx, zy))
			atl(zx, zy, '^');
		    ++num;
		}
	    }
	}
    }
    return num;
}

int dosearch(void)
{
    for (int8_t x = _u.ux - 1; x < _u.ux + 2; ++x) {
	for (int8_t y = _u.uy - 1; y < _u.uy + 2; ++y) {
	    if (x != _u.ux || y != _u.uy) {
		if (_level->l[x][y].typ == SDOOR) {
		    if (rn2(7))
			continue;
		    _level->l[x][y].typ = DOOR;
		    _level->l[x][y].seen = 0;	// force prl
		    prl(x, y);
		    nomul(0);
		} else if (_level->l[x][y].typ == SCORR) {
		    if (rn2(7))
			continue;
		    _level->l[x][y].typ = CORR;
		    _level->l[x][y].seen = 0;	// force prl
		    prl(x, y);
		    nomul(0);
		} else {
		    // Be careful not to find anything in an SCORR or SDOOR
		    for (struct trap* trap = _level->traps; trap; trap = trap->ntrap) {
			if (trap->tx == x && trap->ty == y && !trap->tseen && !rn2(8)) {
			    nomul(0);
			    pline("You find a%s.", traps[trap->ttyp]);
			    if (trap->ttyp == PIERC) {
				deltrap(trap);
				makemon(PM_PIERCER, x, y);
				return 1;
			    }
			    trap->tseen = 1;
			    if (!vism_at(x, y))
				atl(x, y, '^');
			}
		    }
		}
	    }
	}
    }
    return 1;
}

int doidtrap(void)
{
    if (!getdir(1))
	return 0;
    int x = _u.ux + _u.dx;
    int y = _u.uy + _u.dy;
    for (const struct trap* trap = _level->traps; trap; trap = trap->ntrap) {
	if (trap->tx == x && trap->ty == y && trap->tseen) {
	    if (_u.dz)
		if ((_u.dz < 0) != (!_level->stair.dn.x && trap->ttyp == TRAPDOOR))
		    continue;
	    pline("That is a%s.", traps[trap->ttyp]);
	    return 0;
	}
    }
    pline("I can't see a trap there.");
    return 0;
}

void wakeup(struct monst *mtmp)
{
    mtmp->msleep = 0;
    setmangry(mtmp);
}
