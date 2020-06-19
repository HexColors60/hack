// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "mfndpos.h"
#include "mkroom.h"
#include "eshk.h"

static void setpaid(void);
static void addupbill(void);
static void findshk(int);
static struct bill_x *onbill(struct obj *);
static void pay(long, struct monst *);
static int dopayobj(struct bill_x *);
static int getprice(struct obj *);
static int realhunger(void);

#define	ESHK(mon)	((struct eshk *)(&(mon->mextra[0])))
#define	NOTANGRY(mon)	mon->mpeaceful
#define	ANGRY(mon)	!NOTANGRY(mon)

// Descriptor of current shopkeeper. Note that the bill need not be
// per-shopkeeper, since it is valid only when in a shop.
static struct monst *shopkeeper = 0;
static struct bill_x *bill;
static int shlevel = 0;		// level of this shopkeeper
// only accessed here and by save & restore
static long int total;		// filled by addupbill()
static long int followmsg;	// last time of follow message

// invariants: obj->unpaid iff onbill(obj) [unless bp->useup]
// obj->quan <= bp->bquan

const char shtypes[] = {       // 8 shoptypes: 7 specialized, 1 mixed
    RING_SYM, WAND_SYM, WEAPON_SYM, FOOD_SYM, SCROLL_SYM,
    POTION_SYM, ARMOR_SYM, 0
};

static const char *const shopnam[] = {
    "engagement ring", "walking cane", "antique weapon",
    "delicatessen", "second hand book", "liquor",
    "used armor", "assorted antiques"
};

// called in do_name.c
char *shkname (struct monst *mtmp)
{
    return ESHK(mtmp)->shknam;
}

// called in mon.c
void shkdead (struct monst *mtmp)
{
    struct eshk *eshk = ESHK(mtmp);

    if (eshk->shoplevel == _u.dlevel)
	_level->rooms[eshk->shoproom].rtype = 0;
    if (mtmp == shopkeeper) {
	setpaid();
	shopkeeper = 0;
	bill = (struct bill_x *) -1000;	// dump core when
					// referenced
    }
}

void replshk (struct monst *m, struct monst *m2)
{
    if (m == shopkeeper) {
	shopkeeper = m2;
	bill = &(ESHK(shopkeeper)->bill[0]);
    }
}

static void setpaid(void)
{
    // caller has checked that shopkeeper exists
    // either we paid or left the shop or he just died
    for (struct obj* o = invent; o; o = o->nobj)
	o->unpaid = 0;
    for (struct obj* o = _level->objects; o; o = o->nobj)
	o->unpaid = 0;
    for (struct obj* o = fcobj; o; o = o->nobj)
	o->unpaid = 0;
    for (struct monst* m = _level->monsters; m; m = m->nmon)
	for (struct obj* o = m->minvent; o; o = o->nobj)
	    o->unpaid = 0;
    for (struct monst* m = fallen_down; m; m = m->nmon)
	for (struct obj* o = m->minvent; o; o = o->nobj)
	    o->unpaid = 0;
    for (struct obj* o; (o = _level->billobjs);) {
	_level->billobjs = o->nobj;
	free (o);
    }
    ESHK(shopkeeper)->billct = 0;
}

static void addupbill(void)
{
    // delivers result in total
    // caller has checked that shopkeeper exists
    int ct = ESHK(shopkeeper)->billct;
    struct bill_x *bp = bill;
    total = 0;
    while (ct--) {
	total += bp->price * bp->bquan;
	++bp;
    }
}

int inshop(void)
{
    int roomno = inroom(_u.ux, _u.uy);

    // Did we just leave a shop?
    if (_u.uinshop && (_u.uinshop != roomno + 1 || shlevel != _u.dlevel || !shopkeeper)) {
	if (shopkeeper) {
	    if (ESHK(shopkeeper)->billct) {
		if (inroom(shopkeeper->mx, shopkeeper->my)
		    == _u.uinshop - 1)	// ab@unido
		    pline("Somehow you escaped the shop without paying!");
		addupbill();
		pline("You stole for a total worth of %ld zorkmids.", total);
		ESHK(shopkeeper)->robbed += total;
		setpaid();
		if ((_level->rooms[ESHK(shopkeeper)->shoproom].rtype == GENERAL)
		    == (rn2(3) == 0))
		    ESHK(shopkeeper)->following = 1;
	    }
	    shopkeeper = 0;
	    shlevel = 0;
	}
	_u.uinshop = 0;
    }
    // Did we just enter a zoo of some kind?
    if (roomno >= 0) {
	int rt = _level->rooms[roomno].rtype;
	if (rt == ZOO)
	    pline("Welcome to David's treasure zoo!");
	else if (rt == SWAMP)
	    pline("It looks rather muddy down here.");
	else if (rt == MORGUE) {
	    if (midnight())
		pline("Go away! Go away!");
	    else
		pline("You get an uncanny feeling ...");
	} else
	    rt = 0;
	if (rt != 0) {
	    _level->rooms[roomno].rtype = 0;
	    for (struct monst* m = _level->monsters; m; m = m->nmon)
		if (rt != ZOO || !rn2(3))
		    m->msleep = 0;
	}
    }
    // Did we just enter a shop?
    if (roomno >= 0 && _level->rooms[roomno].rtype >= 8) {
	if (shlevel != _u.dlevel || !shopkeeper || ESHK(shopkeeper)->shoproom != roomno)
	    findshk(roomno);
	if (!shopkeeper) {
	    _level->rooms[roomno].rtype = 0;
	    _u.uinshop = 0;
	} else if (!_u.uinshop) {
	    if (!ESHK(shopkeeper)->visitct || strncmp(ESHK(shopkeeper)->customer, plname, PL_NSIZ)) {

		// He seems to be new here
		ESHK(shopkeeper)->visitct = 0;
		ESHK(shopkeeper)->following = 0;
		strncpy(ESHK(shopkeeper)->customer, plname, PL_NSIZ);
		NOTANGRY(shopkeeper) = 1;
	    }
	    if (!ESHK(shopkeeper)->following) {
		bool box, pick;

		pline("Hello %s! Welcome%s to %s's %s shop!", plname, ESHK(shopkeeper)->visitct++ ? " again" : "", shkname(shopkeeper), shopnam[_level->rooms[ESHK(shopkeeper)->shoproom].rtype - 8]);
		box = carrying(ICE_BOX);
		pick = carrying(PICK_AXE);
		if (box || pick) {
		    if (dochug(shopkeeper)) {
			_u.uinshop = 0;	// he died moving
			return 0;
		    }
		    pline("Will you please leave your %s outside?", (box && pick) ? "box and pick-axe" : box ? "box" : "pick-axe");
		}
	    }
	    _u.uinshop = roomno + 1;
	}
    }
    return _u.uinshop;
}

static void findshk (int roomno)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon) {
	if (m->isshk && ESHK(m)->shoproom == roomno && ESHK(m)->shoplevel == _u.dlevel) {
	    shopkeeper = m;
	    bill = &(ESHK(shopkeeper)->bill[0]);
	    shlevel = _u.dlevel;
	    if (ANGRY(shopkeeper) && strncmp(ESHK(shopkeeper)->customer, plname, PL_NSIZ))
		NOTANGRY(shopkeeper) = 1;
	    // _level->billobjs = NULL; -- this is wrong if we save in a shop
	    // (and it is harmless to have too many things in _level->billobjs)
	    return;
	}
    }
    shopkeeper = 0;
    shlevel = 0;
    bill = (struct bill_x *) -1000;	// dump core when referenced
}

static struct bill_x *onbill(struct obj *obj)
{
    struct bill_x *bp;
    if (!shopkeeper)
	return 0;
    for (bp = bill; bp < &bill[ESHK(shopkeeper)->billct]; ++bp)
	if (bp->bo_id == obj->o_id) {
	    if (!obj->unpaid)
		pline("onbill: paid obj on bill?");
	    return bp;
	}
    if (obj->unpaid)
	pline("onbill: unpaid obj not on bill?");
    return 0;
}

// called with two args on merge
void obfree (struct obj *obj, struct obj *merge)
{
    struct bill_x *bp = onbill(obj);
    struct bill_x *bpm;
    if (bp) {
	if (!merge) {
	    bp->useup = 1;
	    obj->unpaid = 0;   // only for doinvbill
	    obj->nobj = _level->billobjs;
	    _level->billobjs = obj;
	    return;
	}
	bpm = onbill(merge);
	if (!bpm) {
	    // this used to be a rename
	    impossible("obfree: not on bill??");
	    return;
	} else {
	    // this was a merger
	    bpm->bquan += bp->bquan;
	    --ESHK(shopkeeper)->billct;
	    *bp = bill[ESHK(shopkeeper)->billct];
	}
    }
    free (obj);
}

static void pay(long tmp, struct monst *shkp)
{
    long robbed = ESHK(shkp)->robbed;

    _u.ugold -= tmp;
    shkp->mgold += tmp;
    _wflags.botl = 1;
    if (robbed) {
	robbed -= tmp;
	if (robbed < 0)
	    robbed = 0;
	ESHK(shkp)->robbed = robbed;
    }
}

int dopay(void)
{
    long ltmp;
    struct bill_x *bp;
    int pass, tmp;

    multi = 0;
    inshop();
    struct monst *shkp;
    for (shkp = _level->monsters; shkp; shkp = shkp->nmon)
	if (shkp->isshk && dist(shkp->mx, shkp->my) < 3)
	    break;
    if (!shkp && _u.uinshop && inroom(shopkeeper->mx, shopkeeper->my) == ESHK(shopkeeper)->shoproom)
	shkp = shopkeeper;

    if (!shkp) {
	pline("There is nobody here to receive your payment.");
	return 0;
    }
    ltmp = ESHK(shkp)->robbed;
    if (shkp != shopkeeper && NOTANGRY(shkp)) {
	if (!ltmp) {
	    pline("You do not owe %s anything.", monnam(shkp));
	} else if (!_u.ugold) {
	    pline("You have no money.");
	} else {
	    long ugold = _u.ugold;

	    if (_u.ugold > ltmp) {
		pline("You give %s the %ld gold pieces he asked for.", monnam(shkp), ltmp);
		pay(ltmp, shkp);
	    } else {
		pline("You give %s all your gold.", monnam(shkp));
		pay(_u.ugold, shkp);
	    }
	    if (ugold < ltmp / 2) {
		pline("Unfortunately, he doesn't look satisfied.");
	    } else {
		ESHK(shkp)->robbed = 0;
		ESHK(shkp)->following = 0;
		if (ESHK(shkp)->shoplevel != _u.dlevel) {
		    // For convenience's sake, let him disappear
		    shkp->minvent = 0;	// %%
		    shkp->mgold = 0;
		    mondead(shkp);
		}
	    }
	}
	return 1;
    }
    if (!ESHK(shkp)->billct) {
	pline("You do not owe %s anything.", monnam(shkp));
	if (!_u.ugold) {
	    pline("Moreover, you have no money.");
	    return 1;
	}
	if (ESHK(shkp)->robbed) {
#define min(a,b)	((a<b)?a:b)
	    pline("But since his shop has been robbed recently,");
	    pline("you %srepay %s's expenses.", (_u.ugold < ESHK(shkp)->robbed) ? "partially " : "", monnam(shkp));
	    pay(min(_u.ugold, ESHK(shkp)->robbed), shkp);
	    ESHK(shkp)->robbed = 0;
	    return 1;
	}
	if (ANGRY(shkp)) {
	    pline("But in order to appease %s,", amonnam(shkp, "angry"));
	    if (_u.ugold >= 1000) {
		ltmp = 1000;
		pline(" you give him 1000 gold pieces.");
	    } else {
		ltmp = _u.ugold;
		pline(" you give him all your money.");
	    }
	    pay(ltmp, shkp);
	    if (strncmp(ESHK(shkp)->customer, plname, PL_NSIZ)
		|| rn2(3)) {
		pline("%s calms down.", Monnam(shkp));
		NOTANGRY(shkp) = 1;
	    } else
		pline("%s is as angry as ever.", Monnam(shkp));
	}
	return 1;
    }
    if (shkp != shopkeeper) {
	impossible("dopay: not to shopkeeper?");
	if (shopkeeper)
	    setpaid();
	return 0;
    }
    for (pass = 0; pass <= 1; ++pass) {
	tmp = 0;
	while (tmp < ESHK(shopkeeper)->billct) {
	    bp = &bill[tmp];
	    if (!pass && !bp->useup) {
		++tmp;
		continue;
	    }
	    if (!dopayobj(bp))
		return 1;
	    bill[tmp] = bill[--ESHK(shopkeeper)->billct];
	}
    }
    pline("Thank you for shopping in %s's %s store!", shkname(shopkeeper), shopnam[_level->rooms[ESHK(shopkeeper)->shoproom].rtype - 8]);
    NOTANGRY(shopkeeper) = 1;
    return 1;
}

// return 1 if paid successfully
// 0 if not enough money
// -1 if object could not be found (but was paid)
static int dopayobj(struct bill_x *bp)
{
    struct obj *obj;
    long ltmp;

    // find the object on one of the lists
    obj = bp_to_obj(bp);

    if (!obj) {
	impossible("Shopkeeper administration out of order.");
	setpaid();	       // be nice to the player
	return 0;
    }
    if (!obj->unpaid && !bp->useup) {
	impossible("Paid object on bill??");
	return 1;
    }
    obj->unpaid = 0;
    ltmp = bp->price * bp->bquan;
    if (ANGRY(shopkeeper))
	ltmp += ltmp / 3;
    if (_u.ugold < ltmp) {
	pline("You don't have gold enough to pay %s.", doname(obj));
	obj->unpaid = 1;
	return 0;
    }
    pay(ltmp, shopkeeper);
    pline("You bought %s for %ld gold piece%s.", doname(obj), ltmp, plur(ltmp));
    if (bp->useup) {
	if (obj == _level->billobjs)
	    _level->billobjs = obj->nobj;
	else for (struct obj* o = _level->billobjs; o; o = o->nobj)
	    if (o->nobj == obj)
		o->nobj = obj->nobj;
	free (obj);
    }
    return 1;
}

// routine called after dying (or quitting) with nonempty bill
void paybill(void)
{
    if (shlevel == _u.dlevel && shopkeeper && ESHK(shopkeeper)->billct) {
	addupbill();
	if (total > _u.ugold) {
	    shopkeeper->mgold += _u.ugold;
	    _u.ugold = 0;
	    pline("%s comes and takes all your possessions.", Monnam(shopkeeper));
	} else {
	    _u.ugold -= total;
	    shopkeeper->mgold += total;
	    pline("%s comes and takes the %ld zorkmids you owed him.", Monnam(shopkeeper), total);
	}
	setpaid();
    }
}

// find obj on one of the lists
struct obj* bp_to_obj (struct bill_x *bp)
{
    const unsigned id = bp->bo_id;
    struct obj* o;
    if (bp->useup)
	o = o_on(id, _level->billobjs);
    else if (!(o = o_on(id, invent)) && !(o = o_on(id, _level->objects)) && !(o = o_on(id, fcobj))) {
	for (struct monst* m = _level->monsters; m; m = m->nmon)
	    if ((o = o_on(id, m->minvent)))
		break;
	for (struct monst* m = fallen_down; m; m = m->nmon)
	    if ((o = o_on(id, m->minvent)))
		break;
    }
    return o;
}

// called in hack.c when we pickup an object
void addtobill (struct obj *obj)
{
    struct bill_x *bp;
    if (!inshop() || (_u.ux == ESHK(shopkeeper)->shk.x && _u.uy == ESHK(shopkeeper)->shk.y) || (_u.ux == ESHK(shopkeeper)->shd.x && _u.uy == ESHK(shopkeeper)->shd.y) || onbill(obj))	// perhaps we threw it away earlier
	return;
    if (ESHK(shopkeeper)->billct == BILLSZ) {
	pline("You got that for free!");
	return;
    }
    bp = &bill[ESHK(shopkeeper)->billct];
    bp->bo_id = obj->o_id;
    bp->bquan = obj->quan;
    bp->useup = 0;
    bp->price = getprice(obj);
    ++ESHK(shopkeeper)->billct;
    obj->unpaid = 1;
}

void splitbill(struct obj *obj, struct obj *otmp)
{
    // otmp has been split off from obj
    struct bill_x *bp;
    int tmp;
    bp = onbill(obj);
    if (!bp) {
	impossible("splitbill: not on bill?");
	return;
    }
    if (bp->bquan < otmp->quan) {
	impossible("Negative quantity on bill??");
    }
    if (bp->bquan == otmp->quan) {
	impossible("Zero quantity on bill??");
    }
    bp->bquan -= otmp->quan;

    // addtobill(otmp);
    if (ESHK(shopkeeper)->billct == BILLSZ)
	otmp->unpaid = 0;
    else {
	tmp = bp->price;
	bp = &bill[ESHK(shopkeeper)->billct];
	bp->bo_id = otmp->o_id;
	bp->bquan = otmp->quan;
	bp->useup = 0;
	bp->price = tmp;
	++ESHK(shopkeeper)->billct;
    }
}

void subfrombill(struct obj *obj)
{
    long ltmp;
    int tmp;
    struct obj *otmp;
    struct bill_x *bp;
    if (!inshop() || (_u.ux == ESHK(shopkeeper)->shk.x && _u.uy == ESHK(shopkeeper)->shk.y) || (_u.ux == ESHK(shopkeeper)->shd.x && _u.uy == ESHK(shopkeeper)->shd.y))
	return;
    if ((bp = onbill(obj)) != 0) {
	obj->unpaid = 0;
	if (bp->bquan > obj->quan) {
	    otmp = newobj();
	    *otmp = *obj;
	    bp->bo_id = otmp->o_id = _wflags.ident++;
	    otmp->quan = (bp->bquan -= obj->quan);
	    otmp->owt = 0;     // superfluous
	    bp->useup = 1;
	    otmp->nobj = _level->billobjs;
	    _level->billobjs = otmp;
	    return;
	}
	--ESHK(shopkeeper)->billct;
	*bp = bill[ESHK(shopkeeper)->billct];
	return;
    }
    if (obj->unpaid) {
	pline("%s didn't notice.", Monnam(shopkeeper));
	obj->unpaid = 0;
	return;		       // %%
    }
    // he dropped something of his own - probably wants to sell it
    if (shopkeeper->msleep || shopkeeper->mfroz || inroom(shopkeeper->mx, shopkeeper->my) != ESHK(shopkeeper)->shoproom)
	return;
    if (ESHK(shopkeeper)->billct == BILLSZ || ((tmp = shtypes[_level->rooms[ESHK(shopkeeper)->shoproom].rtype - 8]) && tmp != obj->olet)
	|| strchr("_0", obj->olet)) {
	pline("%s seems not interested.", Monnam(shopkeeper));
	return;
    }
    ltmp = getprice(obj) * obj->quan;
    if (ANGRY(shopkeeper)) {
	ltmp /= 3;
	NOTANGRY(shopkeeper) = 1;
    } else
	ltmp /= 2;
    if (ESHK(shopkeeper)->robbed) {
	if ((ESHK(shopkeeper)->robbed -= ltmp) < 0)
	    ESHK(shopkeeper)->robbed = 0;
	pline("Thank you for your contribution to restock this recently plundered shop.");
	return;
    }
    if (ltmp > shopkeeper->mgold)
	ltmp = shopkeeper->mgold;
    pay(-ltmp, shopkeeper);
    if (!ltmp)
	pline("%s gladly accepts %s but cannot pay you at present.", Monnam(shopkeeper), doname(obj));
    else
	pline("You sold %s and got %ld gold piece%s.", doname(obj), ltmp, plur(ltmp));
}

// 0: deliver count 1: paged
int doinvbill (int mode)
{
    struct bill_x *bp;
    struct obj *obj;
    long totused, thisused;

    if (mode == 0) {
	int cnt = 0;

	if (shopkeeper)
	    for (bp = bill; bp - bill < ESHK(shopkeeper)->billct; ++bp)
		if (bp->useup || ((obj = bp_to_obj(bp)) && obj->quan < bp->bquan))
		    ++cnt;
	return cnt;
    }
    if (!shopkeeper) {
	impossible("doinvbill: no shopkeeper?");
	return 0;
    }
    set_pager(0);
    if (page_line("Unpaid articles already used up:") || page_line("")) {
	set_pager(2);
	return 0;
    }

    totused = 0;
    for (bp = bill; bp - bill < ESHK(shopkeeper)->billct; ++bp) {
	obj = bp_to_obj(bp);
	if (!obj) {
	    impossible("Bad shopkeeper administration.");
	    set_pager(2);
	    return 0;
	}
	if (bp->useup || bp->bquan > obj->quan) {
	    int oquan = obj->quan;
	    int uquan = (bp->useup ? bp->bquan : bp->bquan - oquan);
	    thisused = bp->price * uquan;
	    totused += thisused;
	    char buf [BUFSZ];
	    struct StringBuilder sb = StringBuilder_new (buf);
	    obj->quan = uquan; // cheat doname
	    StringBuilder_appendf (&sb, "x -  %-45s", doname(obj));
	    obj->quan = oquan; // restore value
	    StringBuilder_appendf (&sb, " %5ld zorkmids", thisused);
	    if (page_line (buf)) {
		set_pager(2);
		return 0;
	    }
	}
    }
    char buf [BUFSZ];
    snprintf (ArrayBlock(buf), "Total:%50ld zorkmids", totused);
    if (page_line("") || page_line(buf)) {
	set_pager(2);
	return 0;
    }
    set_pager(1);
    return 0;
}

static int getprice(struct obj *obj)
{
    int tmp, ac;

    switch (obj->olet) {
	case AMULET_SYM:
	    tmp = 10 * rnd(500);
	    break;
	case TOOL_SYM:
	    tmp = 10 * rnd((obj->otyp == EXPENSIVE_CAMERA) ? 150 : 30);
	    break;
	case RING_SYM:
	    tmp = 10 * rnd(100);
	    break;
	case WAND_SYM:
	    tmp = 10 * rnd(100);
	    break;
	case SCROLL_SYM:
	    tmp = 10 * rnd(50);
	    break;
	case POTION_SYM:
	    tmp = 10 * rnd(50);
	    break;
	case FOOD_SYM:
	    tmp = 10 * rnd(5 + (2000 / realhunger()));
	    break;
	case GEM_SYM:
	    tmp = 10 * rnd(20);
	    break;
	case ARMOR_SYM:
	    ac = ARM_BONUS(obj);
	    if (ac <= -10)     // probably impossible
		ac = -9;
	    tmp = 100 + ac * ac * rnd(10 + ac);
	    break;
	case WEAPON_SYM:
	    if (obj->otyp < BOOMERANG)
		tmp = 5 * rnd(10);
	    else if (obj->otyp == LONG_SWORD || obj->otyp == TWO_HANDED_SWORD)
		tmp = 10 * rnd(150);
	    else
		tmp = 10 * rnd(75);
	    break;
	default:
	    tmp = 10000;
    }
    return tmp;
}

static int realhunger(void)
{			       // not completely foolproof
    int tmp = _u.uhunger;
    struct obj *otmp = invent;
    while (otmp) {
	if (otmp->olet == FOOD_SYM && !otmp->unpaid)
	    tmp += c_Objects[otmp->otyp].nutrition;
	otmp = otmp->nobj;
    }
    return (tmp <= 0) ? 1 : tmp;
}

int shkcatch(struct obj *obj)
{
    struct monst *shkp = shopkeeper;

    if (_u.uinshop && shkp && !shkp->mfroz && !shkp->msleep && _u.dx && _u.dy && inroom(_u.ux + _u.dx, _u.uy + _u.dy) + 1 == _u.uinshop && shkp->mx == ESHK(shkp)->shk.x && shkp->my == ESHK(shkp)->shk.y && _u.ux == ESHK(shkp)->shd.x && _u.uy == ESHK(shkp)->shd.y) {
	pline("%s nimbly catches the %s.", Monnam(shkp), xname(obj));
	obj->nobj = shkp->minvent;
	shkp->minvent = obj;
	return 1;
    }
    return 0;
}

// shk_move: return 1: he moved  0: he didnt  -1: let m_move do it
int shk_move (struct monst *shkp)
{
    struct monst *mtmp;
    const struct permonst *mdat = shkp->data;
    int8_t gx, gy, omx, omy, nx, ny, nix, niy;
    int8_t appr, i;
    int udist;
    int z;
    int8_t shkroom, chi, chcnt, cnt;
    bool uondoor = 0, satdoor, avoid = 0, badinv;
    struct coord poss[9];
    int info[9];
    struct obj *ib = 0;

    omx = shkp->mx;
    omy = shkp->my;

    if ((udist = dist(omx, omy)) < 3) {
	if (ANGRY(shkp)) {
	    hitu(shkp, d(mdat->damn, mdat->damd) + 1);
	    return 0;
	}
	if (ESHK(shkp)->following) {
	    if (strncmp(ESHK(shkp)->customer, plname, PL_NSIZ)) {
		pline("Hello %s! I was looking for %s.", plname, ESHK(shkp)->customer);
		ESHK(shkp)->following = 0;
		return 0;
	    }
	    if (!ESHK(shkp)->robbed) {	// impossible?
		ESHK(shkp)->following = 0;
		return 0;
	    }
	    if (_u.moves > followmsg + 4) {
		pline("Hello %s! Didn't you forget to pay?", plname);
		followmsg = _u.moves;
	    }
	    if (udist < 2)
		return 0;
	}
    }
    shkroom = inroom(omx, omy);
    appr = 1;
    gx = ESHK(shkp)->shk.x;
    gy = ESHK(shkp)->shk.y;
    satdoor = (gx == omx && gy == omy);
    if (ESHK(shkp)->following || ((z = holetime()) >= 0 && z * z <= udist)) {
	gx = _u.ux;
	gy = _u.uy;
	if (shkroom < 0 || shkroom != inroom(_u.ux, _u.uy))
	    if (udist > 4)
		return -1;   // leave it to m_move
    } else if (ANGRY(shkp)) {
	long saveBlind = Blind;
	Blind = 0;
	if (shkp->mcansee && !Invis && cansee(omx, omy)) {
	    gx = _u.ux;
	    gy = _u.uy;
	}
	Blind = saveBlind;
	avoid = false;
    } else {
#define	GDIST(x,y)	((x-gx)*(x-gx)+(y-gy)*(y-gy))
	if (Invis)
	    avoid = false;
	else {
	    uondoor = (_u.ux == ESHK(shkp)->shd.x && _u.uy == ESHK(shkp)->shd.y);
	    if (uondoor) {
		if (ESHK(shkp)->billct)
		    pline("Hello %s! Will you please pay before leaving?", plname);
		badinv = (carrying(PICK_AXE) || carrying(ICE_BOX));
		if (satdoor && badinv)
		    return 0;
		avoid = !badinv;
	    } else {
		avoid = (_u.uinshop && dist(gx, gy) > 8);
		badinv = false;
	    }

	    if (((!ESHK(shkp)->robbed && !ESHK(shkp)->billct) || avoid)
		&& GDIST(omx, omy) < 3) {
		if (!badinv && !online(omx, omy))
		    return 0;
		if (satdoor)
		    appr = gx = gy = 0;
	    }
	}
    }
    if (omx == gx && omy == gy)
	return 0;
    if (shkp->mconf) {
	avoid = false;
	appr = 0;
    }
    nix = omx;
    niy = omy;
    cnt = mfndpos(shkp, poss, info, ALLOW_SSM);
    if (avoid && uondoor) {    // perhaps we cannot avoid him
	for (i = 0; i < cnt; ++i)
	    if (!(info[i] & NOTONL))
		goto notonl_ok;
	avoid = false;
      notonl_ok:
	;
    }
    chi = -1;
    chcnt = 0;
    for (i = 0; i < cnt; ++i) {
	nx = poss[i].x;
	ny = poss[i].y;
	if (_level->l[nx][ny].typ == ROOM || shkroom != ESHK(shkp)->shoproom || ESHK(shkp)->following) {
	    if (uondoor && (ib = sobj_at(ICE_BOX, nx, ny))) {
		nix = nx;
		niy = ny;
		chi = i;
		break;
	    }
	    if (avoid && (info[i] & NOTONL))
		continue;
	    if ((!appr && !rn2(++chcnt)) || (appr && GDIST(nx, ny) < GDIST(nix, niy))) {
		nix = nx;
		niy = ny;
		chi = i;
	    }
	}
    }
    if (nix != omx || niy != omy) {
	if (info[chi] & ALLOW_M) {
	    mtmp = m_at(nix, niy);
	    if (hitmm(shkp, mtmp) == 1 && rn2(3) && hitmm(mtmp, shkp) == 2)
		return 2;
	    return 0;
	} else if (info[chi] & ALLOW_U) {
	    hitu(shkp, d(mdat->damn, mdat->damd) + 1);
	    return 0;
	}
	shkp->mx = nix;
	shkp->my = niy;
	pmon(shkp);
	if (ib) {
	    freeobj(ib);
	    mpickobj(shkp, ib);
	}
	return 1;
    }
    return 0;
}

// He is digging in the shop.
void shopdig(int fall)
{
    if (!fall) {
	if (_u.utraptype == TT_PIT)
	    pline("\"Be careful, sir, or you might fall through the floor.\"");
	else
	    pline("\"Please, do not damage the floor here.\"");
    } else if (dist(shopkeeper->mx, shopkeeper->my) < 3) {
	struct obj *obj, *obj2;

	pline("%s grabs your backpack!", shkname(shopkeeper));
	for (obj = invent; obj; obj = obj2) {
	    obj2 = obj->nobj;
	    if (obj->owornmask)
		continue;
	    freeinv(obj);
	    obj->nobj = shopkeeper->minvent;
	    shopkeeper->minvent = obj;
	    if (obj->unpaid)
		subfrombill(obj);
	}
    }
}

int online(int x, int y)
{
    return x == _u.ux || y == _u.uy || (x - _u.ux) * (x - _u.ux) == (y - _u.uy) * (y - _u.uy);
}

// Does this monster follow me downstairs?
int follower(struct monst *mtmp)
{
    return strchr("1TVWZi&, ", mtmp->data->mlet) || (mtmp->isshk && ESHK(mtmp)->following);
}
