// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

long somegold(void)
{
    return (_u.ugold < 100) ? _u.ugold : (_u.ugold > 10000) ? rnd(10000) : rnd((int) _u.ugold);
}

void stealgold (struct monst *m)
{
    struct gold *gold = g_at(_u.ux, _u.uy);
    if (gold && (!_u.ugold || gold->amount > _u.ugold || !rn2(5))) {
	m->mgold += gold->amount;
	freegold(gold);
	if (Invisible)
	    newsym(_u.ux, _u.uy);
	pline("%s quickly snatches some gold from between your feet!", Monnam(m));
	if (!_u.ugold || !rn2(5)) {
	    rloc(m);
	    m->mflee = 1;
	}
    } else if (_u.ugold) {
	long gstolen = somegold();
	_u.ugold -= gstolen;
	pline("Your purse feels lighter.");
	m->mgold += gstolen;
	rloc(m);
	m->mflee = 1;
	_wflags.botl = 1;
    }
}

// steal armor after he finishes taking it off
static unsigned stealoid;	// object to be stolen
static unsigned stealmid;	// monster doing the stealing

int stealarm(void)
{
    for (struct obj* o = invent; o; o = o->nobj) {
	if (o->o_id == stealoid) {
	    for (struct monst* m = _level->monsters; m; m = m->nmon) {
		if (m->m_id == stealmid) {
		    if (dist(m->mx, m->my) < 3) {
			freeinv(o);
			pline("%s steals %s!", Monnam(m), doname(o));
			mpickobj(m, o);
			m->mflee = 1;
			rloc(m);
		    }
		    break;
		}
	    }
	    break;
	}
    }
    stealoid = 0;
    return 0;
}

// returns 1 when something was stolen
// (or at least, when N should flee now)
// avoid stealing the object stealoid
int steal(struct monst *m)
{
    int named = 0;
    if (!invent) {
	if (Blind)
	    pline("Somebody tries to rob you, but finds nothing to steal.");
	else
	    pline("%s tries to rob you, but she finds nothing to steal!", Monnam(m));
	return 1;	       // let her flee
    }
    int stealchance = 0;
    struct obj *o;
    for (o = invent; o; o = o->nobj)
	if (o != uarm2)
	    stealchance += ((o->owornmask & (W_ARMOR | W_RING)) ? 5 : 1);
    stealchance = rn2(stealchance);
    for (o = invent; o; o = o->nobj)
	if (o != uarm2)
	    if ((stealchance -= ((o->owornmask & (W_ARMOR | W_RING)) ? 5 : 1)) < 0)
		break;
    if (!o) {
	impossible("Steal fails!");
	return 0;
    }
    if (o->o_id == stealoid)
	return 0;
    if ((o->owornmask & (W_ARMOR | W_RING))) {
	switch (o->olet) {
	    case RING_SYM:
		ringoff(o);
		break;
	    case ARMOR_SYM:
		if (multi < 0 || o == uarms) {
		    setworn (NULL, o->owornmask & W_ARMOR);
		    break;
		}
		{
		    int curssv = o->cursed;
		    o->cursed = 0;
		    stop_occupation();
		    pline("%s seduces you and %s off your %s.", Amonnam(m, Blind ? "gentle" : "beautiful"), o->cursed ? "helps you to take" : "you start taking", (o == uarmg) ? "gloves" : (o == uarmh) ? "helmet" : "armor");
		    ++named;
		    armoroff(o);
		    o->cursed = curssv;
		    if (multi < 0) {
			// multi = 0;
			// nomovemsg = 0;
			// afternmv = 0;
			stealoid = o->o_id;
			stealmid = m->m_id;
			afternmv = stealarm;
			return 0;
		    }
		    break;
		}
	    default:
		impossible("Tried to steal a strange worn thing.");
	}
    } else if (o == uwep)
	setuwep (NULL);
    freeinv (o);
    pline ("%s stole %s.", named ? "She" : Monnam(m), doname(o));
    mpickobj (m, o);
    return multi < 0 ? 0 : 1;
}

void mpickobj (struct monst* m, struct obj* o)
{
    o->nobj = m->minvent;
    m->minvent = o;
}

int stealamulet(struct monst *m)
{
    for (struct obj* o = invent; o; o = o->nobj) {
	if (o->olet == AMULET_SYM) {
	    // might be an imitation one
	    if (o == uwep)
		setuwep (NULL);
	    freeinv(o);
	    mpickobj(m, o);
	    pline ("%s stole %s!", Monnam(m), doname(o));
	    return 1;
	}
    }
    return 0;
}

// release the objects the killed animal has stolen
void relobj (struct monst* m, int show)
{
    for (struct obj *o = m->minvent, *o2; o; o = o2) {
	o->ox = m->mx;
	o->oy = m->my;
	o2 = o->nobj;
	o->nobj = _level->objects;
	_level->objects = o;
	stackobj(_level->objects);
	if (show & cansee(m->mx, m->my))
	    atl(o->ox, o->oy, o->olet);
    }
    m->minvent = NULL;
    if (m->mgold || m->data->mlet == 'L') {
	mkgold (d(_u.dlevel,30) + min_u (m->mgold, 10000), m->mx, m->my);
	if (show & cansee(m->mx, m->my))
	    atl(m->mx, m->my, '$');
    }
}
