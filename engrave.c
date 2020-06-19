// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

struct engr *engr_at(int x, int y)
{
    struct engr *ep = _level->engravings;
    while (ep) {
	if (x == ep->engr_x && y == ep->engr_y)
	    return ep;
	ep = ep->nxt_engr;
    }
    return NULL;
}

int sengr_at(const char *s, int x, int y)
{
    struct engr *ep = engr_at(x, y);
    char *t;
    int n;
    if (ep && ep->engr_time <= _u.moves) {
	t = ep->engr_txt;
	// if(!strcmp(s,t)) return 1;
	n = strlen(s);
	while (*t) {
	    if (!strncmp(s, t, n))
		return 1;
	    ++t;
	}
    }
    return 0;
}

void u_wipe_engr(int cnt)
{
    if (!Levitation)
	wipe_engr_at(_u.ux, _u.uy, cnt);
}

void wipe_engr_at(int x, int y, int cnt)
{
    struct engr *ep = engr_at(x, y);
    int lth, pos;
    char ch;
    if (ep) {
	if ((ep->engr_type != DUST) || Levitation) {
	    cnt = rn2(1 + 50 / (cnt + 1)) ? 0 : 1;
	}
	lth = strlen(ep->engr_txt);
	if (lth && cnt > 0) {
	    while (cnt--) {
		pos = rn2(lth);
		if ((ch = ep->engr_txt[pos]) == ' ')
		    continue;
		ep->engr_txt[pos] = (ch != '?') ? '?' : ' ';
	    }
	}
	while (lth && ep->engr_txt[lth - 1] == ' ')
	    ep->engr_txt[--lth] = 0;
	while (ep->engr_txt[0] == ' ')
	    ++ep->engr_txt;
	if (!ep->engr_txt[0])
	    del_engr(ep);
    }
}

void read_engr_at(int x, int y)
{
    struct engr *ep = engr_at(x, y);
    if (ep && ep->engr_txt[0]) {
	switch (ep->engr_type) {
	    default:
	    case DUST:	pline ("Something is written here in the dust."); break;
	    case ENGRAVE: pline ("Something is engraved here on the floor."); break;
	    case BURN:	pline ("Some text has been burned here in the floor."); break;
	}
	pline("You read: \"%s\".", ep->engr_txt);
    }
}

void make_engr_at (int x, int y, const char *s)
{
    struct engr* ep = engr_at (x, y);
    if (ep)
	del_engr(ep);
    uint8_t lth = min_u (strlen(s)+1, UINT8_MAX);
    ep = alloc ((sizeof(struct engr) + lth));
    ep->nxt_engr = _level->engravings;
    _level->engravings = ep;
    ep->engr_txt = (char*)(ep + 1);
    memcpy (ep->engr_txt, s, lth-1);
    ep->engr_txt[lth-1] = 0;
    ep->engr_lth = lth;
    ep->engr_x = x;
    ep->engr_y = y;
    ep->engr_type = DUST;
    ep->engr_time = 0;
}

int doengrave (void)
{
    int len;
    char *sp;
    struct engr *ep, *oep = engr_at(_u.ux, _u.uy);
    int8_t type;
    int spct;			// number of leading spaces
    struct obj *otmp;
    multi = 0;

    // one may write with finger, weapon or wand
    otmp = getobj("#-)/", "write with");
    if (!otmp)
	return 0;

    if (otmp && otmp->otyp == WAN_FIRE && otmp->spe) {
	type = BURN;
	--otmp->spe;
    } else {
	// first wield otmp
	if (otmp != uwep) {
	    if (uwep && uwep->cursed) {
		pline("Since your weapon is welded to your hand,");
		pline("you use the %s.", aobjnam(uwep, NULL));
		otmp = uwep;
	    } else {
		if (!otmp)
		    pline("You are now empty-handed.");
		else if (otmp->cursed)
		    pline("The %s %s to your hand!", aobjnam(otmp, "weld"), (otmp->quan == 1) ? "itself" : "themselves");
		else
		    pline("You now wield %s.", doname(otmp));
		setuwep(otmp);
	    }
	}
	type = DUST;
	if (otmp && (otmp->otyp == DAGGER || otmp->otyp == TWO_HANDED_SWORD || otmp->otyp == LONG_SWORD || otmp->otyp == AXE)) {
	    type = ENGRAVE;
	    if ((int) otmp->spe <= -3) {
		type = DUST;
		pline("Your %s too dull for engraving.", aobjnam(otmp, "are"));
		if (oep && oep->engr_type != DUST)
		    return 1;
	    }
	}
    }
    if (Levitation && type != BURN) {	// riv05!a3
	pline("You can't reach the floor!");
	return 1;
    }
    if (oep && oep->engr_type == DUST) {
	pline("You wipe out the message that was written here.");
	del_engr(oep);
	oep = 0;
    }
    if (type == DUST && oep) {
	pline("You cannot wipe out the message that is %s in the rock.", (oep->engr_type == BURN) ? "burned" : "engraved");
	return 1;
    }
    pline("What do you want to %s on the floor here? ", (type == ENGRAVE) ? "engrave" : (type == BURN) ? "burn" : "write");
    char buf[BUFSZ];
    getlin(buf);
    clrlin();
    spct = 0;
    sp = buf;
    while (*sp == ' ')
	++spct, ++sp;
    len = strlen(sp);
    if (!len || *buf == '\033') {
	if (type == BURN)
	    ++otmp->spe;
	return 0;
    }
    switch (type) {
	case DUST:
	case BURN:
	    if (len > 15) {
		multi = -(len / 10);
		nomovemsg = "You finished writing.";
	    }
	    break;
	case ENGRAVE:	       // here otmp != 0
	    {
		int len2 = (otmp->spe + 3) * 2 + 1;

		pline("Your %s dull.", aobjnam(otmp, "get"));
		if (len2 < len) {
		    len = len2;
		    sp[len] = 0;
		    otmp->spe = -3;
		    nomovemsg = "You cannot engrave more.";
		} else {
		    otmp->spe -= len / 2;
		    nomovemsg = "You finished engraving.";
		}
		multi = -len;
	    }
	    break;
    }
    if (oep)
	len += strlen(oep->engr_txt) + spct;
    len = min_u (len, UINT8_MAX-1);
    ep = alloc (sizeof(struct engr) + len + 1);
    ep->nxt_engr = _level->engravings;
    _level->engravings = ep;
    ep->engr_x = _u.ux;
    ep->engr_y = _u.uy;
    sp = (char*)(ep + 1);    // (char *)ep + sizeof(struct engr)
    ep->engr_txt = sp;
    if (oep) {
	char* spe = stpcpy(sp, oep->engr_txt);
	int btc = len - strlen(oep->engr_txt);
	if (btc > 0) {
	    memcpy (spe, buf, btc);
	    spe[btc] = 0;
	}
	del_engr(oep);
    } else
	strcpy(sp, buf);
    ep->engr_lth = len + 1;
    ep->engr_type = type;
    ep->engr_time = _u.moves - multi;

    // kludge to protect pline against excessively long texts
    if (len > BUFSZ - 20)
	sp[BUFSZ - 20] = 0;

    return 1;
}

void save_engravings (int fd, const struct engr* ehead)
{
    for (const struct engr* e = ehead; e; e = e->nxt_engr) {
	if (!e->engr_lth || !e->engr_txt[0])
	    continue;
	bwrite (fd, &e->engr_lth, sizeof(e->engr_lth));
	bwrite (fd, e, sizeof(struct engr) + e->engr_lth);
    }
    uint8_t eend = 0;
    bwrite (fd, &eend, sizeof(eend));
}

struct engr* rest_engravings (int fd)
{
    struct engr *ehead = NULL, **pp = &ehead;
    for (;;) {
	uint8_t lth;
	mread (fd, &lth, sizeof(lth));
	if (lth == 0)
	    break;
	struct engr* ep = alloc (sizeof(struct engr) + lth);
	mread (fd, ep, sizeof(struct engr) + lth);
	ep->engr_txt = (char*)(ep + 1);
	ep->nxt_engr = NULL;
	*pp = ep;
	pp = &ep->nxt_engr;
    }
    return ehead;
}

void del_engr (struct engr* etd)
{
    if (etd == _level->engravings)
	_level->engravings = etd->nxt_engr;
    else for (struct engr* e = _level->engravings; e; e = e->nxt_engr)
	if (e->nxt_engr == etd)
	    e->nxt_engr = etd->nxt_engr;
    free (etd);
}
