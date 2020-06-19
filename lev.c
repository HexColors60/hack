// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "mkroom.h"

static void savetrapchn (int fd, const struct trap *trap);
static void savegoldchn (int fd, const struct gold* gold);

bool level_exists [MAXLEVEL + 1] = { false };

void savelev (int fd, const struct level* l)
{
    bwrite (fd, l->l, sizeof(l->l));
    savemonchn (fd, l->monsters);
    savegoldchn (fd, l->money);
    savetrapchn (fd, l->traps);
    saveobjchn (fd, l->objects);
    saveobjchn (fd, l->billobjs);
    save_engravings (fd, l->engravings);
    bwrite (fd, l->rooms, sizeof(l->rooms));
    bwrite (fd, l->doors, sizeof(l->doors));
}

void bwrite (int fd, const void* loc, unsigned num)
{
    if (write(fd, loc, num) != (ssize_t) num)
	panic("cannot write %u bytes to file #%d", num, fd);
}

void saveobjchn (int fd, const struct obj* o)
{
    uint32_t n = 0;
    for (const struct obj* oi = o; oi; oi = oi->nobj)
	++n;
    bwrite (fd, &n, sizeof(n));
    for (; o; o = o->nobj)
	bwrite (fd, o, sizeof(struct obj));
}

struct obj* restobjchn (int fd)
{
    uint32_t n = 0;
    mread (fd, &n, sizeof(n));
    struct obj *r = NULL, **pp = &r;
    for (unsigned i = 0; i < n; ++i) {
	struct obj* o = newobj();
	mread (fd, o, sizeof(*o));
	if (!o->o_id)
	    o->o_id = _wflags.ident++;
	*pp = o;
	pp = &o->nobj;
    }
    return r;
}

void savemonchn (int fd, const struct monst* m)
{
    const struct permonst* monbegin = &c_Monsters[0];
    bwrite (fd, &monbegin, sizeof(monbegin));
    uint32_t n = 0;
    for (const struct monst* mi = m; mi; mi = mi->nmon)
	++n;
    bwrite (fd, &n, sizeof(n));
    for (; m; m = m->nmon) {
	uint32_t xl = m->mxlth+m->mnamelth;
	bwrite (fd, &xl, sizeof(xl));
	bwrite (fd, m, sizeof(*m)+xl);
	if (m->minvent)
	    saveobjchn (fd, m->minvent);
    }
}

struct monst* restmonchn (int fd)
{
    struct permonst* monbegin = NULL;
    mread (fd, &monbegin, sizeof(monbegin));
    ptrdiff_t differ = (const char*) &c_Monsters[0] - (const char*) monbegin;
    uint32_t n = 0;
    mread (fd, &n, sizeof(n));
    struct monst *r = NULL, **pp = &r;
    for (unsigned i = 0; i < n; ++i) {
	uint32_t xl = 0;
	mread (fd, &xl, sizeof(xl));
	struct monst* m = newmonst (xl);
	mread (fd, m, sizeof(*m)+xl);
	m->data = (const struct permonst*)((const char*) m->data + differ);
	if (m->minvent)
	    m->minvent = restobjchn (fd);
	*pp = m;
	pp = &m->nmon;
    }
    return r;
}

static void savegoldchn (int fd, const struct gold* gold)
{
    uint32_t n = 0;
    for (const struct gold* ig = gold; ig; ig = ig->ngold)
	++n;
    bwrite (fd, &n, sizeof(n));
    for (; gold; gold = gold->ngold)
	bwrite (fd, gold, sizeof(*gold));
}

static struct gold* restgoldchn (int fd)
{
    uint32_t n = 0;
    mread (fd, &n, sizeof(n));
    struct gold *r = NULL, **pp = &r;
    for (unsigned i = 0; i < n; ++i) {
	struct gold* o = newgold();
	mread (fd, o, sizeof(*o));
	*pp = o;
	pp = &o->ngold;
    }
    return r;
}

static void savetrapchn (int fd, const struct trap *trap)
{
    uint32_t n = 0;
    for (const struct trap* it = trap; it; it = it->ntrap)
	++n;
    bwrite (fd, &n, sizeof(n));
    for (; trap; trap = trap->ntrap)
	bwrite (fd, trap, sizeof(*trap));
}

static struct trap* resttrapchn (int fd)
{
    uint32_t n = 0;
    mread (fd, &n, sizeof(n));
    struct trap *r = NULL, **pp = &r;
    for (unsigned i = 0; i < n; ++i) {
	struct trap* o = newtrap();
	mread (fd, o, sizeof(*o));
	*pp = o;
	pp = &o->ntrap;
    }
    return r;
}

static void remove_genocided_monsters (struct monst** pp)
{
    for (struct monst* m; (m = *pp); pp = &m->nmon) {
	if (is_monster_genocided (m->data->mlet)) {
	    *pp = m->nmon;
	    free (m);
	    m = *pp;
	}
    }
}

static void regenerate_monster_health (unsigned tmoves, struct monst* mc)
{
    for (struct monst *m = mc; m; m = m->nmon)
	m->mhp = min_u (m->mhpmax, m->mhp + (strchr(MREGEN, m->data->mlet) ? tmoves : tmoves/20));
}

void getlev (int fd, struct level* l)
{
    mread (fd, l->l, sizeof(l->l));
    l->monsters = restmonchn(fd);
    remove_genocided_monsters (&l->monsters);
    regenerate_monster_health (_u.moves > l->lastvisit ? _u.moves - l->lastvisit : 0, l->monsters);
    setgd();
    l->money = restgoldchn (fd);
    l->traps = resttrapchn (fd);
    l->objects = restobjchn (fd);
    l->billobjs = restobjchn (fd);
    l->engravings = rest_engravings (fd);
    mread (fd, l->rooms, sizeof(l->rooms));
    mread (fd, l->doors, sizeof(l->doors));
}

void mread (int fd, void* buf, unsigned len)
{
    ssize_t rlen = read (fd, buf, len);
    if (rlen != (int) len) {
	pline ("Read %zd instead of %u bytes.\n", rlen, len);
	panic ("Error reading level file.");
    }
}

void mklev(void)
{
    in_mklev = true;
    makelevel();
    in_mklev = false;
}

// return roomnumber or -1
int inroom(int x, int y)
{
    struct mkroom *croom = &_level->rooms[0];
    while (croom->hx >= 0) {
	if (croom->hx >= x - 1 && croom->lx <= x + 1 && croom->hy >= y - 1 && croom->ly <= y + 1)
	    return croom - _level->rooms;
	++croom;
    }
    return -1;	       // not in room or on door
}

static void freeobjchn (struct obj* oc)
{
    while (oc) {
	struct obj* o = oc;
	oc = oc->nobj;
	o->nobj = NULL;
	free (o);
    }
}

static void freemonchn (struct monst* mc)
{
    while (mc) {
	struct monst* m = mc;
	mc = mc->nmon;
	freeobjchn (m->minvent);
	m->minvent = NULL;
	m->nmon = NULL;
	free (m);
    }
}

static void freegoldchn (struct gold* gc)
{
    while (gc) {
	struct gold* g = gc;
	gc = gc->ngold;
	g->ngold = NULL;
	free (g);
    }
}

static void freetrapchn (struct trap* tc)
{
    while (tc) {
	struct trap* t = tc;
	tc = tc->ntrap;
	t->ntrap = NULL;
	free (t);
    }
}

static void freeengrchn (struct engr* ec)
{
    while (ec) {
	struct engr* e = ec;
	ec = ec->nxt_engr;
	e->nxt_engr = NULL;
	free (e);
    }
}

void level_dtor (struct level* l)
{
    freemonchn (l->monsters);
    freeobjchn (l->objects);
    freegoldchn (l->money);
    freetrapchn (l->traps);
    freeobjchn (l->billobjs);
    freeengrchn (l->engravings);
    memset (l, 0, sizeof(*l));
}
