// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

unsigned rn1 (unsigned x, unsigned y)
{
    return nrand(x) + y;
}

unsigned rn2 (unsigned x)
{
    return nrand(x);
}

unsigned rnd (unsigned x)
{
    return nrand(x) + 1;
}

unsigned d (unsigned n, unsigned x)
{
    unsigned r = n;
    while (n--)
	r += nrand(x);
    return r;
}
