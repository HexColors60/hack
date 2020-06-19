// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "func_tab.h"

//----------------------------------------------------------------------

static char unctrlc(int sym);

//----------------------------------------------------------------------

const struct func_tab cmdlist[] = {
    {'\020', doredotopl},
    {'\022', doredraw},
    {'\024', dotele},
    {'a', doapply},
    // 'A' : UNUSED
    // 'b', 'B' : go sw
    {'d', dodrop},
    {'D', doddrop},
    {'e', doeat},
    {'E', doengrave},
    // 'f', 'F' : multiple go (might become 'fight')
    // 'g', 'G' : UNUSED
    // 'h', 'H' : go west
    {'I', dotypeinv},	       // Robert Viduya
    {'i', ddoinv},
    // 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N' : move commands
    // 'o', doopen,
    {'O', doset},
    {'p', dopay},
    {'P', dowearring},
    {'q', dodrink},
    {'Q', dodone},
    {'r', doread},
    {'R', doremring},
    {'s', dosearch},
    {'S', dosave},
    {'t', dothrow},
    {'T', doremarm},
    // 'u', 'U' : go ne
    {'v', doversion},
    // 'V' : UNUSED
    {'w', dowield},
    {'W', doweararm},
    // 'x', 'X' : UNUSED
    // 'y', 'Y' : go nw
    {'z', dozap},
    // 'Z' : UNUSED
    {'<', doup},
    {'>', dodown},
    {'/', dowhatis},
    {'?', dohelp},
    {'.', donull},
    {' ', donull},
    {',', dopickup},
    {':', dolook},
    {'^', doidtrap},
    {'\\', dodiscovered},      // Robert Viduya
    {WEAPON_SYM, doprwep},
    {ARMOR_SYM, doprarm},
    {RING_SYM, doprring},
    {'$', doprgold},
    {'#', doextcmd},
    {0, 0}
};

const struct ext_func_tab extcmdlist[] = {
    {"dip", dodip},
    {"pray", dopray},
    {NULL, donull}
};

//----------------------------------------------------------------------

void rhack(const char *cmd)
{
    const struct func_tab *tlist = cmdlist;
    bool firsttime = false;
    int res;

    if (!cmd) {
	firsttime = true;
	_wflags.nopick = 0;
	cmd = parse();
    }
    if (!*cmd || (*cmd & 0377) == 0377 || (_wflags.no_rest_on_space && *cmd == ' ')) {
	bell();
	_wflags.move = 0;
	return;		       // probably we just had an interrupt
    }
    if (movecmd(*cmd)) {
      walk:
	if (multi)
	    _wflags.mv = 1;
	domove();
	return;
    }
    if (movecmd(lowc(*cmd))) {
	_wflags.run = 1;
      rush:
	if (firsttime) {
	    if (!multi)
		multi = COLNO;
	    _u.last_str_turn = 0;
	}
	_wflags.mv = 1;
	domove();
	return;
    }
    if ((*cmd == 'f' && movecmd(cmd[1])) || movecmd(unctrlc(*cmd))) {
	_wflags.run = 2;
	goto rush;
    }
    if (*cmd == 'F' && movecmd(lowc(cmd[1]))) {
	_wflags.run = 3;
	goto rush;
    }
    if (*cmd == 'm' && movecmd(cmd[1])) {
	_wflags.run = 0;
	_wflags.nopick = 1;
	goto walk;
    }
    if (*cmd == 'M' && movecmd(lowc(cmd[1]))) {
	_wflags.run = 1;
	_wflags.nopick = 1;
	goto rush;
    }
    while (tlist->f_char) {
	if (*cmd == tlist->f_char) {
	    res = (*(tlist->f_funct)) ();
	    if (!res) {
		_wflags.move = 0;
		multi = 0;
	    }
	    return;
	}
	++tlist;
    }
    {
	char expcmd[10];
	char *cp = expcmd;
	while (*cmd && cp - expcmd < (int) sizeof(expcmd) - 2) {
	    if (*cmd >= 040 && *cmd < 0177)
		*cp++ = *cmd++;
	    else {
		*cp++ = '^';
		*cp++ = *cmd++ ^ 0100;
	    }
	}
	*cp++ = 0;
	pline("Unknown command '%s'.", expcmd);
    }
    multi = _wflags.move = 0;
}

int doextcmd(void)
{			       // here after # - now read a full-word
			       // command
    char buf[BUFSZ];
    const struct ext_func_tab *efp = extcmdlist;

    pline("# ");
    getlin(buf);
    clrlin();
    if (buf[0] == '\033')
	return 0;
    while (efp->ef_txt) {
	if (!strcmp(efp->ef_txt, buf))
	    return (*(efp->ef_funct)) ();
	++efp;
    }
    pline("%s: unknown command.", buf);
    return 0;
}

char lowc(int sym)
{
    return (sym >= 'A' && sym <= 'Z') ? sym + 'a' - 'A' : sym;
}

static char unctrlc(int sym)
{
    return (sym >= ('A' & 037) && sym <= ('Z' & 037)) ? sym + 0140 : sym;
}

// 'rogue'-like direction commands
char sdir[] = "hykulnjb><";
int8_t xdir[10] = { -1, -1, 0, 1, 1, 1, 0, -1, 0, 0 };
int8_t ydir[10] = { 0, -1, -1, -1, 0, 1, 1, 1, 0, 0 };
int8_t zdir[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

// also sets _u.dz, but returns false for <>
int movecmd (int sym)
{
    char *dp;

    _u.dz = 0;
    if (!(dp = strchr(sdir, sym)))
	return 0;
    _u.dx = xdir[dp - sdir];
    _u.dy = ydir[dp - sdir];
    _u.dz = zdir[dp - sdir];
    return !_u.dz;
}

int getdir (bool s)
{
    char dirsym;
    if (s)
	pline("In what direction?");
    dirsym = readchar();
    if (!movecmd(dirsym) && !_u.dz) {
	if (!strchr(quitchars, dirsym))
	    pline("What a strange direction!");
	return 0;
    }
    if (Confusion && !_u.dz)
	confdir();
    return 1;
}

void confdir(void)
{
    int x = rn2(8);
    _u.dx = xdir[x];
    _u.dy = ydir[x];
}

int isok(int x, int y)
{
    // x corresponds to curx, so x==1 is the first column. Ach. %%
    return x >= 1 && x <= COLNO - 1 && y >= 0 && y <= ROWNO - 1;
}
