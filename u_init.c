// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <ctype.h>

enum {
    UNDEF_TYP	= 0,
    UNDEF_SPE	= '\177'
};

char pl_character[PL_CSIZ] = {};
static const char* roles[] = {	// must all have distinct first letter
    "Tourist",
    "Speleologist",
    "Fighter",
    "Knight",
    "Caveman",
    "Wizard"
};
enum { NR_OF_ROLES = ArraySize(roles) };

struct trobj {
    uint8_t	trotyp;
    int8_t	trspe;
    char	trolet;
    uint8_t	trquan:6;
    uint8_t	trknown:1;
};

static struct trobj Cave_man[] = {
    {MACE, 1, WEAPON_SYM, 1, 1},
    {BOW, 1, WEAPON_SYM, 1, 1},
    {ARROW, 0, WEAPON_SYM, 25, 1},	// quan is variable
    {LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1},
    {0, 0, 0, 0, 0}
};

static struct trobj Fighter[] = {
    {TWO_HANDED_SWORD, 0, WEAPON_SYM, 1, 1},
    {RING_MAIL, 0, ARMOR_SYM, 1, 1},
    {0, 0, 0, 0, 0}
};

static struct trobj Knight[] = {
    {LONG_SWORD, 0, WEAPON_SYM, 1, 1},
    {SPEAR, 2, WEAPON_SYM, 1, 1},
    {RING_MAIL, 1, ARMOR_SYM, 1, 1},
    {HELMET, 0, ARMOR_SYM, 1, 1},
    {SHIELD, 0, ARMOR_SYM, 1, 1},
    {PAIR_OF_GLOVES, 0, ARMOR_SYM, 1, 1},
    {0, 0, 0, 0, 0}
};

static struct trobj Speleologist[] = {
    {STUDDED_LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1},
    {UNDEF_TYP, 0, POTION_SYM, 2, 0},
    {FOOD_RATION, 0, FOOD_SYM, 3, 1},
    {PICK_AXE, UNDEF_SPE, TOOL_SYM, 1, 0},
    {ICE_BOX, 0, TOOL_SYM, 1, 0},
    {0, 0, 0, 0, 0}
};

static struct trobj Tinopener[] = {
    {CAN_OPENER, 0, TOOL_SYM, 1, 1},
    {0, 0, 0, 0, 0}
};

static struct trobj Tourist[] = {
    {UNDEF_TYP, 0, FOOD_SYM, 10, 1},
    {POT_EXTRA_HEALING, 0, POTION_SYM, 2, 0},
    {EXPENSIVE_CAMERA, 0, TOOL_SYM, 1, 1},
    {DART, 2, WEAPON_SYM, 25, 1},	// quan is variable
    {0, 0, 0, 0, 0}
};

static struct trobj Wizard[] = {
    {ELVEN_CLOAK, 0, ARMOR_SYM, 1, 1},
    {UNDEF_TYP, UNDEF_SPE, WAND_SYM, 2, 0},
    {UNDEF_TYP, UNDEF_SPE, RING_SYM, 2, 0},
    {UNDEF_TYP, UNDEF_SPE, POTION_SYM, 2, 0},
    {UNDEF_TYP, UNDEF_SPE, SCROLL_SYM, 3, 0},
    {0, 0, 0, 0, 0}
};

static int role_index (char pc);

void u_init(void)
{
    int chosen_role = 0;
    char pc = pl_character[0];
    if (pc) {
	pc = toupper (pc);
	chosen_role = role_index(pc);
	if (chosen_role < 0) {
	    printf("\nUnknown role: %c\n", pc);
	    pl_character[0] = pc = 0;
	}
    }
    if (!pc) {
	printf("\nI'll choose a character for you.\n");
	chosen_role = rn2(NR_OF_ROLES);
	pc = roles[chosen_role][0];
	printf("This game you will be a %s.\n", roles[chosen_role]);
	getret();
	// give him some feedback in case mklev takes much time
	putchar('\n');
	fflush(stdout);
    }

    strncpy(pl_character, roles[chosen_role], PL_CSIZ - 1);
    pl_character[PL_CSIZ - 1] = 0;
    _u.usym = '@';
    _u.ulevel = 1;
    init_uhunger();
    uarm = uarm2 = uarmh = uarms = uarmg = uwep = uleft = uright = 0;

    switch (pc) {
	case 'c':
	case 'C':
	    Cave_man[2].trquan = 12 + rnd(9) * rnd(9);
	    _u.uhp = _u.uhpmax = 16;
	    _u.ustr = _u.ustrmax = 18;
	    ini_inv(Cave_man);
	    break;
	case 't':
	case 'T':
	    Tourist[3].trquan = 20 + rnd(20);
	    _u.ugold = _u.ugold0 = rnd(1000);
	    _u.uhp = _u.uhpmax = 10;
	    _u.ustr = _u.ustrmax = 8;
	    ini_inv(Tourist);
	    if (!rn2(25))
		ini_inv(Tinopener);
	    break;
	case 'w':
	case 'W':
	    for (unsigned i = 1; i <= 4; ++i)
		if (!rn2(5))
		    Wizard[i].trquan += rn2(3) - 1;
	    _u.uhp = _u.uhpmax = 15;
	    _u.ustr = _u.ustrmax = 16;
	    ini_inv(Wizard);
	    break;
	case 's':
	case 'S':
	    Fast = INTRINSIC;
	    Stealth = INTRINSIC;
	    _u.uhp = _u.uhpmax = 12;
	    _u.ustr = _u.ustrmax = 10;
	    ini_inv(Speleologist);
	    if (!rn2(10))
		ini_inv(Tinopener);
	    break;
	case 'k':
	case 'K':
	    _u.uhp = _u.uhpmax = 12;
	    _u.ustr = _u.ustrmax = 10;
	    ini_inv(Knight);
	    break;
	case 'f':
	case 'F':
	    _u.uhp = _u.uhpmax = 14;
	    _u.ustr = _u.ustrmax = 17;
	    ini_inv(Fighter);
	    break;
	default:	       // impossible
	    _u.uhp = _u.uhpmax = 12;
	    _u.ustr = _u.ustrmax = 16;
    }
    find_ac();
    if (!rn2(20)) {
	int d = rn2(7) - 2;	// biased variation
	_u.ustr += d;
	_u.ustrmax += d;
    }

    // make sure he can carry all he has - especially for T's
    while (inv_weight() > 0 && _u.ustr < 118)
	++_u.ustr, ++_u.ustrmax;
}

void ini_inv(struct trobj *trop)
{
    struct obj *obj;
    while (trop->trolet) {
	obj = mkobj(trop->trolet);
	obj->known = trop->trknown;
	// not obj->dknown = 1; - let him look at it at least once
	obj->cursed = 0;
	if (obj->olet == WEAPON_SYM) {
	    obj->quan = trop->trquan;
	    trop->trquan = 1;
	}
	if (trop->trspe != UNDEF_SPE)
	    obj->spe = trop->trspe;
	if (trop->trotyp != UNDEF_TYP)
	    obj->otyp = trop->trotyp;
	else if (obj->otyp == WAN_WISHING)	// gitpyr!robert
	    obj->otyp = WAN_DEATH;
	obj->owt = weight(obj);	// defined after setting otyp+quan
	obj = addinv(obj);
	if (obj->olet == ARMOR_SYM) {
	    switch (obj->otyp) {
		case SHIELD:
		    if (!uarms)
			setworn(obj, W_ARMS);
		    break;
		case HELMET:
		    if (!uarmh)
			setworn(obj, W_ARMH);
		    break;
		case PAIR_OF_GLOVES:
		    if (!uarmg)
			setworn(obj, W_ARMG);
		    break;
		case ELVEN_CLOAK:
		    if (!uarm2)
			setworn(obj, W_ARM);
		    break;
		default:
		    if (!uarm)
			setworn(obj, W_ARM);
	    }
	}
	if (obj->olet == WEAPON_SYM)
	    if (!uwep)
		setuwep(obj);
	if (--trop->trquan)
	    continue;	       // make a similar object
	++trop;
    }
}

void plnamesuffix(void)
{
    char *p;
    if ((p = strrchr(plname, '-')) != NULL) {
	*p = 0;
	pl_character[0] = p[1];
	pl_character[1] = 0;
	if (!plname[0]) {
	    askname();
	    plnamesuffix();
	}
    }
}

static int role_index (char pc)
{
    for (int i = 0; i < NR_OF_ROLES; ++i)
	if (roles[i][0] == pc)
	    return i;
    return -1;
}

void you_dtor (void)
{
    // nothing to clean up right now
}
