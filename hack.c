// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

// called when teleporting or when walking out of a lit room
void unsee(void)
{
    if (seehx)
	seehx = 0;
    else {
	for (int x = _u.ux - 1; x < _u.ux + 2; ++x) {
	    for (int y = _u.uy - 1; y < _u.uy + 2; ++y) {
		if (!isok(x, y))
		    continue;
		struct rm* lev = &_level->l[x][y];
		if (!lev->lit && lev->scrsym == '.') {
		    lev->scrsym = ' ';
		    lev->new = 1;
		    on_scr(x, y);
		}
	    }
	}
    }
}

// called: in hack.eat.c: seeoff(0) - blind after eating rotten food in
// hack.mon.c: seeoff(0) - blinded by a yellow light in hack.mon.c: seeoff(1)
// - seeoff(0) - blind after drinking potion in
// hack.do.c:  seeoff(1) - go up or down the stairs in hack.trap.c:seeoff(1)
// - fall through trapdoor
// mode 1 to redo @, 0 to leave them
// 1 means misc movement, 0 means blindness
void seeoff (int mode)
{
    int x, y;
    struct rm *lev;

    if (_u.udispl && mode) {
	_u.udispl = 0;
	_level->l[_u.udisx][_u.udisy].scrsym = news0(_u.udisx, _u.udisy);
    }
    if (seehx)
	seehx = 0;
    else if (!mode) {
	for (x = _u.ux - 1; x < _u.ux + 2; ++x)
	    for (y = _u.uy - 1; y < _u.uy + 2; ++y) {
		if (!isok(x, y))
		    continue;
		lev = &_level->l[x][y];
		if (!lev->lit && lev->scrsym == '.')
		    lev->seen = 0;
	    }
    }
}

void domove(void)
{
    int8_t oldx, oldy;
    struct monst *mtmp = NULL;
    struct rm *tmpr, *ust;
    struct trap *trap = NULL;
    struct obj *otmp = NULL;

    u_wipe_engr(rnd(5));

    if (inv_weight() > 0) {
	pline("You collapse under your load.");
	nomul(0);
	return;
    }
    if (Confusion) {
	do {
	    confdir();
	} while (!isok(_u.ux + _u.dx, _u.uy + _u.dy) || IS_ROCK(_level->l[_u.ux + _u.dx][_u.uy + _u.dy].typ));
    }
    if (!isok(_u.ux + _u.dx, _u.uy + _u.dy)) {
	nomul(0);
	return;
    }

    ust = &_level->l[_u.ux][_u.uy];
    oldx = _u.ux;
    oldy = _u.uy;
    if ((trap = t_at(_u.ux + _u.dx, _u.uy + _u.dy)) && trap->tseen)
	nomul(0);
    if ((mtmp = m_at(_u.ux + _u.dx, _u.uy + _u.dy))) {
	// attack monster
	nomul(0);
	gethungry();
	if (multi < 0)
	    return;	       // we just fainted

	// try to attack; note that it might evade
	if (attack (mtmp))
	    return;
    }
    // not attacking an animal, so we try to move
    if (_u.utrap) {
	if (_u.utraptype == TT_PIT) {
	    pline("You are still in a pit.");
	    --_u.utrap;
	} else {
	    pline("You are caught in a beartrap.");
	    if ((_u.dx && _u.dy) || !rn2(5))
		--_u.utrap;
	}
	return;
    }
    tmpr = &_level->l[_u.ux + _u.dx][_u.uy + _u.dy];
    if (IS_ROCK(tmpr->typ) || (_u.dx && _u.dy && (tmpr->typ == DOOR || ust->typ == DOOR))) {
	_wflags.move = 0;
	nomul(0);
	return;
    }
    while ((otmp = sobj_at(ENORMOUS_ROCK, _u.ux + _u.dx, _u.uy + _u.dy)) != NULL) {
	int8_t rx = _u.ux + 2 * _u.dx, ry = _u.uy + 2 * _u.dy;
	struct trap *ttmp;
	nomul(0);
	if (isok(rx, ry) && !IS_ROCK(_level->l[rx][ry].typ) && (_level->l[rx][ry].typ != DOOR || !(_u.dx && _u.dy)) && !sobj_at(ENORMOUS_ROCK, rx, ry)) {
	    if (m_at(rx, ry)) {
		pline("You hear a monster behind the rock.");
		pline("Perhaps that's why you cannot move it.");
		goto cannot_push;
	    }
	    if ((ttmp = t_at(rx, ry)) != NULL)
		switch (ttmp->ttyp) {
		    case PIT:
			pline("You push the rock into a pit!");
			deltrap(ttmp);
			delobj(otmp);
			pline("It completely fills the pit!");
			continue;
		    case TELEP_TRAP:
			pline("You push the rock and suddenly it disappears!");
			delobj(otmp);
			continue;
		}
	    if (_level->l[rx][ry].typ == POOL) {
		_level->l[rx][ry].typ = ROOM;
		mnewsym(rx, ry);
		prl(rx, ry);
		pline("You push the rock into the water.");
		pline("Now you can cross the water!");
		delobj(otmp);
		continue;
	    }
	    otmp->ox = rx;
	    otmp->oy = ry;
	    // pobj(otmp);
	    if (cansee(rx, ry))
		atl(rx, ry, otmp->olet);
	    if (Invisible)
		newsym(_u.ux + _u.dx, _u.uy + _u.dy);

	    {
		static long lastmovetime;
		// note: this var contains garbage initially and after a restore
		if (_u.moves > lastmovetime + 2 || _u.moves < lastmovetime)
		    pline("With great effort you move the enormous rock.");
		lastmovetime = _u.moves;
	    }
	} else {
	    pline("You try to move the enormous rock, but in vain.");
	  cannot_push:
	    if ((!invent || inv_weight() + 90 <= 0) && (!_u.dx || !_u.dy || (IS_ROCK(_level->l[_u.ux][_u.uy + _u.dy].typ)
									   && IS_ROCK(_level->l[_u.ux + _u.dx][_u.uy].typ)))) {
		pline("However, you can squeeze yourself into a small opening.");
		break;
	    } else
		return;
	}
    }
    if (_u.dx && _u.dy && IS_ROCK(_level->l[_u.ux][_u.uy + _u.dy].typ) && IS_ROCK(_level->l[_u.ux + _u.dx][_u.uy].typ) && invent && inv_weight() + 40 > 0) {
	pline("You are carrying too much to get through.");
	nomul(0);
	return;
    }
    _u.ux += _u.dx;
    _u.uy += _u.dy;
    if (_wflags.run) {
	if (tmpr->typ == DOOR || (_level->stair.up.x == _u.ux && _level->stair.up.y == _u.uy) || (_level->stair.dn.x == _u.ux && _level->stair.dn.y == _u.uy))
	    nomul(0);
    }
    if (tmpr->typ == POOL && !Levitation)
	drown();	       // not necessarily fatal

    // if(_u.udispl) {
    // _u.udispl = 0;
    // newsym(oldx,oldy);
    // }
    if (!Blind) {
	if (ust->lit) {
	    if (tmpr->lit) {
		if (tmpr->typ == DOOR)
		    prl1(_u.ux + _u.dx, _u.uy + _u.dy);
		else if (ust->typ == DOOR)
		    nose1(oldx - _u.dx, oldy - _u.dy);
	    } else {
		unsee();
		prl1(_u.ux + _u.dx, _u.uy + _u.dy);
	    }
	} else {
	    if (tmpr->lit)
		setsee();
	    else {
		prl1(_u.ux + _u.dx, _u.uy + _u.dy);
		if (tmpr->typ == DOOR) {
		    if (_u.dy) {
			prl(_u.ux - 1, _u.uy);
			prl(_u.ux + 1, _u.uy);
		    } else {
			prl(_u.ux, _u.uy - 1);
			prl(_u.ux, _u.uy + 1);
		    }
		}
	    }
	    nose1(oldx - _u.dx, oldy - _u.dy);
	}
    } else {
	pru();
    }
    if (!_wflags.nopick)
	pickup(1);
    if (trap)
	dotrap(trap);	       // fall into pit, arrow trap, etc.
    inshop();
    if (!Blind)
	read_engr_at(_u.ux, _u.uy);
}

void movobj (struct obj* o, int ox, int oy)
{
    // Some dirty programming to get display right
    freeobj (o);
    unpobj (o);
    o->nobj = _level->objects;
    _level->objects = o;
    o->ox = ox;
    o->oy = oy;
}

int dopickup(void)
{
    if (!g_at(_u.ux, _u.uy) && !o_at(_u.ux, _u.uy)) {
	pline("There is nothing here to pick up.");
	return 0;
    }
    if (Levitation) {
	pline("You cannot reach the floor.");
	return 1;
    }
    pickup(0);
    return 1;
}

void pickup(int all)
{
    struct gold *gold;
    struct obj *obj, *obj2;
    int wt;

    if (Levitation)
	return;
    while ((gold = g_at(_u.ux, _u.uy)) != NULL) {
	pline("%ld gold piece%s.", gold->amount, plur(gold->amount));
	_u.ugold += gold->amount;
	_wflags.botl = 1;
	freegold(gold);
	if (_wflags.run)
	    nomul(0);
	if (Invisible)
	    newsym(_u.ux, _u.uy);
    }

    // check for more than one object
    if (!all) {
	int ct = 0;

	for (obj = _level->objects; obj; obj = obj->nobj)
	    if (obj->ox == _u.ux && obj->oy == _u.uy)
		++ct;
	if (ct < 2)
	    ++all;
	else
	    pline("There are several objects here.");
    }
    for (obj = _level->objects; obj; obj = obj2) {
	obj2 = obj->nobj;      // perhaps obj will be picked up
	if (obj->ox == _u.ux && obj->oy == _u.uy) {
	    if (_wflags.run)
		nomul(0);

	    if (!all) {
		char c;

		pline("Pick up %s ? [ynaq]", doname(obj));
		while (!strchr("ynaq ", (c = readchar())))
		    bell();
		if (c == 'q')
		    return;
		if (c == 'n')
		    continue;
		if (c == 'a')
		    all = 1;
	    }
	    if (obj->otyp == DEAD_COCKATRICE && !uarmg) {
		pline("Touching the dead cockatrice is a fatal mistake.");
		pline("You turn to stone.");
		killer = "cockatrice cadaver";
		done("died");
	    }
	    if (obj->otyp == SCR_SCARE_MONSTER) {
		if (!obj->spe)
		    obj->spe = 1;
		else {
		    // Note: perhaps the 1st pickup
		    // failed: you cannot carry anymore,
		    // and so we never dropped it - let's
		    // assume that treading on it twice
		    // also destroys the scroll
		    pline("The scroll turns to dust as you pick it up.");
		    delobj(obj);
		    continue;
		}
	    }
	    wt = inv_weight() + obj->owt;
	    if (wt > 0) {
		if (obj->quan > 1) {
		    // see how many we can lift
		    int savequan = obj->quan;
		    int iw = inv_weight();
		    int qq;
		    for (qq = 1; qq < savequan; ++qq) {
			obj->quan = qq;
			if (iw + weight(obj) > 0)
			    break;
		    }
		    obj->quan = savequan;
		    --qq;
		    // we can carry qq of them
		    if (!qq)
			goto too_heavy;
		    pline("You can only carry %s of the %s lying here.", (qq == 1) ? "one" : "some", doname(obj));
		    splitobj (obj, qq);
		    // note: obj2 is set already, so
		    // we'll never encounter the other
		    // half; if it should be otherwise
		    // then write obj2 =
		    // splitobj(obj,qq);
		    goto lift_some;
		}
	      too_heavy:
		pline("There %s %s here, but %s.", (obj->quan == 1) ? "is" : "are", doname(obj), !invent ? "it is too heavy for you to lift" : "you cannot carry anymore");
		break;
	    }
	  lift_some:
	    if (inv_cnt() >= 52) {
		pline("Your knapsack cannot accomodate anymore items.");
		break;
	    }
	    if (wt > -5)
		pline("You have a little trouble lifting");
	    freeobj(obj);
	    if (Invisible)
		newsym(_u.ux, _u.uy);
	    addtobill(obj);    // sets obj->unpaid if necessary
	    {
		int pickquan = obj->quan;
		int mergquan;
		if (!Blind)
		    obj->dknown = 1;	// this is done by
					// prinv(), but addinv()
					// needs it already for
					// merging
		obj = addinv(obj);	// might merge it with
					// other objects
		mergquan = obj->quan;
		obj->quan = pickquan;	// to fool prinv()
		prinv(obj);
		obj->quan = mergquan;
	    }
	}
    }
}

// stop running if we see something interesting
// turn around a corner if that is the only way we can proceed
// do not turn left or right twice
void lookaround(void)
{
    int x, y, i, x0 = 0, y0 = 0, m0 = 0, i0 = 9;
    int corrct = 0, noturn = 0;
    struct monst *mtmp;
    if (Blind || _wflags.run == 0)
	return;
    if (_wflags.run == 1 && _level->l[_u.ux][_u.uy].typ == ROOM)
	return;
    for (x = _u.ux - 1; x <= _u.ux + 1; ++x)
	for (y = _u.uy - 1; y <= _u.uy + 1; ++y) {
	    if (x == _u.ux && y == _u.uy)
		continue;
	    if (!_level->l[x][y].typ)
		continue;
	    if ((mtmp = m_at(x, y)) && (!mtmp->minvis || See_invisible)) {
		if (x == _u.ux + _u.dx && y == _u.uy + _u.dy)
		    goto stop;
	    } else
		mtmp = 0;      // invisible M cannot
			       // influence us
	    if (x == _u.ux - _u.dx && y == _u.uy - _u.dy)
		continue;
	    switch (_level->l[x][y].scrsym) {
		case '|':
		case '-':
		case '.':
		case ' ':
		    break;
		case '+':
		    if (x != _u.ux && y != _u.uy)
			break;
		    if (_wflags.run != 1)
			goto stop;
		    // fallthrough
		case CORR_SYM:
		  corr:
		    if (_wflags.run == 1 || _wflags.run == 3) {
			i = DIST(x, y, _u.ux + _u.dx, _u.uy + _u.dy);
			if (i > 2)
			    break;
			if (corrct == 1 && DIST(x, y, x0, y0) != 1)
			    noturn = 1;
			if (i < i0) {
			    i0 = i;
			    x0 = x;
			    y0 = y;
			    m0 = mtmp ? 1 : 0;
			}
		    }
		    ++corrct;
		    break;
		case '^':
		    if (_wflags.run == 1)
			goto corr;	// if you must
		    if (x == _u.ux + _u.dx && y == _u.uy + _u.dy)
			goto stop;
		    break;
		default:      // e.g. objects or trap or stairs
		    if (_wflags.run == 1)
			goto corr;
		    if (mtmp)
			break; // d
		  stop:
		    nomul(0);
		    return;
	    }
	}
    if (corrct > 1 && _wflags.run == 2)
	goto stop;
    if ((_wflags.run == 1 || _wflags.run == 3) && !noturn && !m0 && i0 && (corrct == 1 || (corrct == 2 && i0 == 1))) {
	// make sure that we do not turn too far
	if (i0 == 2) {
	    if (_u.dx == y0 - _u.uy && _u.dy == _u.ux - x0)
		i = 2;	       // straight turn right
	    else
		i = -2;	       // straight turn left
	} else if (_u.dx && _u.dy) {
	    if ((_u.dx == _u.dy && y0 == _u.uy) || (_u.dx != _u.dy && y0 != _u.uy))
		i = -1;	       // half turn left
	    else
		i = 1;	       // half turn right
	} else {
	    if ((x0 - _u.ux == y0 - _u.uy && !_u.dy) || (x0 - _u.ux != y0 - _u.uy && _u.dy))
		i = 1;	       // half turn right
	    else
		i = -1;	       // half turn left
	}
	i += _u.last_str_turn;
	if (i <= 2 && i >= -2) {
	    _u.last_str_turn = i;
	    _u.dx = x0 - _u.ux, _u.dy = y0 - _u.uy;
	}
    }
}

// something like lookaround, but we are not running
// react only to monsters that might hit us
int monster_nearby(void)
{
    if (Blind)
	return 0;
    for (int x = _u.ux - 1; x <= _u.ux + 1; ++x) {
	for (int y = _u.uy - 1; y <= _u.uy + 1; ++y) {
	    if (x == _u.ux && y == _u.uy)
		continue;
	    struct monst* mtmp = m_at(x,y);
	    if (mtmp && !mtmp->mpeaceful
		&& !strchr("Ea", mtmp->data->mlet)
		&& !mtmp->mfroz && !mtmp->msleep
		&& (!mtmp->minvis || See_invisible))
		return 1;
	}
    }
    return 0;
}

int cansee(int x, int y)
{
    if (Blind)
	return 0;
    if (dist(x, y) < 3)
	return 1;
    if (_level->l[x][y].lit && seelx <= x && x <= seehx && seely <= y && y <= seehy)
	return 1;
    return 0;
}

int sgn(int a)
{
    return (a > 0) ? 1 : (a == 0) ? 0 : -1;
}

void setsee(void)
{
    int x, y;

    if (Blind) {
	pru();
	return;
    }
    if (!_level->l[_u.ux][_u.uy].lit) {
	seelx = _u.ux - 1;
	seehx = _u.ux + 1;
	seely = _u.uy - 1;
	seehy = _u.uy + 1;
    } else {
	for (seelx = _u.ux; _level->l[seelx - 1][_u.uy].lit; --seelx) {}
	for (seehx = _u.ux; _level->l[seehx + 1][_u.uy].lit; ++seehx) {}
	for (seely = _u.uy; _level->l[_u.ux][seely - 1].lit; --seely) {}
	for (seehy = _u.uy; _level->l[_u.ux][seehy + 1].lit; ++seehy) {}
    }
    for (y = seely; y <= seehy; ++y)
	for (x = seelx; x <= seehx; ++x) {
	    prl(x, y);
	}
    if (!_level->l[_u.ux][_u.uy].lit)
	seehx = 0;	       // seems necessary elsewhere
    else {
	if (seely == _u.uy)
	    for (x = _u.ux - 1; x <= _u.ux + 1; ++x)
		prl(x, seely - 1);
	if (seehy == _u.uy)
	    for (x = _u.ux - 1; x <= _u.ux + 1; ++x)
		prl(x, seehy + 1);
	if (seelx == _u.ux)
	    for (y = _u.uy - 1; y <= _u.uy + 1; ++y)
		prl(seelx - 1, y);
	if (seehx == _u.ux)
	    for (y = _u.uy - 1; y <= _u.uy + 1; ++y)
		prl(seehx + 1, y);
    }
}

void nomul(int nval)
{
    if (multi < 0)
	return;
    multi = nval;
    _wflags.mv = _wflags.run = 0;
}

int abon(void)
{
    if (_u.ustr == 3)
	return -3;
    else if (_u.ustr < 6)
	return -2;
    else if (_u.ustr < 8)
	return -1;
    else if (_u.ustr < 17)
	return 0;
    else if (_u.ustr < 69)
	return 1;	       // up to 18/50
    else if (_u.ustr < 118)
	return 2;
    else
	return 3;
}

int dbon(void)
{
    if (_u.ustr < 6)
	return -1;
    else if (_u.ustr < 16)
	return 0;
    else if (_u.ustr < 18)
	return 1;
    else if (_u.ustr == 18)
	return 2;	       // up to 18
    else if (_u.ustr < 94)
	return 3;	       // up to 18/75
    else if (_u.ustr < 109)
	return 4;	       // up to 18/90
    else if (_u.ustr < 118)
	return 5;	       // up to 18/99
    else
	return 6;
}

void losestr(			// may kill you; cause may be poison or
		int num		// monster like 'A'
    ) {
    _u.ustr -= num;
    while (_u.ustr < 3) {
	++_u.ustr;
	_u.uhp -= 6;
	_u.uhpmax -= 6;
    }
    _wflags.botl = 1;
}

void losehp(int n, const char *knam)
{
    _u.uhp -= n;
    if (_u.uhp > _u.uhpmax)
	_u.uhpmax = _u.uhp;      // perhaps n was negative
    _wflags.botl = 1;
    if (_u.uhp < 1) {
	killer = knam;	       // the thing that killed you
	done("died");
    }
}

void losehp_m(int n, struct monst *mtmp)
{
    _u.uhp -= n;
    _wflags.botl = 1;
    if (_u.uhp < 1)
	done_in_by(mtmp);
}

void losexp(void)
{			       // hit by V or W
    int num;

    if (_u.ulevel > 1)
	pline("Goodbye level %u.", _u.ulevel--);
    else
	_u.uhp = -1;
    num = rnd(10);
    _u.uhp -= num;
    _u.uhpmax -= num;
    _u.uexp = newuexp();
    _wflags.botl = 1;
}

int inv_weight(void)
{
    struct obj *otmp = invent;
    int wt = (_u.ugold + 500) / 1000;
    int carrcap;
    if (Levitation)	       // pugh@cornell
	carrcap = MAX_CARR_CAP;
    else {
	carrcap = 5 * (((_u.ustr > 18) ? 20 : _u.ustr) + _u.ulevel);
	if (carrcap > MAX_CARR_CAP)
	    carrcap = MAX_CARR_CAP;
	if (Wounded_legs & LEFT_SIDE)
	    carrcap -= 10;
	if (Wounded_legs & RIGHT_SIDE)
	    carrcap -= 10;
    }
    while (otmp) {
	wt += otmp->owt;
	otmp = otmp->nobj;
    }
    return wt - carrcap;
}

int inv_cnt(void)
{
    struct obj *otmp = invent;
    int ct = 0;
    while (otmp) {
	++ct;
	otmp = otmp->nobj;
    }
    return ct;
}

long newuexp(void)
{
    return 10 * (1L << (_u.ulevel - 1));
}
