// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <signal.h>

#define	gamename	"hack"

static void global_dtors (void);

int (*afternmv) (void);
int (*occupation) (void);
const char* occtxt = NULL;	// defined when occupation != NULL

int main (int argc, const char* const* argv)
{
    atexit (global_dtors);
    initoptions();
    // Set player name if not set in options
    if (!*plname)
	snprintf (ArrayBlock(plname), "%s", player_name());

    if (argc > 1 && !strncmp(argv[1], "-s", 2)) {
	savescore();
	return EXIT_SUCCESS;
    }

    // It seems he really wants to play.
    // Remember tty modes, to be restored on exit.
    gettty();
    srandrand();
    startup();
    cls();
    _u.uhp = 1;	       // prevent RIP on early quits
    _u.ux = FAR;       // prevent nscr()
    _level = &_levels[0];	// display routines currently don't work without an active level
    signal(SIGHUP, hangup);
    signal(SIGSEGV, hangup);
    signal(SIGABRT, hangup);
    signal(SIGINT, done1);
    init_objects();

    // Process options.
    while (argc > 1 && argv[1][0] == '-') {
	++argv;
	--argc;
	switch (argv[0][1]) {
	    case 'u':
		if (argv[0][2])
		    strncpy(plname, argv[0] + 2, sizeof(plname) - 1);
		else if (argc > 1) {
		    --argc;
		    ++argv;
		    strncpy(plname, argv[0], sizeof(plname) - 1);
		} else
		    printf("Player name expected after -u\n");
		break;
	    default:
		// allow -T for Tourist, etc.
		strncpy(pl_character, argv[0] + 1, sizeof(pl_character) - 1);

		// printf("Unknown option: %s\n", *argv);
	}
    }

    if (!*plname || !strncmp(plname, "player", 4) || !strncmp(plname, "games", 4))
	askname();
    plnamesuffix();	       // strip suffix from name; calls askname()
    // again if suffix was whole name
    // accepts any suffix
    setftty();

    if (dorecover()) {
	pline("Hello %s, welcome to %s!", plname, gamename);
	_wflags.move = 0;
    } else {
	_wflags.ident = 1;
	u_init();

	signal(SIGINT, done1);
	_u.dlevel = 1;
	_u.maxdlevel = 1;
	_level = &_levels[_u.dlevel-1];
	mklev();
	_u.ux = _level->stair.up.x;
	_u.uy = _level->stair.up.y;
	inshop();
	setsee();
	_wflags.botlx = 1;
	{
	    struct monst *mtmp;
	    if ((mtmp = m_at(_u.ux, _u.uy)) != NULL)
		mnexto(mtmp);  // riv05!a3
	}
	seemons();
	docrt();
	_wflags.move = 1;
    }

    _wflags.moonphase = phase_of_the_moon();
    if (_wflags.moonphase == FULL_MOON) {
	pline("You are lucky! Full moon tonight.");
	++_u.uluck;
    } else if (_wflags.moonphase == NEW_MOON) {
	pline("Be careful! New moon tonight.");
    }
    initrack();

    for (;;) {
	if (_wflags.move) {      // actual time passed

	    settrack();

	    if (_u.moves % 2 == 0 || (!(Fast & ~INTRINSIC) && (!Fast || rn2(3)))) {
		movemon();
		if (!rn2(70))
		    makemon(NULL, 0, 0);
	    }
	    if (Glib)
		glibr();
	    do_timed_tasks();
	    ++_u.moves;
	    if (_wflags.time)
		_wflags.botl = 1;
	    if (_u.uhp < 1) {
		pline("You die...");
		done("died");
	    }
	    if (_u.uhp * 10 < _u.uhpmax && _u.moves - wailmsg > 50) {
		wailmsg = _u.moves;
		if (_u.uhp == 1)
		    pline("You hear the wailing of the Banshee...");
		else
		    pline("You hear the howling of the CwnAnnwn...");
	    }
	    if (_u.uhp < _u.uhpmax) {
		if (_u.ulevel > 9) {
		    if (Regeneration || !(_u.moves % 3)) {
			_wflags.botl = 1;
			_u.uhp += rnd((int) _u.ulevel - 9);
			if (_u.uhp > _u.uhpmax)
			    _u.uhp = _u.uhpmax;
		    }
		} else if (Regeneration || (!(_u.moves % (22 - _u.ulevel * 2)))) {
		    _wflags.botl = 1;
		    ++_u.uhp;
		}
	    }
	    if (Teleportation && !rn2(85))
		tele();
	    if (Searching && multi >= 0)
		dosearch();
	    gethungry();
	    invault();
	    amulet();
	}
	if (multi < 0) {
	    if (!++multi) {
		pline(nomovemsg ? nomovemsg : "You can move again.");
		nomovemsg = 0;
		if (afternmv)
		    (*afternmv) ();
		afternmv = 0;
	    }
	}
	find_ac();
	if (!_wflags.mv || Blind) {
	    seeobjs();
	    seemons();
	    nscr();
	}
	if (_wflags.botl || _wflags.botlx)
	    bot();

	_wflags.move = 1;

	if (multi >= 0 && occupation) {
	    if (monster_nearby())
		stop_occupation();
	    else if ((*occupation) () == 0)
		occupation = 0;
	    continue;
	}
	if (multi > 0) {
	    lookaround();
	    if (!multi) {      // lookaround may clear multi
		_wflags.move = 0;
		continue;
	    }
	    if (_wflags.mv) {
		if (multi < COLNO && !--multi)
		    _wflags.mv = _wflags.run = 0;
		domove();
	    } else {
		--multi;
		rhack(save_cm);
	    }
	} else if (multi == 0)
	    rhack (NULL);
	if (multi && multi % 7 == 0)
	    fflush(stdout);
    }
}

// plname is filled either by an option (-u Player) or by askname.
// It may still contain a suffix denoting pl_character.
void askname (void)
{
    int c, ct;
    printf ("\nWho are you? ");
    ct = 0;
    while ((c = getchar()) != '\n') {
	if (c == EOF)
	    error("End of input\n");
	// some people get confused when their erase char is not ^H
	if (c == '\010') {
	    if (ct)
		--ct;
	    continue;
	}
	if (c != '-')
	    if (c < 'A' || (c > 'Z' && c < 'a') || c > 'z')
		c = '_';
	if ((size_t) ct < sizeof(plname) - 1)
	    plname[ct++] = c;
    }
    plname[ct] = 0;
    if (ct == 0)
	askname();
}

void impossible(const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    vpline(s, ap);
    va_end(ap);
    pline("Program in disorder - perhaps you'd better Quit.");
}

void stop_occupation(void)
{
    if (occupation) {
	pline ("You stop %s.", occtxt);
	occupation = 0;
    }
}

static void global_dtors (void)
{
    you_dtor();
    for (unsigned i = 0; i < ArraySize(_levels); ++i)
	level_dtor (&_levels[i]);
    _level = NULL;
}
