// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

static const char fl[][16] = {
    "magic missile",
    "bolt of fire",
    "sleep ray",
    "bolt of cold",
    "death ray"
};

// Routines for IMMEDIATE wands.
// bhitm: monster mtmp was hit by the effect of wand otmp
void bhitm(struct monst *mtmp, struct obj *otmp)
{
    wakeup(mtmp);
    switch (otmp->otyp) {
	case WAN_STRIKING:
	    if ((int) rnd(20) < mtmp->data->ac + 10) {
		unsigned r = d(2, 12);
		hit("wand", mtmp, exclam(r));
		mtmp->mhp -= r;
		if (mtmp->mhp < 1)
		    killed(mtmp);
	    } else
		miss("wand", mtmp);
	    break;
	case WAN_SLOW_MONSTER:
	    mtmp->mspeed = MSLOW;
	    break;
	case WAN_SPEED_MONSTER:
	    mtmp->mspeed = MFAST;
	    break;
	case WAN_UNDEAD_TURNING:
	    if (strchr(UNDEAD, mtmp->data->mlet)) {
		mtmp->mhp -= rnd(8);
		if (mtmp->mhp < 1)
		    killed(mtmp);
		else
		    mtmp->mflee = 1;
	    }
	    break;
	case WAN_CANCELLATION:
	    mtmp->mcan = 1;
	    break;
	case WAN_TELEPORTATION:
	    rloc(mtmp);
	    break;
	case WAN_MAKE_INVISIBLE:
	    mtmp->minvis = 1;
	    break;
	default:
	    impossible("What an interesting wand (%u)", otmp->otyp);
    }
}

// object obj was hit by the effect of wand otmp
// returns true if sth was done
int bhito (struct obj *obj, struct obj *otmp)
{
    int res = true;
    switch (otmp->otyp) {
	case WAN_POLYMORPH:
	    // preserve symbol and quantity, but turn rocks into gems
	    mkobj_at((obj->otyp == ROCK || obj->otyp == ENORMOUS_ROCK)
		     ? GEM_SYM : obj->olet, obj->ox, obj->oy)->quan = obj->quan;
	    delobj(obj);
	    break;
	case WAN_STRIKING:
	    if (obj->otyp == ENORMOUS_ROCK)
		fracture_rock(obj);
	    else
		res = false;
	    break;
	case WAN_CANCELLATION:
	    if (obj->spe && obj->olet != AMULET_SYM) {
		obj->known = 0;
		obj->spe = 0;
	    }
	    break;
	case WAN_TELEPORTATION:
	    rloco(obj);
	    break;
	case WAN_MAKE_INVISIBLE:
	    obj->oinvis = 1;
	    break;
	case WAN_UNDEAD_TURNING:
	    res = revive(obj);
	    break;
	case WAN_SLOW_MONSTER:	// no effect on objects
	case WAN_SPEED_MONSTER:
	    res = false;
	    break;
	default:
	    impossible("What an interesting wand (%u)", otmp->otyp);
    }
    return res;
}

int dozap(void)
{
    struct obj *obj;
    int8_t zx, zy;

    obj = getobj("/", "zap");
    if (!obj)
	return 0;
    if (obj->spe < 0 || (obj->spe == 0 && rn2(121))) {
	pline("Nothing Happens.");
	return 1;
    }
    if (obj->spe == 0)
	pline("You wrest one more spell from the worn-out wand.");
    if (!(c_Objects[obj->otyp].bits & NODIR) && !getdir(1))
	return 1;	       // make him pay for knowing !NODIR
    --obj->spe;
    if (c_Objects[obj->otyp].bits & IMMEDIATE) {
	if (_u.dz) {
	    if (_u.dz > 0) {
		struct obj *otmp = o_at(_u.ux, _u.uy);
		if (otmp)
		    bhito(otmp, obj);
	    }
	} else
	    bhit(_u.dx, _u.dy, rn1(8, 6), 0, bhitm, bhito, obj);
    } else {
	switch (obj->otyp) {
	    case WAN_LIGHT:
		litroom(true);
		break;
	    case WAN_SECRET_DOOR_DETECTION:
		if (!findit())
		    return 1;
		break;
	    case WAN_CREATE_MONSTER:
		{
		    unsigned cnt = 1;
		    if (!rn2(23))
			cnt += rn2(7) + 1;
		    while (cnt--)
			makemon (NULL, _u.ux, _u.uy);
		}
		break;
	    case WAN_WISHING:
		{
		    char buf[BUFSZ];
		    if (_u.uluck < -(int)rn2(5)) {
			pline("Unfortunately, nothing happens.");
			break;
		    }
		    pline ("You may wish for an object. What do you want? ");
		    getlin (buf);
		    if (buf[0] == '\033')
			buf[0] = 0;
		    struct obj* o = readobjnam(buf);
		    o = addinv(o);
		    prinv(o);
		    break;
		}
	    case WAN_DIGGING:
		// Original effect (approximately): from CORR: dig
		// until we pierce a wall from ROOM: piece wall and
		// dig until we reach an ACCESSIBLE place. Currently:
		// dig for digdepth positions; also down on request
		// of Lennart Augustsson.
		{
		    struct rm *room;
		    int digdepth;
		    if (_u.dz) {
			if (_u.dz < 0) {
			    pline("You loosen a rock from the ceiling.");
			    pline("It falls on your head!");
			    losehp(1, "falling rock");
			    mksobj_at(ROCK, _u.ux, _u.uy);
			    _level->objects->quan = 1;
			    stackobj(_level->objects);
			    if (Invisible)
				newsym(_u.ux, _u.uy);
			} else {
			    dighole();
			}
			break;
		    }
		    zx = _u.ux + _u.dx;
		    zy = _u.uy + _u.dy;
		    digdepth = 8 + rn2(18);
		    Tmp_at(-1, '*');	// open call
		    while (--digdepth >= 0) {
			if (!isok(zx, zy))
			    break;
			room = &_level->l[zx][zy];
			Tmp_at(zx, zy);
			if (!_level->stair.dn.x) {
			    if (zx < 3 || zx > COLNO - 3 || zy < 3 || zy > ROWNO - 3)
				break;
			    if (room->typ == HWALL || room->typ == VWALL) {
				room->typ = ROOM;
				break;
			    }
			} else if (room->typ == HWALL || room->typ == VWALL || room->typ == SDOOR || room->typ == LDOOR) {
			    room->typ = DOOR;
			    digdepth -= 2;
			} else if (room->typ == SCORR || !room->typ) {
			    room->typ = CORR;
			    --digdepth;
			}
			mnewsym(zx, zy);
			zx += _u.dx;
			zy += _u.dy;
		    }
		    mnewsym(zx, zy);	// not always necessary
		    Tmp_at(-1, -1);	// closing call
		    break;
		}
	    default:
		buzz((int) obj->otyp - WAN_MAGIC_MISSILE, _u.ux, _u.uy, _u.dx, _u.dy);
		break;
	}
	if (!is_object_known (obj->otyp)) {
	    set_object_known (obj->otyp);
	    more_experienced(0, 10);
	}
    }
    return 1;
}

const char *exclam(int force)
{
    // force == 0 occurs e.g. with sleep ray
    // note that large force is usual with wands so that !! would require
    // information about hand/weapon/wand
    return (force < 0) ? "?" : (force <= 4) ? "." : "!";
}

// force is usually either "." or "!"
void hit(const char *str, struct monst *mtmp, const char *force)
{
    if (!cansee(mtmp->mx, mtmp->my))
	pline("The %s hits it.", str);
    else
	pline("The %s hits %s%s", str, monnam(mtmp), force);
}

void miss(const char *str, struct monst *mtmp)
{
    if (!cansee(mtmp->mx, mtmp->my))
	pline("The %s misses it.", str);
    else
	pline("The %s misses %s.", str, monnam(mtmp));
}

//
// bhit: called when a weapon is thrown (sym = obj->olet) or when an
// IMMEDIATE wand is zapped (sym = 0); the weapon falls down at end of range
// or when a monster is hit; the monster is returned, and bhitpos is set to
// the final position of the weapon thrown; the ray of a wand may affect
// several objects and monsters on its path - for each of these an argument
// function is called.
// range - direction and range
// sym - symbol displayed on path
// obj - 2nd arg to fhitm/fhito
struct monst *bhit (int ddx, int ddy, int range, int sym, void (*fhitm) (struct monst *, struct obj *), int (*fhito) (struct obj *, struct obj *), struct obj *obj)
{
    struct monst *mtmp;
    struct obj *otmp;
    int typ;

    bhitpos.x = _u.ux;
    bhitpos.y = _u.uy;

    if (sym)
	tmp_at(-1, sym);       // open call
    while (range-- > 0) {
	bhitpos.x += ddx;
	bhitpos.y += ddy;
	typ = _level->l[bhitpos.x][bhitpos.y].typ;
	if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != NULL) {
	    if (sym) {
		tmp_at(-1, -1);	// close call
		return mtmp;
	    }
	    (*fhitm) (mtmp, obj);
	    range -= 3;
	}
	if (fhito && (otmp = o_at(bhitpos.x, bhitpos.y))) {
	    if ((*fhito) (otmp, obj))
		--range;
	}
	if (!ZAP_POS(typ)) {
	    bhitpos.x -= ddx;
	    bhitpos.y -= ddy;
	    break;
	}
	if (sym)
	    tmp_at(bhitpos.x, bhitpos.y);
    }

    // leave last symbol unless in a pool
    if (sym)
	tmp_at(-1, (_level->l[bhitpos.x][bhitpos.y].typ == POOL) ? -1 : 0);
    return 0;
}

struct monst *boomhit(int dx, int dy)
{
    int i, ct;
    struct monst *mtmp;
    char sym = ')';

    bhitpos.x = _u.ux;
    bhitpos.y = _u.uy;

    for (i = 0; i < 8; ++i)
	if (xdir[i] == dx && ydir[i] == dy)
	    break;
    tmp_at(-1, sym);	       // open call
    for (ct = 0; ct < 10; ++ct) {
	if (i == 8)
	    i = 0;
	sym = ')' + '(' - sym;
	tmp_at(-2, sym);       // change let call
	dx = xdir[i];
	dy = ydir[i];
	bhitpos.x += dx;
	bhitpos.y += dy;
	if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != NULL) {
	    tmp_at(-1, -1);
	    return mtmp;
	}
	if (!ZAP_POS(_level->l[bhitpos.x][bhitpos.y].typ)) {
	    bhitpos.x -= dx;
	    bhitpos.y -= dy;
	    break;
	}
	if (bhitpos.x == _u.ux && bhitpos.y == _u.uy) {	// ct == 9
	    if (rn2(20) >= _u.ulevel+10u) {	// we hit ourselves
		thitu(10, rnd(10), "boomerang");
		break;
	    } else {	       // we catch it
		tmp_at(-1, -1);
		pline("Skillfully, you catch the boomerang.");
		return &youmonst;
	    }
	}
	tmp_at(bhitpos.x, bhitpos.y);
	if (ct % 5 != 0)
	    ++i;
    }
    tmp_at(-1, -1);	       // do not leave last symbol
    return 0;
}

char dirlet(int dx, int dy)
{
    return dx == dy ? '\\' : (dx && dy) ? '/' : dx ? '-' : '|';
}

// type == -1: monster spitting fire at you
// type == -1,-2,-3: bolts sent out by wizard
// called with dx = dy = 0 with vertical bolts
void buzz(int type, int sx, int sy, int dx, int dy)
{
    int abstype = abs(type);
    const char *fltxt = (type == -1) ? "blaze of fire" : fl[abstype];
    struct rm *lev;
    int8_t range;
    struct monst *mon;

    if (type < 0)
	pru();
    range = rn1(7, 7);
    Tmp_at(-1, dirlet(dx, dy));	// open call
    while (range-- > 0) {
	sx += dx;
	sy += dy;
	if ((lev = &_level->l[sx][sy])->typ)
	    Tmp_at(sx, sy);
	else {
	    int bounce = 0;
	    if (cansee(sx - dx, sy - dy))
		pline("The %s bounces!", fltxt);
	    if (ZAP_POS(_level->l[sx][sy - dy].typ))
		bounce = 1;
	    if (ZAP_POS(_level->l[sx - dx][sy].typ)) {
		if (!bounce || rn2(2))
		    bounce = 2;
	    }
	    switch (bounce) {
		case 0:
		    dx = -dx;
		    dy = -dy;
		    continue;
		case 1:
		    dy = -dy;
		    sx -= dx;
		    break;
		case 2:
		    dx = -dx;
		    sy -= dy;
		    break;
	    }
	    Tmp_at(-2, dirlet(dx, dy));
	    continue;
	}
	if (lev->typ == POOL && abstype == 1) {	// fire
	    range -= 3;
	    lev->typ = ROOM;
	    if (cansee(sx, sy)) {
		mnewsym(sx, sy);
		pline("The water evaporates.");
	    } else
		pline("You hear a hissing sound.");
	}
	if ((mon = m_at(sx, sy)) && (type != -1 || mon->data->mlet != 'D')) {
	    wakeup(mon);
	    if ((int) rnd(20) < mon->data->ac+18) {
		int tmp = zhit(mon, abstype);
		if (mon->mhp < 1) {
		    if (type < 0) {
			if (cansee(mon->mx, mon->my))
			    pline("%s is killed by the %s!", Monnam(mon), fltxt);
			mondied(mon);
		    } else
			killed(mon);
		} else
		    hit(fltxt, mon, exclam(tmp));
		range -= 2;
	    } else
		miss(fltxt, mon);
	} else if (sx == _u.ux && sy == _u.uy) {
	    nomul(0);
	    if ((int) rnd(20) < _u.uac+18) {
		int dam = 0;
		range -= 2;
		pline("The %s hits you!", fltxt);
		switch (abstype) {
		    case 0:
			dam = d(2, 6);
			break;
		    case 1:
			if (Fire_resistance)
			    pline("You don't feel hot!");
			else
			    dam = d(6, 6);
			if (!rn2(3))
			    burn_scrolls();
			break;
		    case 2:
			nomul(-rnd(25));	// sleep ray
			break;
		    case 3:
			if (Cold_resistance)
			    pline("You don't feel cold!");
			else
			    dam = d(6, 6);
			break;
		    case 4:
			_u.uhp = -1;
		}
		losehp(dam, fltxt);
	    } else
		pline("The %s whizzes by you!", fltxt);
	    stop_occupation();
	}
	if (!ZAP_POS(lev->typ)) {
	    int bounce = 0, rmn;
	    if (cansee(sx, sy))
		pline("The %s bounces!", fltxt);
	    --range;
	    if (!dx || !dy || !rn2(20)) {
		dx = -dx;
		dy = -dy;
	    } else {
		if (ZAP_POS(rmn = _level->l[sx][sy - dy].typ) && (IS_ROOM(rmn) || ZAP_POS(_level->l[sx + dx][sy - dy].typ)))
		    bounce = 1;
		if (ZAP_POS(rmn = _level->l[sx - dx][sy].typ) && (IS_ROOM(rmn) || ZAP_POS(_level->l[sx - dx][sy + dy].typ)))
		    if (!bounce || rn2(2))
			bounce = 2;

		switch (bounce) {
		    case 0:
			dy = -dy;
			dx = -dx;
			break;
		    case 1:
			dy = -dy;
			break;
		    case 2:
			dx = -dx;
			break;
		}
		Tmp_at(-2, dirlet(dx, dy));
	    }
	}
    }
    Tmp_at(-1, -1);
}

// returns damage to mon
int zhit (struct monst *mon, int type)
{
    int tmp = 0;

    switch (type) {
	case 0:	       // magic missile
	    tmp = d(2, 6);
	    break;
	case -1:	       // Dragon blazing fire
	case 1:	       // fire
	    if (strchr("Dg", mon->data->mlet))
		break;
	    tmp = d(6, 6);
	    if (strchr("YF", mon->data->mlet))
		tmp += 7;
	    break;
	case 2:	       // sleep
	    mon->mfroz = 1;
	    break;
	case 3:	       // cold
	    if (strchr("YFgf", mon->data->mlet))
		break;
	    tmp = d(6, 6);
	    if (mon->data->mlet == 'D')
		tmp += 7;
	    break;
	case 4:	       // death
	    if (strchr(UNDEAD, mon->data->mlet))
		break;
	    tmp = mon->mhp + 1;
	    break;
    }
    mon->mhp -= tmp;
    return tmp;
}

#define	CORPSE_I_TO_C(otyp)	(char) ((otyp >= DEAD_ACID_BLOB)\
		     ?  'a' + (otyp - DEAD_ACID_BLOB)\
		     :	'@' + (otyp - DEAD_HUMAN))
int revive(struct obj *obj)
{
    struct monst *mtmp = NULL;

    if (obj->olet == FOOD_SYM && obj->otyp > CORPSE) {
	// do not (yet) revive shopkeepers
	// Note: this might conceivably produce two monsters at the
	// same position - strange, but harmless
	mtmp = mkmon_at(CORPSE_I_TO_C(obj->otyp), obj->ox, obj->oy);
	delobj(obj);
    }
    return ! !mtmp;	       // true if some monster created
}

void rloco(struct obj *obj)
{
    int tx, ty, otx, oty;

    otx = obj->ox;
    oty = obj->oy;
    do {
	tx = rn1(COLNO - 3, 2);
	ty = rn2(ROWNO);
    } while (!goodpos(tx, ty));
    obj->ox = tx;
    obj->oy = ty;
    if (cansee(otx, oty))
	newsym(otx, oty);
}

// fractured by pick-axe or wand of striking
// no texts here!
void fracture_rock (struct obj *obj)
{
    // unpobj(obj);
    obj->otyp = ROCK;
    obj->quan = 7 + rn2(60);
    obj->owt = weight(obj);
    obj->olet = WEAPON_SYM;
    if (cansee(obj->ox, obj->oy))
	prl(obj->ox, obj->oy);
}

void burn_scrolls(void)
{
    struct obj *obj, *obj2;
    int cnt = 0;

    for (obj = invent; obj; obj = obj2) {
	obj2 = obj->nobj;
	if (obj->olet == SCROLL_SYM) {
	    ++cnt;
	    useup(obj);
	}
    }
    if (cnt > 1) {
	pline("Your scrolls catch fire!");
	losehp(cnt, "burning scrolls");
    } else if (cnt) {
	pline("Your scroll catches fire!");
	losehp(1, "burning scroll");
    }
}
