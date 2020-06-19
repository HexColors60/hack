// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

void initoptions(void)
{
    _wflags.end_top = 5;
    _wflags.end_around = 4;

    char* opts = getenv("HACKOPTIONS");
    if (opts)
	parseoptions(opts, true);
}

void parseoptions (char *opts, bool from_env)
{
    char *op, *op2;
    unsigned num;
    bool negated;

    if ((op = strchr(opts, ',')) != NULL) {
	*op++ = 0;
	parseoptions(op, from_env);
    }
    if ((op = strchr(opts, ' ')) != NULL) {
	op2 = op;
	while (*op++)
	    if (*op != ' ')
		*op2++ = *op;
    }
    if (!*opts)
	return;
    negated = false;
    while ((*opts == '!') || !strncmp(opts, "no", 2)) {
	if (*opts == '!')
	    ++opts;
	else
	    opts += 2;
	negated = !negated;
    }

    if (!strncmp(opts, "standout", 8)) {
	_wflags.standout = !negated;
	return;
    }
    if (!strncmp(opts, "null", 3)) {
	_wflags.nonull = negated;
	return;
    }
    if (!strncmp(opts, "tombstone", 4)) {
	_wflags.notombstone = negated;
	return;
    }
    if (!strncmp(opts, "time", 4)) {
	_wflags.time = !negated;
	_wflags.botl = 1;
	return;
    }
    if (!strncmp(opts, "restonspace", 4)) {
	_wflags.no_rest_on_space = negated;
	return;
    }
    if (!strncmp(opts, "fixinv", 4)) {
	if (from_env)
	    _wflags.invlet_not_constant = negated;
	else
	    pline("The fixinvlet option must be in HACKOPTIONS.");
	return;
    }
    // name:string
    if (!strncmp(opts, "name", 4)) {
	if (!from_env) {
	    pline("The playername can be set only from HACKOPTIONS.");
	    return;
	}
	op = strchr(opts, ':');
	if (!op)
	    goto bad;
	strncpy(plname, op + 1, sizeof(plname) - 1);
	return;
    }
    // endgame:5t[op] 5a[round] o[wn]
    if (!strncmp(opts, "endgame", 3)) {
	op = strchr(opts, ':');
	if (!op)
	    goto bad;
	++op;
	while (*op) {
	    num = 1;
	    if (digit(*op)) {
		num = atoi(op);
		while (digit(*op))
		    ++op;
	    } else if (*op == '!') {
		negated = !negated;
		++op;
	    }
	    switch (*op) {
		case 't':	_wflags.end_top = num; break;
		case 'a':	_wflags.end_around = num; break;
		case 'o':	_wflags.end_own = !negated; break;
		default:	goto bad;
	    }
	    while (letter(*++op));
	    if (*op == '/')
		++op;
	}
	return;
    }
  bad:
    if (!from_env) {
	if (!strncmp(opts, "help", 4)) {
	    pline("%s%s%s", "To set options use `HACKOPTIONS=\"<options>\"' in your environment, or ", "give the command 'O' followed by the line `<options>' while playing. ", "Here <options> is a list of <option>s separated by commas.");
	    pline("%s%s%s", "Simple (bool) options are rest_on_space, news, time, ", "null, tombstone, (fe)male. ", "These can be negated by prefixing them with '!' or \"no\".");
	    pline("%s", "A string option is name, as in HACKOPTIONS=\"name:Merlin-W\".");
	    pline("%s%s%s", "A compound option is endgame; it is followed by a description of what ", "parts of the scorelist you want to see. You might for example say: ", "`endgame:own scores/5 top scores/4 around my score'.");
	    return;
	}
	pline("Bad option: %s.", opts);
	pline("Type `O help<cr>' for help.");
	return;
    }
    puts("Bad syntax in HACKOPTIONS.");
    puts("Use for example:");
    puts("HACKOPTIONS=\"!restonspace,notombstone,endgame:own/5 topscorers/4 around me\"");
    getret();
}

int doset(void)
{
    char buf [BUFSZ];
    pline ("What options do you want to set? ");
    getlin (buf);
    if (!buf[0] || buf[0] == '\033') {
	struct StringBuilder sb = StringBuilder_new (buf);
	StringBuilder_append (&sb, "HACKOPTIONS=");
	if (_wflags.standout)
	    StringBuilder_append (&sb, "standout,");
	if (_wflags.nonull)
	    StringBuilder_append (&sb, "nonull,");
	if (_wflags.time)
	    StringBuilder_append (&sb, "time,");
	if (_wflags.notombstone)
	    StringBuilder_append (&sb, "notombstone,");
	if (_wflags.no_rest_on_space)
	    StringBuilder_append (&sb, "!rest_on_space,");
	if (_wflags.end_top != 5 || _wflags.end_around != 4 || _wflags.end_own) {
	    StringBuilder_appendf (&sb, "endgame: %u topscores/%u around me", _wflags.end_top, _wflags.end_around);
	    if (_wflags.end_own)
		StringBuilder_append (&sb, "/own scores");
	} else if (StringBuilder_ptr(&sb) > &buf[0] && StringBuilder_ptr(&sb)[-1] == ',')
	    StringBuilder_backspace (&sb);
	pline(buf);
    } else
	parseoptions(buf, false);
    return 0;
}
