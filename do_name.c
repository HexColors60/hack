// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

struct coord getpos(int force, const char *goal)
{
    int cx, cy, i, c;
    struct coord cc;
    pline("(For instructions type a ?)");
    cx = _u.ux;
    cy = _u.uy;
    curs(cx, cy + 2);
    while ((c = readchar()) != '.') {
	for (i = 0; i < 8; ++i)
	    if (sdir[i] == c) {
		if (1 <= cx + xdir[i] && cx + xdir[i] <= COLNO)
		    cx += xdir[i];
		if (0 <= cy + ydir[i] && cy + ydir[i] <= ROWNO - 1)
		    cy += ydir[i];
		goto nxtc;
	    }
	if (c == '?') {
	    pline("Use [hjkl] to move the cursor to %s.", goal);
	    pline("Type a . when you are at the right place.");
	} else {
	    pline("Unknown direction: '%s' (%s).", visctrl(c), force ? "use hjkl or ." : "aborted");
	    if (force)
		goto nxtc;
	    cc.x = -1;
	    cc.y = 0;
	    return cc;
	}
      nxtc:;
	curs(cx, cy + 2);
    }
    cc.x = cx;
    cc.y = cy;
    return cc;
}

char* xmonnam (struct monst* mtmp, int vb)
{
    static const char ghostnames[][8] = {	// these names should have length < PL_NSIZ
	"adri", "andries", "andreas", "bert", "david", "dirk", "emile",
	"frans", "fred", "greg", "hether", "jay", "john", "jon", "kay",
	"kenny", "maud", "michiel", "mike", "peter", "robert", "ron",
	"tom", "wilmar"
    };
    static char buf[BUFSZ];
    if (mtmp->mnamelth && !vb) {
	snprintf (ArrayBlock(buf), "%s", NAME(mtmp));
	return buf;
    }
    switch (mtmp->data->mlet) {
	case ' ':
	    {
		const char *gn = (char *) mtmp->mextra;
		if (!*gn) {    // might also look in scorefile
		    gn = ghostnames[rn2(ArraySize(ghostnames))];
		    if (!rn2(2))
			strcpy((char *) mtmp->mextra, !rn2(5) ? plname : gn);
		}
		snprintf (ArrayBlock(buf), "%s's ghost", gn);
	    }
	    break;
	case '@':
	    if (mtmp->isshk) {
		snprintf (ArrayBlock(buf), "%s", shkname(mtmp));
		break;
	    }
	    // fallthrough
	default:
	    snprintf (ArrayBlock(buf), "the %s%s", mtmp->minvis ? "invisible " : "", mtmp->data->mname);
    }
    if (vb && mtmp->mnamelth) {
	strcat(buf, " called ");
	strcat(buf, NAME(mtmp));
    }
    return buf;
}

char *lmonnam(struct monst *mtmp)
{
    return xmonnam(mtmp, 1);
}

char *monnam(struct monst *mtmp)
{
    return xmonnam(mtmp, 0);
}

char *Monnam(struct monst *mtmp)
{
    char *bp = monnam(mtmp);
    if ('a' <= *bp && *bp <= 'z')
	*bp += ('A' - 'a');
    return bp;
}

char* amonnam (struct monst *mtmp, const char *adj)
{
    const char* bp = monnam(mtmp);
    if (!strncmp(bp, "the ", strlen("the ")))
	bp += strlen("the ");
    static char buf [BUFSZ];
    snprintf (ArrayBlock(buf), "the %s %s", adj, bp);
    return buf;
}

char *Amonnam(struct monst *mtmp, const char *adj)
{
    char *bp = amonnam(mtmp, adj);
    *bp = 'T';
    return bp;
}

char *Xmonnam(struct monst *mtmp)
{
    char *bp = Monnam(mtmp);
    if (!strncmp(bp, "The ", 4)) {
	bp += 2;
	*bp = 'A';
    }
    return bp;
}

char *visctrl(int c)
{
    static char ccc[3];
    if (c < 040) {
	ccc[0] = '^';
	ccc[1] = c + 0100;
	ccc[2] = 0;
    } else {
	ccc[0] = c;
	ccc[1] = 0;
    }
    return ccc;
}
