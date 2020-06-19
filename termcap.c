// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "flag.h"
#include <termios.h>
#include <termcap.h>

static char tbuf[512];
char *HO, *CL, *CE, *UP, *CM, *ND, *XD, *BC, *SO, *SE, *TI, *TE;
static char *VS, *VE;
static int SG;
char PC = '\0';
char *CD;			// tested in pri.c: docorner()
int CO, LI;			// used in pri.c and whatis.c

void startup(void)
{
    char* term = getenv("TERM");
    if (!term)
	error("Can't get TERM.");
    if (!strncmp(term, "5620", 4))
	_wflags.nonull = 1;      // this should be a termcap flag
    char* tptr = alloc (1024);
    if (tgetent(tptr, term) < 1)
	error("Unknown terminal type: %s.", term);
    char* tbufptr = tbuf;
    char* pc = tgetstr("pc", &tbufptr);
    if (pc)
	PC = *pc;
    if (!(BC = tgetstr("bc", &tbufptr))) {
	if (!tgetflag("bs"))
	    error("Terminal must backspace.");
	BC = tbufptr;
	tbufptr += 2;
	*BC = '\b';
    }
    HO = tgetstr("ho", &tbufptr);
    CO = tgetnum("co");
    LI = tgetnum("li");
    if (CO < COLNO || LI < ROWNO + 2)
	setclipped();
    if (!(CL = tgetstr("cl", &tbufptr)))
	error("Hack needs CL.");
    ND = tgetstr("nd", &tbufptr);
    if (tgetflag("os"))
	error("Hack can't have OS.");
    CE = tgetstr("ce", &tbufptr);
    UP = tgetstr("up", &tbufptr);
    // It seems that xd is no longer supported, and we should use a
    // linefeed instead; unfortunately this requires resetting CRMOD, and
    // many output routines will have to be modified slightly. Let's
    // leave that till the next release.
    XD = tgetstr("xd", &tbufptr);
    // not:                 XD = tgetstr("do", &tbufptr);
    if (!(CM = tgetstr("cm", &tbufptr))) {
	if (!UP && !HO)
	    error("Hack needs CM or UP or HO.");
	printf("Playing hack on terminals without cm is suspect...\n");
	getret();
    }
    SO = tgetstr("so", &tbufptr);
    SE = tgetstr("se", &tbufptr);
    SG = tgetnum("sg");	       // -1: not fnd; else # of spaces left by so
    if (!SO || !SE || (SG > 0))
	SO = SE = 0;
    CD = tgetstr("cd", &tbufptr);
    set_whole_screen();	       // uses LI and CD
    if (tbufptr - tbuf > (int) sizeof(tbuf))
	error("TERMCAP entry too big...\n");
    free (tptr);
}

void start_screen(void)
{
    xputs(TI);
    xputs(VS);
}

void end_screen(void)
{
    xputs(VE);
    xputs(TE);
}

// Cursor movements
void curs (int x, int y)
{
    if (y == cury && x == curx)
	return;
    if (!ND && (curx != x || x <= 3)) {	// Extremely primitive
	cmov(x, y);	       // bunker!wtm
	return;
    }
    if (abs(cury - y) <= 3 && abs(curx - x) <= 3)
	nocmov(x, y);
    else if ((x <= 3 && abs(cury - y) <= 3) || (!CM && x < abs(curx - x))) {
	putchar('\r');
	curx = 1;
	nocmov(x, y);
    } else if (!CM) {
	nocmov(x, y);
    } else
	cmov(x, y);
}

void nocmov(int x, int y)
{
    if (cury > y) {
	if (UP) {
	    while (cury > y) { // Go up.
		xputs(UP);
		--cury;
	    }
	} else if (CM) {
	    cmov(x, y);
	} else if (HO) {
	    home();
	    curs(x, y);
	}		       // else impossible("...");
    } else if (cury < y) {
	if (XD) {
	    while (cury < y) {
		xputs(XD);
		++cury;
	    }
	} else if (CM) {
	    cmov(x, y);
	} else {
	    while (cury < y) {
		xputc('\n');
		curx = 1;
		++cury;
	    }
	}
    }
    if (curx < x) {	       // Go to the right.
	if (!ND)
	    cmov(x, y);
	else		       // bah
	    // should instead print what is there already
	    while (curx < x) {
		xputs(ND);
		++curx;
	    }
    } else if (curx > x) {
	while (curx > x) {     // Go to the left.
	    xputs(BC);
	    --curx;
	}
    }
}

void cmov(int x, int y)
{
    xputs(tgoto(CM, x - 1, y - 1));
    cury = y;
    curx = x;
}

int xputc(int c)
{
    return fputc(c, stdout);
}

void xputs(char *s)
{
    tputs(s, 1, xputc);
}

void cl_end(void)
{
    if (CE)
	xputs(CE);
    else {		       // no-CE fix - free after Harold Rynes
	// this looks terrible, especially on a slow terminal but is
	// better than nothing
	int cx = curx, cy = cury;
	while (curx < COLNO) {
	    xputc(' ');
	    ++curx;
	}
	curs(cx, cy);
    }
}

void clear_screen(void)
{
    xputs(CL);
    curx = cury = 1;
}

void home(void)
{
    if (HO)
	xputs(HO);
    else if (CM)
	xputs(tgoto(CM, 0, 0));
    else
	curs(1, 1);	       // using UP ...
    curx = cury = 1;
}

void standoutbeg(void)
{
    if (SO)
	xputs(SO);
}

void standoutend(void)
{
    if (SE)
	xputs(SE);
}

void backsp(void)
{
    xputs(BC);
    --curx;
}

void bell(void)
{
    putchar('\007');    // curx does not change
    fflush(stdout);
}

// free after Robert Viduya must only be called with curx = 1
void cl_eos(void)
{
    if (CD)
	xputs(CD);
    else {
	int cx = curx, cy = cury;
	while (cury <= LI - 2) {
	    cl_end();
	    xputc('\n');
	    curx = 1;
	    ++cury;
	}
	cl_end();
	curs(cx, cy);
    }
}
