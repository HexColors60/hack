// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
char POISONOUS[] = "ADKSVabhks";

// hunger texts used on bottom line (each 8 chars long)
#define	SATIATED	0
#define NOT_HUNGRY	1
#define	HUNGRY		2
#define	WEAK		3
#define	FAINTING	4
#define FAINTED		5
#define STARVED		6

const char *const hu_stat[] = {
    "Satiated",
    "        ",
    "Hungry  ",
    "Weak    ",
    "Fainting",
    "Fainted ",
    "Starved "
};

void init_uhunger(void)
{
    _u.uhunger = 900;
    _u.uhs = NOT_HUNGRY;
}

static const struct {
    const char *txt;
    int nut;
} c_tintxts[] = {
    {
    "It contains first quality peaches - what a surprise!", 40}, {
    "It contains salmon - not bad!", 60}, {
    "It contains apple juice - perhaps not what you hoped for.", 20}, {
    "It contains some nondescript substance, tasting awfully.", 500}, {
    "It contains rotten meat. You vomit.", -50}, {
    "It turns out to be empty.", 0}
};

static struct {
    struct obj *tin;
    int usedtime, reqtime;
} tin;

int opentin(void)
{
    if (!carried(tin.tin))     // perhaps it was stolen?
	return 0;	       // %% probably we should use tinoid
    if (tin.usedtime++ >= 50) {
	pline("You give up your attempt to open the tin.");
	return 0;
    }
    if (tin.usedtime < tin.reqtime)
	return 1;	       // still busy

    pline("You succeed in opening the tin.");
    useup(tin.tin);
    unsigned r = rn2(2 * ArraySize(c_tintxts));
    if (r < ArraySize(c_tintxts)) {
	pline(c_tintxts[r].txt);
	lesshungry(c_tintxts[r].nut);
	if (r == 1) {	       // SALMON
	    Glib = rnd(15);
	    pline("Eating salmon made your fingers very slippery.");
	}
    } else {
	pline("It contains spinach - this makes you feel like Popeye!");
	lesshungry(600);
	if (_u.ustr < 118)
	    _u.ustr += rnd(((_u.ustr < 17) ? 19 : 118) - _u.ustr);
	if (_u.ustr > _u.ustrmax)
	    _u.ustrmax = _u.ustr;
	_wflags.botl = 1;
    }
    return 0;
}

int Meatdone(void)
{
    _u.usym = '@';
    prme();
    return 0;
}

int doeat(void)
{
    struct obj *otmp;
    int tmp;

    // Is there some food (probably a heavy corpse) here on the ground?
    if (!Levitation)
	for (otmp = _level->objects; otmp; otmp = otmp->nobj) {
	    if (otmp->ox == _u.ux && otmp->oy == _u.uy && otmp->olet == FOOD_SYM) {
		pline("There %s %s here; eat %s? [ny] ", (otmp->quan == 1) ? "is" : "are", doname(otmp), (otmp->quan == 1) ? "it" : "one");
		if (readchar() == 'y') {
		    if (otmp->quan != 1)
			(void) splitobj(otmp, 1);
		    freeobj(otmp);
		    otmp = addinv(otmp);
		    addtobill(otmp);
		    goto gotit;
		}
	    }
	}
    otmp = getobj("%", "eat");
    if (!otmp)
	return 0;
  gotit:
    if (otmp->otyp == TIN) {
	if (uwep) {
	    switch (uwep->otyp) {
		case CAN_OPENER:
		    tmp = 1;
		    break;
		case DAGGER:
		    tmp = 3;
		    break;
		case PICK_AXE:
		case AXE:
		    tmp = 6;
		    break;
		default:
		    goto no_opener;
	    }
	    pline("Using your %s you try to open the tin.", aobjnam(uwep, NULL));
	} else {
	  no_opener:
	    pline("It is not so easy to open this tin.");
	    if (Glib) {
		pline("The tin slips out of your hands.");
		if (otmp->quan > 1) {
		    struct obj *obj;

		    obj = splitobj(otmp, 1);
		    if (otmp == uwep)
			setuwep(obj);
		}
		dropx(otmp);
		return 1;
	    }
	    tmp = 10 + rn2(1 + 500 / ((int) (_u.ulevel + _u.ustr)));
	}
	tin.reqtime = tmp;
	tin.usedtime = 0;
	tin.tin = otmp;
	occupation = opentin;
	occtxt = "opening the tin";
	return 1;
    }
    const struct objclass* ftmp = &c_Objects[otmp->otyp];
    multi = -ftmp->oc_delay;
    if (otmp->otyp >= CORPSE && eatcorpse(otmp))
	goto eatx;
    if (!rn2(7) && otmp->otyp != FORTUNE_COOKIE) {
	pline("Blecch!  Rotten food!");
	if (!rn2(4)) {
	    pline("You feel rather light headed.");
	    Confusion += d(2, 4);
	} else if (!rn2(4) && !Blind) {
	    pline("Everything suddenly goes dark.");
	    Blind = d(2, 10);
	    seeoff(0);
	} else if (!rn2(3)) {
	    if (Blind)
		pline("The world spins and you slap against the floor.");
	    else
		pline("The world spins and goes dark.");
	    nomul(-rnd(10));
	    nomovemsg = "You are conscious again.";
	}
	lesshungry(ftmp->nutrition / 4);
    } else {
	if (_u.uhunger >= 1500) {
	    pline("You choke over your food.");
	    pline("You die...");
	    killer = ftmp->oc_name;
	    done("choked");
	}
	switch (otmp->otyp) {
	    case FOOD_RATION:
		if (_u.uhunger <= 200)
		    pline("That food really hit the spot!");
		else if (_u.uhunger <= 700)
		    pline("That satiated your stomach!");
		else {
		    pline("You're having a hard time getting all that food down.");
		    multi -= 2;
		}
		lesshungry(ftmp->nutrition);
		if (multi < 0)
		    nomovemsg = "You finished your meal.";
		break;
	    case TRIPE_RATION:
		pline("Yak - dog food!");
		more_experienced(1, 0);
		_wflags.botl = 1;
		if (rn2(2)) {
		    pline("You vomit.");
		    morehungry(20);
		    if (Sick) {
			Sick = 0;	// David Neves
			pline("What a relief!");
		    }
		} else
		    lesshungry(ftmp->nutrition);
		break;
	    default:
		if (otmp->otyp >= CORPSE)
		    pline("That %s tasted terrible!", ftmp->oc_name);
		else
		    pline("That %s was delicious!", ftmp->oc_name);
		lesshungry(ftmp->nutrition);
		if (otmp->otyp == DEAD_LIZARD && (Confusion > 2))
		    Confusion = 2;
		else if (otmp->otyp == FORTUNE_COOKIE) {
		    if (Blind) {
			pline("This cookie has a scrap of paper inside!");
			pline("What a pity, that you cannot read it!");
		    } else
			print_rumor();
		} else if (otmp->otyp == LUMP_OF_ROYAL_JELLY) {
		    // This stuff seems to be VERY healthy!
		    if (_u.ustrmax < 118)
			++_u.ustrmax;
		    if (_u.ustr < _u.ustrmax)
			++_u.ustr;
		    _u.uhp += rnd(20);
		    if (_u.uhp > _u.uhpmax) {
			if (!rn2(17))
			    ++_u.uhpmax;
			_u.uhp = _u.uhpmax;
		    }
		    heal_legs();
		}
		break;
	}
    }
  eatx:
    if (multi < 0 && !nomovemsg) {
	static char msgbuf [BUFSZ];
	snprintf (ArrayBlock(msgbuf), "You finished eating the %s.", ftmp->oc_name);
	nomovemsg = msgbuf;
    }
    useup(otmp);
    return 1;
}

// called in hack.main.c
void gethungry(void)
{
    --_u.uhunger;
    if (_u.moves % 2) {
	if (Regeneration)
	    --_u.uhunger;
	if (Hunger)
	    --_u.uhunger;
	// a3:  if(Hunger & LEFT_RING) --_u.uhunger; if(Hunger &
	// RIGHT_RING) --_u.uhunger; etc.
    }
    if (_u.moves % 20 == 0) {     // jimt@asgb
	if (uleft)
	    --_u.uhunger;
	if (uright)
	    --_u.uhunger;
    }
    newuhs(true);
}

// called after vomiting and after performing feats of magic
void morehungry(int num)
{
    _u.uhunger -= num;
    newuhs(true);
}

// called after eating something (and after drinking fruit juice)
void lesshungry(int num)
{
    _u.uhunger += num;
    newuhs(false);
}

int unfaint(void)
{
    _u.uhs = FAINTING;
    _wflags.botl = 1;
    return 0;
}

void newuhs (bool incr)
{
    int newhs, h = _u.uhunger;

    newhs = (h > 1000) ? SATIATED : (h > 150) ? NOT_HUNGRY : (h > 50) ? HUNGRY : (h > 0) ? WEAK : FAINTING;

    if (newhs == FAINTING) {
	if (_u.uhs == FAINTED)
	    newhs = FAINTED;
	if (_u.uhs <= WEAK || rn2(20 - _u.uhunger / 10) >= 19) {
	    if (_u.uhs != FAINTED && multi >= 0) {
		pline("You faint from lack of food.");
		nomul(-10 + (_u.uhunger / 10));
		nomovemsg = "You regain consciousness.";
		afternmv = unfaint;
		newhs = FAINTED;
	    }
	} else if (_u.uhunger < -(int) (200 + 25 * _u.ulevel)) {
	    _u.uhs = STARVED;
	    _wflags.botl = 1;
	    bot();
	    pline("You die from starvation.");
	    done("starved");
	}
    }
    if (newhs != _u.uhs) {
	if (newhs >= WEAK && _u.uhs < WEAK)
	    losestr(1);	       // this may kill you -- see below
	else if (newhs < WEAK && _u.uhs >= WEAK && _u.ustr < _u.ustrmax)
	    losestr(-1);
	switch (newhs) {
	    case HUNGRY:
		pline(!incr ? "You only feel hungry now." : (_u.uhunger < 145) ? "You feel hungry." : "You are beginning to feel hungry.");
		break;
	    case WEAK:
		pline(!incr ? "You feel weak now." : (_u.uhunger < 45) ? "You feel weak." : "You are beginning to feel weak.");
		break;
	}
	_u.uhs = newhs;
	_wflags.botl = 1;
	if (_u.uhp < 1) {
	    pline("You die from hunger and exhaustion.");
	    killer = "exhaustion";
	    done("starved");
	}
    }
}

#define	CORPSE_I_TO_C(otyp)	(char) ((otyp >= DEAD_ACID_BLOB)\
		     ?  'a' + (otyp - DEAD_ACID_BLOB)\
		     :	'@' + (otyp - DEAD_HUMAN))
int poisonous(struct obj *otmp)
{
    return strchr(POISONOUS, CORPSE_I_TO_C(otmp->otyp)) != 0;
}

// returns 1 if some text was printed
int eatcorpse(struct obj *otmp)
{
    char let = CORPSE_I_TO_C(otmp->otyp);
    int tp = 0;
    if (let != 'a' && _u.moves > otmp->age + 50 + rn2(100)) {
	++tp;
	pline("Ulch -- that meat was tainted!");
	pline("You get very sick.");
	Sick = 10 + rn2(10);
	_u.usick_cause = c_Objects[otmp->otyp].oc_name;
    } else if (strchr(POISONOUS, let) && rn2(5)) {
	++tp;
	pline("Ecch -- that must have been poisonous!");
	if (!Poison_resistance) {
	    losestr(rnd(4));
	    losehp(rnd(15), "poisonous corpse");
	} else
	    pline("You don't seem affected by the poison.");
    } else if (strchr("ELNOPQRUuxz", let) && rn2(5)) {
	++tp;
	pline("You feel sick.");
	losehp(rnd(8), "cadaver");
    }
    switch (let) {
	case 'L':
	case 'N':
	case 't':
	    Teleportation |= INTRINSIC;
	    break;
	case 'W':
	    pluslvl();
	    break;
	case 'n':
	    _u.uhp = _u.uhpmax;
	    _wflags.botl = 1;
	    // fallthrough
	case '@':
	    pline("You cannibal! You will be sorry for this!");
	    // not ++tp;
	    // fallthrough
	case 'd':
	    Aggravate_monster |= INTRINSIC;
	    break;
	case 'I':
	    if (!Invis) {
		Invis = 50 + rn2(100);
		if (!See_invisible)
		    newsym(_u.ux, _u.uy);
	    } else {
		Invis |= INTRINSIC;
		See_invisible |= INTRINSIC;
	    }
	    // fallthrough
	case 'y':
	case 'B':
	    Confusion = 50;
	    break;
	case 'D':
	    Fire_resistance |= INTRINSIC;
	    break;
	case 'E':
	    Telepat |= INTRINSIC;
	    break;
	case 'F':
	case 'Y':
	    Cold_resistance |= INTRINSIC;
	    break;
	case 'k':
	case 's':
	    Poison_resistance |= INTRINSIC;
	    break;
	case 'c':
	    pline("You turn to stone.");
	    killer = "dead cockatrice";
	    done("died");
	    // NOTREACHED
	case 'a':
	    if (Stoned) {
		pline("What a pity - you just destroyed a future piece of art!");
		++tp;
		Stoned = 0;
	    }
	    break;
    }
    return tp;
}
