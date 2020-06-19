// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <signal.h>
#include <sys/wait.h>

static bool whole_screen = true;
enum { PAGMIN = 12 };	       // minimum # of lines for page below level map

void set_whole_screen (void)
{			       // called in termcap as soon as LI is known
    whole_screen = (LI - ROWNO - 2 <= PAGMIN || !CD);
}

// 0: open  1: wait+close  2: close
void set_pager (int mode)
{
    static bool so;
    if (mode == 0) {
	if (!whole_screen) {
	    clrlin();		// clear topline
	    curs(1, ROWNO + 4);	// use part of screen below level map
	} else
	    cls();
	so = _wflags.standout;
	_wflags.standout = 1;
    } else {
	if (mode == 1) {
	    curs(1, LI);
	    more();
	}
	_wflags.standout = so;
	if (whole_screen)
	    docrt();
	else {
	    curs(1, ROWNO + 4);
	    cl_eos();
	}
    }
}

// returns 1 if we should quit
int page_line (const char *s)
{
    if (cury == LI - 1) {
	if (!*s)
	    return 0;	       // suppress blank lines at top
	putchar('\n');
	++cury;
	cmore("q\033");
	if (morc) {
	    morc = 0;
	    return 1;
	}
	if (whole_screen)
	    cls();
	else {
	    curs(1, ROWNO + 4);
	    cl_eos();
	}
    }
    puts(s);
    ++cury;
    return 0;
}

// Flexible pager: feed it with a number of lines and it will decide
// whether these should be fed to the pager above, or displayed in a
// corner.
// Call:
//      cornline(0, title or 0) : initialize
//      cornline(1, text)       : add text to the chain of texts
//      cornline(2, morcs)      : output everything and cleanup
//      cornline(3, 0)          : cleanup
//
void cornline (int mode, const char* text)
{
    static struct line {
	struct line *next_line;
	char *line_text;
    } *texthead, *texttail;
    static int maxlen;
    static int linect;

    if (mode == 0) {
	texthead = 0;
	maxlen = 0;
	linect = 0;
	if (text) {
	    cornline(1, text); // title
	    cornline(1, "");   // blank line
	}
	return;
    }
    if (mode == 1) {
	int len;

	if (!text)
	    return;	       // superfluous, just to be sure
	++linect;
	len = strlen(text);
	if (len > maxlen)
	    maxlen = len;
	struct line* tl = alloc (len + sizeof(struct line) + 1);
	tl->next_line = 0;
	tl->line_text = (char*)(tl + 1);
	strcpy (tl->line_text, text);
	if (!texthead)
	    texthead = tl;
	else
	    texttail->next_line = tl;
	texttail = tl;
	return;
    }
    // --- now we really do it ---
    if (mode == 2 && linect == 1)	// topline only
	pline(texthead->line_text);
    else if (mode == 2) {
	int curline, lth;

	if (_wflags.toplin == 1)
	    more();	       // ab@unido
	remember_topl();

	lth = CO - maxlen - 2; // Use full screen width
	if (linect < LI && lth >= 10) {	// in a corner
	    home();
	    cl_end();
	    _wflags.toplin = 0;
	    curline = 1;
	    for (struct line* tl = texthead; tl; tl = tl->next_line) {
		curs(lth, curline);
		if (curline > 1)
		    cl_end();
		putsym(' ');
		putstr(tl->line_text);
		++curline;
	    }
	    curs(lth, curline);
	    cl_end();
	    cmore(text);
	    home();
	    cl_end();
	    docorner(lth, curline - 1);
	} else {	       // feed to pager
	    set_pager(0);
	    for (struct line* tl = texthead; tl; tl = tl->next_line) {
		if (page_line (tl->line_text)) {
		    set_pager(2);
		    goto cleanup;
		}
	    }
	    if (text) {
		cgetret(text);
		set_pager(2);
	    } else
		set_pager(1);
	}
    }
  cleanup:
    for (struct line* tl; (tl = texthead);) {
	texthead = tl->next_line;
	free (tl);
    }
}

static void show_long_text (const char* text)
{
    int pfd[2];
    if (0 > pipe (pfd))
	return perror ("pipe");
    pid_t pid = fork();
    switch (pid) {
	case -1:
	    close (pfd[0]);
	    close (pfd[1]);
	    return perror ("fork");
	case 0: {
	    close (pfd[1]);
	    dup2 (pfd[0], STDIN_FILENO);
	    const char* pagerbin = getenv("PAGER");
	    if (!pagerbin)
		pagerbin = _PATH_PAGER;
	    execlp (pagerbin, pagerbin, NULL);
	    perror ("execlp");
	    exit (EXIT_FAILURE); }
	default: {
	    close (pfd[0]);
	    for (const char *itext = text, *itextend = itext+strlen(text); itext < itextend;) {
		ssize_t bw = write (pfd[1], itext, itextend-itext);
		if (bw <= 0 && errno != EINTR)
		    break;
		itext += bw;
	    }
	    close (pfd[1]);
	    int status;
	    waitpid (pid, &status, 0);
	    } break;
    }
}

int dohelp (void)
{
    //{{{ Long help text
    static const char c_LongHelpText[] =
	"	Welcome to HACK!\n"
	"\n"
	"	Hack is a Dungeons and Dragons like game where you (the adventurer)\n"
	"descend into the depths of the dungeon in search of the Amulet of Yendor\n"
	"(reputed to be hidden on the twentieth level). On the way you will find\n"
	"useful (or useless) items, (quite possibly with magic properties) and\n"
	"assorted monsters. You attack a monster by trying to move into the space\n"
	"a monster is in (but often it is much wiser to leave it alone).\n"
	"\n"
	"	Unlike most adventure games, which give you a verbal description of\n"
	"your location, hack gives you a visual image of the dungeon level you are on.\n"
	"The full symbol map may be viewed by pressing /.\n"
	"\n"
	"	Hack uses the following symbols:\n"
	"		A to Z and a to z: monsters.\n"
	"		- and |  These form the walls of a room (or maze).\n"
	"		. this is the floor of a room.\n"
	"		#  this is a corridor.\n"
	"		>  this is the staircase to the next level.\n"
	"		<  the staircase to the previous level.\n"
	"		`  A large boulder.\n"
	"		@  You (usually).\n"
	"		^  A trap.\n"
	"		)  A weapon of some sort.\n"
	"		(  Some other useful object (key, rope, dynamite, camera, ...)\n"
	"		[  A suit of armor.\n"
	"		%  A piece of food (not necessarily healthy ...).\n"
	"		/  A wand.\n"
	"		=  A ring.\n"
	"		?  A scroll.\n"
	"		!  A magic potion.\n"
	"		$  A pile or pot of gold.\n"
	"\n"
	"Commands:\n"
	"	Hack knows the following commands:\n"
	"	?	help: print this list.\n"
	"	Q	Quit the game.\n"
	"	S	Save the game.\n"
	"	!	Escape to a shell.\n"
	"	<	up: go up the staircase (if you are standing on it).\n"
	"	>	down: go down (just like up).\n"
	"	kjhlyubn - go one step in the direction indicated.\n"
	"		k: north (i.e., to the top of the screen),\n"
	"		j: south, h: west, l: east, y: nw, u: ne, b: sw, n: se.\n"
	"	KJHLYUBN - Go in that direction until you hit a wall or run\n"
	"		into something.\n"
	"	m	(followed by one of kjhlyubn): move without picking up\n"
	"		any objects.\n"
	"	M	(followed by one of KJHLYUBN): Move far, no pickup.\n"
	"	f	(followed by one of kjhlyubn): move until something\n"
	"		interesting is found.\n"
	"	F	(followed by one of KJHLYUBN): as previous, but forking\n"
	"		of corridors is not considered interesting.\n"
	"	i	print your inventory.\n"
	"	I	print selected parts of your inventory, like in\n"
	"		I* - print all gems in inventory;\n"
	"		IU - print all unpaid items;\n"
	"		IX - print all used up items that are on your shopping bill;\n"
	"		I$ - count your money.\n"
	"	s	search for secret doors and traps around you.\n"
	"	^	ask for the type of a trap you found earlier.\n"
	"	)	ask for current wielded weapon.\n"
	"	[	ask for current armor.\n"
	"	=	ask for current rings.\n"
	"	$	count how many gold pieces you are carrying.\n"
	"	.	rest, do nothing.\n"
	"	,	pick up some things.\n"
	"	:	look at what is here.\n"
	"	^T	teleport.\n"
	"	^R	redraw the screen.\n"
	"	^P	repeat last message\n"
	"		(subsequent ^P's repeat earlier messages).\n"
	"	/	(followed by any symbol): tell what this symbol represents.\n"
	"	\\	tell what has been discovered.\n"
	"	e	eat food.\n"
	"	w	wield weapon. w- means: wield nothing, use bare hands.\n"
	"	q	drink (quaff) a potion.\n"
	"	r	read a scroll.\n"
	"	T	Takeoff armor.\n"
	"	R	Remove Ring.\n"
	"	W	Wear armor.\n"
	"	P	Put on a ring.\n"
	"	z	zap a wand.\n"
	"	t	throw an object or shoot an arrow.\n"
	"	p	pay your shopping bill.\n"
	"	d	drop something. d7a: drop seven items of object a.\n"
	"	D	Drop several things.\n"
	"		In answer to the question \"What kinds of things do you\n"
	"		want to drop? [!%= au]\" you should give zero or more\n"
	"		object symbols possibly followed by 'a' and/or 'u'.\n"
	"		'a' means: drop all such objects, without asking for\n"
	"			confirmation.\n"
	"		'u' means: drop only unpaid objects (when in a shop).\n"
	"	a	use, apply - Generic command for using a key to lock\n"
	"		or unlock a door, using a camera, using a rope, etc.\n"
	"	c	call: name a certain object or class of objects.\n"
	"	C	Call: Name an individual monster.\n"
	"	E	Engrave: Write a message in the dust on the floor.\n"
	"		E- means: use fingers for writing.\n"
	"	O	Set options. You will be asked to enter an option line.\n"
	"		If this is empty, the current options are reported.\n"
	"		Otherwise it should be a list of options separated by commas.\n"
	"		Possible boolean options are: oneline, time, tombstone,\n"
	"		rest_on_space, fixinvlet.\n"
	"		They can be negated by prefixing them with '!' or \"no\".\n"
	"		A string option is name; it supplies the answer to the question\n"
	"		\"Who are you?\"; it may have a suffix.\n"
	"		A compound option is endgame; it is followed by a description\n"
	"		of what parts of the list of topscorers should be printed\n"
	"		when the game is finished.\n"
	"		Usually one will not want to use the 'O' command, but instead\n"
	"		put a HACKOPTIONS=\"....\" line in one's environment.\n"
	"	v	print version number.\n"
	"\n"
	"	You can put a number before a command to repeat it that many times,\n"
	"	as in \"20s\" or \"40.\".\n"
	"\n"
	"	At present, some information is displayed on the bottom line.\n"
	"	You see on what dungeon level you are, how many hit points you have\n"
	"	now (and will have when fully recovered), what your armor class is\n"
	"	(the lower the better), your strength, experience level and the\n"
	"	state of your stomach.\n"
	"\n"
	"	Have Fun, and Good Hacking!\n"
	;
    //}}}
    //{{{ Short help text
    static const char c_ShortHelpText[] =
	"y k u	Move commands:\n"
	" \\|/		hykulnjb: single move in specified direction\n"
	"h-+-l		HYKULNJB: repeated move in specified direction\n"
	" /|\\			(until stopped by e.g. a wall)\n"
	"b j n		f<dir>: fast movement in direction <dir>\n"
	"			(until something interesting is seen)\n"
	"		m<dir>: move without picking up objects\n"
	"\n"
	"Meta commands:\n"
	"Q	quit	leave the game\n"
	"S	save	save the game (to be continued later)\n"
	"!	sh	escape to some SHELL\n"
	"O	set	set options\n"
	"?	help	print information\n"
	"/	whatis	give name (and sometimes more info) of specified monster\n"
	"\\	known	print list of what's been discovered\n"
	"v	version	print version number\n"
	"^R	redraw	redraw the screen (^R denotes the symbol CTRL/R)\n"
	"^P	print	repeat last message  (subsequent ^P's repeat earlier messages)\n"
	"\n"
	"Game commands:\n"
	"^T	teleport teleport\n"
	"a	apply, use  use something (a key, camera, etc.)\n"
	"c	call	give a name to a class of objects\n"
	"d	drop	drop an object. d7a: drop seven items of object a.\n"
	"e	eat	eat something\n"
	"i	invent	list the inventory (all objects you are carrying)\n"
	"I	invent	list selected parts of the inventory\n"
	"		IU: list unpaid objects\n"
	"		IX: list unpaid but used up items\n"
	"		I$: count your money\n"
	"p	pay	pay your bill\n"
	"q	drink	quaff a potion\n"
	"r	read	read a scroll\n"
	"s	search	search for secret doors, hidden traps and monsters\n"
	"t	throw	throw or shoot a weapon\n"
	"w	wield	wield a weapon	(w-  wield nothing)\n"
	"z	zap	zap a wand\n"
	"D	Drop	drop several things\n"
	"E	Engrave write a message in the dust on the floor  (E-  use fingers)\n"
	"P	wear	put on a ring\n"
	"R	remove	remove a ring\n"
	"T	remove	take off some armor\n"
	"W	wear	put on some armor\n"
	"<	up	go up the stairs\n"
	">	down	go down the stairs\n"
	"^	trap_id	identify a previously found trap\n"
	"),[,=		ask for current weapon, armor, rings, respectively\n"
	"$	gold	count your gold\n"
	".	rest	wait a moment\n"
	",	pickup	pick up all you can carry\n"
	":	look	look at what is here\n"
	;
    //}}}

    pline("Long or short help? ");
    char c;
    while (((c = readchar()) != 'l') && (c != 's') && !strchr(quitchars, c))
	bell();
    if (!strchr(quitchars, c))
	show_long_text (c == 'l' ? c_LongHelpText : c_ShortHelpText);
    return 0;
}

int dowhatis (void)
{
    //{{{ Symbol descriptions
    static const char c_SymbolDescription[] =
	"@	human (or you)\n"
	"-	a wall\n"
	"|	a wall\n"
	"+	a door\n"
	".	the floor of a room\n"
	" 	a dark part of a room\n"
	"#	a corridor\n"
	"}	water filled area\n"
	"<	the staircase to the previous level\n"
	">	the staircase to the next level\n"
	"^	a trap\n"
	"$	a pile, pot or chest of gold\n"
	"%	a piece of food\n"
	"!	a potion\n"
	"*	a gem\n"
	"?	a scroll\n"
	"=	a ring\n"
	"/	a wand\n"
	"[	a suit of armor\n"
	")	a weapon\n"
	"(	a useful item (camera, key, rope etc.)\n"
	"`	an enormous rock\n"
	"\"	an amulet\n"
	",	a trapper\n"
	";	a giant eel\n"
	"'	a lurker above\n"
	"&	a demon\n"
	"A	a giant ant\n"
	"B	a giant bat\n"
	"C	a centaur;\n"
	"	Of all the monsters put together by  the  Greek  imagination\n"
	"	the  Centaurs (Kentauroi) constituted a class in themselves.\n"
	"	Despite a strong streak  of  sensuality  in  their  make-up,\n"
	"	their  normal  behaviour  was  moral, and they took a kindly\n"
	"	thought of man's welfare. The attempted outrage of Nessos on\n"
	"	Deianeira,  and  that  of the whole tribe of Centaurs on the\n"
	"	Lapith women, are more than offset  by  the  hospitality  of\n"
	"	Pholos  and  by  the  wisdom of Cheiron, physician, prophet,\n"
	"	lyrist, and the instructor of Achilles.  Further,  the  Cen-\n"
	"	taurs  were  peculiar in that their nature, which united the\n"
	"	body of a horse with the trunk and head of a  man,  involved\n"
	"	an  unthinkable  duplication  of  vital organs and important\n"
	"	members. So grotesque a combination seems  almost  un-Greek.\n"
	"	These  strange  creatures were said to live in the caves and\n"
	"	clefts of the mountains, myths associating  them  especially\n"
	"	with the hills of Thessaly and the range of Erymanthos.\n"
	"	               [Mythology of all races, Vol. 1, pp. 270-271]\n"
	"D	a dragon;\n"
	"	In the West the dragon was the natural  enemy  of  man.  Although\n"
	"	preferring to live in bleak and desolate regions, whenever it was\n"
	"	seen among men it left in its wake a  trail  of  destruction  and\n"
	"	disease. Yet any attempt to slay this beast was a perilous under-\n"
	"	taking. For the dragon's assailant had to contend not  only  with\n"
	"	clouds  of  sulphurous fumes pouring from its fire-breathing nos-\n"
	"	trils, but also with the thrashings of its tail, the most  deadly\n"
	"	part of its serpent-like body.\n"
	"	[From: Mythical Beasts by Deirdre Headon (The Leprechaun Library)]\n"
	"E	a floating eye\n"
	"F	a freezing sphere\n"
	"G	a gnome;\n"
	"	... And then a gnome came by, carrying a bundle, an old fellow\n"
	"	three times as large as an imp and wearing clothes of a sort,\n"
	"	especially a hat. And he was clearly just as frightened as the\n"
	"	imps though he could not go so fast. Ramon Alonzo saw that there\n"
	"	must be some great trouble that was vexing magical things; and,\n"
	"	since gnomes speak the language of men, and will answer if spoken\n"
	"	to gently, he raised his hat, and asked of the gnome his name.\n"
	"	The gnome did not stop his hasty shuffle a moment as he answered\n"
	"	'Alaraba' and grabbed the rim of his hat but forgot to doff it.\n"
	"	'What is the trouble, Alaraba?' said Ramon Alonzo.\n"
	"	'White magic. Run!' said the gnome ...\n"
	"			[From: The Charwoman's Shadow, by Lord Dunsany.]\n"
	"H	a hobgoblin;\n"
	"	Hobgoblin. Used by the  Puritans  and  in  later  times  for\n"
	"	wicked  goblin  spirits,  as in Bunyan's 'Hobgoblin nor foul\n"
	"	friend', but its more correct use is for the friendly  spir-\n"
	"	its  of  the brownie type.  In 'A midsummer night's dream' a\n"
	"	fairy says to Shakespeare's Puck:\n"
	"	        Those that Hobgoblin call you, and sweet Puck,\n"
	"	        You do their work, and they shall have good luck:\n"
	"	        Are you not he?\n"
	"	and obviously Puck would not wish to be called  a  hobgoblin\n"
	"	if that was an ill-omened word.\n"
	"	Hobgoblins are on the whole, good-humoured and ready  to  be\n"
	"	helpful,  but fond of practical joking, and like most of the\n"
	"	fairies rather nasty people to annoy. Boggarts hover on  the\n"
	"	verge of hobgoblindom.  Bogles are just over the edge.\n"
	"	One Hob mentioned by Henderson, was Hob Headless who haunted\n"
	"	the  road  between Hurworth and Neasham, but could not cross\n"
	"	the little river Kent, which flowed into the  Tess.  He  was\n"
	"	exorcised  and  laid under a large stone by the roadside for\n"
	"	ninety-nine years and a day. If anyone was so unwary  as  to\n"
	"	sit  on  that stone, he would be unable to quit it for ever.\n"
	"	The ninety-nine years is nearly up, so trouble may  soon  be\n"
	"	heard of on the road between Hurworth and Neasham.\n"
	"	               [Katharine Briggs, A  dictionary  of Fairies]\n"
	"I	an invisible stalker\n"
	"J	a jackal\n"
	"K	a kobold\n"
	"L	a leprechaun;\n"
	"	The Irish Leprechaun is the Faeries' shoemaker and is  known\n"
	"	under  various  names  in different parts of Ireland: Cluri-\n"
	"	caune in Cork, Lurican in Kerry, Lurikeen in Kildare and Lu-\n"
	"	rigadaun  in  Tipperary.  Although he works for the Faeries,\n"
	"	the Leprechaun is not of the same species. He is small,  has\n"
	"	dark  skin  and wears strange clothes.  His nature has some-\n"
	"	thing of the manic-depressive about it: first  he  is  quite\n"
	"	happy,  whistling merrily as he nails a sole on to a shoe; a\n"
	"	few minutes later, he is sullen and  morose,  drunk  on  his\n"
	"	home-made  heather ale. The Leprechaun's two great loves are\n"
	"	tobacco and whiskey, and he is a first-rate con-man,  impos-\n"
	"	sible  to  out-fox.  No  one, no matter how clever, has ever\n"
	"	managed to cheat him out of his hidden pot of  gold  or  his\n"
	"	magic  shilling. At the last minute he always thinks of some\n"
	"	way to divert his captor's attention  and  vanishes  in  the\n"
	"	twinkling  of  an eye.\n"
	"	                  [From: A Field Guide to the Little People\n"
	"	                     by  Nancy Arrowsmith & George Moorse. ]\n"
	"N	a nymph\n"
	"O	an orc\n"
	"P	a purple worm\n"
	"Q	a quasit\n"
	"R	a rust monster\n"
	"S	a snake\n"
	"T	a troll\n"
	"U	an umber hulk\n"
	"V	a vampire\n"
	"W	a wraith\n"
	"X	a xorn\n"
	"Y	a yeti\n"
	"Z	a zombie\n"
	"a	an acid blob\n"
	"b	a giant beetle\n"
	"c	a cockatrice;\n"
	"	Once in a great while, when the positions of the  stars  are\n"
	"	just  right, a seven-year-old rooster will lay an egg. Then,\n"
	"	along will come a snake, to coil around the egg, or a  toad,\n"
	"	to  squat  upon  the  egg, keeping it warm and helping it to\n"
	"	hatch. When it hatches, out comes a creature  called  basil-\n"
	"	isk, or cockatrice, the most deadly of all creatures. A sin-\n"
	"	gle glance from its yellow, piercing toad's eyes  will  kill\n"
	"	both  man  and beast. Its power of destruction is said to be\n"
	"	so great that sometimes simply to hear its  hiss  can  prove\n"
	"	fatal.  Its breath is so venomous that it causes all vege-\n"
	"	tation to wither.\n"
	"	There is, however, one  creature  which  can  withstand  the\n"
	"	basilisk's deadly gaze, and this is the weasel. No one knows\n"
	"	why this is so, but although the fierce weasel can slay  the\n"
	"	basilisk,  it will itself be killed in the struggle. Perhaps\n"
	"	the weasel knows the basilisk's fatal weakness: if  it  ever\n"
	"	sees  its own reflection in a mirror it will perish instant-\n"
	"	ly. But even a dead basilisk is dangerous, for  it  is  said\n"
	"	that merely touching its lifeless body can cause a person to\n"
	"	sicken and die.\n"
	"	    [From: Mythical Beasts by Deirdre Headon (The Leprechaun\n"
	"	           Library) and other sources. ]\n"
	"d	a dog\n"
	"e	an ettin\n"
	"f	a fog cloud\n"
	"g	a gelatinous cube\n"
	"h	a homunculus\n"
	"i	an imp;\n"
	"	 ... imps ... little creatures of two feet high  that  could\n"
	"	gambol and jump prodigiously; ...\n"
	"			[From: The Charwoman's Shadow, by Lord Dunsany.]\n"
	"\n"
	"	An 'imp' is an off-shoot or cutting. Thus an 'ymp tree'  was\n"
	"	a grafted tree, or one grown from a cutting, not from seed.\n"
	"	'Imp' properly means a small devil, an off-shoot  of  Satan,\n"
	"	but  the distinction between goblins or bogles and imps from\n"
	"	hell is hard to make, and many in the  Celtic  countries  as\n"
	"	well as the English Puritans regarded all fairies as devils.\n"
	"	The fairies of tradition often hover  uneasily  between  the\n"
	"	ghostly and the diabolic state.\n"
	"	                 [Katharine Briggs, A dictionary of Fairies]\n"
	"j	a jaguar\n"
	"k	a killer bee\n"
	"l	a leocrotta\n"
	"m	a minotaur\n"
	"n	a nurse\n"
	"o	an owlbear\n"
	"p	a piercer\n"
	"q	a quivering blob\n"
	"r	a giant rat\n"
	"s	a scorpion\n"
	"t	a tengu;\n"
	"	The tengu was the  most  troublesome  creature  of  Japanese\n"
	"	legend.   Part  bird  and part man, with red beak for a nose\n"
	"	and flashing eyes, the tengu was notorious for  stirring  up\n"
	"	feuds  and  prolonging  enmity between families. Indeed, the\n"
	"	belligerent tengus were supposed to have  been  man's  first\n"
	"	instructors in the use of arms.\n"
	"	                    [From: Mythical Beasts by Deirdre Headon\n"
	"	                                 (The Leprechaun Library). ]\n"
	"u	a unicorn;\n"
	"	Men have always sought the elusive unicorn, for  the  single\n"
	"	twisted  horn  which projected from its forehead was thought\n"
	"	to be a powerful talisman. It was said that the unicorn  had\n"
	"	simply  to  dip  the tip of its horn in a muddy pool for the\n"
	"	water to become pure. Men also believed that to  drink  from\n"
	"	this horn was a protection against all sickness, and that if\n"
	"	the horn was ground to a powder it would act as an  antidote\n"
	"	to  all poisons. Less than 200 years ago in France, the horn\n"
	"	of a unicorn was used in a ceremony to test the  royal  food\n"
	"	for poison.\n"
	"	Although only the size of a small horse, the  unicorn  is  a\n"
	"	very  fierce  beast,  capable  of killing an elephant with a\n"
	"	single thrust from its horn.  Its  fleetness  of  foot  also\n"
	"	makes  this solitary creature difficult to capture.\n"
	"	                    [From: Mythical Beasts by Deirdre Headon\n"
	"	                                 (The Leprechaun Library). ]\n"
	"v	a violet fungi\n"
	"x	a xan;\n"
	"	The xan were animals sent to prick the legs of the Lords of Xibalba.\n"
	"y	a yellow light\n"
	"z	a zruty;\n"
	"	The zruty are wild and gigantic beings, living in the wildernesses\n"
	"	of the Tatra mountains.\n"
	"1	The wizard of Yendor\n"
	;
    //}}}
    show_long_text (c_SymbolDescription);
    return 0;
}
