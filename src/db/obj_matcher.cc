/* ************************************************************************
*   File: obj_matcher.cc                            NOT Part of CircleMUD *
*  Usage: Matcher objects intended to be used to match obj_data           *
*         structures in the virtual object list                           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: obj_matcher.cc                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
// Copyright 2002 by John Rothe, all rights reserved.
//
#include "structs.h"
#include "specs.h"
#include "comm.h"
#include "materials.h"
#include "obj_matcher.h"
#include "interpreter.h"

//
// TYPE
//
bool ObjectTypeMatcher::init(char_data *ch, Tokenizer &tokens ) {
	char arg[256];
	if (! tokens.hasNext() ) {
		send_to_char(ch, "Show objects of what type?\r\n");
		return false;
	}
	while( tokens.next(arg) ) {
		if( arg[0] == '&' )
			break;
		if (!is_number(arg)) {
			type = search_block(arg, item_types, 0);
		} else {
			type = atoi(arg);
		}
		if ( type < 0 || type > NUM_ITEM_TYPES ) {
			send_to_char(ch, "Type olc help otypes for a valid list of types.\r\n");
			return false;
		}
	}
	setReady(true);
	return true;
}
bool ObjectTypeMatcher::isMatch( obj_data *obj ) {
    return ( isReady() && GET_OBJ_TYPE(obj) == type );
}

//
// MATERIAL
//
bool ObjectMaterialMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Show objects of what material?\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        if (!is_number(arg)) {
            material = search_block(arg, material_names, 0);
        } else {
            material = atoi(arg);
        }
        if ( material < 0 || material > TOP_MATERIAL ) {
            send_to_char(ch, "Type olc help material for a valid list.\r\n");
            return false;
        }
    }
    setReady(true);
    return true;
}

bool ObjectMaterialMatcher::isMatch( obj_data *obj ) {
    return isReady() && GET_OBJ_MATERIAL(obj) == material;
}

//
// APPLY
//
bool ObjectApplyMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Show objects with what apply?\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        if (!is_number(arg)) {
            apply = search_block(arg, apply_types, 0);
        } else {
            apply = atoi(arg);
        }
        if ( apply < 0 || apply > NUM_APPLIES ) {
            send_to_char(ch, "Type olc help apply for a valid list.\r\n");
            return false;
        }
    }
    setReady(true);
    return true;
}
bool ObjectApplyMatcher::isMatch( obj_data *obj ) {
    for (int k = 0; k < MAX_OBJ_AFFECT; k++) {
        if (obj->affected[k].location == apply) 
            return isReady();
    }
    return false;
}


//
// SPECIAL
//
bool ObjectSpecialMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Show objects with what special?\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        spec = find_spec_index_arg(arg);
        if ( spec < 0 ) {
            send_to_char(ch, "Type show specials for a valid list.\r\n");
            return false;
        }
    }
    setReady(true);
    return true;
}

bool ObjectSpecialMatcher::isMatch( obj_data *obj ) {
    return isReady() &&
           ( obj->shared->func && obj->shared->func == spec_list[spec].func );
}

//
// AFFECT
//
bool ObjectAffectMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Show objects with what affect?\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        if( (!(index = 1) || (affect = search_block(arg, affected_bits_desc,  0)) < 0)
        &&  (!(index = 2) || (affect = search_block(arg, affected2_bits_desc, 0)) < 0)
        &&  (!(index = 3) || (affect = search_block(arg, affected3_bits_desc, 0)) < 0)) 
        {
            send_to_char(ch, "There is no affect '%s'.\r\n", arg);
            return false;
        }
    }
    setReady(true);
    return true;
}

bool ObjectAffectMatcher::isMatch( obj_data *obj ) {
    return isReady() &&
           IS_SET(obj->obj_flags.bitvector[index - 1], (1 << affect));
}

//
// COST
//
bool ObjectCostMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Usage: 'show objects cost < amount'\r\n");
        send_to_char(ch, "Usage: 'show objects cost > amount'\r\n");
        send_to_char(ch, "Usage: 'show objects cost > small amount < large amount'\r\n");
        send_to_char(ch, "Usage: 'show objects cost amount'\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        if( arg[0] == '<' && tokens.hasNext() ) {
            tokens.next(arg);
            costBelow = atoi(arg);
        } else if( arg[0] == '>' && tokens.hasNext() ) {
            tokens.next(arg);
            costAbove = atoi(arg);
        } else if (!is_number(arg)) {
            send_to_char(ch, "That's not a valid cost: %s\r\n", arg);
            return false;
        } else {
            costAbove = atoi(arg);
        }
    }
    setReady(true);
    return true;
}

bool ObjectCostMatcher::isMatch( obj_data *obj ) {
    return isReady() && 
           ( GET_OBJ_COST(obj) > costAbove && 
             GET_OBJ_COST(obj) < costBelow );
}


//
// SPELL
//
bool ObjectSpellMatcher::init( char_data *ch, Tokenizer &tokens ) {
    char arg[256];
    char spellName[256];
    spellName[0] = '\0';
    if (! tokens.hasNext() ) {
        send_to_char(ch, "Show objects with what spell?\r\n");
        return false;
    }
    while( tokens.next(arg) ) {
        if( arg[0] == '&' )
            break;
        if( spellName[0] != '\0' )
            strncat( spellName, " ", 256 ); 
        strncat( spellName, arg, 256 );
    }
    if (is_number(spellName)) {
        spell = atoi(spellName);
    } else {
        spell = search_block(spellName, spells, 0);
    }
    if (spell < 0 || spell > TOP_SPELL_DEFINE) {
        if( spellName[0] == '\'' ) {
            send_to_char(ch, "Thou shalt not quote thy spell name.\r\n");
        } else { 
            send_to_char(ch, "What kinda spell is '%s'?\r\n",spellName);
        }
        return false;
    }
    setReady(true);
    return true;
}


bool ObjectSpellMatcher::isMatch( obj_data *obj ) {
    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_WAND:	// val 3 
        case ITEM_STAFF:	// val 3
            if (GET_OBJ_VAL(obj, 3) != spell)
                return false;
            break;
        case ITEM_WEAPON:	// val 0
            if (GET_OBJ_VAL(obj, 0) != spell)
                return false;
            break;
        case ITEM_SCROLL:	// val 1,2,3
        case ITEM_POTION:	// val 1,2,3
        case ITEM_PILL:	// ""
            if (GET_OBJ_VAL(obj, 1) != spell && GET_OBJ_VAL(obj, 2) != spell
                && GET_OBJ_VAL(obj, 3) != spell)
                return false;
            break;
        case ITEM_FOOD:	// Val 2 is spell
            if (GET_OBJ_VAL(obj, 2) != spell)
                return false;
            break;
        default:
            return false;
    }
    return isReady() ;
}

//
// WORN
//
bool ObjectWornMatcher::init( char_data *ch, Tokenizer &tokens ) {
	char arg[256];
	if (! tokens.hasNext() ) {
		send_to_char(ch, "Show objects worn in what position?\r\n");
		return false;
	}
	while( tokens.next(arg) ) {
		if( arg[0] == '&' )
			break;
		if (!is_number(arg)) {
			worn = search_block(arg, wear_eqpos, 0);
		} else {
			worn = atoi(arg);
		}
		if ( worn < 0 || worn > NUM_WEAR_FLAGS ) {
			send_to_char(ch, "That's not a valid wear position: %s\r\n", arg);
			return false;
		}
	}
	setReady(true);
	return true;
}

bool ObjectWornMatcher::isMatch( obj_data *obj ) {
    return ( isReady() && CAN_WEAR(obj, wear_bitvectors[worn]) );
}


//
// EXTRA
//
bool ObjectExtraMatcher::init( char_data *ch, Tokenizer &tokens ) {
	char arg[256];
	if (! tokens.hasNext() ) {
		send_to_char(ch, "Usage: 'show objects extra < [!] flag> ...'\r\n");
        send_to_char(ch, "  Any number of flags may be listed.\r\n");
        send_to_char(ch, "  Prefixing a flag with '!' will find objs without it.\r\n");
		return false;
	}
	while( tokens.next(arg) ) {
		if( arg[0] == '&' )
			break;
        if( arg[0] == '!' && strlen(arg) > 1 ) {
            if( addNoExtra( ch, (arg + 1) ) ) {
                continue;
            } else {
                return false;
            }
        } else {
            if( addExtra( ch, arg ) ) {
                continue;
            } else {
                return false;
            }
        }
	}
	setReady(true);
	return true;
}


bool ObjectExtraMatcher::addExtra( char_data *ch, char *arg ) {
    int i =  search_block(arg, extra_names, 0);
    if( i > 0 )  {
        extra |= (1 << i);
        return true;
    }
    i =  search_block(arg, extra2_names, 0);
    if( i > 0 )  {
        extra2 |= (1 << i);
        return true;
    }
    i =  search_block(arg, extra3_names, 0);
    if( i > 0 )  {
        extra3 |= (1 << i);
        return true;
    }

    send_to_char(ch, "There is no extra '%s'.\r\n", arg);
    return false;
}

bool ObjectExtraMatcher::addNoExtra( char_data *ch, char *arg ) {
    int i =  search_block(arg, extra_names, 0);
    if( i > 0 )  {
        noextra |= (1 << i);
        return true;
    }
    i =  search_block(arg, extra2_names, 0);
    if( i > 0 )  {
        noextra2 |= (1 << i);
        return true;
    }
    i =  search_block(arg, extra3_names, 0);
    if( i > 0 )  {
        noextra3 |= (1 << i);
        return true;
    }

    send_to_char(ch, "There is no extra '%s'.\r\n", arg);
    return false;
}


bool ObjectExtraMatcher::isMatch( obj_data *obj ) {
	if( extra != 0 && (GET_OBJ_EXTRA(obj) & extra) != extra ) {
		return false;
	}
	if( extra2 != 0 && (GET_OBJ_EXTRA2(obj) & extra2) != extra2 ) {
		return false;
	}
	if( extra3 != 0 && (GET_OBJ_EXTRA3(obj) & extra3) != extra3 ) {
		return false;
	}
    if( noextra != 0 && (GET_OBJ_EXTRA(obj) & noextra) != 0 ) {
		return false;
	}
	if( noextra2 != 0 && (GET_OBJ_EXTRA2(obj) & noextra2) != 0 ) {
		return false;
	}
	if( noextra3 != 0 && (GET_OBJ_EXTRA3(obj) & noextra3) != 0 ) {
		return false;
	}

	return isReady();
}

