// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

// mhitu: monster hits you
//        returns 1 if monster dies (e.g. 'y', 'F'), 0 otherwise
int mhitu(struct monst *mtmp)
{
    const struct permonst *mdat = mtmp->data;
    int tmp, ctmp;

    nomul(0);

    if (mdat->mlet == 'c' && Stoned)
	return 0;

    // make eels visible the moment they hit/miss us
    if (mdat->mlet == ';' && mtmp->minvis && cansee(mtmp->mx, mtmp->my)) {
	mtmp->minvis = 0;
	pmon(mtmp);
    }
    if (!strchr("1&DuxynNF", mdat->mlet))
	tmp = hitu(mtmp, d(mdat->damn, mdat->damd));
    else
	tmp = 0;
    if (strchr(UNDEAD, mdat->mlet) && midnight())
	tmp += hitu(mtmp, d(mdat->damn, mdat->damd));

    ctmp = tmp && !mtmp->mcan && (!uarm || c_Objects[uarm->otyp].a_can < (int) rnd(3) || !rn2(50));
    switch (mdat->mlet) {
	case '1':
	    if (wiz_hit(mtmp))
		return 1;    // he disappeared
	    break;
	case '&':
	    if (!mtmp->mcan && !rn2(13)) {
		makemon(PM_DEMON, _u.ux, _u.uy);
	    } else {
		hitu(mtmp, d(2, 6));
		hitu(mtmp, d(2, 6));
		hitu(mtmp, rnd(3));
		hitu(mtmp, rnd(3));
		hitu(mtmp, rn1(4, 2));
	    }
	    break;
	case ',':
	    hitu (mtmp, rnd(12));
	    break;
	case '\'':
	    hitu (mtmp, rnd(12));
	    break;
	case 'A':
	    if (ctmp && rn2(2)) {
		if (Poison_resistance)
		    pline("The sting doesn't seem to affect you.");
		else {
		    pline("You feel weaker!");
		    losestr(1);
		}
	    }
	    break;
	case 'C':
	    hitu (mtmp, rnd(6));
	    break;
	case 'c':
	    if (!rn2(5)) {
		pline("You hear %s's hissing!", monnam(mtmp));
		if (ctmp || !rn2(20) || (_wflags.moonphase == NEW_MOON && !carrying(DEAD_LIZARD))) {
		    Stoned = 5;
		    // pline("You get turned to stone!");
		    // done_in_by(mtmp);
		}
	    }
	    break;
	case 'D':
	    if (rn2(6) || mtmp->mcan) {
		hitu(mtmp, d(3, 10));
		hitu(mtmp, rnd(8));
		hitu(mtmp, rnd(8));
		break;
	    }
	    kludge("%s breathes fire!", "The dragon");
	    buzz(-1, mtmp->mx, mtmp->my, _u.ux - mtmp->mx, _u.uy - mtmp->my);
	    break;
	case 'd':
	    hitu(mtmp, d(2, (_wflags.moonphase == FULL_MOON) ? 3 : 4));
	    break;
	case 'e':
	    hitu(mtmp, d(3, 6));
	    break;
	case 'F':
	    if (mtmp->mcan)
		break;
	    kludge("%s explodes!", "The freezing sphere");
	    if (Cold_resistance)
		pline("You don't seem affected by it.");
	    else {
		uint8_t dn = 3;
		if (_u.ulevel/2 < 17-rnd(18)) {
		    pline("You get blasted!");
		    dn = 6;
		} else
		    pline("You duck the blast...");
		losehp_m(d(dn, 6), mtmp);
	    }
	    mondead(mtmp);
	    return 1;
	case 'g':
	    if (ctmp && multi >= 0 && !rn2(3)) {
		kludge("You are frozen by %ss juices", "the cube'");
		nomul(-rnd(10));
	    }
	    break;
	case 'h':
	    if (ctmp && multi >= 0 && !rn2(5)) {
		nomul(-rnd(10));
		kludge("You are put to sleep by %ss bite!", "the homunculus'");
	    }
	    break;
	case 'j':
	    tmp = hitu(mtmp, rnd(3));
	    tmp &= hitu(mtmp, rnd(3));
	    if (tmp) {
		hitu(mtmp, rnd(4));
		hitu(mtmp, rnd(4));
	    }
	    break;
	case 'k':
	    if ((hitu(mtmp, rnd(4)) || !rn2(3)) && ctmp) {
		poisoned("bee's sting", mdat->mname);
	    }
	    break;
	case 'L':
	    if (tmp)
		stealgold(mtmp);
	    break;
	case 'N':
	    if (mtmp->mcan && !Blind) {
		pline("%s tries to seduce you, but you seem not interested.", Amonnam(mtmp, "plain"));
		if (rn2(3))
		    rloc(mtmp);
	    } else if (steal(mtmp)) {
		rloc(mtmp);
		mtmp->mflee = 1;
	    }
	    break;
	case 'n':
	    if (!uwep && !uarm && !uarmh && !uarms && !uarmg) {
		pline("%s hits! (I hope you don't mind)", Monnam(mtmp));
		_u.uhp += rnd(7);
		if (!rn2(7))
		    ++_u.uhpmax;
		if (_u.uhp > _u.uhpmax)
		    _u.uhp = _u.uhpmax;
		_wflags.botl = 1;
		if (!rn2(50))
		    rloc(mtmp);
	    } else {
		hitu(mtmp, d(2, 6));
		hitu(mtmp, d(2, 6));
	    }
	    break;
	case 'o':
	    tmp = hitu(mtmp, rnd(24));
	    break;
	case 'P':
	    hitu(mtmp, d(2, 4));
	    break;
	case 'Q':
	    hitu(mtmp, rnd(2));
	    hitu(mtmp, rnd(2));
	    break;
	case 'R':
	    if (tmp && uarmh && !uarmh->rustfree && (int) uarmh->spe >= -1) {
		pline("Your helmet rusts!");
		--uarmh->spe;
	    } else if (ctmp && uarm && !uarm->rustfree &&	// Mike Newton
		       uarm->otyp < STUDDED_LEATHER_ARMOR && (int) uarm->spe >= -1) {
		pline("Your armor rusts!");
		--uarm->spe;
	    }
	    break;
	case ';':
	    hitu (mtmp, rnd(12));
	    break;
	case 'S':
	    if (ctmp && !rn2(8))
		poisoned("snake's bite", mdat->mname);
	    break;
	case 's':
	    if (tmp && !rn2(8))
		poisoned("scorpion's sting", mdat->mname);
	    hitu(mtmp, rnd(8));
	    hitu(mtmp, rnd(8));
	    break;
	case 'T':
	    hitu(mtmp, rnd(6));
	    hitu(mtmp, rnd(6));
	    break;
	case 't':
	    if (!rn2(5))
		rloc(mtmp);
	    break;
	case 'u':
	    mtmp->mflee = 1;
	    break;
	case 'U':
	    hitu(mtmp, d(3, 4));
	    hitu(mtmp, d(3, 4));
	    break;
	case 'v':
	    break;
	case 'V':
	    if (tmp)
		_u.uhp -= 4;
	    if (ctmp)
		losexp();
	    break;
	case 'W':
	    if (ctmp)
		losexp();
	    break;
	case 'X':
	    hitu(mtmp, rnd(5));
	    hitu(mtmp, rnd(5));
	    hitu(mtmp, rnd(5));
	    break;
	case 'x':
	    {
		long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
		pline("%s pricks in your %s leg!", Monnam(mtmp), (side == RIGHT_SIDE) ? "right" : "left");
		set_wounded_legs(side, rnd(50));
		losehp_m(2, mtmp);
		break;
	    }
	case 'y':
	    if (mtmp->mcan)
		break;
	    mondead(mtmp);
	    if (!Blind) {
		pline("You are blinded by a blast of light!");
		Blind = d(4, 12);
		seeoff(0);
	    }
	    return 1;
	case 'Y':
	    hitu(mtmp, rnd(6));
	    break;
    }
    if (_u.uhp < 1)
	done_in_by(mtmp);
    return 0;
}

int hitu(struct monst *mtmp, int dam)
{
    int tmp, res;

    nomul(0);

    if (mtmp->mhide && mtmp->mundetected) {
	mtmp->mundetected = 0;
	if (!Blind) {
	    struct obj *obj;
	    if ((obj = o_at(mtmp->mx, mtmp->my)) != NULL)
		pline("%s was hidden under %s!", Xmonnam(mtmp), doname(obj));
	}
    }
    tmp = _u.uac;
    // give people with Ac = -10 at least some vulnerability
    if (tmp < 0) {
	dam += tmp;	       // decrease damage
	if (dam <= 0)
	    dam = 1;
	tmp = -rn2(-tmp);
    }
    tmp += mtmp->data->mlevel;
    if (multi < 0)
	tmp += 4;
    if ((Invis && mtmp->data->mlet != 'I') || !mtmp->mcansee)
	tmp -= 2;
    if (mtmp->mtrapped)
	tmp -= 2;
    if (tmp <= (int) rnd(20)) {
	if (Blind)
	    pline("It misses.");
	else
	    pline("%s misses.", Monnam(mtmp));
	res = 0;
    } else {
	if (Blind)
	    pline("It hits!");
	else
	    pline("%s hits!", Monnam(mtmp));
	losehp_m(dam, mtmp);
	res = 1;
    }
    stop_occupation();
    return res;
}
