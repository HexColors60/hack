// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <signal.h>

int dosave (void)
{
    if (dosave0 (true)) {
	settty ("Be seeing you ...\n");
	exit (EXIT_SUCCESS);
    }
    return 0;
}

_Noreturn void hangup (int n UNUSED)
{
    dosave0 (false);
    exit (EXIT_FAILURE);
}

// returns 1 if save successful
bool dosave0 (bool failok)
{
    // Create the saved game directory first, if absent
    const char* homedir = getenv ("HOME");
    if (!homedir)
	return false;
    char savefile [PATH_MAX];
    snprintf (ArrayBlock(savefile), _PATH_SAVED_GAMES, homedir);
    if (0 != access (savefile, R_OK))
	mkpath (savefile, S_IRWXU);
    snprintf (ArrayBlock(savefile), HACK_SAVEFILE, homedir);

    int fd = creat (savefile, S_IRUSR| S_IWUSR);
    if (fd < 0) {
	if (failok)
	    pline("Cannot save game: %s (Continue or Quit)", strerror(errno));
	return false;
    }
    bwrite (fd, &_u, sizeof(struct you));
    saveobjchn (fd, invent);
    saveobjchn (fd, fcobj);
    savemonchn (fd, fallen_down);
    bwrite (fd, &_wflags, sizeof(struct worldflag));
    bwrite (fd, pl_character, sizeof pl_character);
    savenames (fd);
    savegenocided (fd);
    for (unsigned i = 0; i < _u.maxdlevel; ++i)
	savelev (fd, &_levels[i]);
    close (fd);
    return true;
}

bool dorecover (void)
{
    char savefile [PATH_MAX];
    snprintf (ArrayBlock(savefile), HACK_SAVEFILE, getenv("HOME"));
    int fd = open (savefile, O_RDONLY);
    if (fd < 0)
	return false;
    mread (fd, &_u, sizeof(struct you));
    if (_u.maxdlevel < 1 || _u.maxdlevel > MAXLEVEL) {
	close (fd);
	you_dtor();
	return false;
    }
    invent = restobjchn (fd);
    for (struct obj* o = invent; o; o = o->nobj)
	if (o->owornmask)
	    setworn (o, o->owornmask);
    fcobj = restobjchn (fd);
    fallen_down = restmonchn (fd);
    mread (fd, &_wflags, sizeof(struct worldflag));
    mread (fd, pl_character, sizeof pl_character);
    restnames (fd);
    restgenocided (fd);
    for (unsigned i = 0; i < _u.maxdlevel; ++i) {
	level_dtor (&_levels[i]);
	getlev (fd, &_levels[i]);
    }
    if (_u.dlevel < 1 || _u.dlevel > _u.maxdlevel)
	_u.dlevel = _u.maxdlevel;
    _level = &_levels[_u.dlevel-1];
    setsee();	// only to recompute seelx etc. - these weren't saved
    docrt();
    close (fd);
    unlink (savefile);
    return true;
}
