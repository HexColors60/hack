// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "mfndpos.h"

int warnlevel;			// used by movemon and dochugw
long lastwarntime;
int lastwarnlev;
const char warnings[][8] = { "white", "pink", "red", "ruby", "purple", "black" };

void movemon(void)
{
    struct monst *m;
    int fr;

    warnlevel = 0;

    while (1) {
	// find a monster that we haven't treated yet
	//
	// note that m or m->nmon might get killed while m
	// moves, so we cannot just walk down the chain (even new
	// monsters might get created!)
	for (m = _level->monsters; m; m = m->nmon)
	    if (m->mlstmv < _u.moves)
		goto next_mon;
	// treated all monsters
	break;

      next_mon:
	m->mlstmv = _u.moves;

	// most monsters drown in pools
	{
	    bool inpool, iseel;

	    inpool = (_level->l[m->mx][m->my].typ == POOL);
	    iseel = (m->data->mlet == ';');
	    if (inpool && !iseel) {
		if (cansee(m->mx, m->my))
		    pline("%s drowns.", Monnam(m));
		mondead(m);
		continue;
	    }
	    // but eels have a difficult time outside
	    if (iseel && !inpool) {
		if (m->mhp > 1)
		    --m->mhp;
		m->mflee = 1;
		m->mfleetim += 2;
	    }
	}
	if (m->mblinded && !--m->mblinded)
	    m->mcansee = 1;
	if (m->mfleetim && !--m->mfleetim)
	    m->mflee = 0;
	if (m->mspeed != MSLOW || !(_u.moves % 2)) {
	    // continue if the monster died fighting
	    fr = -1;
	    if (Conflict && cansee(m->mx, m->my)
		&& (fr = fightm(m)) == 2)
		continue;
	    if (fr < 0 && dochugw(m))
		continue;
	}
	if (m->mspeed == MFAST && dochugw(m))
	    continue;
    }

    warnlevel -= _u.ulevel;
    if (warnlevel >= (int) ArraySize(warnings))
	warnlevel = ArraySize(warnings) - 1;
    if (warnlevel >= 0) {
	if (warnlevel > lastwarnlev || _u.moves > lastwarntime + 5) {
	    const char *rr;
	    switch (Warning & (LEFT_RING | RIGHT_RING)) {
		case LEFT_RING:
		    rr = "Your left ring glows";
		    break;
		case RIGHT_RING:
		    rr = "Your right ring glows";
		    break;
		case LEFT_RING | RIGHT_RING:
		    rr = "Both your rings glow";
		    break;
		default:
		    rr = "Your fingertips glow";
		    break;
	    }
	    pline("%s %s!", rr, warnings[warnlevel]);
	    lastwarntime = _u.moves;
	    lastwarnlev = warnlevel;
	}
    }
    dmonsfree();	       // remove all dead monsters
}

int dochugw (struct monst *m)
{
    int x = m->mx, y = m->my;
    int d = dochug(m), dd;
    // monster still alive
    if (!d && Warning && !m->mpeaceful && m->data->mlevel > warnlevel
	    && (dd = dist(m->mx, m->my)) < dist(x, y)
	    && dd < 100 && !canseemon(m))
	warnlevel = m->data->mlevel;
    return d;
}

// returns 1 if monster died moving, 0 otherwise
int dochug(struct monst *m)
{
    int tmp = 0, nearby, scared;
    const struct permonst* mdat = m->data;

    // regenerate monsters
    if ((!(_u.moves % 20) || strchr(MREGEN, mdat->mlet)) && m->mhp < m->mhpmax)
	++m->mhp;

    if (m->mfroz)
	return 0;	       // frozen monsters don't do anything

    if (m->msleep) {
	// wake up, or get out of here.
	// ettins are hard to surprise
	// Nymphs and Leprechauns do not easily wake up
	if (cansee(m->mx, m->my)
		&& (!Stealth || (mdat->mlet == 'e' && rn2(10)))
		&& (!strchr("NL", mdat->mlet) || !rn2(50))
		&& (Aggravate_monster || strchr("d1", mdat->mlet) || !rn2(7)))
	    m->msleep = 0;
	else
	    return 0;
    }
    // not frozen or sleeping: wipe out texts written in the dust
    wipe_engr_at(m->mx, m->my, 1);

    // confused monsters get unconfused with small probability
    if (m->mconf && !rn2(50))
	m->mconf = 0;

    // some monsters teleport
    if (m->mflee && strchr("tNL", mdat->mlet) && !rn2(40)) {
	rloc(m);
	return 0;
    }
    if (mdat->mmove < rnd(6))
	return 0;

    // fleeing monsters might regain courage
    if (m->mflee && !m->mfleetim && m->mhp == m->mhpmax && !rn2(25))
	m->mflee = 0;

    nearby = (dist(m->mx, m->my) < 3);
    scared = (nearby && (sengr_at("Elbereth", _u.ux, _u.uy) || sobj_at(SCR_SCARE_MONSTER, _u.ux, _u.uy)));
    if (scared && !m->mflee) {
	m->mflee = 1;
	m->mfleetim = (rn2(7) ? rnd(10) : rnd(100));
    }
    if (!nearby || m->mflee || m->mconf || (m->minvis && !rn2(3)) || (strchr("BIuy", mdat->mlet) && !rn2(4)) || (mdat->mlet == 'L' && !_u.ugold && (m->mgold || rn2(2))) || (!m->mcansee && !rn2(4)) || m->mpeaceful) {
	tmp = m_move(m); // 2: monster died moving
	if (tmp == 2 || (tmp && mdat->mmove <= 12))
	    return tmp == 2;
    }
    if (!strchr("Ea", mdat->mlet) && nearby && !m->mpeaceful && _u.uhp > 0 && !scared) {
	if (mhitu(m))
	    return 1;	       // monster died (e.g. 'y' or 'F')
    }
    // extra movement for fast monsters
    if (mdat->mmove > rnd(12)+12)
	tmp = m_move(m);
    return tmp == 2;
}

int m_move (struct monst *m)
{
    struct monst *m2;
    int nx, ny, omx, omy, appr, nearer, cnt, i, j;
    int8_t gx, gy, nix, niy, chcnt;
    int8_t chi;
    bool likegold = 0, likegems = 0, likeobjs = 0;
    char msym = m->data->mlet;
    int8_t mmoved = 0;		// not strictly nec.: chi >= 0 will
				// do
    struct coord poss[9];
    int info[9];

    if (m->mfroz || m->msleep)
	return 0;
    if (m->mtrapped) {
	i = mintrap(m);
	if (i == 2)
	    return 2;	       // he died
	if (i == 1)
	    return 0;	       // still in trap, so didnt move
    }
    if (m->mhide && o_at(m->mx, m->my) && rn2(10))
	return 0;	       // do not leave hiding place

    // likewise for shopkeeper
    if (m->isshk) {
	mmoved = shk_move(m);
	if (mmoved >= 0)
	    goto postmov;
	mmoved = 0;	       // follow player outside shop
    }
    // and for the guard
    if (m->isgd) {
	mmoved = gd_move();
	goto postmov;
    }
    // teleport if that lies in our nature ('t') or when badly wounded
    // ('1')
    if ((msym == 't' && !rn2(5))
	|| (msym == '1' && (m->mhp < 7 || (!_level->stair.dn.x && !rn2(5))
			    || _level->l[_u.ux][_u.uy].typ == STAIRS))) {
	if (m->mhp < 7 || (msym == 't' && rn2(2)))
	    rloc(m);
	else
	    mnexto(m);
	mmoved = 1;
	goto postmov;
    }
    // spit fire ('D') or use a wand ('1') when appropriate
    if (strchr("D1", msym))
	inrange(m);

    if (msym == 'U' && !m->mcan && canseemon(m) && m->mcansee && rn2(5)) {
	if (!Confusion)
	    pline("%s's gaze has confused you!", Monnam(m));
	else
	    pline("You are getting more and more confused.");
	if (rn2(3))
	    m->mcan = 1;
	Confusion += d(3, 4);  // timeout
    }
    appr = 1;
    if (m->mflee)
	appr = -1;
    if (m->mconf || Invis || !m->mcansee || (strchr("BIy", msym) && !rn2(3)))
	appr = 0;
    omx = m->mx;
    omy = m->my;
    gx = _u.ux;
    gy = _u.uy;
    if (msym == 'L' && appr == 1 && m->mgold > _u.ugold)
	appr = -1;

    // random criterion for 'smell' or track finding ability should use
    // m->msmell or sth
    if (msym == '@' || ('a' <= msym && msym <= 'z')) {
	struct coord *cp;
	int8_t mroom;
	mroom = inroom(omx, omy);
	if (mroom < 0 || mroom != inroom(_u.ux, _u.uy)) {
	    cp = gettrack(omx, omy);
	    if (cp) {
		gx = cp->x;
		gy = cp->y;
	    }
	}
    }
    // look for gold or jewels nearby
    likegold = (strchr("LOD", msym) != NULL);
    likegems = (strchr("ODu", msym) != NULL);
    likeobjs = m->mhide;
#define	SRCHRADIUS	25
    {
	int8_t mind = SRCHRADIUS;	// not too far away
	int dd;
	if (likegold) {
	    struct gold *gold;
	    for (gold = _level->money; gold; gold = gold->ngold)
		if ((dd = DIST(omx, omy, gold->gx, gold->gy)) < mind) {
		    mind = dd;
		    gx = gold->gx;
		    gy = gold->gy;
		}
	}
	if (likegems || likeobjs) {
	    for (struct obj* otmp = _level->objects; otmp; otmp = otmp->nobj) {
		if (likeobjs || otmp->olet == GEM_SYM) {
		    if (msym != 'u' || c_Objects[otmp->otyp].g_val != 0) {
			if ((dd = DIST(omx, omy, otmp->ox, otmp->oy)) < mind) {
			    mind = dd;
			    gx = otmp->ox;
			    gy = otmp->oy;
			}
		    }
		}
	    }
	}
	if (mind < SRCHRADIUS && appr == -1) {
	    if (dist(omx, omy) < 10) {
		gx = _u.ux;
		gy = _u.uy;
	    } else
		appr = 1;
	}
    }
    nix = omx;
    niy = omy;
    cnt = mfndpos(m, poss, info, msym == 'u' ? NOTONL : (msym == '@' || msym == '1') ? (ALLOW_SSM | ALLOW_TRAPS) : strchr(UNDEAD, msym) ? NOGARLIC : ALLOW_TRAPS);
    // ALLOW_ROCK for some monsters ?
    chcnt = 0;
    chi = -1;
    for (i = 0; i < cnt; ++i) {
	nx = poss[i].x;
	ny = poss[i].y;
	for (j = 0; j < MTSZ && j < cnt - 1; ++j)
	    if (nx == m->mtrack[j].x && ny == m->mtrack[j].y)
		if (rn2(4 * (cnt - j)))
		    goto nxti;
	nearer = (DIST(nx, ny, gx, gy) < DIST(nix, niy, gx, gy));
	if ((appr == 1 && nearer) || (appr == -1 && !nearer) || !mmoved || (!appr && !rn2(++chcnt))) {
	    nix = nx;
	    niy = ny;
	    chi = i;
	    mmoved = 1;
	}
      nxti:;
    }
    if (mmoved) {
	if (info[chi] & ALLOW_M) {
	    m2 = m_at(nix, niy);
	    if (hitmm(m, m2) == 1 && rn2(4) && hitmm(m2, m) == 2)
		return 2;
	    return 0;
	}
	if (info[chi] & ALLOW_U) {
	    hitu(m, d(m->data->damn, m->data->damd) + 1);
	    return 0;
	}
	m->mx = nix;
	m->my = niy;
	for (j = MTSZ - 1; j > 0; --j)
	    m->mtrack[j] = m->mtrack[j - 1];
	m->mtrack[0].x = omx;
	m->mtrack[0].y = omy;
    } else {
	if (msym == 'u' && rn2(2)) {
	    rloc(m);
	    return 0;
	}
    }
  postmov:
    if (mmoved == 1) {
	if (mintrap(m) == 2)	// he died
	    return 2;
	if (likegold)
	    mpickgold(m);
	if (likegems)
	    mpickgems(m);
	if (m->mhide)
	    m->mundetected = 1;
    }
    pmon(m);
    return mmoved;
}

void mpickgold(struct monst *m)
{
    struct gold *gold;
    while ((gold = g_at(m->mx, m->my)) != NULL) {
	m->mgold += gold->amount;
	freegold(gold);
	if (_level->l[m->mx][m->my].scrsym == '$')
	    newsym(m->mx, m->my);
    }
}

void mpickgems(struct monst *m)
{
    for (struct obj* o = _level->objects; o; o = o->nobj) {
	if (o->olet == GEM_SYM && o->ox == m->mx && o->oy == m->my
		&& (m->data->mlet != 'u' || c_Objects[o->otyp].g_val != 0)) {
	    freeobj(o);
	    mpickobj(m, o);
	    if (_level->l[m->mx][m->my].scrsym == GEM_SYM)
		newsym(m->mx, m->my);	// %%
	    return;    // pick only one object
	}
    }
}

// return number of acceptable neighbour positions
int mfndpos(struct monst *mon, struct coord poss[9], int info[9], int flag)
{
    int x, y, nx, ny, cnt = 0, ntyp;
    struct monst *m;
    int nowtyp;
    bool pool;

    x = mon->mx;
    y = mon->my;
    nowtyp = _level->l[x][y].typ;

    pool = (mon->data->mlet == ';');
  nexttry:		       // eels prefer the water, but if there is no
			       // water nearby, they will crawl over land
    if (mon->mconf) {
	flag |= ALLOW_ALL;
	flag &= ~NOTONL;
    }
    for (nx = x - 1; nx <= x + 1; ++nx)
	for (ny = y - 1; ny <= y + 1; ++ny)
	    if (nx != x || ny != y)
		if (isok(nx, ny))
		    if (!IS_ROCK(ntyp = _level->l[nx][ny].typ))
			if (!(nx != x && ny != y && (nowtyp == DOOR || ntyp == DOOR)))
			    if ((ntyp == POOL) == pool) {
				info[cnt] = 0;
				if (nx == _u.ux && ny == _u.uy) {
				    if (!(flag & ALLOW_U))
					continue;
				    info[cnt] = ALLOW_U;
				} else if ((m = m_at(nx, ny)) != NULL) {
				    if (!(flag & ALLOW_M))
					continue;
				    info[cnt] = ALLOW_M;
				}
				if (sobj_at(CLOVE_OF_GARLIC, nx, ny)) {
				    if (flag & NOGARLIC)
					continue;
				    info[cnt] |= NOGARLIC;
				}
				if (sobj_at(SCR_SCARE_MONSTER, nx, ny) || (!mon->mpeaceful && sengr_at("Elbereth", nx, ny))) {
				    if (!(flag & ALLOW_SSM))
					continue;
				    info[cnt] |= ALLOW_SSM;
				}
				if (sobj_at(ENORMOUS_ROCK, nx, ny)) {
				    if (!(flag & ALLOW_ROCK))
					continue;
				    info[cnt] |= ALLOW_ROCK;
				}
				if (!Invis && online(nx, ny)) {
				    if (flag & NOTONL)
					continue;
				    info[cnt] |= NOTONL;
				}
				// we cannot avoid traps of an unknown kind
				{
				    struct trap* t = t_at(nx, ny);
				    if (t) {
					unsigned tt = 1 << t->ttyp;
					if (mon->mtrapseen & tt) {
					    if (!(flag & tt))
						continue;
					    info[cnt] |= tt;
					}
				    }
				}
				poss[cnt].x = nx;
				poss[cnt].y = ny;
				++cnt;
			    }
    if (!cnt && pool && nowtyp != POOL) {
	pool = false;
	goto nexttry;
    }
    return cnt;
}

int dist(int x, int y)
{
    return (x - _u.ux) * (x - _u.ux) + (y - _u.uy) * (y - _u.uy);
}

void poisoned(const char *string, const char *pname)
{
    int i;

    if (Blind)
	pline("It was poisoned.");
    else
	pline("The %s was poisoned!", string);
    if (Poison_resistance) {
	pline("The poison doesn't seem to affect you.");
	return;
    }
    i = rn2(10);
    if (i == 0) {
	_u.uhp = -1;
	pline("I am afraid the poison was deadly ...");
    } else if (i <= 5) {
	losestr(rn1(3, 3));
    } else {
	losehp(rn1(10, 6), pname);
    }
    if (_u.uhp < 1) {
	killer = pname;
	done("died");
    }
}

void mondead (struct monst *m)
{
    relobj(m, 1);
    unpmon(m);
    relmon(m);
    if (m->isshk)
	shkdead(m);
    if (m->isgd)
	gddead();
    monfree (m);
}

// called when monster is moved to larger structure
void replmon (struct monst *m, struct monst *m2)
{
    relmon (m);
    monfree (m);
    m2->nmon = _level->monsters;
    _level->monsters = m2;
    if (m2->isshk)
	replshk(m, m2);
    if (m2->isgd)
	replgd(m, m2);
}

void relmon (struct monst* mtr)
{
    if (mtr == _level->monsters)
	_level->monsters = _level->monsters->nmon;
    else for (struct monst* m = _level->monsters; m; m = m->nmon)
	if (m->nmon == mtr)
	    m->nmon = mtr->nmon;
}

// we do not free monsters immediately, in order to
// have their name available shortly after their demise
static struct monst* fdmon = NULL;	// chain of dead monsters, need not to be saved

void monfree (struct monst* m)
{
    m->nmon = fdmon;
    fdmon = m;
}

void dmonsfree (void)
{
    for (struct monst *m; (m = fdmon); free(m))
	fdmon = m->nmon;
}

void killed (struct monst *m)
{
    int x, y;
    const struct permonst *mdat;

    mdat = m->data;
    pline("You destroy %s!", Blind ? "it" : monnam(m));
    if (_u.umconf) {
	if (!Blind)
	    pline("Your hands stop glowing blue.");
	_u.umconf = 0;
    }

    // punish bad behaviour
    if (mdat->mlet == '@')
	Telepat = 0, _u.uluck -= 2;
    if (m->mpeaceful)
	--_u.uluck;
    if (mdat->mlet == 'u')
	_u.uluck -= 5;
    if (_u.uluck < LUCKMIN)
	_u.uluck = LUCKMIN;

    // give experience points
    unsigned xp = 1 + square(mdat->mlevel);
    if (mdat->ac < 3)
	xp += 2 * (7 - mdat->ac);
    if (strchr("AcsSDXaeRTVWU&In:P", mdat->mlet))
	xp += 2 * mdat->mlevel;
    if (strchr("DeV&P", mdat->mlet))
	xp += (7 * mdat->mlevel);
    if (mdat->mlevel > 6)
	xp += 50;
    if (mdat->mlet == ';')
	xp += 1000;

    more_experienced(xp, 0);
    _wflags.botl = 1;
    while (_u.ulevel < 14 && _u.uexp >= newuexp()) {
	pline("Welcome to experience level %u.", ++_u.ulevel);
	xp = rnd(10);
	if (xp < 3)
	    xp = rnd(10);
	_u.uhpmax += xp;
	_u.uhp += xp;
	_wflags.botl = 1;
    }

    // dispose of monster and make cadaver
    x = m->mx;
    y = m->my;
    mondead(m);
    char tmp = mdat->mlet;
    if (tmp == 'm') {	       // he killed a minotaur, give him a wand of digging
	// note: the dead minotaur will be on top of it!
	mksobj_at(WAN_DIGGING, x, y);
	// if(cansee(x,y)) atl(x,y,_level->objects->olet);
	stackobj(_level->objects);
    } else if (!letter(tmp) || (!strchr("mw", tmp) && !rn2(3)))
	tmp = 0;

    if (ACCESSIBLE(_level->l[x][y].typ))	// might be dead eel
	if (x != _u.ux || y != _u.uy)	// might be here after
	    if (strchr("NTVm&", mdat->mlet) || rn2(5)) {
		struct obj *obj2 = mkobj_at(tmp, x, y);
		if (cansee(x, y))
		    atl(x, y, obj2->olet);
		stackobj(obj2);
	    }
}

void kludge(const char *str, const char *arg)
{
    if (Blind) {
	if (*str == '%')
	    pline(str, "It");
	else
	    pline(str, "it");
    } else
	pline(str, arg);
}

// Make monster m next to you (if possible)
void mnexto (struct monst *m)
{
    struct coord mm = enexto(_u.ux, _u.uy);
    m->mx = mm.x;
    m->my = mm.y;
    pmon(m);
}

int ishuman(struct monst *m)
{
    return m->data->mlet == '@';
}

void setmangry(struct monst *m)
{
    if (!m->mpeaceful)
	return;
    m->mpeaceful = false;
    if (ishuman(m))
	pline("%s gets angry!", Monnam(m));
}

//
// not one hundred procent correct: now a snake may hide under an invisible
// object
int canseemon(struct monst *m)
{
    return (!m->minvis || See_invisible)
	    && (!m->mhide || !o_at(m->mx, m->my))
	    && cansee(m->mx, m->my);
}

struct monst *fallen_down = 0;	// monsters that fell through a trapdoor
// they will appear on the next level @ goes to, even if he goes up!

void fall_down(struct monst *m)
{
    relmon(m);
    m->nmon = fallen_down;
    fallen_down = m;
    unpmon(m);
}
