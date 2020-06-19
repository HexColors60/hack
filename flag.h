// This file is free software, distributed under the BSD license.

#pragma once

struct worldflag {
    unsigned ident;		// social security number for each monster
    unsigned toplin:2;		// a top line (message) has been printed
				// 0: top line empty; 2: no --More-- reqd.
    unsigned cbreak:1;		// in cbreak mode, rogue format
    unsigned standout:1;	// use standout for --More--
    unsigned nonull:1;		// avoid sending nulls to the terminal
    unsigned time:1;		// display elapsed 'time'
    unsigned notombstone:1;
    unsigned no_rest_on_space:1;// spaces are ignored
    unsigned invlet_not_constant:1;	// let objects keep their inventory symbol
    unsigned move:1;
    unsigned mv:1;
    unsigned run:3;		// 0: h (etc), 1: H (etc), 2: fh (etc)
				// 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF-
    unsigned nopick:1;		// do not pickup objects
    unsigned echo:1;		// 1 to echo characters
    unsigned botl:1;		// partially redo status line
    unsigned botlx:1;		// print an entirely new bottom line
    unsigned nscrinh:1;		// inhibit nscr() in pline();
    unsigned made_amulet:1;
    unsigned no_of_wizards:2;	// 0, 1 or 2 (wizard and his shadow)
				// reset from 2 to 1, but never to 0
    unsigned moonphase:3;
    unsigned end_own:1;		// idem (list all own scores)

    uint16_t end_top;		// describe desired score list
    uint16_t end_around;
};

enum {
    NEW_MOON,
    FULL_MOON = 4
};

extern struct worldflag _wflags;
