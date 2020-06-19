// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

int doread(void)
{
    struct obj *scroll;
    bool confused = (Confusion != 0);
    bool known = false;

    scroll = getobj("?", "read");
    if (!scroll)
	return 0;
    if (!scroll->dknown && Blind) {
	pline("Being blind, you cannot read the formula on the scroll.");
	return 0;
    }
    if (Blind)
	pline("As you pronounce the formula on it, the scroll disappears.");
    else
	pline("As you read the scroll, it disappears.");
    if (confused)
	pline("Being confused, you mispronounce the magic words ... ");

    switch (scroll->otyp) {
	case SCR_ENCHANT_ARMOR:
	    {
		struct obj *otmp = some_armor();
		if (!otmp) {
		    strange_feeling(scroll, "Your skin glows then fades.");
		    return 1;
		}
		if (confused) {
		    pline("Your %s glows silver for a moment.", c_Objects[otmp->otyp].oc_name);
		    otmp->rustfree = 1;
		    break;
		}
		if (otmp->spe > 3 && rn2(otmp->spe)) {
		    pline("Your %s glows violently green for a while, then evaporates.", c_Objects[otmp->otyp].oc_name);
		    useup(otmp);
		    break;
		}
		pline("Your %s glows green for a moment.", c_Objects[otmp->otyp].oc_name);
		otmp->cursed = 0;
		++otmp->spe;
		break;
	    }
	case SCR_DESTROY_ARMOR:
	    if (confused) {
		struct obj *otmp = some_armor();
		if (!otmp) {
		    strange_feeling(scroll, "Your bones itch.");
		    return 1;
		}
		pline("Your %s glows purple for a moment.", c_Objects[otmp->otyp].oc_name);
		otmp->rustfree = 0;
		break;
	    }
	    if (uarm) {
		pline("Your armor turns to dust and falls to the floor!");
		useup(uarm);
	    } else if (uarmh) {
		pline("Your helmet turns to dust and is blown away!");
		useup(uarmh);
	    } else if (uarmg) {
		pline("Your gloves vanish!");
		useup(uarmg);
		selftouch("You");
	    } else {
		strange_feeling(scroll, "Your skin itches.");
		return 1;
	    }
	    break;
	case SCR_CONFUSE_MONSTER:
	    if (confused) {
		pline("Your hands begin to glow purple.");
		Confusion += rnd(100);
	    } else {
		pline("Your hands begin to glow blue.");
		_u.umconf = 1;
	    }
	    break;
	case SCR_SCARE_MONSTER:
	    {
		int ct = 0;
		for (struct monst* m = _level->monsters; m; m = m->nmon) {
		    if (cansee(m->mx, m->my)) {
			if (confused)
			    m->mflee = m->mfroz = m->msleep = 0;
			else
			    m->mflee = 1;
			++ct;
		    }
		}
		if (!ct) {
		    if (confused)
			pline("You hear sad wailing in the distance.");
		    else
			pline("You hear maniacal laughter in the distance.");
		}
		break;
	    }
	case SCR_BLANK_PAPER:
	    if (confused)
		pline("You see strange patterns on this scroll.");
	    else
		pline("This scroll seems to be blank.");
	    break;
	case SCR_REMOVE_CURSE:
	    {
		struct obj *obj;
		if (confused)
		    pline("You feel like you need some help.");
		else
		    pline("You feel like someone is helping you.");
		for (obj = invent; obj; obj = obj->nobj)
		    if (obj->owornmask)
			obj->cursed = confused;
		break;
	    }
	case SCR_CREATE_MONSTER:
	    {
		int cnt = 1;

		if (!rn2(73))
		    cnt += rnd(4);
		if (confused)
		    cnt += 12;
		while (cnt--)
		    makemon(confused ? PM_ACID_BLOB : NULL, _u.ux, _u.uy);
		break;
	    }
	case SCR_ENCHANT_WEAPON:
	    if (uwep && confused) {
		pline("Your %s glows silver for a moment.", c_Objects[uwep->otyp].oc_name);
		uwep->rustfree = 1;
	    } else if (!chwepon(scroll, 1))	// tests for !uwep
		return 1;
	    break;
	case SCR_DAMAGE_WEAPON:
	    if (uwep && confused) {
		pline("Your %s glows purple for a moment.", c_Objects[uwep->otyp].oc_name);
		uwep->rustfree = 0;
	    } else if (!chwepon(scroll, -1))	// tests for !uwep
		return 1;
	    break;
	case SCR_GENOCIDE:
	    {
		char buf[BUFSZ];

		pline("You have found a scroll of genocide!");
		known = true;
		if (confused)
		    *buf = _u.usym;
		else
		    do {
			pline("What monster do you want to genocide (Type the letter)? ");
			getlin(buf);
		    } while (strlen(buf) != 1 || !monstersym(*buf));
		if (is_monster_genocided (*buf)) {
		    pline("Such monsters do not exist in this world.");
		    break;
		} else
		    make_monster_genocided (*buf);
		for (struct monst *m = _level->monsters, *nm; m; m = nm) {
		    nm = m->nmon;
		    if (m->data->mlet == *buf)
			mondead(m);
		}
		pline("Wiped out all %c's.", *buf);
		if (*buf == _u.usym) {
		    killer = "scroll of genocide";
		    _u.uhp = -1;
		}
		break;
	    }
	case SCR_LIGHT:
	    if (!Blind)
		known = true;
	    litroom(!confused);
	    break;
	case SCR_TELEPORTATION:
	    if (confused)
		level_tele();
	    else {
		int uroom = inroom(_u.ux, _u.uy);
		tele();
		if (uroom != inroom(_u.ux, _u.uy))
		    known = true;
	    }
	    break;
	case SCR_GOLD_DETECTION:
	    // Unfortunately this code has become slightly less elegant,
	    // now that gold and traps no longer are of the same type.
	    if (confused) {
		if (!_level->traps) {
		    strange_feeling(scroll, "Your toes stop itching.");
		    return 1;
		} else {
		    for (const struct trap* t = _level->traps; t; t = t->ntrap)
			if (t->tx != _u.ux || t->ty != _u.uy)
			    goto outtrapmap;
		    // only under me - no separate display required
		    pline ("Your toes itch!");
		    break;
		  outtrapmap:
		    cls();
		    for (const struct trap* t = _level->traps; t; t = t->ntrap)
			at(t->tx, t->ty, '$');
		    prme();
		    pline ("You feel very greedy!");
		}
	    } else {
		if (!_level->money) {
		    strange_feeling(scroll, "You feel materially poor.");
		    return 1;
		} else {
		    known = true;
		    for (const struct gold* g = _level->money; g; g = g->ngold)
			if (g->gx != _u.ux || g->gy != _u.uy)
			    goto outgoldmap;
		    // only under me - no separate display required
		    pline ("You notice some gold between your feet.");
		    break;
		  outgoldmap:
		    cls();
		    for (const struct gold* g = _level->money; g; g = g->ngold)
			at(g->gx, g->gy, '$');
		    prme();
		    pline ("You feel very greedy, and sense gold!");
		}
	    }
	    // common sequel
	    more();
	    docrt();
	    break;
	case SCR_FOOD_DETECTION:
	    {
		int ct = 0, ctu = 0;
		struct obj *obj;
		char foodsym = confused ? POTION_SYM : FOOD_SYM;

		for (obj = _level->objects; obj; obj = obj->nobj)
		    if (obj->olet == FOOD_SYM) {
			if (obj->ox == _u.ux && obj->oy == _u.uy)
			    ++ctu;
			else
			    ++ct;
		    }
		if (!ct && !ctu) {
		    strange_feeling(scroll, "Your nose twitches.");
		    return 1;
		} else if (!ct) {
		    known = true;
		    pline("You smell %s close nearby.", confused ? "something" : "food");

		} else {
		    known = true;
		    cls();
		    for (obj = _level->objects; obj; obj = obj->nobj)
			if (obj->olet == foodsym)
			    at(obj->ox, obj->oy, FOOD_SYM);
		    prme();
		    pline("Your nose tingles and you smell %s!", confused ? "something" : "food");
		    more();
		    docrt();
		}
		break;
	    }
	case SCR_IDENTIFY:
	    // known = true;
	    if (confused)
		pline("You identify this as an identify scroll.");
	    else
		pline("This is an identify scroll.");
	    useup(scroll);
	    set_object_known (SCR_IDENTIFY);
	    if (!confused)
		while (!ggetobj("identify", identify, rn2(5) ? 1 : rn2(5))
		       && invent);
	    return 1;
	case SCR_MAGIC_MAPPING:
	    {
		struct rm *lev;
		int num, zx, zy;

		known = true;
		pline("On this scroll %s a map!", confused ? "was" : "is");
		for (zy = 0; zy < ROWNO; ++zy)
		    for (zx = 0; zx < COLNO; ++zx) {
			if (confused && rn2(7))
			    continue;
			lev = &(_level->l[zx][zy]);
			if ((num = lev->typ) == 0)
			    continue;
			if (num == SCORR) {
			    lev->typ = CORR;
			    lev->scrsym = CORR_SYM;
			} else if (num == SDOOR) {
			    lev->typ = DOOR;
			    lev->scrsym = '+';
			    // do sth in doors ?
			} else if (lev->seen)
			    continue;
			if (num != ROOM) {
			    lev->seen = lev->new = 1;
			    if (lev->scrsym == ' ' || !lev->scrsym)
				newsym(zx, zy);
			    else
				on_scr(zx, zy);
			}
		    }
		break;
	    }
	case SCR_AMNESIA:
	    {
		int zx, zy;

		known = true;
		for (zx = 0; zx < COLNO; ++zx)
		    for (zy = 0; zy < ROWNO; ++zy)
			if (!confused || rn2(7))
			    if (!cansee(zx, zy))
				_level->l[zx][zy].seen = 0;
		docrt();
		pline("Thinking of Maud you forget everything else.");
		break;
	    }
	case SCR_FIRE:
	    {
		int num = 0;
		struct monst *mtmp;

		known = true;
		if (confused) {
		    pline("The scroll catches fire and you burn your hands.");
		    losehp(1, "scroll of fire");
		} else {
		    pline("The scroll erupts in a tower of flame!");
		    if (Fire_resistance)
			pline("You are uninjured.");
		    else {
			num = rnd(6);
			_u.uhpmax -= num;
			losehp(num, "scroll of fire");
		    }
		}
		num = (2 * num + 1) / 3;
		for (mtmp = _level->monsters; mtmp; mtmp = mtmp->nmon) {
		    if (dist(mtmp->mx, mtmp->my) < 3) {
			mtmp->mhp -= num;
			if (strchr("FY", mtmp->data->mlet))
			    mtmp->mhp -= 3 * num;	// this might well kill
							// 'F's
			if (mtmp->mhp < 1) {
			    killed(mtmp);
			    break;	// primitive
			}
		    }
		}
		break;
	    }
	default:
	    impossible("What weird language is this written in? (%u)", scroll->otyp);
    }
    if (!is_object_known (scroll->otyp)) {
	if (known && !confused) {
	    set_object_known (scroll->otyp);
	    more_experienced(0, 10);
	}
    }
    useup(scroll);
    return 1;
}

int identify (struct obj* o)
{
    set_object_known (o->otyp);
    o->known = o->dknown = 1;
    prinv(o);
    return 1;
}

void litroom(bool on)
{
    int num, zx, zy;

    // first produce the text (provided he is not blind)
    if (Blind)
	goto do_it;
    if (!on) {
	if (!_level->stair.dn.x || _level->l[_u.ux][_u.uy].typ == CORR || !_level->l[_u.ux][_u.uy].lit) {
	    pline("It seems even darker in here than before.");
	    return;
	} else
	    pline("It suddenly becomes dark in here.");
    } else {
	if (!_level->stair.dn.x) {
	    pline("Nothing Happens.");
	    return;
	}
	if (_level->l[_u.ux][_u.uy].typ == CORR) {
	    pline("The corridor lights up around you, then fades.");
	    return;
	} else if (_level->l[_u.ux][_u.uy].lit) {
	    pline("The light here seems better now.");
	    return;
	} else
	    pline("The room is lit.");
    }

  do_it:
    if (_level->l[_u.ux][_u.uy].lit == on)
	return;
    if (_level->l[_u.ux][_u.uy].typ == DOOR) {
	if (IS_ROOM(_level->l[_u.ux][_u.uy + 1].typ))
	    zy = _u.uy + 1;
	else if (IS_ROOM(_level->l[_u.ux][_u.uy - 1].typ))
	    zy = _u.uy - 1;
	else
	    zy = _u.uy;
	if (IS_ROOM(_level->l[_u.ux + 1][_u.uy].typ))
	    zx = _u.ux + 1;
	else if (IS_ROOM(_level->l[_u.ux - 1][_u.uy].typ))
	    zx = _u.ux - 1;
	else
	    zx = _u.ux;
    } else {
	zx = _u.ux;
	zy = _u.uy;
    }
    for (seelx = _u.ux; (num = _level->l[seelx - 1][zy].typ) != CORR && num != 0; --seelx);
    for (seehx = _u.ux; (num = _level->l[seehx + 1][zy].typ) != CORR && num != 0; ++seehx);
    for (seely = _u.uy; (num = _level->l[zx][seely - 1].typ) != CORR && num != 0; --seely);
    for (seehy = _u.uy; (num = _level->l[zx][seehy + 1].typ) != CORR && num != 0; ++seehy);
    for (zy = seely; zy <= seehy; ++zy)
	for (zx = seelx; zx <= seehx; ++zx) {
	    _level->l[zx][zy].lit = on;
	    if (!Blind && dist(zx, zy) > 2) {
		if (on)
		    prl(zx, zy);
		else
		    nosee(zx, zy);
	    }
	}
    if (!on)
	seehx = 0;
}

// Test whether we may genocide all monsters with symbol  ch
int monstersym (int ch)
{
    // can't genocide certain monsters
    if (strchr("12 &:", ch))
	return false;
    if (ch == pm_eel.mlet)
	return true;
    for (unsigned i = 0; i < ArraySize(c_Monsters); ++i)
	if (c_Monsters[i].mlet == ch)
	    return true;
    return false;
}
