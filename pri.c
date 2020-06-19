// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

int8_t scrlx, scrhx, scrly, scrhy;	// corners of new area on screen

_Noreturn void panic (const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    static bool s_Panicking = false;
    if (s_Panicking)
	exit (EXIT_FAILURE);
    s_Panicking = true;
    home();
    puts(" Suddenly, the dungeon collapses.");
    puts(" ERROR:  ");
    vprintf(fmt, ap);
    va_end(ap);
    more();		       // contains a fflush()
    done("panicked");
}

static void short_sleep (void)
{
    // delay 50 ms - could also use a 'nap'-system call
    // or the usleep call like this :-)
    usleep(50000);
}

void atl(int x, int y, int ch)
{
    struct rm *crm = &_level->l[x][y];

    if (x < 0 || x > COLNO - 1 || y < 0 || y > ROWNO - 1) {
	impossible("atl(%d,%d,%c)", x, y, ch);
	return;
    }
    if (crm->seen && crm->scrsym == ch)
	return;
    crm->scrsym = ch;
    crm->new = 1;
    on_scr(x, y);
}

void on_scr(int x, int y)
{
    if (x < scrlx)
	scrlx = x;
    if (x > scrhx)
	scrhx = x;
    if (y < scrly)
	scrly = y;
    if (y > scrhy)
	scrhy = y;
}

//
// call: (x,y) - display (-1,0) - close (leave last symbol) (-1,-1)- close
// (undo last symbol) (-1,let)-open: initialize symbol (-2,let)-change let

void tmp_at(int x, int y)
{
    static int8_t prevx, prevy;
    static char let;
    if ((int) x == -2) {       // change let call
	let = y;
	return;
    }
    if ((int) x == -1 && (int) y >= 0) {	// open or close call
	let = y;
	prevx = -1;
	return;
    }
    if (prevx >= 0 && cansee(prevx, prevy)) {
	short_sleep();
	prl(prevx, prevy);     // in case there was a monster
	at(prevx, prevy, _level->l[prevx][prevy].scrsym);
    }
    if (x >= 0) {	       // normal call
	if (cansee(x, y))
	    at(x, y, let);
	prevx = x;
	prevy = y;
    } else {		       // close call
	let = 0;
	prevx = -1;
    }
}

// like the previous, but the symbols are first erased on completion
void Tmp_at(int x, int y)
{
    static char let;
    static int8_t cnt;
    static struct coord tc[COLNO];	// but watch reflecting beams!
    int xx, yy;
    if (x == -1) {
	if (y > 0) {	       // open call
	    let = y;
	    cnt = 0;
	    return;
	}
	// close call (do not distinguish y==0 and y==-1)
	while (cnt--) {
	    xx = tc[cnt].x;
	    yy = tc[cnt].y;
	    prl(xx, yy);
	    at(xx, yy, _level->l[xx][yy].scrsym);
	}
	cnt = let = 0;	       // superfluous
	return;
    }
    if ((int) x == -2) {       // change let call
	let = y;
	return;
    }
    // normal call
    if (cansee(x, y)) {
	if (cnt)
	    short_sleep();
	at(x, y, let);
	tc[cnt].x = x;
	tc[cnt].y = y;
	if (++cnt >= COLNO)
	    panic("Tmp_at overflow?");
	_level->l[x][y].new = 0;    // prevent pline-nscr erasing ---
    }
}

_Noreturn void setclipped(void)
{
    error("Hack needs a screen of size at least %d by %d.\n", ROWNO + 2, COLNO);
}

void at(int x, int y, int ch)
{
    if (x < 0 || x > COLNO - 1 || y < 0 || y > ROWNO - 1) {
	impossible("At gets 0%o at %d %d.", ch, x, y);
	return;
    }
    if (!ch) {
	impossible("At gets null at %d %d.", x, y);
	return;
    }
    y += 2;
    curs(x, y);
    putchar(ch);
    ++curx;
}

void prme(void)
{
    if (!Invisible)
	at(_u.ux, _u.uy, _u.usym);
}

int doredraw(void)
{
    docrt();
    return 0;
}

void docrt(void)
{
    int x, y;
    struct rm *room;

    cls();

    // Some ridiculous code to get display of @ and monsters (almost)
    // right
    if (!Invisible) {
	_level->l[(_u.udisx = _u.ux)][(_u.udisy = _u.uy)].scrsym = _u.usym;
	_level->l[_u.udisx][_u.udisy].seen = 1;
	_u.udispl = 1;
    } else
	_u.udispl = 0;

    seemons();		       // reset old positions
    for (struct monst* m = _level->monsters; m; m = m->nmon)
	m->mdispl = 0;
    seemons();		       // force new positions to be shown
    // This nonsense should disappear soon
    // ---------------------------------

    for (y = 0; y < ROWNO; ++y)
	for (x = 0; x < COLNO; ++x)
	    if ((room = &_level->l[x][y])->new) {
		room->new = 0;
		at(x, y, room->scrsym);
	    } else if (room->seen)
		at(x, y, room->scrsym);
    scrlx = COLNO;
    scrly = ROWNO;
    scrhx = scrhy = 0;
    _wflags.botlx = 1;
    bot();
}

void docorner(int xmin, int ymax)
{
    int x, y;
    struct rm *room;

    seemons();		       // reset old positions
    for (struct monst* m = _level->monsters; m; m = m->nmon)
	if (m->mx >= xmin && m->my < ymax)
	    m->mdispl = 0;
    seemons();		       // force new positions to be shown

    for (y = 0; y < ymax; ++y) {
	if (y > ROWNO && CD)
	    break;
	curs(xmin, y + 2);
	cl_end();
	if (y < ROWNO) {
	    for (x = xmin; x < COLNO; ++x) {
		if ((room = &_level->l[x][y])->new) {
		    room->new = 0;
		    at(x, y, room->scrsym);
		} else if (room->seen)
		    at(x, y, room->scrsym);
	    }
	}
    }
    if (ymax > ROWNO) {
	cornbot(xmin - 1);
	if (ymax > ROWNO + 1 && CD) {
	    curs(1, ROWNO + 3);
	    cl_eos();
	}
    }
}

void curs_on_u(void)
{
    curs(_u.ux, _u.uy + 2);
}

void pru(void)
{
    if (_u.udispl && (Invisible || _u.udisx != _u.ux || _u.udisy != _u.uy))
	// if(! _level->l[_u.udisx][_u.udisy].new)
	if (!vism_at(_u.udisx, _u.udisy))
	    newsym(_u.udisx, _u.udisy);
    if (Invisible) {
	_u.udispl = 0;
	prl(_u.ux, _u.uy);
    } else if (!_u.udispl || _u.udisx != _u.ux || _u.udisy != _u.uy) {
	atl(_u.ux, _u.uy, _u.usym);
	_u.udispl = 1;
	_u.udisx = _u.ux;
	_u.udisy = _u.uy;
    }
    _level->l[_u.ux][_u.uy].seen = 1;
}

// print a position that is visible for @
void prl(int x, int y)
{
    struct rm *room;
    struct monst *mtmp;
    struct obj *otmp;

    if (x == _u.ux && y == _u.uy && (!Invisible)) {
	pru();
	return;
    }
    if (!isok(x, y))
	return;
    room = &_level->l[x][y];
    if ((!room->typ) || (IS_ROCK(room->typ) && _level->l[_u.ux][_u.uy].typ == CORR))
	return;
    if ((mtmp = m_at(x, y)) && !mtmp->mhide && (!mtmp->minvis || See_invisible)) {
	pmon(mtmp);
    } else if ((otmp = o_at(x, y)) && room->typ != POOL)
	atl(x, y, otmp->olet);
    else if (mtmp && (!mtmp->minvis || See_invisible)) {
	// must be a hiding monster, but not hiding right now
	// assume for the moment that long worms do not hide
	pmon(mtmp);
    } else if (g_at(x, y) && room->typ != POOL)
	atl(x, y, '$');
    else if (!room->seen || room->scrsym == ' ') {
	room->new = room->seen = 1;
	newsym(x, y);
	on_scr(x, y);
    }
    room->seen = 1;
}

char news0(int x, int y)
{
    struct obj *otmp;
    struct trap *ttmp;
    struct rm *room;
    char tmp;

    room = &_level->l[x][y];
    if (!room->seen)
	tmp = ' ';
    else if (room->typ == POOL)
	tmp = POOL_SYM;
    else if (!Blind && (otmp = o_at(x, y)))
	tmp = otmp->olet;
    else if (!Blind && g_at(x, y))
	tmp = '$';
    else if (x == _level->stair.up.x && y == _level->stair.up.y)
	tmp = '<';
    else if (x == _level->stair.dn.x && y == _level->stair.dn.y)
	tmp = '>';
    else if ((ttmp = t_at(x, y)) && ttmp->tseen)
	tmp = '^';
    else
	switch (room->typ) {
	    case SCORR:
	    case SDOOR:
		tmp = room->scrsym;
		break;
	    case HWALL:
		tmp = '-';
		break;
	    case VWALL:
		tmp = '|';
		break;
	    case LDOOR:
	    case DOOR:
		tmp = '+';
		break;
	    case CORR:
		tmp = CORR_SYM;
		break;
	    case ROOM:
		if (room->lit || cansee(x, y) || Blind)
		    tmp = '.';
		else
		    tmp = ' ';
		break;
		// case POOL:
		// tmp = POOL_SYM;
		// break;
	    default:
		tmp = ERRCHAR;
	}
    return tmp;
}

void newsym(int x, int y)
{
    atl(x, y, news0(x, y));
}

// used with wand of digging (or pick-axe): fill scrsym and force display
// also when a POOL evaporates
void mnewsym(int x, int y)
{
    struct rm *room;
    char newscrsym;

    if (!vism_at(x, y)) {
	room = &_level->l[x][y];
	newscrsym = news0(x, y);
	if (room->scrsym != newscrsym) {
	    room->scrsym = newscrsym;
	    room->seen = 0;
	}
    }
}

void nosee(int x, int y)
{
    struct rm *room;

    if (!isok(x, y))
	return;
    room = &_level->l[x][y];
    if (room->scrsym == '.' && !room->lit && !Blind) {
	room->scrsym = ' ';
	room->new = 1;
	on_scr(x, y);
    }
}

void prl1(int x, int y)
{
    if (_u.dx) {
	if (_u.dy) {
	    prl(x - (2 * _u.dx), y);
	    prl(x - _u.dx, y);
	    prl(x, y);
	    prl(x, y - _u.dy);
	    prl(x, y - (2 * _u.dy));
	} else {
	    prl(x, y - 1);
	    prl(x, y);
	    prl(x, y + 1);
	}
    } else {
	prl(x - 1, y);
	prl(x, y);
	prl(x + 1, y);
    }
}

void nose1(int x, int y)
{
    if (_u.dx) {
	if (_u.dy) {
	    nosee(x, _u.uy);
	    nosee(x, _u.uy - _u.dy);
	    nosee(x, y);
	    nosee(_u.ux - _u.dx, y);
	    nosee(_u.ux, y);
	} else {
	    nosee(x, y - 1);
	    nosee(x, y);
	    nosee(x, y + 1);
	}
    } else {
	nosee(x - 1, y);
	nosee(x, y);
	nosee(x + 1, y);
    }
}

int vism_at(int x, int y)
{
    struct monst *mtmp;

    return (x == _u.ux && y == _u.uy && !Invisible)
	    ? 1 : (mtmp = m_at(x, y))
	    ? ((Blind && Telepat) || canseemon(mtmp)) : 0;
}

void unpobj(struct obj *obj)
{
    // if(obj->odispl){ if(!vism_at(obj->odx, obj->ody)) newsym(obj->odx,
    // obj->ody); obj->odispl = 0; }
    if (!vism_at(obj->ox, obj->oy))
	newsym(obj->ox, obj->oy);
}

void seeobjs(void)
{
    struct obj *obj, *obj2;
    for (obj = _level->objects; obj; obj = obj2) {
	obj2 = obj->nobj;
	if (obj->olet == FOOD_SYM && obj->otyp >= CORPSE && obj->age + 250 < _u.moves)
	    delobj(obj);
    }
    for (obj = invent; obj; obj = obj2) {
	obj2 = obj->nobj;
	if (obj->olet == FOOD_SYM && obj->otyp >= CORPSE && obj->age + 250 < _u.moves)
	    useup(obj);
    }
}

void seemons(void)
{
    for (struct monst* m = _level->monsters; m; m = m->nmon) {
	if (m->data->mlet == ';')
	    m->minvis = (_level->l[m->mx][m->my].typ == POOL);
	pmon(m);
    }
}

void pmon(struct monst *mon)
{
    int show = (Blind && Telepat) || canseemon(mon);
    if (mon->mdispl) {
	if (mon->mdx != mon->mx || mon->mdy != mon->my || !show)
	    unpmon(mon);
    }
    if (show && !mon->mdispl) {
	atl(mon->mx, mon->my, (!mon->mappearance || _u.uprops[PROP(RIN_PROTECTION_FROM_SHAPE_CHANGERS)].p_flgs) ? mon->data->mlet : mon->mappearance);
	mon->mdispl = 1;
	mon->mdx = mon->mx;
	mon->mdy = mon->my;
    }
}

void unpmon(struct monst *mon)
{
    if (mon->mdispl) {
	newsym(mon->mdx, mon->mdy);
	mon->mdispl = 0;
    }
}

void nscr(void)
{
    int x, y;
    struct rm *room;

    if (_u.ux == FAR || _wflags.nscrinh)
	return;
    pru();
    for (y = scrly; y <= scrhy; ++y)
	for (x = scrlx; x <= scrhx; ++x)
	    if ((room = &_level->l[x][y])->new) {
		room->new = 0;
		at(x, y, room->scrsym);
	    }
    scrhx = scrhy = 0;
    scrlx = COLNO;
    scrly = ROWNO;
}

// 100 suffices for bot(); no relation with COLNO
static char oldbot [100];
static char newbot [100];

void cornbot(int lth)
{
    if (lth < (int) sizeof(oldbot)) {
	oldbot[lth] = 0;
	_wflags.botl = 1;
    }
}

void bot (void)
{
    char *ob = oldbot, *nb = newbot;
    int i;
    if (_wflags.botlx)
	*ob = 0;
    _wflags.botl = _wflags.botlx = 0;
    struct StringBuilder sb = StringBuilder_new (newbot);
    StringBuilder_appendf (&sb, "Level %-2hhu  Gold %-5u  Hp %3d(%hu)  Ac %-2d  Str ", _u.dlevel, _u.ugold, _u.uhp, _u.uhpmax, _u.uac);
    if (_u.ustr > 18) {
	if (_u.ustr > 117)
	    StringBuilder_append (&sb, "18/**");
	else
	    StringBuilder_appendf (&sb, "18/%02d", _u.ustr - 18);
    } else
	StringBuilder_appendf (&sb, "%-2d   ", _u.ustr);
    StringBuilder_appendf (&sb, "  Exp %2hhu/%-5u ", _u.ulevel, _u.uexp);
    StringBuilder_append (&sb, hu_stat[_u.uhs]);
    if (_wflags.time)
	StringBuilder_appendf (&sb, "  %u", _u.moves);
    if (strlen(newbot) >= COLNO) {
	char *bp0, *bp1;
	bp0 = bp1 = newbot;
	do {
	    if (*bp0 != ' ' || bp0[1] != ' ' || bp0[2] != ' ')
		*bp1++ = *bp0;
	} while (*bp0++);
    }
    for (i = 1; i < COLNO; ++i) {
	if (*ob != *nb) {
	    curs(i, ROWNO + 2);
	    putchar(*nb ? *nb : ' ');
	    ++curx;
	}
	if (*ob)
	    ++ob;
	if (*nb)
	    ++nb;
    }
    strcpy(oldbot, newbot);
}

void cls(void)
{
    if (_wflags.toplin == 1)
	more();
    _wflags.toplin = 0;

    clear_screen();

    _wflags.botlx = 1;
}
