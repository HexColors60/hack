// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include "objects.h"

enum { N_OC_KNOWN_BYTES = NROFOBJECTS+7/8 };
static uint8_t _oc_known [N_OC_KNOWN_BYTES] = {};
static uint8_t _oc_descr [NROFOBJECTS] = {};

static const char _oc_syms[] = {
    ILLOBJ_SYM, AMULET_SYM, FOOD_SYM, WEAPON_SYM,
    TOOL_SYM, ROCK_SYM, ARMOR_SYM, POTION_SYM,
    SCROLL_SYM, WAND_SYM, RING_SYM, GEM_SYM
};
static uint8_t _oc_base [ArraySize(_oc_syms)] = {};

static unsigned object_sym_base_index (char sym)
{
    for (unsigned i = 0; i < ArraySize(_oc_syms); ++i)
	if (_oc_syms[i] == sym)
	    return i;
    return 0;
}

uint8_t object_sym_base (char sym)
    { return _oc_base[object_sym_base_index(sym)]; }
void set_object_known (unsigned oi)
    { _oc_known [oi/8] |= (1<<(oi%8)); }
void set_object_unknown (unsigned oi)
    { _oc_known [oi/8] &= ~(1<<(oi%8)); }
bool is_object_known (unsigned oi)
    { return _oc_known [oi/8] & (1<<(oi%8)); }

const char* object_description (unsigned oi)
{
    return is_object_known(oi)
	? c_Objects[oi].oc_name
	: c_Objects[_oc_descr[oi]].oc_descr;
}

unsigned probtype (char let)
{
    unsigned i = object_sym_base (let);
    int prob = rn2(100);
    while ((prob -= c_Objects[i].oc_prob) >= 0)
	++i;
    assert (c_Objects[i].oc_olet == let && c_Objects[i].oc_name);
    return i;
}

void init_objects (void)
{
    assert (NROFOBJECTS == ArraySize(c_Objects));

    // Determine bases for each object type
    for (unsigned i = 1; i < ArraySize(c_Objects); ++i) {
	if (!c_Objects[i].oc_descr)
	    set_object_known (i);
	if (c_Objects[i].oc_olet != c_Objects[i-1].oc_olet)
	    _oc_base[object_sym_base_index(c_Objects[i].oc_olet)] = i;
    }

    // Randomize descriptions, starting with potions = 7
    iota_u8 (ArrayBlock (_oc_descr));
    for (unsigned b = 7; b < ArraySize(_oc_base); ++b) {
	unsigned f = _oc_base[b], l = NROFOBJECTS;
	if (b < ArraySize(_oc_base)-1)
	    l = _oc_base[b+1];
	random_shuffle_u8 (&_oc_descr[f], l-f);
    }

    #ifndef NDEBUG
	// Verify probability counts adding to 100
	for (unsigned b = 0; b < ArraySize(_oc_base); ++b) {
	    unsigned f = _oc_base[b], l = NROFOBJECTS;
	    if (b < ArraySize(_oc_base)-1)
		l = _oc_base[b+1];
	    unsigned prob = 0;
	    for (unsigned o = f; o < l; ++o)
		prob += c_Objects[o].oc_prob;
	    if (prob != 100) {
		printf ("Probabilities for object type %u (%c) add up to %u\n", b, _oc_syms[b], prob);
		assert (prob == 100 && "Object probabilities do not add up to 100");
	    }
	}
    #endif
}

void savenames (int fd)
{
    bwrite (fd, _oc_known, sizeof(_oc_known));
    bwrite (fd, _oc_descr, sizeof(_oc_descr));
}

void restnames (int fd)
{
    mread (fd, _oc_known, sizeof(_oc_known));
    mread (fd, _oc_descr, sizeof(_oc_descr));
}

int dodiscovered(void)
{
    cornline (0, "Discoveries");
    unsigned ct = 0;
    for (unsigned i = 0; i < ArraySize(c_Objects); ++i) {
	if (is_object_known(i) && c_Objects[i].oc_descr) {
	    ++ct;
	    cornline (1, typename(i));
	}
    }
    if (ct == 0) {
	pline("You haven't discovered anything yet...");
	cornline (3, NULL);
    } else
	cornline (2, NULL);
    return 0;
}
