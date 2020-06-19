// This file is free software, distributed under the BSD license.

#include	"hack.h"
#include	"extern.h"
#include	"mkroom.h"

static void use_camera(struct obj *);
static int in_ice_box(struct obj *);
static int ck_ice_box(struct obj *);
static int out_ice_box(struct obj *);
static void use_ice_box(struct obj *);
static struct monst *bchit(int, int, int, int);
static int dig(void);
static int use_pick_axe(struct obj *);

int doapply(void)
{
    int res = 1;

    struct obj* obj = getobj("(", "use or apply");
    if (!obj)
	return 0;

    switch (obj->otyp) {
	case EXPENSIVE_CAMERA:
	    use_camera(obj);
	    break;
	case ICE_BOX:
	    use_ice_box(obj);
	    break;
	case PICK_AXE:
	    res = use_pick_axe(obj);
	    break;
	case CAN_OPENER:
	    if (!carrying(TIN)) {
		pline("You have no can to open.");
		nomul(0);
		return 0;
	    }
	    pline("You cannot open a tin without eating its contents.");
	    pline("In order to eat, use the 'e' command.");
	    if (obj != uwep)
		pline("Opening the tin will be much easier if you wield the can-opener.");
	    nomul(0);
	    return 0;
	default:
	    pline("Sorry, I don't know how to use that.");
	    nomul(0);
	    return 0;
    }
    nomul(0);
    return res;
}

static void use_camera (struct obj* obj UNUSED)
{
    if (!getdir(1)) {	       // ask: in what direction?
	_wflags.move = multi = 0;
	return;
    }
    if (_u.dz) {
	pline("You take a picture of the %s.", (_u.dz > 0) ? "floor" : "ceiling");
	return;
    }
    struct monst* mtmp = bchit (_u.dx, _u.dy, COLNO, '!');
    if (mtmp) {
	if (mtmp->msleep) {
	    mtmp->msleep = 0;
	    pline("The flash awakens %s.", monnam(mtmp));	// a3
	} else if (mtmp->data->mlet != 'y') {
	    if (mtmp->mcansee || mtmp->mblinded) {
		int tmp = dist(mtmp->mx, mtmp->my);
		int tmp2;
		if (cansee(mtmp->mx, mtmp->my))
		    pline("%s is blinded by the flash!", Monnam(mtmp));
		setmangry(mtmp);
		if (tmp < 9 && !mtmp->isshk && rn2(4)) {
		    mtmp->mflee = 1;
		    if (rn2(4))
			mtmp->mfleetim = rnd(100);
		}
		if (tmp < 3)
		    mtmp->mcansee = mtmp->mblinded = 0;
		else {
		    tmp2 = mtmp->mblinded;
		    tmp2 += rnd(1 + 50 / tmp);
		    if (tmp2 > 127)
			tmp2 = 127;
		    mtmp->mblinded = tmp2;
		    mtmp->mcansee = 0;
		}
	    }
	}
    }
}

static struct obj* current_ice_box = NULL;	// a local variable of use_ice_box, to be
						// used by its local procedures in/ck_ice_box

static int in_ice_box (struct obj *obj)
{
    if (obj == current_ice_box) {
	pline ("You must be kidding.");
	return 0;
    }
    if (obj->owornmask & (W_ARMOR | W_RING)) {
	pline ("You cannot refrigerate something you are wearing.");
	return 0;
    }
    if (obj->owt + current_ice_box->owt > 70) {
	pline ("It won't fit.");
	return 1;	       // be careful!
    }
    if (obj == uwep) {
	if (uwep->cursed) {
	    pline("Your weapon is welded to your hand!");
	    return 0;
	}
	setuwep (NULL);
    }
    current_ice_box->owt += obj->owt;
    freeinv (obj);
    obj->o_cnt_id = current_ice_box->o_id;
    obj->nobj = fcobj;
    fcobj = obj;
    obj->age = _u.moves - obj->age;	// actual age
    return 1;
}

static int ck_ice_box (struct obj *obj)
{
    return obj->o_cnt_id == current_ice_box->o_id;
}

static int out_ice_box(struct obj *obj)
{
    struct obj *otmp;
    if (obj == fcobj)
	fcobj = fcobj->nobj;
    else {
	for (otmp = fcobj; otmp->nobj != obj; otmp = otmp->nobj)
	    if (!otmp->nobj)
		panic("out_ice_box");
	otmp->nobj = obj->nobj;
    }
    current_ice_box->owt -= obj->owt;
    obj->age = _u.moves - obj->age;	// simulated point of time
    addinv(obj);
    return 0;
}

static void use_ice_box (struct obj *obj)
{
    int cnt = 0;
    current_ice_box = obj;     // for use by in/out_ice_box
    for (struct obj* otmp = fcobj; otmp; otmp = otmp->nobj)
	if (otmp->o_cnt_id == obj->o_id)
	    ++cnt;
    if (!cnt)
	pline("Your ice-box is empty.");
    else {
	pline("Do you want to take something out of the ice-box? [yn] ");
	if (readchar() == 'y')
	    if (askchain(fcobj, NULL, 0, out_ice_box, ck_ice_box, 0))
		return;
	pline("That was all. Do you wish to put something in? [yn] ");
	if (readchar() != 'y')
	    return;
    }
    // call getobj: 0: allow cnt; #: allow all types; %: expect food
    struct obj* otmp = getobj("0#%", "put in");
    if (!otmp || !in_ice_box(otmp))
	_wflags.move = multi = 0;
}

static struct monst *bchit(int ddx, int ddy, int range, int sym)
{
    struct monst *mtmp = NULL;
    int bchx = _u.ux, bchy = _u.uy;

    if (sym)
	Tmp_at(-1, sym);       // open call
    while (range--) {
	bchx += ddx;
	bchy += ddy;
	if ((mtmp = m_at(bchx, bchy)) != NULL)
	    break;
	if (!ZAP_POS(_level->l[bchx][bchy].typ)) {
	    bchx -= ddx;
	    bchy -= ddy;
	    break;
	}
	if (sym)
	    Tmp_at(bchx, bchy);
    }
    if (sym)
	Tmp_at(-1, -1);
    return mtmp;
}

static int dig_effort = 0;	// effort expended on current pos
static uint8_t dig_level = 0;
static struct coord dig_pos = { 0, 0 };
static bool dig_down = false;

static int dig (void)
{
    int dpx = dig_pos.x, dpy = dig_pos.y;

    // perhaps a nymph stole his pick-axe while he was busy digging
    // or perhaps he teleported away
    if (!uwep || uwep->otyp != PICK_AXE || dig_level != _u.dlevel || ((dig_down && (dpx != _u.ux || dpy != _u.uy)) || (!dig_down && dist(dpx, dpy) > 2)))
	return 0;

    dig_effort += 10 + abon() + uwep->spe + rn2(5);
    if (dig_down) {
	if (!_level->stair.dn.x) {
	    pline("The floor here seems too hard to dig in.");
	    return 0;
	}
	if (dig_effort > 250) {
	    dighole();
	    return 0;	       // done with digging
	}
	if (dig_effort > 50) {
	    struct trap *ttmp = t_at(dpx, dpy);
	    if (!ttmp) {
		ttmp = maketrap(dpx, dpy, PIT);
		ttmp->tseen = 1;
		pline("You have dug a pit.");
		_u.utrap = rn1(4, 2);
		_u.utraptype = TT_PIT;
		return 0;
	    }
	}
    } else if (dig_effort > 100) {
	const char* digtxt = "Now what exactly was it that you were digging in?";
	struct rm* lev = &_level->l[dpx][dpy];
	struct obj* obj = sobj_at(ENORMOUS_ROCK, dpx, dpy);
	if (obj) {
	    fracture_rock(obj);
	    digtxt = "The rock falls apart.";
	} else if (!lev->typ || lev->typ == SCORR) {
	    lev->typ = CORR;
	    digtxt = "You succeeded in cutting away some rock.";
	} else if (lev->typ == HWALL || lev->typ == VWALL || lev->typ == SDOOR) {
	    lev->typ = _level->stair.dn.x ? DOOR : ROOM;
	    digtxt = "You just made an opening in the wall.";
	}
	mnewsym(dpx, dpy);
	prl(dpx, dpy);
	pline(digtxt);	       // after mnewsym & prl
	return 0;
    } else {
	if (IS_WALL(_level->l[dpx][dpy].typ)) {
	    int rno = inroom(dpx, dpy);
	    if (rno >= 0 && _level->rooms[rno].rtype >= 8) {
		pline("This wall seems too hard to dig into.");
		return 0;
	    }
	}
	pline("You hit the rock with all your might.");
    }
    return 1;
}

// When will hole be finished? Very rough indication used by shopkeeper.
int holetime(void)
{
    return occupation == dig ? (250 - dig_effort) / 20 : -1;
}

void dighole(void)
{
    if (!_level->stair.dn.x) {
	pline("The floor here seems too hard to dig in.");
	return;
    }
    struct trap* ttmp = t_at(_u.ux, _u.uy);
    if (ttmp)
	ttmp->ttyp = TRAPDOOR;
    else
	ttmp = maketrap(_u.ux, _u.uy, TRAPDOOR);
    ttmp->tseen = 1;
    pline("You've made a hole in the floor.");
    if (inshop())
	shopdig(1);
    pline("You fall through ...");
    if (_u.utraptype == TT_PIT) {
	_u.utrap = 0;
	_u.utraptype = 0;
    }
    goto_level(_u.dlevel + 1, false);
}

static int use_pick_axe(struct obj *obj)
{
    char dirsyms[12];
    char *dsp = dirsyms, *sdp = sdir;
    struct monst *mtmp;
    struct rm *lev;
    int rx, ry, res = 0;

    if (obj != uwep) {
	if (uwep && uwep->cursed) {
	    // Andreas Bormann - ihnp4!decvax!mcvax!unido!ab
	    pline("Since your weapon is welded to your hand,");
	    pline("you cannot use that pick-axe.");
	    return 0;
	}
	pline("You now wield %s.", doname(obj));
	setuwep(obj);
	res = 1;
    }
    while (*sdp) {
	movecmd(*sdp);  // sets _u.dx and _u.dy and _u.dz
	rx = _u.ux + _u.dx;
	ry = _u.uy + _u.dy;
	if (_u.dz > 0 || (_u.dz == 0 && isok(rx, ry) && (IS_ROCK(_level->l[rx][ry].typ)
						       || sobj_at(ENORMOUS_ROCK, rx, ry))))
	    *dsp++ = *sdp;
	++sdp;
    }
    *dsp = 0;
    pline("In what direction do you want to dig? [%s] ", dirsyms);
    if (!getdir(0))	       // no txt
	return res;
    if (_u.dz < 0)
	pline("You cannot reach the ceiling.");
    else if (_u.dz == 0) {
	if (Confusion)
	    confdir();
	rx = _u.ux + _u.dx;
	ry = _u.uy + _u.dy;
	if ((mtmp = m_at(rx, ry)) && attack(mtmp))
	    return 1;
	if (!isok(rx, ry)) {
	    pline("Clash!");
	    return 1;
	}
	lev = &_level->l[rx][ry];
	if (lev->typ == DOOR)
	    pline("Your %s against the door.", aobjnam(obj, "clang"));
	else if (!IS_ROCK(lev->typ)
		 && !sobj_at(ENORMOUS_ROCK, rx, ry)) {
	    // ACCESSIBLE or POOL
	    pline("You swing your %s through thin air.", aobjnam(obj, NULL));
	} else {
	    if (dig_pos.x != rx || dig_pos.y != ry || dig_level != _u.dlevel || dig_down) {
		dig_down = false;
		dig_pos.x = rx;
		dig_pos.y = ry;
		dig_level = _u.dlevel;
		dig_effort = 0;
		pline("You start digging.");
	    } else
		pline("You continue digging.");
	    occupation = dig;
	    occtxt = "digging";
	}
    } else if (Levitation) {
	pline("You cannot reach the floor.");
    } else {
	if (dig_pos.x != _u.ux || dig_pos.y != _u.uy || dig_level != _u.dlevel || !dig_down) {
	    dig_down = true;
	    dig_pos.x = _u.ux;
	    dig_pos.y = _u.uy;
	    dig_level = _u.dlevel;
	    dig_effort = 0;
	    pline("You start digging in the floor.");
	    if (inshop())
		shopdig(0);
	} else
	    pline("You continue digging in the floor.");
	occupation = dig;
	occtxt = "digging";
    }
    return 1;
}
