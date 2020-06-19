// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <termios.h>
#include <termcap.h>

static char erase_char, kill_char;
static bool settty_needed = false;
struct termios inittyb, curttyb;

static void releasetty (void);

//
// Get initial state of terminal, set ospeed (for termcap routines)
// and switch off tab expansion if necessary.
// Called by startup() in termcap.c and after returning from ! or ^Z
void gettty(void)
{
    if (tcgetattr(0, &inittyb) < 0)
	perror("Hack (gettty)");
    curttyb = inittyb;
    ospeed = cfgetospeed(&inittyb);
    erase_char = inittyb.c_cc[VERASE];
    kill_char = inittyb.c_cc[VKILL];
    getioctls();

    // do not expand tabs - they might be needed inside a cm sequence
    if (curttyb.c_oflag & XTABS) {
	curttyb.c_oflag &= ~XTABS;
	setctty();
    }
    settty_needed = true;
    atexit (releasetty);
}

static void releasetty (void)
{
    if (!settty_needed)
	return;
    clear_screen();
    end_screen();
    if (tcsetattr(0, TCSADRAIN, &inittyb) < 0)
	perror("Hack (settty)");
    _wflags.echo = (inittyb.c_lflag & ECHO) ? ON : OFF;
    _wflags.cbreak = (inittyb.c_lflag & ICANON) ? OFF : ON;
    setioctls();
    settty_needed = false;
}

// reset terminal to original state
void settty (const char *s)
{
    releasetty();
    if (s)
	printf("%s", s);
    fflush(stdout);
}

void setctty(void)
{
    if (tcsetattr(0, TCSADRAIN, &curttyb) < 0)
	perror("Hack (setctty)");
}

void setftty(void)
{
    int change = 0;
    _wflags.cbreak = ON;
    _wflags.echo = OFF;
    // Should use (ECHO|CRMOD) here instead of ECHO
    if (curttyb.c_lflag & ECHO) {
	curttyb.c_lflag &= ~ECHO;
	++change;
    }
    if (curttyb.c_lflag & ICANON) {
	curttyb.c_lflag &= ~ICANON;
	// be satisfied with one character; no timeout
	curttyb.c_cc[VMIN] = 1;
	curttyb.c_cc[VTIME] = 0;
	++change;
    }
    if (change) {
	setctty();
    }
    start_screen();
}

// fatal error
// VARARGS1
_Noreturn void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (settty_needed)
	settty (NULL);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    exit(1);
}

//
// Read a line closed with '\n' into the array char bufp[BUFSZ].
// (The '\n' is not stored. The string is closed with a '\0'.)
// Reading can be interrupted by an escape ('\033') - now the
// resulting string is "\033".
void getlin(char *bufp)
{
    char *obufp = bufp;
    int c;

    _wflags.toplin = 2;	       // nonempty, no --More-- required
    for (;;) {
	fflush(stdout);
	if ((c = getchar()) == EOF) {
	    *bufp = 0;
	    return;
	}
	if (c == '\033') {
	    *obufp = c;
	    obufp[1] = 0;
	    return;
	}
	if (c == erase_char || c == '\b') {
	    if (bufp != obufp) {
		--bufp;
		putstr("\b \b");	// putsym converts \b
	    } else
		bell();
	} else if (c == '\n') {
	    *bufp = 0;
	    return;
	} else if (' ' <= c && c < '\177') {
	    // avoid isprint() - some people don't have it ' ' is
	    // not always a printing char
	    *bufp = c;
	    bufp[1] = 0;
	    putstr(bufp);
	    if (bufp - obufp < BUFSZ - 1 && bufp - obufp < COLNO)
		++bufp;
	} else if (c == kill_char || c == '\177') {	// Robert Viduya
	    // this test last - @ might be the kill_char
	    while (bufp != obufp) {
		--bufp;
		putstr("\b \b");
	    }
	} else
	    bell();
    }
}

void getret(void)
{
    cgetret("");
}

void cgetret(const char *s)
{
    putsym('\n');
    if (_wflags.standout)
	standoutbeg();
    putstr("Hit ");
    putstr(_wflags.cbreak ? "space" : "return");
    putstr(" to continue: ");
    if (_wflags.standout)
	standoutend();
    xwaitforspace(s);
}

char morc;			// tell the outside world what char he used

void xwaitforspace(const char *s	// chars allowed besides space or return
    )
{
    int c;

    morc = 0;

    while ((c = readchar()) != '\n') {
	if (_wflags.cbreak) {
	    if (c == ' ')
		break;
	    if (s && strchr(s, c)) {
		morc = c;
		break;
	    }
	    bell();
	}
    }
}

char *parse(void)
{
    static char inputline[COLNO];
    int foo;

    _wflags.move = 1;
    if (!Invisible)
	curs_on_u();
    else
	home();
    while ((foo = readchar()) >= '0' && foo <= '9')
	multi = 10 * multi + foo - '0';
    if (multi) {
	--multi;
	save_cm = inputline;
    }
    inputline[0] = foo;
    inputline[1] = 0;
    if (foo == 'f' || foo == 'F') {
	inputline[1] = getchar();
	inputline[2] = 0;
    }
    if (foo == 'm' || foo == 'M') {
	inputline[1] = getchar();
	inputline[2] = 0;
    }
    clrlin();
    return inputline;
}

char readchar(void)
{
    int sym;

    fflush(stdout);
    if ((sym = getchar()) == EOF)
	end_of_input();
    if (_wflags.toplin == 1)
	_wflags.toplin = 2;
    return (char) sym;
}

_Noreturn void end_of_input(void)
{
    settty("End of input?\n");
    exit(0);
}
