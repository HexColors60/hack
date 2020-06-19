// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <signal.h>

//----------------------------------------------------------------------

enum {
    NAMSZ	= PL_NSIZ,
    DTHSZ	= 64-(NAMSZ+11),
    POINTSMIN	= 1,	// must be > 0
    MAXSCORES	= 10	// must be >= 10
};

#define SCOREFILE_MAGIC	"hack\0"

struct score {
    unsigned	points;
    uint16_t	hp;
    uint16_t	maxhp;
    uint8_t	level;
    uint8_t	maxlevel;
    char	plchar;
    char	name [NAMSZ];
    char	death [DTHSZ];
};

//----------------------------------------------------------------------

static void outheader (void);
static void outentry (unsigned rank, const struct score* t1, int so);
static const char* itoa (int a);

//----------------------------------------------------------------------

static int done_stopprint = 0;
static int done_hup = 0;

//----------------------------------------------------------------------

int dodone(void)
{
    done1(0);
    return 0;
}

void done1 (int n UNUSED)
{
    signal(SIGINT, SIG_IGN);
    pline("Really quit?");
    if (readchar() != 'y') {
	signal(SIGINT, done1);
	clrlin();
	fflush(stdout);
	if (multi > 0)
	    nomul(0);
	return;
    }
    done("quit");
    // NOTREACHED
}

static void done_intr (int n UNUSED)
{
    ++done_stopprint;
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

static void done_hangup(int n)
{
    ++done_hup;
    signal(SIGHUP, SIG_IGN);
    done_intr(n);
}

void done_in_by (struct monst* mtmp)
{
    static char buf [BUFSZ];
    pline ("You die ...");
    if (mtmp->data->mlet == ' ') {
	snprintf (ArrayBlock(buf), "the ghost of %s", (const char*) mtmp->mextra);
	killer = buf;
    } else if (mtmp->mnamelth) {
	snprintf (ArrayBlock(buf), "%s called %s", mtmp->data->mname, NAME(mtmp));
	killer = buf;
    } else if (mtmp->minvis) {
	snprintf (ArrayBlock(buf), "invisible %s", mtmp->data->mname);
	killer = buf;
    } else
	killer = mtmp->data->mname;
    done("died");
}

// called with arg "died", "drowned", "escaped", "quit", "choked",
// "panicked", "burned", "starved" or "tricked"
// Be careful not to call panic from here!
void done (const char* st1)
{
    signal(SIGINT, done_intr);
    signal(SIGQUIT, done_intr);
    signal(SIGHUP, done_hangup);
    if (*st1 == 'q' && _u.uhp < 1)
	st1 = "died";
    else if (*st1 == 's')
	killer = "starvation";
    else if (*st1 == 'd' && st1[1] == 'r')
	killer = "drowning";
    else if (*st1 == 'p')
	killer = "panic";
    else if (*st1 == 't')
	killer = "trickery";
    else if (!strchr("bcd", *st1))
	killer = st1;
    paybill();
    if (_wflags.toplin == 1)
	more();
    if (strchr("bcds", *st1) && !_wflags.notombstone)
	outrip();
    if (*st1 == 'c')
	killer = st1;	       // after outrip()
    settty(NULL);	       // does a clear_screen()
    if (!done_stopprint)
	printf("Goodbye %s %s...\n\n", pl_character, plname);
    {
	long int tmp;
	tmp = _u.ugold - _u.ugold0;
	if (tmp < 0)
	    tmp = 0;
	if (*st1 == 'd' || *st1 == 'b')
	    tmp -= tmp / 10;
	_u.urexp += tmp;
	_u.urexp += 50 * _u.maxdlevel;
	if (_u.maxdlevel > 20)
	    _u.urexp += 1000 * ((_u.maxdlevel > 30) ? 10 : _u.maxdlevel - 20);
    }
    if (*st1 == 'e') {
	struct obj *otmp;
	int i;
	unsigned worthlessct = 0;
	bool has_amulet = false;

	killer = st1;
	printf("You escaped from the dungeon with %u points,\n", _u.urexp);
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (otmp->olet == GEM_SYM) {
		set_object_known (otmp->otyp);
		i = otmp->quan * c_Objects[otmp->otyp].g_val;
		if (i == 0) {
		    worthlessct += otmp->quan;
		    continue;
		}
		_u.urexp += i;
		if (!done_stopprint)
		    printf("\t%s (worth %d Zorkmids),\n", doname(otmp), i);
	    } else if (otmp->olet == AMULET_SYM) {
		otmp->known = 1;
		i = (otmp->spe < 0) ? 2 : 5000;
		_u.urexp += i;
		if (!done_stopprint)
		    printf("\t%s (worth %d Zorkmids),\n", doname(otmp), i);
		if (otmp->spe >= 0) {
		    has_amulet = true;
		    killer = "escaped (with amulet)";
		}
	    }
	}
	if (worthlessct)
	    if (!done_stopprint)
		printf("\t%u worthless piece%s of coloured glass,\n", worthlessct, plur(worthlessct));
	if (has_amulet)
	    _u.urexp *= 2;
    } else if (!done_stopprint)
	printf("You %s on dungeon level %hhu with %u points,\n", st1, _u.dlevel, _u.urexp);
    if (!done_stopprint)
	printf("and %u piece%s of gold, after %u move%s.\n", _u.ugold, plur(_u.ugold), _u.moves, plur(_u.moves));
    if (!done_stopprint)
	printf("You were level %u with a maximum of %hu hit points when you %s.\n", _u.ulevel, _u.uhpmax, st1);
    if (*st1 == 'e' && !done_stopprint) {
	getret();	       // all those pieces of coloured glass ...
	cls();
    }
    savescore();
    if (done_stopprint)
	printf("\n\n");
    exit (EXIT_SUCCESS);
}

//----------------------------------------------------------------------

static int compare_scores (const void* s1, const void* s2)
    { return sign (((const struct score*)s2)->points - ((const struct score*)s1)->points); }

static void read_scores (struct score* sc, unsigned nsc)
{
    read_score_file (HACK_SCOREFILE, SCOREFILE_MAGIC, sc, nsc*sizeof(struct score));
    // Check each score and zero if invalid
    for (struct score *s = sc; s < sc+nsc; ++s)
	if (!s->name[0] || s->name[sizeof(s->name)-1]
		|| !s->death[0] || s->death[sizeof(s->death)-1]
		|| s->maxlevel < 1 || s->maxlevel > MAXLEVEL
		|| s->level < 1 || s->level > s->maxlevel
		|| s->hp > s->maxhp || s->points < POINTSMIN
		|| !strchr ("TSFKCW", s->plchar))
	    memset (s, 0, sizeof(*s));
    // Resort to account for the above zeroing
    qsort (sc, nsc, sizeof(sc[0]), compare_scores);
}

void savescore (void)
{
    struct score scores [MAXSCORES] = {};
    read_scores (ArrayBlock(scores));

    // Create a new entry if player score is high enough
    if (_u.urexp > scores[ArraySize(scores)-1].points) {
	struct score* nsc = &scores[ArraySize(scores)-1];
	nsc->points = _u.urexp;
	nsc->level = _u.dlevel;
	nsc->maxlevel = _u.maxdlevel;
	nsc->hp = _u.uhp < 0 ? 0 : _u.uhp;
	nsc->maxhp = _u.uhpmax;
	nsc->plchar = pl_character[0];
	snprintf (ArrayBlock(nsc->name), "%s", plname);
	snprintf (ArrayBlock(nsc->death), "%s", killer);
	// Resort into correct position
	qsort (ArrayBlock(scores), sizeof(*nsc), compare_scores);
	// And write the score file
	write_score_file (HACK_SCOREFILE, SCOREFILE_MAGIC, scores, sizeof(scores));
	puts ("You made the top ten list!\n");
    }

    outheader();
    for (unsigned i = 0; i < ArraySize(scores) && scores[i].points; ++i) {
	const struct score* sc = &scores[i];
	const bool ismyscore = sc->points == _u.urexp
			&& sc->plchar == pl_character[0]
			&& 0 == strcmp (plname, sc->name)
			&& 0 == strcmp (killer, sc->death);
	outentry (i+1, sc, ismyscore);
    }
}

static void outheader (void)
{
    puts ("Number Points  Name                                                    Hp [max]");
}

// so>0: standout line; so=0: ordinary line; so<0: no output, return lth
static void outentry (unsigned rank, const struct score* t1, int so)
{
    bool quit = false, killed = false, starv = false;
    char linebuf [BUFSZ];
    struct StringBuilder sb = StringBuilder_new (linebuf);
    if (rank)
	StringBuilder_appendf (&sb, "%3d", rank);
    else
	StringBuilder_append (&sb, "   ");
    StringBuilder_appendf (&sb, " %6u %8s", t1->points, t1->name);
    if (t1->plchar == 'X')
	StringBuilder_append (&sb, " ");
    else
	StringBuilder_appendf (&sb, "-%c ", t1->plchar);
    if (!strncmp("escaped", t1->death, 7)) {
	if (!strcmp(" (with amulet)", t1->death + 7))
	    StringBuilder_append (&sb, "escaped the dungeon with amulet");
	else
	    StringBuilder_appendf (&sb, "escaped the dungeon [max level %d]", t1->maxlevel);
    } else {
	if (!strncmp(t1->death, "quit", 4)) {
	    quit = true;
	    if (t1->maxhp < 3 * t1->hp && t1->maxlevel < 4)
		StringBuilder_append (&sb, "cravenly gave up");
	    else
		StringBuilder_append (&sb, "quit");
	} else if (!strcmp(t1->death, "choked"))
	    StringBuilder_append (&sb, "choked on food");
	else if (!strncmp(t1->death, "starv", 5))
	    StringBuilder_append (&sb, "starved to death"), starv = true;
	else
	    StringBuilder_append (&sb, "was killed"), killed = true;
	StringBuilder_appendf (&sb, " on%s level %d", (killed || starv) ? "" : " dungeon", t1->level);
	if (t1->maxlevel != t1->level)
	    StringBuilder_appendf (&sb, " [max %d]", t1->maxlevel);
	if (quit && t1->death[4])
	    StringBuilder_appendf (&sb, "%s", t1->death + 4);
    }
    if (killed)
	StringBuilder_appendf (&sb, " by %s%s", (!strncmp(t1->death, "trick", 5) || !strncmp(t1->death, "the ", 4))
		? "" : strchr(vowels, *t1->death) ? "an " : "a ", t1->death);
    StringBuilder_append (&sb, ".");
    if (t1->maxhp) {
	char hpbuf [10];
	snprintf (ArrayBlock(hpbuf), (t1->hp > 0) ? itoa(t1->hp) : "-");
	int hppos = COLNO - 7 - strlen(hpbuf);
	if (StringBuilder_ptr(&sb) <= &linebuf[hppos]) {
	    while (StringBuilder_ptr(&sb) < &linebuf[hppos])
		StringBuilder_append (&sb, " ");
	    StringBuilder_appendf (&sb, "%s [%d]", hpbuf, t1->maxhp);
	}
    }
    if (so == 0)
	puts (linebuf);
    else if (so > 0) {
	standoutbeg();
	puts (linebuf);
	standoutend();
    }
}

static const char* itoa (int a)
{
    static char buf[12];
    snprintf (ArrayBlock(buf), "%d", a);
    return (buf);
}

char *eos(char *s)
{
    while (*s)
	++s;
    return (s);
}
