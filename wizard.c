// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

enum {
    WIZSHOT	= 6,	// one chance in WIZSHOT that wizard will try magic
    BOLT_LIM	= 8	// from this distance D and 1 will try to hit you
};

const char wizapp[] = "@DNPTUVXcemntx";

// If he has found the Amulet, make the wizard appear after some time
void amulet (void)
{
    if (!_wflags.made_amulet || !_wflags.no_of_wizards)
	return;
    // find wizard, and wake him if necessary
    for (struct monst* m = _level->monsters; m; m = m->nmon) {
	if (m->data->mlet == '1' && m->msleep && !rn2(40)) {
	    for (struct obj* o = invent; o; o = o->nobj) {
		if (o->olet == AMULET_SYM && !o->spe) {
		    m->msleep = 0;
		    if (dist(m->mx, m->my) > 2)
			pline("You get the creepy feeling that somebody noticed your taking the Amulet.");
		    return;
		}
	    }
	}
    }
}

int wiz_hit(struct monst *m)
{
    // if we have stolen or found the amulet, we disappear
    if (m->minvent && m->minvent->olet == AMULET_SYM && m->minvent->spe == 0) {
	// vanish -- very primitive
	fall_down(m);
	return 1;
    }
    // if it is lying around someplace, we teleport to it
    if (!carrying(AMULET_OF_YENDOR)) {
	for (struct obj* o = _level->objects; o; o = o->nobj) {
	    if (o->olet == AMULET_SYM && !o->spe) {
		if ((_u.ux != o->ox || _u.uy != o->oy) && !m_at(o->ox, o->oy)) {

		    // teleport to it and pick it up
		    m->mx = o->ox;
		    m->my = o->oy;
		    freeobj(o);
		    mpickobj(m, o);
		    pmon(m);
		    return 0;
		}
		goto hithim;
	    }
	}
	return 0;	       // we don't know where it is
    }
  hithim:
    if (rn2(2)) {	       // hit - perhaps steal
	// if hit 1/20 chance of stealing amulet & vanish - amulet is
	// on level 26 again.
	if (hitu(m, d(m->data->damn, m->data->damd)) && !rn2(20))
	    stealamulet(m);
    } else
	inrange(m);	       // try magic
    return 0;
}

void inrange (struct monst* m)
{
    int8_t tx, ty;

    // do nothing if cancelled (but make '1' say something)
    if (m->data->mlet != '1' && m->mcan)
	return;

    // spit fire only when both in a room or both in a corridor
    if (inroom(_u.ux, _u.uy) != inroom(m->mx, m->my))
	return;
    tx = _u.ux - m->mx;
    ty = _u.uy - m->my;
    if ((!tx && abs(ty) < BOLT_LIM) || (!ty && abs(tx) < BOLT_LIM)
	|| (abs(tx) == abs(ty) && abs(tx) < BOLT_LIM)) {
	switch (m->data->mlet) {
	    case 'D':
		// spit fire in the direction of @ (not nec. hitting)
		buzz(-1, m->mx, m->my, sgn(tx), sgn(ty));
		break;
	    case '1':
		if (rn2(WIZSHOT))
		    break;
		// if you zapped wizard with wand of cancellation, he
		// has to shake off the effects before he can throw
		// spells successfully.  1/2 the time they fail
		// anyway
		if (m->mcan || rn2(2)) {
		    if (canseemon(m))
			pline("%s makes a gesture, then curses.", Monnam(m));
		    else
			pline("You hear mumbled cursing.");
		    if (!rn2(3)) {
			m->mspeed = 0;
			m->minvis = 0;
		    }
		    if (!rn2(3))
			m->mcan = 0;
		} else {
		    if (canseemon(m)) {
			if (!rn2(6) && !Invis) {
			    pline("%s hypnotizes you.", Monnam(m));
			    nomul(rn2(3) + 3);
			    break;
			} else
			    pline("%s chants an incantation.", Monnam(m));
		    } else
			pline("You hear a mumbled incantation.");
		    switch (rn2(Invis ? 5 : 6)) {
			case 0:
			    // create a nasty monster from a deep level
			    // (for the moment, 'nasty' is not implemented)
			    makemon(NULL, _u.ux, _u.uy);
			    break;
			case 1:
			    pline("\"Destroy the thief, my pets!\"");
			    aggravate();	// aggravate all the monsters
			    // fallthrough
			case 2:
			    if (_wflags.no_of_wizards == 1 && rnd(5) == 0)
				// if only 1 wizard, clone himself
				clonewiz(m);
			    break;
			case 3:
			    if (m->mspeed == MSLOW)
				m->mspeed = 0;
			    else
				m->mspeed = MFAST;
			    break;
			case 4:
			    m->minvis = 1;
			    break;
			case 5:
			    // Only if not Invisible
			    pline("You hear a clap of thunder!");
			    // shoot a bolt of fire or cold, or a sleep ray
			    buzz(-rnd(3), m->mx, m->my, sgn(tx), sgn(ty));
			    break;
		    }
		}
	}
	if (_u.uhp < 1)
	    done_in_by(m);
    }
}

void aggravate (void)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon) {
	m->msleep = 0;
	if (m->mfroz && !rn2(5))
	    m->mfroz = 0;
    }
}

void clonewiz (struct monst *m)
{
    struct monst *m2 = makemon(PM_WIZARD, m->mx, m->my);
    if (m2) {
	_wflags.no_of_wizards = 2;
	unpmon(m2);
	m2->mappearance = wizapp[rn2(sizeof(wizapp) - 1)];
	pmon(m);
    }
}
