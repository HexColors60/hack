// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

static int drop(struct obj *);

int dodrop(void)
{
    return drop(getobj("0$#", "drop"));
}

static int drop(struct obj* obj)
{
    if (!obj)
	return 0;
    if (obj->olet == '$') {    // pseudo object
	unsigned amount = obj->ogold;
	if (!amount)
	    pline("You didn't drop any gold pieces.");
	else {
	    mkgold (amount, _u.ux, _u.uy);
	    pline ("You dropped %u gold piece%s.", amount, plur(amount));
	    if (Invisible)
		newsym (_u.ux, _u.uy);
	}
	free (obj);
	return 1;
    }
    if (obj->owornmask & (W_ARMOR | W_RING)) {
	pline("You cannot drop something you are wearing.");
	return 0;
    }
    if (obj == uwep) {
	if (uwep->cursed) {
	    pline("Your weapon is welded to your hand!");
	    return 0;
	}
	setuwep (NULL);
    }
    pline("You dropped %s.", doname(obj));
    dropx(obj);
    return 1;
}

// Called in several places - should not produce texts
void dropx(struct obj *obj)
{
    freeinv(obj);
    dropy(obj);
}

void dropy(struct obj *obj)
{
    obj->ox = _u.ux;
    obj->oy = _u.uy;
    obj->nobj = _level->objects;
    _level->objects = obj;
    if (Invisible)
	newsym(_u.ux, _u.uy);
    subfrombill(obj);
    stackobj(obj);
}

// drop several things
int doddrop(void)
{
    return ggetobj("drop", drop, 0);
}

int dodown(void)
{
    if (_u.ux != _level->stair.dn.x || _u.uy != _level->stair.dn.y) {
	pline("You can't go down here.");
	return 0;
    }
    if (Levitation) {
	pline("You're floating high above the stairs.");
	return 0;
    }
    goto_level(_u.dlevel + 1, true);
    return 1;
}

int doup(void)
{
    if (_u.ux != _level->stair.up.x || _u.uy != _level->stair.up.y) {
	pline("You can't go up here.");
	return 0;
    }
    if (!Levitation && inv_weight() + 5 > 0) {
	pline("Your load is too heavy to climb the stairs.");
	return 1;
    }
    goto_level(_u.dlevel - 1, true);
    return 1;
}

void goto_level (unsigned newlevel, bool at_stairs)
{
    bool up = (newlevel < _u.dlevel);

    if (!newlevel)
	done("escaped");       // in fact < 0 is impossible
    if (newlevel > MAXLEVEL)
	newlevel = MAXLEVEL;   // strange ...
    if (newlevel == _u.dlevel)
	return;		       // this can happen

    _u.utrap = 0;	       // needed in level_tele
    seeoff(1);
    _wflags.nscrinh = 1;
    _u.ux = FAR;		       // hack
    inshop();	       // probably was a trapdoor
    _level->lastvisit = _u.moves;

    _u.dlevel = newlevel;
    _level = &_levels[_u.dlevel-1];
    if (_u.dlevel > _u.maxdlevel) {
	_u.maxdlevel = _u.dlevel;
	mklev();
    }

    if (at_stairs) {
	if (up) {
	    _u.ux = _level->stair.dn.x;
	    _u.uy = _level->stair.dn.y;
	    if (!_u.ux) {			// entering a maze from below?
		_u.ux = _level->stair.up.x;	// this will confuse the player!
		_u.uy = _level->stair.up.y;
	    }
	} else {
	    _u.ux = _level->stair.up.x;
	    _u.uy = _level->stair.up.y;
	    if (inv_weight() + 5 > 0) {
		pline("You fall down the stairs.");	// %%
		losehp(rnd(3), "fall");
		selftouch("Falling, you");
	    }
	}
	{
	    struct monst *mtmp = m_at(_u.ux, _u.uy);
	    if (mtmp)
		mnexto(mtmp);
	}
    } else {		       // trapdoor or level_tele
	do {
	    _u.ux = rnd(COLNO - 1);
	    _u.uy = rn2(ROWNO);
	} while (_level->l[_u.ux][_u.uy].typ != ROOM || m_at(_u.ux, _u.uy));
	selftouch("Falling, you");
    }
    inshop();
    initrack();

    {
	struct monst *mtmp;
	if ((mtmp = m_at(_u.ux, _u.uy)) != NULL)
	    mnexto(mtmp);      // riv05!a3
    }
    _wflags.nscrinh = 0;
    setsee();
    seeobjs();		       // make old cadavers disappear - riv05!a3
    docrt();
    pickup(1);
    read_engr_at(_u.ux, _u.uy);
}

int donull(void)
{
    return 1;		       // Do nothing, but let other things happen
}

int dopray(void)
{
    nomovemsg = "You finished your prayer.";
    nomul(-3);
    return 1;
}

int dothrow(void)
{
    struct obj *obj;
    struct monst *mon;
    int tmp;

    obj = getobj("#)", "throw");	// it is also possible to throw food or jewels
    if (!obj || !getdir(1))		// ask "in what direction?"
	return 0;
    if (obj->owornmask & (W_ARMOR | W_RING)) {
	pline("You can't throw something you are wearing.");
	return 0;
    }
    u_wipe_engr(2);

    if (obj == uwep) {
	if (obj->cursed) {
	    pline("Your weapon is welded to your hand.");
	    return 1;
	}
	if (obj->quan > 1)
	    setuwep (splitobj(obj, 1));
	else
	    setuwep (NULL);
    } else if (obj->quan > 1)
	splitobj(obj, 1);
    freeinv(obj);
    if (_u.dz) {
	if (_u.dz < 0) {
	    pline("%s hits the ceiling, then falls back on top of your head.", Doname(obj));	// note: obj->quan == 1
	    if (obj->olet == POTION_SYM)
		potionhit(&youmonst, obj);
	    else {
		if (uarmh)
		    pline("Fortunately, you are wearing a helmet!");
		losehp(uarmh ? 1 : rnd((int) (obj->owt)), "falling object");
		dropy(obj);
	    }
	} else {
	    pline("%s hits the floor.", Doname(obj));
	    if (obj->otyp == EXPENSIVE_CAMERA) {
		pline("It is shattered in a thousand pieces!");
		obfree (obj, NULL);
	    } else if (obj->otyp == EGG) {
		pline("\"Splash!\"");
		obfree (obj, NULL);
	    } else if (obj->olet == POTION_SYM) {
		pline("The flask breaks, and you smell a peculiar odor ...");
		potionbreathe(obj);
		obfree (obj, NULL);
	    } else {
		dropy(obj);
	    }
	}
	return 1;
    } else if (obj->otyp == BOOMERANG) {
	mon = boomhit(_u.dx, _u.dy);
	if (mon == &youmonst) {	// the thing was caught
	    addinv(obj);
	    return 1;
	}
    } else {
	if (obj->otyp == PICK_AXE && shkcatch(obj))
	    return 1;
	mon = bhit (_u.dx, _u.dy, obj->otyp == ICE_BOX ? 1 : 8, obj->olet, NULL, NULL, obj);
    }
    if (mon) {
	// awake monster if sleeping
	wakeup(mon);

	if (obj->olet == WEAPON_SYM) {
	    tmp = -1 + _u.ulevel + mon->data->ac + abon();
	    if (obj->otyp < ROCK) {
		if (!uwep || uwep->otyp != obj->otyp + (BOW - ARROW))
		    tmp -= 4;
		else {
		    tmp += uwep->spe;
		}
	    } else if (obj->otyp == BOOMERANG)
		tmp += 4;
	    tmp += obj->spe;
	    if (tmp >= (int) rnd(20)) {
		if (!hmon(mon, obj, 1))
		    mon = 0;
		// weapons thrown disappear sometimes
		if (obj->otyp < BOOMERANG && rn2(3)) {
		    // check bill; free
		    obfree (obj, NULL);
		    return 1;
		}
	    } else
		miss(c_Objects[obj->otyp].oc_name, mon);
	} else if (obj->olet == POTION_SYM && _u.ulevel > rn2(15)) {
	    potionhit(mon, obj);
	    return 1;
	} else {
	    if (cansee(bhitpos.x, bhitpos.y))
		pline("You miss %s.", monnam(mon));
	    else
		pline("You miss it.");
	    if (obj->olet == GEM_SYM && mon->data->mlet == 'u') {
		if (obj->dknown && is_object_known(obj->otyp)) {
		    if (c_Objects[obj->otyp].g_val > 0) {
			_u.uluck += 5;
			goto valuable;
		    } else {
			pline("%s is not interested in your junk.", Monnam(mon));
		    }
		} else {       // value unknown to @
		    ++_u.uluck;
		  valuable:
		    if (_u.uluck > LUCKMAX)	// dan@ut-ngp
			_u.uluck = LUCKMAX;
		    pline("%s graciously accepts your gift.", Monnam(mon));
		    mpickobj(mon, obj);
		    rloc(mon);
		    return 1;
		}
	    }
	}
    }
    // the code following might become part of dropy()
    obj->ox = bhitpos.x;
    obj->oy = bhitpos.y;
    obj->nobj = _level->objects;
    _level->objects = obj;
    // prevent him from throwing articles to the exit and escaping
    // subfrombill(obj);
    stackobj(obj);
    if (cansee(bhitpos.x, bhitpos.y))
	prl(bhitpos.x, bhitpos.y);
    return 1;
}

// split obj so that it gets size num
// remainder is put in the object structure delivered by this call
struct obj *splitobj (struct obj *obj, int num)
{
    struct obj* o = newobj();
    *o = *obj;	// copies whole structure
    obj->nobj = o;
    o->o_id = _wflags.ident++;
    obj->quan = num;
    obj->owt = weight(obj);
    o->quan -= num;
    o->owt = weight(o);  // -= obj->owt ?
    if (obj->unpaid)
	splitbill(obj, o);
    return o;
}

void more_experienced(int exp, int rexp)
{
    _u.uexp += exp;
    _u.urexp += 4 * exp + rexp;
    if (exp)
	_wflags.botl = 1;
}

void set_wounded_legs(long side, int timex)
{
    if (!Wounded_legs || (Wounded_legs & TIMEOUT))
	Wounded_legs |= side + timex;
    else
	Wounded_legs |= side;
}

void heal_legs(void)
{
    if (Wounded_legs) {
	if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
	    pline("Your legs feel somewhat better.");
	else
	    pline("Your leg feels somewhat better.");
	Wounded_legs = 0;
    }
}
