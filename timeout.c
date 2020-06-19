// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"

void do_timed_tasks (void)
{
    struct prop *upp;
    if (Stoned)
	stoned_dialogue();
    for (upp = _u.uprops; upp < _u.uprops + ArraySize(_u.uprops); ++upp)
	if ((upp->p_flgs & TIMEOUT) && !--upp->p_flgs) {
	    if (upp->p_tofn)
		(*upp->p_tofn) ();
	    else
		switch (upp - _u.uprops) {
		    case STONED:
			killer = "cockatrice";
			done("died");
			break;
		    case SICK:
			pline("You die because of food poisoning.");
			killer = _u.usick_cause;
			done("died");
			break;
		    case FAST:
			pline("You feel yourself slowing down.");
			break;
		    case CONFUSION:
			pline("You feel less confused now.");
			break;
		    case BLIND:
			pline("You can see again.");
			setsee();
			break;
		    case INVIS:
			on_scr(_u.ux, _u.uy);
			pline("You are no longer invisible.");
			break;
		    case WOUNDED_LEGS:
			heal_legs();
			break;
		}
	}
}

void stoned_dialogue(void)
{
    // He is being petrified - dialogue by inmet!tower
    static const char *const c_StonedTexts[] = {
	"You are slowing down.",	// 5
	"Your limbs are stiffening.",	// 4
	"Your limbs have turned to stone.",// 3
	"You have turned to stone.",	// 2
	"You are a statue."		// 1
    };
    unsigned i = (Stoned & TIMEOUT);
    if (i > 0 && i <= ArraySize(c_StonedTexts))
	pline (c_StonedTexts[ArraySize(c_StonedTexts) - i]);
    if (i == 5)
	Fast = 0;
    if (i == 3)
	nomul(-3);
}
