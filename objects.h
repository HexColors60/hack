// This file is free software, distributed under the BSD license.

#pragma once
#include "config.h"
#include "objclass.h"

// objects have letter " % ) ( 0 _ ` [ ! ? / = *

const struct objclass c_Objects[] = {

    { .oc_name = "strange object", .oc_olet = ILLOBJ_SYM, .oc_prob = 100 },
    { .oc_name = "amulet of Yendor", .oc_olet = AMULET_SYM, .oc_prob = 100, .oc_weight = 2 },

#define	FOOD(name,prob,delay,weight,nutrit)\
    { .oc_name = name, .oc_merge = true,\
	.oc_olet = FOOD_SYM, .oc_prob = prob, .oc_delay = delay,\
	.oc_weight = weight, .nutrition = nutrit }

    // food 4 can be read
    // food 5 improves your vision
    // food 6 makes you stronger (like Popeye)
    // foods CORPSE up to CORPSE+52 are cadavers

    FOOD("food ration", 50, 5, 4, 800),
    FOOD("tripe ration", 20, 1, 2, 200),
    FOOD("pancake", 3, 1, 1, 200),
    FOOD("dead lizard", 3, 0, 1, 40),
    FOOD("fortune cookie", 7, 0, 1, 40),
    FOOD("carrot", 2, 0, 1, 50),
    FOOD("tin", 7, 0, 1, 0),
    FOOD("orange", 1, 0, 1, 80),
    FOOD("apple", 1, 0, 1, 50),
    FOOD("pear", 1, 0, 1, 50),
    FOOD("melon", 1, 0, 1, 100),
    FOOD("banana", 1, 0, 1, 80),
    FOOD("candy bar", 1, 0, 1, 100),
    FOOD("egg", 1, 0, 1, 80),
    FOOD("clove of garlic", 1, 0, 1, 40),
    FOOD("lump of royal jelly", 0, 0, 1, 200),

    FOOD("dead human", 0, 4, 40, 400),
    FOOD("dead giant ant", 0, 1, 3, 30),
    FOOD("dead giant bat", 0, 1, 3, 30),
    FOOD("dead centaur", 0, 5, 50, 500),
    FOOD("dead dragon", 0, 15, 150, 1500),
    FOOD("dead floating eye", 0, 1, 1, 10),
    FOOD("dead freezing sphere", 0, 1, 1, 10),
    FOOD("dead gnome", 0, 1, 10, 100),
    FOOD("dead hobgoblin", 0, 2, 20, 200),
    FOOD("dead stalker", 0, 4, 40, 400),
    FOOD("dead jackal", 0, 1, 10, 100),
    FOOD("dead kobold", 0, 1, 10, 100),
    FOOD("dead leprechaun", 0, 4, 40, 400),
    FOOD("dead nymph", 0, 4, 40, 400),
    FOOD("dead orc", 0, 2, 20, 200),
    FOOD("dead purple worm", 0, 7, 70, 700),
    FOOD("dead quasit", 0, 2, 20, 200),
    FOOD("dead rust monster", 0, 5, 50, 500),
    FOOD("dead snake", 0, 1, 10, 100),
    FOOD("dead troll", 0, 4, 40, 400),
    FOOD("dead umber hulk", 0, 5, 50, 500),
    FOOD("dead vampire", 0, 4, 40, 400),
    FOOD("dead wraith", 0, 1, 1, 10),
    FOOD("dead xorn", 0, 7, 70, 700),
    FOOD("dead yeti", 0, 7, 70, 700),
    FOOD("dead zombie", 0, 1, 3, 30),
    FOOD("dead acid blob", 0, 1, 3, 30),
    FOOD("dead giant beetle", 0, 1, 1, 10),
    FOOD("dead cockatrice", 0, 1, 3, 30),
    FOOD("dead dog", 0, 2, 20, 200),
    FOOD("dead ettin", 0, 1, 3, 30),
    FOOD("dead fog cloud", 0, 1, 1, 10),
    FOOD("dead gelatinous cube", 0, 1, 10, 100),
    FOOD("dead homunculus", 0, 2, 20, 200),
    FOOD("dead imp", 0, 1, 1, 10),
    FOOD("dead jaguar", 0, 3, 30, 300),
    FOOD("dead killer bee", 0, 1, 1, 10),
    FOOD("dead leocrotta", 0, 5, 50, 500),
    FOOD("dead minotaur", 0, 7, 70, 700),
    FOOD("dead nurse", 0, 4, 40, 400),
    FOOD("dead owlbear", 0, 7, 70, 700),
    FOOD("dead piercer", 0, 2, 20, 200),
    FOOD("dead quivering blob", 0, 1, 10, 100),
    FOOD("dead giant rat", 0, 1, 3, 30),
    FOOD("dead giant scorpion", 0, 1, 10, 100),
    FOOD("dead tengu", 0, 3, 30, 300),
    FOOD("dead unicorn", 0, 3, 30, 300),
    FOOD("dead violet fungi", 0, 1, 10, 100),
    FOOD("dead xan", 0, 3, 30, 300),
    FOOD("dead yellow light", 0, 1, 1, 10),
    FOOD("dead zruty", 0, 6, 60, 600),

    // weapons ... - ROCK come several at a time
    // weapons ... - (ROCK-1) are shot using idem+(BOW-ARROW)
    // weapons AXE, SWORD, THSWORD are good for worm-cutting
    // weapons (PICK-)AXE and DAGGER are good for tin-opening
#define WEAPON(name,prob,wt,ldam,sdam)\
    { .oc_name = name, .oc_olet = WEAPON_SYM, .oc_prob = prob,\
	.oc_weight = wt, .wldam = ldam, .wsdam = sdam }

    WEAPON("arrow", 7, 0, 6, 6),
    WEAPON("sling bullet", 7, 0, 4, 6),
    WEAPON("crossbow bolt", 7, 0, 4, 6),
    WEAPON("dart", 7, 0, 3, 2),
    WEAPON("rock", 6, 1, 3, 3),
    WEAPON("boomerang", 2, 3, 9, 9),
    WEAPON("mace", 9, 3, 6, 7),
    WEAPON("axe", 6, 3, 6, 4),
    WEAPON("flail", 6, 3, 6, 5),
    WEAPON("long sword", 8, 3, 8, 12),
    WEAPON("two handed sword", 6, 4, 12, 6),
    WEAPON("dagger", 6, 3, 4, 3),
    WEAPON("spear", 6, 3, 6, 8),
    WEAPON("bow", 6, 3, 4, 6),
    WEAPON("sling", 5, 3, 6, 6),
    WEAPON("crossbow", 6, 3, 4, 6),

    { .oc_name = "can opener", .oc_merge = true, .oc_olet = TOOL_SYM, .oc_prob = 40, .oc_weight = 1 },
    { .oc_name = "pick-axe", .oc_merge = true, .oc_olet = TOOL_SYM, .oc_prob = 40, .oc_weight = 5, .wldam = 6, .wsdam = 3 },
    { .oc_name = "expensive camera", .oc_merge = true, .oc_olet = TOOL_SYM, .oc_prob = 10, .oc_weight = 3 },
    { .oc_name = "ice box", .oc_olet = TOOL_SYM, .oc_prob = 10, .oc_weight = 40 },

    { .oc_name = "enormous rock", .oc_olet = ROCK_SYM, .oc_prob = 100, .oc_weight = 200 },

#define ARMOR(name,prob,delay,ac,can)\
    { .oc_name = name, .oc_olet = ARMOR_SYM, .oc_prob = prob,\
	.oc_delay = delay, .oc_weight = 8, .a_ac = ac, .a_can = can }
    ARMOR("helmet", 3, 1, 9, 0),
    ARMOR("plate mail", 5, 5, 3, 2),
    ARMOR("splint mail", 8, 5, 4, 1),
    ARMOR("banded mail", 10, 5, 4, 0),
    ARMOR("chain mail", 10, 5, 5, 1),
    ARMOR("scale mail", 10, 5, 6, 0),
    ARMOR("ring mail", 15, 5, 7, 0),
    // the armors below do not rust
    ARMOR("studded leather armor", 13, 3, 7, 1),
    ARMOR("leather armor", 17, 3, 8, 0),
    ARMOR("elven cloak", 5, 0, 9, 3),
    ARMOR("shield", 3, 0, 9, 0),
    ARMOR("pair of gloves", 1, 1, 9, 0),

#define POTION(name,color,prob)\
    { .oc_name = name, .oc_descr = color, .oc_merge = true,\
	.oc_olet = POTION_SYM, .oc_prob = prob, .oc_weight = 2 }
    POTION("restore strength", "smoky", 2),
    POTION("booze", "bubbly", 20),
    POTION("invisibility", "glowing", 5),
    POTION("fruit juice", "orange", 20),
    POTION("healing", "pink", 10),
    POTION("paralysis", "puce", 1),
    POTION("monster detection", "purple", 10),
    POTION("object detection", "yellow", 10),
    POTION("sickness", "white", 1),
    POTION("confusion", "swirly", 1),
    POTION("gain strength", "purple-red", 1),
    POTION("speed", "ruby", 2),
    POTION("blindness", "dark green", 1),
    POTION("gain level", "emerald", 5),
    POTION("extra healing", "sky blue", 10),
    POTION("levitation", "brown", 1),

#define SCROLL(name,text,prob)\
    { .oc_name = name, .oc_descr = text, .oc_merge = true,\
	.oc_olet = SCROLL_SYM, .oc_prob = prob, .oc_weight = 3 }
    SCROLL("enchant armor", "ZELGO MER", 8),
    SCROLL("destroy armor", "JUYED AWK YACC", 5),
    SCROLL("confuse monster", "NR 9", 5),
    SCROLL("scare monster", "XIXAXA XOXAXA XUXAXA", 4),
    SCROLL("blank paper", "READ ME", 3),
    SCROLL("remove curse", "PRATYAVAYAH", 6),
    SCROLL("enchant weapon", "DAIYEN FOOELS", 6),
    SCROLL("damage weapon", "HACKEM MUCHE", 5),
    SCROLL("create monster", "LEP GEX VEN ZEA", 5),
    SCROLL("genocide", "ELBIB YLOH", 2),
    SCROLL("light", "VERR YED HORRE", 10),
    SCROLL("teleportation", "VENZAR BORGAVVE", 5),
    SCROLL("gold detection", "THARR", 4),
    SCROLL("food detection", "YUM YUM", 1),
    SCROLL("identify", "KERNOD WEL", 18),
    SCROLL("magic mapping", "ELAM EBOW", 5),
    SCROLL("amnesia", "DUAM XNAHT", 3),
    SCROLL("fire", "ANDOVA BEGARIN", 5),

#define	WAND(name,metal,prob,flags)\
    { .oc_name = name, .oc_descr = metal, .oc_olet = WAND_SYM,\
	.oc_prob = prob, .oc_weight = 3, .bits = flags }
    WAND("light", "iridium", 10, NODIR),
    WAND("secret door detection", "tin", 5, NODIR),
    WAND("create monster", "platinum", 5, NODIR),
    WAND("wishing", "glass", 1, NODIR),
    WAND("striking", "zinc", 9, IMMEDIATE),
    WAND("slow monster", "balsa", 5, IMMEDIATE),
    WAND("speed monster", "copper", 5, IMMEDIATE),
    WAND("undead turning", "silver", 5, IMMEDIATE),
    WAND("polymorph", "brass", 5, IMMEDIATE),
    WAND("cancellation", "maple", 5, IMMEDIATE),
    WAND("teleportation", "pine", 5, IMMEDIATE),
    WAND("make invisible", "marble", 9, IMMEDIATE),
    WAND("digging", "iron", 5, RAY),
    WAND("magic missile", "oak", 10, RAY),
    WAND("fire", "steel", 5, RAY),
    WAND("sleep", "curved", 5, RAY),
    WAND("cold", "short", 5, RAY),
    WAND("death", "runed", 1, RAY),

#define	RING(name,stone,prob,spec)\
    { .oc_name = name, .oc_descr = stone, .oc_olet = RING_SYM, .oc_prob = prob, .oc_weight = 1, .bits = spec }
    RING("adornment", "engagement", 20, 0),
    RING("teleportation", "wooden", 1, 0),
    RING("regeneration", "black onyx", 1, 0),
    RING("searching", "topaz", 10, 0),
    RING("see invisible", "pearl", 1, 0),
    RING("stealth", "sapphire", 10, 0),
    RING("levitation", "moonstone", 10, 0),
    RING("poison resistance", "agate", 10, 0),
    RING("aggravate monster", "tiger eye", 1, 0),
    RING("hunger", "shining", 1, 0),
    RING("fire resistance", "gold", 5, 0),
    RING("cold resistance", "copper", 1, 0),
    RING("protection from shape changers", "diamond", 10, 0),
    RING("conflict", "jade", 1, 0),
    RING("gain strength", "ruby", 1, SPEC),
    RING("increase damage", "silver", 5, SPEC),
    RING("protection", "granite", 10, SPEC),
    RING("warning", "ivory", 1, 0),
    RING("teleport control", "iron", 1, 0),

#define	GEM(name,color,prob,gval)\
    { .oc_name = name, .oc_descr = color, .oc_merge = true, .oc_olet = GEM_SYM,\
	.oc_prob = prob, .oc_weight = 1, .g_val = gval }
    GEM("diamond", "blue", 1, 4000),
    GEM("ruby", "red", 1, 3500),
    GEM("sapphire", "blue", 1, 3000),
    GEM("emerald", "green", 1, 2500),
    GEM("turquoise", "green", 1, 2000),
    GEM("aquamarine", "blue", 1, 1500),
    GEM("tourmaline", "green", 1, 1000),
    GEM("topaz", "yellow", 1, 900),
    GEM("opal", "yellow", 1, 800),
    GEM("garnet", "dark", 1, 700),
    GEM("amethyst", "violet", 2, 650),
    GEM("agate", "green", 2, 600),
    GEM("onyx", "white", 2, 550),
    GEM("jasper", "yellowish brown", 2, 500),
    GEM("jade", "green", 2, 450),
    GEM("blue glass", "blue", 20, 0),
    GEM("red glass", "red", 20, 0),
    GEM("yellow glass", "yellow", 20, 0),
    GEM("green glass", "green", 20, 0),
};
