// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

enum { NOINVSYM = '#' };

static int lastinvnr = 51;	// 0 ... 51

static void assigninvlet(struct obj *);
static char *xprname(struct obj *, char);

static void assigninvlet(struct obj *otmp)
{
    bool inuse[52];
    int i;
    struct obj *obj;

    for (i = 0; i < 52; ++i)
	inuse[i] = false;
    for (obj = invent; obj; obj = obj->nobj)
	if (obj != otmp) {
	    i = obj->invlet;
	    if ('a' <= i && i <= 'z')
		inuse[i - 'a'] = true;
	    else if ('A' <= i && i <= 'Z')
		inuse[i - 'A' + 26] = true;
	    if (i == otmp->invlet)
		otmp->invlet = 0;
	}
    if ((i = otmp->invlet) && (('a' <= i && i <= 'z') || ('A' <= i && i <= 'Z')))
	return;
    for (i = lastinvnr + 1; i != lastinvnr; ++i) {
	if (i == 52) {
	    i = -1;
	    continue;
	}
	if (!inuse[i])
	    break;
    }
    otmp->invlet = (inuse[i] ? NOINVSYM : (i < 26) ? ('a' + i) : ('A' + i - 26));
    lastinvnr = i;
}

struct obj *addinv(struct obj *obj)
{
    struct obj *otmp;

    // merge or attach to end of chain
    if (!invent) {
	invent = obj;
	otmp = 0;
    } else
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (merged(otmp, obj, 0))
		return otmp;
	    if (!otmp->nobj) {
		otmp->nobj = obj;
		break;
	    }
	}
    obj->nobj = 0;

    if (!_wflags.invlet_not_constant) {
	assigninvlet(obj);
	// The ordering of the chain is nowhere significant
	// so in case you prefer some other order than the
	// historical one, change the code below.
	if (otmp) {	       // find proper place in chain
	    otmp->nobj = 0;
	    if ((invent->invlet ^ 040) > (obj->invlet ^ 040)) {
		obj->nobj = invent;
		invent = obj;
	    } else
		for (otmp = invent;; otmp = otmp->nobj) {
		    if (!otmp->nobj || (otmp->nobj->invlet ^ 040) > (obj->invlet ^ 040)) {
			obj->nobj = otmp->nobj;
			otmp->nobj = obj;
			break;
		    }
		}
	}
    }
    return obj;
}

void useup(struct obj *obj)
{
    if (obj->quan > 1) {
	--obj->quan;
	obj->owt = weight(obj);
    } else {
	setnotworn(obj);
	freeinv(obj);
	obfree (obj, NULL);
    }
}

void freeinv (struct obj* otf)
{
    if (otf == invent)
	invent = invent->nobj;
    else for (struct obj* o = invent; o; o = o->nobj)
	if (o->nobj == otf)
	    o->nobj = otf->nobj;
}

// destroy object in _level->objects chain (if unpaid, it remains on the bill)
void delobj(struct obj *obj)
{
    freeobj(obj);
    unpobj(obj);
    obfree (obj, NULL);
}

// unlink obj from chain starting with _level->objects
void freeobj (struct obj* otf)
{
    if (otf == _level->objects)
	_level->objects = _level->objects->nobj;
    else for (struct obj* o = _level->objects; o; o = o->nobj)
	if (o->nobj == otf)
	    o->nobj = otf->nobj;
}

// Note: freegold throws away its argument!
void freegold (struct gold* gtf)
{
    if (gtf == _level->money)
	_level->money = gtf->ngold;
    else for (struct gold* g = _level->money; g; g = g->ngold)
	if (g->ngold == gtf)
	    g->ngold = gtf->ngold;
    free (gtf);
}

void deltrap (struct trap* ttf)
{
    if (ttf == _level->traps)
	_level->traps = _level->traps->ntrap;
    else for (struct trap* t = _level->traps; t; t = t->ntrap)
	if (t->ntrap == ttf)
	    t->ntrap = ttf->ntrap;
    free (ttf);
}

struct monst* m_at (int x, int y)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon)
	if (m->mx == x && m->my == y)
	    return m;
    return NULL;
}

struct obj* o_at (int x, int y)
{
    for (struct obj* o = _level->objects; o; o = o->nobj)
	if (o->ox == x && o->oy == y)
	    return o;
    return NULL;
}

struct obj* sobj_at (int n, int x, int y)
{
    for (struct obj* o = _level->objects; o; o = o->nobj)
	if (o->ox == x && o->oy == y && o->otyp == n)
	    return o;
    return NULL;
}

bool carried (const struct obj* q)
{
    for (const struct obj* o = invent; o; o = o->nobj)
	if (o == q)
	    return true;
    return false;
}

bool carrying (int type)
{
    for (const struct obj* o = invent; o; o = o->nobj)
	if (o->otyp == type)
	    return true;
    return false;
}

struct obj* o_on (unsigned id, struct obj* o)
{
    for (; o; o = o->nobj)
	if (o->o_id == id)
	    return o;
    return NULL;
}

struct trap* t_at (int x, int y)
{
    for (struct trap* trap = _level->traps; trap; trap = trap->ntrap)
	if (trap->tx == x && trap->ty == y)
	    return trap;
    return NULL;
}

struct gold* g_at (int x, int y)
{
    for (struct gold* gold = _level->money; gold; gold = gold->ngold)
	if (gold->gx == x && gold->gy == y)
	    return gold;
    return NULL;
}

// make dummy object structure containing gold - for temporary use only
struct obj* mkgoldobj (long q)
{
    struct obj* o = newobj();
    // should set o_id etc. but o will be freed soon
    o->olet = '$';
    o->ogold = q;
    _u.ugold -= q;
    _wflags.botl = 1;
    return o;
}

// getobj returns:
//      struct obj *xxx:	object to do something with.
//      NULL			no object.
struct obj* getobj(const char *let, const char *word)
{
    struct obj *otmp;
    char ilet, ilet1, ilet2;
    char buf[BUFSZ];
    char lets[BUFSZ];
    int foo = 0, foo2;
    char *bp = buf;
    int8_t allowcnt = 0;		// 0, 1 or 2
    bool allowgold = false;
    bool allowall = false;
    bool allownone = false;
    int8_t foox = 0;
    long cnt;

    if (*let == '0')
	++let, allowcnt = 1;
    if (*let == '$')
	++let, allowgold = true;
    if (*let == '#')
	++let, allowall = true;
    if (*let == '-')
	++let, allownone = true;
    if (allownone)
	*bp++ = '-';
    if (allowgold)
	*bp++ = '$';
    if (bp > buf && bp[-1] == '-')
	*bp++ = ' ';

    ilet = 'a';
    for (otmp = invent; otmp; otmp = otmp->nobj) {
	if (!*let || strchr(let, otmp->olet)) {
	    bp[foo++] = _wflags.invlet_not_constant ? ilet : otmp->invlet;

	    // ugly check: remove inappropriate things
	    if ((!strcmp(word, "take off") && !(otmp->owornmask & (W_ARMOR - W_ARM2)))
		|| (!strcmp(word, "wear") && (otmp->owornmask & (W_ARMOR | W_RING)))
		|| (!strcmp(word, "wield") && (otmp->owornmask & W_WEP))) {
		--foo;
		++foox;
	    }
	}
	if (ilet == 'z')
	    ilet = 'A';
	else
	    ++ilet;
    }
    bp[foo] = 0;
    if (foo == 0 && bp > buf && bp[-1] == ' ')
	*--bp = 0;
    strcpy(lets, bp);   // necessary since we destroy buf
    if (foo > 5) {	       // compactify string
	foo = foo2 = 1;
	ilet2 = bp[0];
	ilet1 = bp[1];
	while ((ilet = bp[++foo2] = bp[++foo]) != '\0') {
	    if (ilet == ilet1 + 1) {
		if (ilet1 == ilet2 + 1)
		    bp[foo2 - 1] = ilet1 = '-';
		else if (ilet2 == '-') {
		    bp[--foo2] = ++ilet1;
		    continue;
		}
	    }
	    ilet2 = ilet1;
	    ilet1 = ilet;
	}
    }
    if (!foo && !allowall && !allowgold && !allownone) {
	pline("You don't have anything %sto %s.", foox ? "else " : "", word);
	return 0;
    }
    for (;;) {
	if (!buf[0])
	    pline("What do you want to %s [*]? ", word);
	else
	    pline("What do you want to %s [%s or ?*]? ", word, buf);

	cnt = 0;
	ilet = readchar();
	while (digit(ilet) && allowcnt) {
	    if (cnt < 100000000)
		cnt = 10 * cnt + (ilet - '0');
	    else
		cnt = 999999999;
	    allowcnt = 2;      // signal presence of cnt
	    ilet = readchar();
	}
	if (digit(ilet)) {
	    pline("No count allowed with this command.");
	    continue;
	}
	if (strchr(quitchars, ilet))
	    return NULL;
	if (ilet == '-')
	    return NULL;
	if (ilet == '$') {
	    if (!allowgold) {
		pline("You cannot %s gold.", word);
		continue;
	    }
	    if (!(allowcnt == 2 && cnt < _u.ugold))
		cnt = _u.ugold;
	    return mkgoldobj(cnt);
	}
	if (ilet == '?') {
	    doinv(lets);
	    if (!(ilet = morc))
		continue;
	    // he typed a letter (not a space) to more()
	} else if (ilet == '*') {
	    doinv (NULL);
	    if (!(ilet = morc))
		continue;
	    // ...
	}
	if (!_wflags.invlet_not_constant) {
	    for (otmp = invent; otmp; otmp = otmp->nobj)
		if (otmp->invlet == ilet)
		    break;
	} else {
	    if (ilet >= 'A' && ilet <= 'Z')
		ilet += 'z' - 'A' + 1;
	    ilet -= 'a';
	    for (otmp = invent; otmp && ilet; --ilet, otmp = otmp->nobj);
	}
	if (!otmp) {
	    pline("You don't have that object.");
	    continue;
	}
	if (cnt < 0 || otmp->quan < cnt) {
	    pline("You don't have that many! [You have %u]", otmp->quan);
	    continue;
	}
	break;
    }
    if (!allowall && let && !strchr(let, otmp->olet)) {
	pline("That is a silly thing to %s.", word);
	return 0;
    }
    if (allowcnt == 2) {       // cnt given
	if (cnt == 0)
	    return 0;
	if (cnt != otmp->quan) {
	    struct obj *obj;
	    obj = splitobj(otmp, (int) cnt);
	    if (otmp == uwep)
		setuwep(obj);
	}
    }
    return otmp;
}

int ckunpaid(struct obj *otmp)
{
    return otmp->unpaid;
}

// interactive version of getobj - used for Drop and Identify
// return the number of times fn was called successfully
int ggetobj(const char *word, int (*fn) (struct obj *), int max)
{
    char buf[BUFSZ];
    char *ip;
    char sym;
    int oletct = 0, iletct = 0;
    bool allflag = false;
    char olets[20], ilets[20];
    int (*ckfn)(struct obj*) = NULL;
    int8_t allowgold = (_u.ugold && !strcmp(word, "drop")) ? 1 : 0;	// BAH
    if (!invent && !allowgold) {
	pline("You have nothing to %s.", word);
	return 0;
    } else {
	struct obj *otmp = invent;
	int uflg = 0;

	if (allowgold)
	    ilets[iletct++] = '$';
	ilets[iletct] = 0;
	while (otmp) {
	    if (!strchr(ilets, otmp->olet)) {
		ilets[iletct++] = otmp->olet;
		ilets[iletct] = 0;
	    }
	    if (otmp->unpaid)
		uflg = 1;
	    otmp = otmp->nobj;
	}
	ilets[iletct++] = ' ';
	if (uflg)
	    ilets[iletct++] = 'u';
	if (invent)
	    ilets[iletct++] = 'a';
	ilets[iletct] = 0;
    }
    pline("What kinds of thing do you want to %s? [%s] ", word, ilets);
    getlin(buf);
    if (buf[0] == '\033') {
	clrlin();
	return 0;
    }
    ip = buf;
    olets[0] = 0;
    while ((sym = *ip++) != '\0') {
	if (sym == ' ')
	    continue;
	if (sym == '$') {
	    if (allowgold == 1)
		(*fn) (mkgoldobj(_u.ugold));
	    else if (!_u.ugold)
		pline("You have no gold.");
	    allowgold = 2;
	} else if (sym == 'a' || sym == 'A')
	    allflag = true;
	else if (sym == 'u' || sym == 'U')
	    ckfn = ckunpaid;
	else if (strchr("!%?[()=*/\"0", sym)) {
	    if (!strchr(olets, sym)) {
		olets[oletct++] = sym;
		olets[oletct] = 0;
	    }
	} else
	    pline("You don't have any %c's.", sym);
    }
    if (allowgold == 2 && !oletct)
	return 1;	       // he dropped gold (or at least tried to)
    else
	return askchain(invent, olets, allflag, fn, ckfn, max);
}

// Walk through the chain starting at objchn and ask for all objects
// with olet in olets (if nonNULL) and satisfying ckfn (if nonNULL)
// whether the action in question (i.e., fn) has to be performed.
// If allflag then no questions are asked. Max gives the max nr of
// objects to be treated. Return the number of objects treated.
int askchain(struct obj *objchn, char *olets, int allflag, int (*fn) (struct obj *), int (*ckfn) (struct obj *), int max)
{
    struct obj *otmp, *otmp2;
    char sym, ilet;
    int cnt = 0;
    ilet = 'a' - 1;
    for (otmp = objchn; otmp; otmp = otmp2) {
	if (ilet == 'z')
	    ilet = 'A';
	else
	    ++ilet;
	otmp2 = otmp->nobj;
	if (olets && *olets && !strchr(olets, otmp->olet))
	    continue;
	if (ckfn && !(*ckfn) (otmp))
	    continue;
	if (!allflag) {
	    pline(xprname(otmp, ilet));
	    addtopl(" [nyaq]? ");
	    sym = readchar();
	} else
	    sym = 'y';

	switch (sym) {
	    case 'a':
		allflag = 1;
		// fallthrough
	    case 'y':
		cnt += (*fn) (otmp);
		if (--max == 0)
		    goto ret;
		// fallthrough
	    case 'n':
	    default:
		break;
	    case 'q':
		goto ret;
	}
    }
    pline(cnt ? "That was all." : "No applicable objects.");
  ret:
    return cnt;
}

// should of course only be called for things in invent
char obj_to_let (struct obj *obj)
{
    struct obj *otmp;
    char ilet;

    if (!_wflags.invlet_not_constant)
	return obj->invlet;
    ilet = 'a';
    for (otmp = invent; otmp && otmp != obj; otmp = otmp->nobj)
	if (++ilet > 'z')
	    ilet = 'A';
    return otmp ? ilet : NOINVSYM;
}

void prinv(struct obj *obj)
{
    pline(xprname(obj, obj_to_let(obj)));
}

static char* xprname (struct obj* obj, char let)
{
    static char li [BUFSZ];
    snprintf (ArrayBlock(li), "%c - %s.", _wflags.invlet_not_constant ? let : obj->invlet, doname(obj));
    return li;
}

int ddoinv (void)
{
    doinv (NULL);
    return 0;
}

// called with 0 or "": all objects in inventory
// otherwise: all objects with (serial) letter in lets
void doinv(char *lets)
{
    struct obj *otmp;
    char ilet;
    int ct = 0;
    char any[BUFSZ];

    morc = 0;		       // just to be sure

    if (!invent) {
	pline("Not carrying anything.");
	return;
    }
    cornline (0, NULL);
    ilet = 'a';
    for (otmp = invent; otmp; otmp = otmp->nobj) {
	if (!_wflags.invlet_not_constant)
	    ilet = otmp->invlet;
	if (!lets || !*lets || strchr(lets, ilet)) {
	    cornline(1, xprname(otmp, ilet));
	    any[ct++] = ilet;
	}
	if (_wflags.invlet_not_constant)
	    if (++ilet > 'z')
		ilet = 'A';
    }
    any[ct] = 0;
    cornline(2, any);
}

int dotypeinv(void)
{			       // free after Robert Viduya
    // Changed to one type only, so he doesnt have to type cr
    char c, ilet;
    char stuff[BUFSZ];
    int stct;
    struct obj *otmp;
    bool billx = inshop() && doinvbill(0);
    bool unpd = false;

    if (!invent && !_u.ugold && !billx) {
	pline("You aren't carrying anything.");
	return 0;
    }
    stct = 0;
    if (_u.ugold)
	stuff[stct++] = '$';
    stuff[stct] = 0;
    for (otmp = invent; otmp; otmp = otmp->nobj) {
	if (!strchr(stuff, otmp->olet)) {
	    stuff[stct++] = otmp->olet;
	    stuff[stct] = 0;
	}
	if (otmp->unpaid)
	    unpd = true;
    }
    if (unpd)
	stuff[stct++] = 'u';
    if (billx)
	stuff[stct++] = 'x';
    stuff[stct] = 0;

    if (stct > 1) {
	pline("What type of object [%s] do you want an inventory of? ", stuff);
	c = readchar();
	if (strchr(quitchars, c))
	    return 0;
    } else
	c = stuff[0];

    if (c == '$')
	return doprgold();

    if (c == 'x' || c == 'X') {
	if (billx)
	    doinvbill(1);
	else
	    pline("No used-up objects on the shopping bill.");
	return 0;
    }
    if ((c == 'u' || c == 'U') && !unpd) {
	pline("You are not carrying any unpaid objects.");
	return 0;
    }
    stct = 0;
    ilet = 'a';
    for (otmp = invent; otmp; otmp = otmp->nobj) {
	if (!_wflags.invlet_not_constant)
	    ilet = otmp->invlet;
	if (c == otmp->olet || (c == 'u' && otmp->unpaid))
	    stuff[stct++] = ilet;
	if (_wflags.invlet_not_constant)
	    if (++ilet > 'z')
		ilet = 'A';
    }
    stuff[stct] = '\0';
    if (stct == 0)
	pline("You have no such objects.");
    else
	doinv(stuff);

    return 0;
}

// look at what is here
int dolook(void)
{
    struct obj *otmp = NULL, *otmp0 = NULL;
    struct gold *gold = NULL;
    const char *verb = Blind ? "feel" : "see";
    int ct = 0;

    if (Blind) {
	pline("You try to feel what is lying here on the floor.");
	if (Levitation) {  // ab@unido
	    pline("You cannot reach the floor!");
	    return 1;
	}
    }
    otmp0 = o_at(_u.ux, _u.uy);
    gold = g_at(_u.ux, _u.uy);
    if (!otmp0 && !gold) {
	pline("You %s no objects here.", verb);
	return !!Blind;
    }
    cornline(0, "Things that are here:");
    for (otmp = otmp0; otmp; otmp = otmp->nobj) {
	if (otmp->ox == _u.ux && otmp->oy == _u.uy) {
	    ++ct;
	    cornline(1, doname(otmp));
	    if (Blind && otmp->otyp == DEAD_COCKATRICE && !uarmg) {
		pline("Touching the dead cockatrice is a fatal mistake ...");
		pline("You die ...");
		killer = "dead cockatrice";
		done("died");
	    }
	}
    }

    if (gold) {
	char gbuf [30];
	snprintf (ArrayBlock(gbuf), "%ld gold piece%s", gold->amount, plur(gold->amount));
	if (!ct++)
	    pline("You %s here %s.", verb, gbuf);
	else
	    cornline(1, gbuf);
    }
    if (ct == 1 && !gold) {
	pline("You %s here %s.", verb, doname(otmp0));
	cornline(3, NULL);
    }
    if (ct > 1)
	cornline(2, NULL);
    return ! !Blind;
}

void stackobj (struct obj *obj)
{
    for (struct obj* o = _level->objects; o; o = o->nobj)
	if (o != obj)
	    if (o->ox == obj->ox && o->oy == obj->oy && merged(obj, o, 1))
		return;
}

// merge o1 with o2 and delete o2 if types agree
bool merged (struct obj *o1, struct obj *o2, int lose)
{
    if (o2->otyp == o1->otyp
	    && o2->unpaid == o1->unpaid
	    && o2->spe == o1->spe
	    && o2->dknown == o1->dknown
	    && o2->cursed == o1->cursed
	    && (strchr("%*?!", o2->olet)
		|| (o2->known == o1->known
		    && (o2->olet == WEAPON_SYM
			&& o2->otyp < BOOMERANG)))) {
	o1->quan += o2->quan;
	o1->owt += o2->owt;
	if (lose)
	    freeobj(o2);
	obfree (o2, o1);
	return true;
    } else
	return false;
}

static long goldcounted = 0;

// Gold is no longer displayed; in fact, when you have a lot of money,
// it may take a while before you have counted it all.
// [Bug: d$ and pickup still tell you how much it was.]
int countgold(void)
{
    if ((goldcounted += 100 * (_u.ulevel + 1)) >= _u.ugold) {
	long eps = 0;
	if (!rn2(2))
	    eps = rnd((int) (_u.ugold / 100 + 1));
	pline("You probably have about %ld gold pieces.", _u.ugold + eps);
	return 0;	       // done
    }
    return 1;		       // continue
}

int doprgold(void)
{
    if (!_u.ugold)
	pline("You do not carry any gold.");
    else if (_u.ugold <= 500)
	pline("You are carrying %u gold pieces.", _u.ugold);
    else {
	pline("You sit down in order to count your gold pieces.");
	goldcounted = 500;
	occupation = countgold;
	occtxt = "counting your gold";
    }
    return 1;
}

// --- end of gold counting section ---
int doprwep(void)
{
    if (!uwep)
	pline("You are empty handed.");
    else
	prinv(uwep);
    return 0;
}

int doprarm(void)
{
    if (!uarm && !uarmg && !uarms && !uarmh)
	pline("You are not wearing any armor.");
    else {
	char lets[6];
	int ct = 0;

	if (uarm)
	    lets[ct++] = obj_to_let(uarm);
	if (uarm2)
	    lets[ct++] = obj_to_let(uarm2);
	if (uarmh)
	    lets[ct++] = obj_to_let(uarmh);
	if (uarms)
	    lets[ct++] = obj_to_let(uarms);
	if (uarmg)
	    lets[ct++] = obj_to_let(uarmg);
	lets[ct] = 0;
	doinv(lets);
    }
    return 0;
}

int doprring(void)
{
    if (!uleft && !uright)
	pline("You are not wearing any rings.");
    else {
	char lets[3];
	int ct = 0;

	if (uleft)
	    lets[ct++] = obj_to_let(uleft);
	if (uright)
	    lets[ct++] = obj_to_let(uright);
	lets[ct] = 0;
	doinv(lets);
    }
    return 0;
}

int digit(int c)
{
    return c >= '0' && c <= '9';
}
