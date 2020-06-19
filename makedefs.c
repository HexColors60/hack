// This file is free software, distributed under the BSD license.

#include "common.h"
#include <fcntl.h>

// construct definitions of object constants
enum {
    LINSZ = 1000,
    STRSZ = 40
};

static int fd;
static char string[STRSZ];

static void readline(void);
static char nextchar(void);
static int skipuntil(const char *);
static int getentry(void);
static void capitalize(char *);
static int letter(int);
static int digit(int);

int main (int argc, const char* const* argv)
{
    int i = 0;
    int propct = 0;
    char *sp;
    if (argc != 2) {
	fprintf(stderr, "usage: makedefs file\n");
	return EXIT_FAILURE;
    }
    if ((fd = open(argv[1], O_RDONLY)) < 0) {
	perror(argv[1]);
	return EXIT_FAILURE;
    }
    skipuntil("c_Objects[] = {");
    printf ("#pragma once\nenum {\n");
    while (getentry()) {
	if (!*string) {
	    ++i;
	    continue;
	}
	for (sp = string; *sp; ++sp)
	    if (*sp == ' ' || *sp == '\t' || *sp == '-')
		*sp = '_';
	if (!strncmp (string, "RIN_", 4)) {
	    capitalize (string + 4);
	    printf ("#define %s\t\t_u.uprops[%d].p_flgs\n", string + 4, ++propct);
	}
	for (sp = string; *sp; ++sp)
	    capitalize(sp);
	if (i && !(i%10))
	    printf ("    %s, // %d\n", string, i);
	else
	    printf ("    %s,\n", string);
	++i;
    }
    printf ("    NROFOBJECTS,\n\n");
    printf ("    CORPSE\t= DEAD_HUMAN,\n");
    printf ("    LAST_GEM\t= (JADE+1),\n");
    printf ("    LAST_RING\t= %d,\n};\n", propct);
    fflush(stdout);
    if (ferror(stdout)) {
	perror("standard output");
	return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static char line[LINSZ], *lp = line, *lp0 = line, *lpe = line;
static int eof = 0;

static void readline (void)
{
    ssize_t n = read (fd, lp0, (line + LINSZ) - lp0);
    if (n < 0) {
	perror ("read");
	exit (EXIT_FAILURE);
    } else if (!n)
	++eof;
    lpe = lp0 + n;
}

static char nextchar (void)
{
    if (lp == lpe) {
	readline();
	lp = lp0;
    }
    return lp == lpe ? 0 : *lp++;
}

static int skipuntil (const char *s)
{
    const char *sp0;
    char *sp1;
    for (;;) {
	while (*s != nextchar()) {
	    if (eof) {
		printf ("Cannot skipuntil %s\n", s);
		exit (EXIT_FAILURE);
	    }
	}
	if ((int) strlen(s) > lpe - lp + 1) {
	    char *lp1, *lp2;
	    lp2 = lp;
	    lp1 = lp = lp0;
	    while (lp2 != lpe)
		*lp1++ = *lp2++;
	    lp2 = lp0;	       // save value
	    lp0 = lp1;
	    readline();
	    lp0 = lp2;
	    if ((int) strlen(s) > lpe - lp + 1) {
		printf ("error in skipuntil");
		exit (EXIT_FAILURE);
	    }
	}
	sp0 = s + 1;
	sp1 = lp;
	while (*sp0 && *sp0 == *sp1)
	    ++sp0, ++sp1;
	if (!*sp0) {
	    lp = sp1;
	    return 1;
	}
    }
}

static int getentry(void)
{
    int inbraces = 0, inparens = 0, stringseen = 0, commaseen = 0;
    int prefix = 0;
    enum { NSZ = 10 };
    char identif[NSZ], *ip;
    string[0] = string[4] = 0;
    // read until {...} or XXX(...) followed by , skip comment and
    // #define lines deliver 0 on failure
    for (char ch; (ch = nextchar());) {
      swi:
	if (letter(ch)) {
	    ip = identif;
	    do {
		if (ip < identif + NSZ - 1)
		    *ip++ = ch;
		ch = nextchar();
	    } while (letter(ch) || digit(ch));
	    *ip = 0;
	    while (ch == ' ' || ch == '\t')
		ch = nextchar();
	    if (ch == '(' && !inparens && !stringseen)
		if (!strcmp(identif, "WAND") || !strcmp(identif, "RING") || !strcmp(identif, "POTION") || !strcmp(identif, "SCROLL"))
		    strncpy(string, identif, 3), string[3] = '_', prefix = 4;
	}
	switch (ch) {
	    case '/':
		// watch for comment
		ch = nextchar();
		if (ch == '*')
		    skipuntil("*/");
		else if (ch == '/')
		    while (ch != '\n')
			ch = nextchar();
		goto swi;
	    case '{':
		++inbraces;
		break;
	    case '(':
		++inparens;
		break;
	    case '}':
		--inbraces;
		if (inbraces < 0)
		    return 0;
		break;
	    case ')':
		--inparens;
		if (inparens < 0) {
		    printf("too many ) ?");
		    exit(1);
		}
		break;
	    case '\n':
		// watch for #define at begin of line
		if ((ch = nextchar()) == '#') {
		    char pch;
		    // skip until '\n' not preceded by '\\'
		    do {
			pch = ch;
			ch = nextchar();
		    } while (ch != '\n' || pch == '\\');
		    continue;
		}
		goto swi;
	    case ',':
		if (!inparens && !inbraces) {
		    if (prefix && !string[prefix])
			string[0] = 0;
		    if (stringseen)
			return 1;
		    printf("unexpected ,\n");
		    exit(1);
		}
		++commaseen;
		break;
	    case '\'':
		if ((ch = nextchar()) == '\\')
		    ch = nextchar();
		if (nextchar() != '\'') {
		    printf("strange character denotation?\n");
		    exit(1);
		}
		break;
	    case '"': {
		char *sp = string + prefix;
		char pch;
		int store = (inbraces || inparens)
		    && !stringseen++ && !commaseen;
		do {
		    pch = ch;
		    ch = nextchar();
		    if (store && sp < string + STRSZ)
			*sp++ = ch;
		} while (ch != '"' || pch == '\\');
		if (store)
		    *--sp = 0;
		break;
	    }
	}
    }
    return 0;
}

static void capitalize(char *sp)
{
    if ('a' <= *sp && *sp <= 'z')
	*sp += 'A' - 'a';
}

static int letter(int ch)
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

static int digit(int ch)
{
    return '0' <= ch && ch <= '9';
}
