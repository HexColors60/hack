// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

char mkobjstr[] = "))[[!!!!????%%%%/=**))[[!!!!????%%%%/=**(%";

struct obj *mkobj_at(int let, int x, int y)
{
    struct obj *o = mkobj(let);
    o->ox = x;
    o->oy = y;
    o->nobj = _level->objects;
    _level->objects = o;
    return o;
}

void mksobj_at(int otyp, int x, int y)
{
    struct obj *o = mksobj(otyp);
    o->ox = x;
    o->oy = y;
    o->nobj = _level->objects;
    _level->objects = o;
}

struct obj *mkobj(int let)
{
    if (!let)
	let = mkobjstr[rn2(sizeof(mkobjstr) - 1)];
    return mksobj(letter(let) ? CORPSE + ((let > 'Z') ? (let - 'a' + 'Z' - '@' + 1) : (let - '@'))
		   : (int) probtype(let)
	    )
	;
}

struct obj* mksobj (int otyp)
{
    struct obj* o = newobj();
    o->age = _u.moves;
    o->o_id = _wflags.ident++;
    o->quan = 1;
    o->otyp = otyp;
    o->olet = c_Objects[otyp].oc_olet;
    o->dknown = strchr("/=!?*", o->olet) ? 0 : 1;
    switch (o->olet) {
	case WEAPON_SYM:
	    o->quan = (o->otyp <= ROCK) ? rn1(6, 6) : 1;
	    if (!rn2(11))
		o->spe = rnd(3);
	    else if (!rn2(10)) {
		o->cursed = 1;
		o->spe = -rnd(3);
	    }
	    break;
	case FOOD_SYM:
	    if (o->otyp >= CORPSE)
		break;
	    // fallthrough
	case GEM_SYM:
	    o->quan = rn2(6) ? 1 : 2;
	case TOOL_SYM:
	case ROCK_SYM:
	case POTION_SYM:
	case SCROLL_SYM:
	case AMULET_SYM:
	    break;
	case ARMOR_SYM:
	    if (!rn2(8))
		o->cursed = 1;
	    if (!rn2(10))
		o->spe = rnd(3);
	    else if (!rn2(9)) {
		o->spe = -rnd(3);
		o->cursed = 1;
	    }
	    break;
	case WAND_SYM:
	    if (o->otyp == WAN_WISHING)
		o->spe = 3;
	    else
		o->spe = rn1(5, (c_Objects[o->otyp].bits & NODIR) ? 11 : 4);
	    break;
	case RING_SYM:
	    if (c_Objects[o->otyp].bits & SPEC) {
		if (!rn2(3)) {
		    o->cursed = 1;
		    o->spe = -rnd(2);
		} else
		    o->spe = rnd(2);
	    } else if (o->otyp == RIN_TELEPORTATION || o->otyp == RIN_AGGRAVATE_MONSTER || o->otyp == RIN_HUNGER || !rn2(9))
		o->cursed = 1;
	    break;
	default:
	    panic("impossible mkobj");
    }
    o->owt = weight(o);
    return o;
}

int letter(int c)
{
    return ('@' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

int weight(struct obj *obj)
{
    int wt = c_Objects[obj->otyp].oc_weight;
    return wt ? wt * obj->quan : (obj->quan + 1) / 2;
}

void mkgold(long num, int x, int y)
{
    struct gold *gold;
    long amount = (num ? num : 1 + (rnd(_u.dlevel + 2) * rnd(30)));

    if ((gold = g_at(x, y)) != NULL)
	gold->amount += amount;
    else {
	gold = newgold();
	gold->ngold = _level->money;
	gold->gx = x;
	gold->gy = y;
	gold->amount = amount;
	_level->money = gold;
	// do sth with display?
    }
}
