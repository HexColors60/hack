// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

static bool far_noise;
static long noisetime;

// hitmm returns 0 (miss), 1 (hit), or 2 (kill)
int hitmm(struct monst *magr, struct monst *mdef)
{
    const struct permonst *pa = magr->data, *pd = mdef->data;
    int8_t tmp;
    bool vis;
    if (strchr("Eauy", pa->mlet))
	return 0;
    if (magr->mfroz)
	return 0;	       // riv05!a3
    tmp = pd->ac + pa->mlevel;
    if (mdef->mconf || mdef->mfroz || mdef->msleep) {
	tmp += 4;
	if (mdef->msleep)
	    mdef->msleep = 0;
    }
    bool hit = (tmp > (int) rnd(20));
    if (hit)
	mdef->msleep = 0;
    vis = (cansee(magr->mx, magr->my) && cansee(mdef->mx, mdef->my));
    if (vis)
	pline ("%s %s %s.", Monnam(magr), hit ? "hits" : "misses", monnam(mdef));
    else {
	bool far = (dist(magr->mx, magr->my) > 15);
	if (far != far_noise || _u.moves - noisetime > 10) {
	    far_noise = far;
	    noisetime = _u.moves;
	    pline("You hear some noises%s.", far ? " in the distance" : "");
	}
    }
    if (hit) {
	if (magr->data->mlet == 'c') {
	    magr->mhpmax += 3;
	    if (vis)
		pline("%s is turned to stone!", Monnam(mdef));
	    monstone(mdef);
	    hit = 2;
	} else if ((mdef->mhp -= d(pa->damn, pa->damd)) < 1) {
	    magr->mhpmax += 1 + rn2(pd->mlevel + 1);
	    if (vis)
		pline("%s is killed!", Monnam(mdef));
	    mondied(mdef);
	    hit = 2;
	}
    }
    return hit;
}

// drop (perhaps) a cadaver and remove monster
void mondied(struct monst *mdef)
{
    const struct permonst *pd = mdef->data;
    if (letter(pd->mlet) && rn2(3)) {
	mkobj_at(pd->mlet, mdef->mx, mdef->my);
	if (cansee(mdef->mx, mdef->my)) {
	    unpmon(mdef);
	    atl(mdef->mx, mdef->my, _level->objects->olet);
	}
	stackobj(_level->objects);
    }
    mondead(mdef);
}

// drop a rock and remove monster
void monstone(struct monst *mdef)
{
    if (strchr(mlarge, mdef->data->mlet))
	mksobj_at(ENORMOUS_ROCK, mdef->mx, mdef->my);
    else
	mksobj_at(ROCK, mdef->mx, mdef->my);
    if (cansee(mdef->mx, mdef->my)) {
	unpmon(mdef);
	atl(mdef->mx, mdef->my, _level->objects->olet);
    }
    mondead(mdef);
}

int fightm (struct monst* fm)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon)
	if (m != fm && DIST(m->mx, m->my, fm->mx, fm->my) < 3 && rn2(4))
	    return hitmm (fm, m);
    return -1;
}

// u is hit by sth, but not a monster
int thitu (int tlev, int dam, const char *name)
{
    char buf [BUFSZ];
    if (strchr (vowels, name[0]))
	snprintf (ArrayBlock(buf), "an %s", name);
    else
	snprintf (ArrayBlock(buf), "a %s", name);
    if (_u.uac + tlev <= (int) rnd(20)) {
	if (Blind)
	    pline("It misses.");
	else
	    pline("You are almost hit by %s!", buf);
	return 0;
    } else {
	if (Blind)
	    pline("You are hit!");
	else
	    pline("You are hit by %s!", buf);
	losehp(dam, name);
	return 1;
    }
}

const char mlarge[] = "bCDdegIlmnoPSsTUwY',&";

bool hmon(			// return true if mon still alive
		struct monst *mon, struct obj *obj, int thrown)
{
    int tmp;
    bool hittxt = false;

    if (!obj) {
	tmp = rnd(2);	       // attack with bare hands
	if (mon->data->mlet == 'c' && !uarmg) {
	    pline("You hit the cockatrice with your bare hands.");
	    pline("You turn to stone ...");
	    done_in_by(mon);
	}
    } else if (obj->olet == WEAPON_SYM || obj->otyp == PICK_AXE) {
	if (obj == uwep && (obj->otyp > SPEAR || obj->otyp < BOOMERANG))
	    tmp = rnd(2);
	else {
	    if (strchr(mlarge, mon->data->mlet)) {
		tmp = rnd(c_Objects[obj->otyp].wldam);
		if (obj->otyp == TWO_HANDED_SWORD)
		    tmp += d(2, 6);
		else if (obj->otyp == FLAIL)
		    tmp += rnd(4);
	    } else {
		tmp = rnd(c_Objects[obj->otyp].wsdam);
	    }
	    tmp += obj->spe;
	    if (!thrown && obj == uwep && obj->otyp == BOOMERANG && !rn2(3)) {
		pline ("As you hit %s, the boomerang breaks into splinters.", monnam(mon));
		freeinv (obj);
		setworn (NULL, obj->owornmask);
		obfree (obj, NULL);
		++tmp;
	    }
	}
	if (mon->data->mlet == 'O' && obj->otyp == TWO_HANDED_SWORD)
	    tmp += rnd(10);
    } else
	switch (obj->otyp) {
	    case EXPENSIVE_CAMERA:
		pline("You succeed in destroying your camera. Congratulations!");
		freeinv(obj);
		if (obj->owornmask)
		    setworn (NULL, obj->owornmask);
		obfree (obj, NULL);
		return true;
	    case DEAD_COCKATRICE:
		pline("You hit %s with the cockatrice corpse.", monnam(mon));
		if (mon->data->mlet == 'c') {
		    tmp = 1;
		    hittxt = true;
		    break;
		}
		pline("%s is turned to stone!", Monnam(mon));
		killed(mon);
		return false;
	    case CLOVE_OF_GARLIC:	// no effect against demons
		if (strchr(UNDEAD, mon->data->mlet))
		    mon->mflee = 1;
		tmp = 1;
		break;
	    default:
		// non-weapons can damage because of their weight
		// (but not too much)
		tmp = obj->owt / 10;
		if (tmp < 1)
		    tmp = 1;
		else
		    tmp = rnd(tmp);
		if (tmp > 6)
		    tmp = 6;
	}

	// NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG)

    tmp += _u.udaminc + dbon();
    if (tmp < 1)
	tmp = 1;
    mon->mhp -= tmp;
    if (mon->mhp < 1) {
	killed(mon);
	return false;
    }
    if (!hittxt) {
	if (thrown)	// this assumes that we cannot throw plural things
	    hit (xname(obj), mon, exclam(tmp));	// or: c_Objects[obj->otyp].oc_name
	else if (Blind)
	    pline("You hit it.");
	else
	    pline("You hit %s%s", monnam(mon), exclam(tmp));
    }
    if (_u.umconf && !thrown) {
	if (!Blind) {
	    pline("Your hands stop glowing blue.");
	    if (!mon->mfroz && !mon->msleep)
		pline("%s appears confused.", Monnam(mon));
	}
	mon->mconf = 1;
	_u.umconf = 0;
    }
    return true;	       // mon still alive
}

// try to attack; return false if monster evaded
// _u.dx and _u.dy must be set
int attack(struct monst *mtmp)
{
    int8_t tmp;
    bool malive = true;
    const struct permonst *mdat;
    mdat = mtmp->data;

    u_wipe_engr(3);	       // andrew@orca: prevent unlimited pick-axe
			       // attacks

    if (mdat->mlet == 'L'
	    && !mtmp->mfroz
	    && !mtmp->msleep
	    && !mtmp->mconf
	    && mtmp->mcansee
	    && !rn2(7)
	    && (m_move(mtmp) == 2 // he died
		|| mtmp->mx != _u.ux + _u.dx || mtmp->my != _u.uy + _u.dy))	// he moved:
	return false;

    wakeup(mtmp);

    if (mtmp->mhide && mtmp->mundetected) {
	struct obj *obj;

	mtmp->mundetected = 0;
	if ((obj = o_at(mtmp->mx, mtmp->my)) && !Blind)
	    pline("Wait! There's a %s hiding under %s!", mdat->mname, doname(obj));
	return true;
    }
    tmp = _u.uluck + _u.ulevel + mdat->ac + abon();
    if (uwep) {
	if (uwep->olet == WEAPON_SYM || uwep->otyp == PICK_AXE)
	    tmp += uwep->spe;
	if (uwep->otyp == TWO_HANDED_SWORD)
	    tmp -= 1;
	else if (uwep->otyp == DAGGER)
	    tmp += 2;
	else if (uwep->otyp == SPEAR && strchr("XDne", mdat->mlet))
	    tmp += 2;
    }
    if (mtmp->msleep) {
	mtmp->msleep = 0;
	tmp += 2;
    }
    if (mtmp->mfroz) {
	tmp += 4;
	if (!rn2(10))
	    mtmp->mfroz = 0;
    }
    if (mtmp->mflee)
	tmp += 2;
    if (_u.utrap)
	tmp -= 3;

    // with a lot of luggage, your agility diminishes
    tmp -= (inv_weight() + 40) / 20;

    if (tmp <= (int) rnd(20)) {
	if (Blind)
	    pline("You miss it.");
	else
	    pline("You miss %s.", monnam(mtmp));
    } else {
	// we hit the monster; be careful: it might die!

	if ((malive = hmon(mtmp, uwep, 0)) == true) {
	    // monster still alive
	    if (!rn2(25) && mtmp->mhp < mtmp->mhpmax / 2) {
		mtmp->mflee = 1;
		if (!rn2(3))
		    mtmp->mfleetim = rnd(100);
	    }
	}
	if (mdat->mlet == 'a') {
	    if (rn2(2)) {
		pline("You are splashed by the blob's acid!");
		losehp_m(rnd(6), mtmp);
		if (!rn2(30))
		    corrode_armor();
	    }
	    if (!rn2(6))
		corrode_weapon();
	}
    }
    if (malive && mdat->mlet == 'E' && canseemon(mtmp)
	&& !mtmp->mcan && rn2(3)) {
	if (mtmp->mcansee) {
	    pline("You are frozen by the floating eye's gaze!");
	    nomul((_u.ulevel > 6 || rn2(4)) ? -21+(int)nrand(20) : -200);
	} else {
	    pline("The blinded floating eye cannot defend itself.");
	    if (!rn2(500))
		if ((int) _u.uluck > LUCKMIN)
		    --_u.uluck;
	}
    }
    return true;
}
