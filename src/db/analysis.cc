/* ************************************************************************
*   File: analysis.cc                               NOT Part of CircleMUD *
*  Usage: Analysis routines for the DB.( Objects, Mobiles etc. )          *
*         Mostly searching at this point.                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: analysis.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
// Copyright 2002 by John Rothe, all rights reserved.
//


#include <iostream>
#include <fstream>
#include <slist>
#include <string.h>
using namespace std;

#include "structs.h"
#include "utils.h"
#include "character_list.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "security.h"
#include "olc.h"
#include "materials.h"
#include "clan.h"
#include "specs.h"
#include "flow_room.h"
#include "smokes.h"
#include "paths.h"
#include "login.h"
#include "bomb.h"
#include "guns.h"
#include "elevators.h"
#include "fight.h"
#include "defs.h"
#include "tokenizer.h"

class ObjectMatcher {
	public:
		ObjectMatcher( const char *key ) { 
			this->_ready = false;
			this->key = key; 
		}
		virtual ~ObjectMatcher() { }
		bool isKey( const char *key ) { 
			return (strncmp( this->key.c_str(), key, strlen(key) ) == 0 );
		}
		virtual bool init( char_data *ch, Tokenizer &tokens ) { 
			return false; 
		}
		virtual bool isMatch( obj_data *obj ) { 
			return false;
		}
		bool isReady() { 
			return _ready; 
		}
		void setReady(bool ready) { 
			_ready = ready; 
		}
		const char* getKey() { return key.c_str(); }
	private:
		bool _ready;
	private:
		string key;
};


class ObjectTypeMatcher : public ObjectMatcher {
	public:
		ObjectTypeMatcher() : ObjectMatcher::ObjectMatcher("type") { 
			type = -1;
		}
		virtual ~ObjectTypeMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			return ( isReady() && GET_OBJ_TYPE(obj) == type );
		}
	private:
		int type;
};

class ObjectMaterialMatcher : public ObjectMatcher {
	public:
		ObjectMaterialMatcher() : ObjectMatcher::ObjectMatcher("material") { 
			material = -1;
		}
		virtual ~ObjectMaterialMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			return isReady() && GET_OBJ_MATERIAL(obj) == material;
		}
	private:
		int material;
};

class ObjectApplyMatcher : public ObjectMatcher {
	public:
		ObjectApplyMatcher() : ObjectMatcher::ObjectMatcher("apply") { 
			apply = -1;
		}
		virtual ~ObjectApplyMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			for (int k = 0; k < MAX_OBJ_AFFECT; k++) {
				if (obj->affected[k].location == apply) 
					return isReady();
			}
			return false;
		}
	private:
		int apply;
};

class ObjectSpecialMatcher : public ObjectMatcher {
	public:
		ObjectSpecialMatcher() : ObjectMatcher::ObjectMatcher("special") { 
			spec = -1;
		}
		virtual ~ObjectSpecialMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			return isReady() &&
				   ( obj->shared->func && obj->shared->func == spec_list[spec].func );
		}
	private:
		int spec;
};

class ObjectAffectMatcher : public ObjectMatcher {
	public:
		ObjectAffectMatcher() : ObjectMatcher::ObjectMatcher("affect") { 
			index = -1;
			affect = -1;
		}
		virtual ~ObjectAffectMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			return isReady() &&
				   IS_SET(obj->obj_flags.bitvector[index - 1], (1 << affect));
		}
	private:
		int index;
		int affect;
};


class ObjectCostMatcher : public ObjectMatcher {
	public:
		ObjectCostMatcher() : ObjectMatcher::ObjectMatcher("cost") { 
			costAbove = INT_MIN;
			costBelow = INT_MAX;
		}
		virtual ~ObjectCostMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
			char arg[256];
			if (! tokens.hasNext() ) {
				send_to_char(ch, "Show objects with cost above what?\r\n");
				return false;
			}
			while( tokens.next(arg) ) {
				if( arg[0] == '&' )
					break;

				if (!is_number(arg)) {
					send_to_char(ch, "That's not a valid cost: %s\r\n", arg);
					return false;
				}
				costAbove = atoi(arg);
			}
			setReady(true);
			return true;
		}
		virtual bool isMatch( obj_data *obj ) {
			return isReady() && 
				   ( GET_OBJ_COST(obj) > costAbove && 
				     GET_OBJ_COST(obj) < costBelow );
		}
	private:
		int costBelow;
		int costAbove;
};


class ObjectSpellMatcher : public ObjectMatcher {
	public:
		ObjectSpellMatcher() : ObjectMatcher::ObjectMatcher("spell") { 
			spell = -1;
		}
		virtual ~ObjectSpellMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
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
	private:
		int spell;
};

class ObjectWornMatcher : public ObjectMatcher {
	public:
		ObjectWornMatcher() : ObjectMatcher::ObjectMatcher("worn") { 
			worn = INT_MAX;
		}
		virtual ~ObjectWornMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
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
		virtual bool isMatch( obj_data *obj ) {
			return ( isReady() && CAN_WEAR(obj, wear_bitvectors[worn]) );
		}
	private:
		int worn;
};

class ObjectExtraMatcher : public ObjectMatcher {
	public:
		ObjectExtraMatcher() : ObjectMatcher::ObjectMatcher("extra") { 
			extra = extra2 = extra3 = 0;
		}
		virtual ~ObjectExtraMatcher() { }
		virtual bool init( char_data *ch, Tokenizer &tokens ) {
			char arg[256];
			if (! tokens.hasNext() ) {
				send_to_char(ch, "Show objects with what extra flags?\r\n");
				return false;
			}
			while( tokens.next(arg) ) {
				if( arg[0] == '&' )
					break;
				int i =  search_block(arg, extra_names, 0);
				if( i > 0 )  {
					extra |= (1 << i);
					continue;
				}
				i =  search_block(arg, extra2_names, 0);
				if( i > 0 )  {
					extra2 |= (1 << i);
					continue;
				}
				i =  search_block(arg, extra3_names, 0);
				if( i > 0 )  {
					extra3 |= (1 << i);
					continue;
				}

				send_to_char(ch, "There is no extra '%s'.\r\n", arg);
				return false;
			}
			setReady(true);
			return true;
		}
		virtual bool isMatch( obj_data *obj ) {
			if( extra != 0 && (GET_OBJ_EXTRA(obj) & extra) != extra ) {
				return false;
			}
			if( extra2 != 0 && (GET_OBJ_EXTRA2(obj) & extra2) != extra2 ) {
				return false;
			}
			if( extra3 != 0 && (GET_OBJ_EXTRA3(obj) & extra3) != extra3 ) {
				return false;
			}
			return isReady() ;
		}
	private:
		int extra;
		int extra2;
		int extra3;
};



/**
 * A table of ObjectMatcher objects used to do matching searches
 * of virtual objects.
**/
class ObjectMatcherTable {
	private:
		vector<ObjectMatcher*> table;
		bool _ready;
	public:
		ObjectMatcherTable() : table() {
			table.push_back(new ObjectTypeMatcher());
			table.push_back(new ObjectApplyMatcher());
			table.push_back(new ObjectMaterialMatcher());
			table.push_back(new ObjectSpecialMatcher());
			table.push_back(new ObjectWornMatcher());
			table.push_back(new ObjectSpellMatcher());
			table.push_back(new ObjectExtraMatcher());
			table.push_back(new ObjectAffectMatcher());
			_ready = false;
		}

		~ObjectMatcherTable() {
			for( unsigned int i = 0; i < table.size(); i++  ) {
				delete table[i];
			}
		}
		void getOptionList(char *buf) {
			buf[0] = '\0';
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( i > 0 ) strcat( buf, ", ");
				strcat(buf, table[i]->getKey() );
			}
		}
		/** returns the number of matchers in the table. **/
		int size() {
			return (int) table.size();
		}
		/** returns true if all matchers initialized properly. **/
		bool init(char_data *ch, Tokenizer &tokens) {
			char arg[256];
			while( tokens.next(arg) ) {
				for( unsigned int i = 0; i < table.size(); i++  ) {
					if( table[i]->isKey(arg) ) {
						if( table[i]->init(ch, tokens) ) 
							break;
					}
				}
			}
			_ready = true;
			return true;
		}
		/** returns true if the object is matched by all matchers. **/
		bool isMatch( obj_data *obj ) {
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( table[i]->isReady() && !table[i]->isMatch(obj) ) {
					return false;
				} 
			}
			return true;
		}
		/** returns true if the given matcher is being used. **/
		bool isUsed( const char* matcherKey ) {
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( table[i]->isKey(matcherKey) )
					return table[i]->isReady();
			}
			return false;
		}
		bool isReady() {
			if(! _ready ) return false;
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( table[i]->isReady() ) {
					return true;
				}
			}
			return false;
		}
};


/** 
 * prints the names of the spells on the given object into
 * the given buffer with color for ch.
**/
void
get_spell_names( char_data *ch, char *buf, obj_data *obj ) 
{
	const char *spell1 = "0"; 
	const char *spell2 = "0";
	const char *spell3 = "0";

	switch (GET_OBJ_TYPE(obj)) {
		case ITEM_WAND:	// val 3 
		case ITEM_STAFF:	// val 3
			spell1 = spell_to_str(GET_OBJ_VAL(obj, 3));
			break;
		case ITEM_WEAPON:	// val 0
			spell1 = spell_to_str(GET_OBJ_VAL(obj, 0));
			break;
		case ITEM_SCROLL:	// val 1,2,3
		case ITEM_POTION:	// val 1,2,3
		case ITEM_PILL:	// ""
			spell1 = spell_to_str(GET_OBJ_VAL(obj, 1));
			spell2 = spell_to_str(GET_OBJ_VAL(obj, 2));
			spell3 = spell_to_str(GET_OBJ_VAL(obj, 3));
			break;
		case ITEM_FOOD:	// Val 2 is spell
			spell1 = spell_to_str(GET_OBJ_VAL(obj, 2));
			break;
	}
	sprintf(buf, "[%s%s%s,%s%s%s,%s%s%s]",
		CCCYN(ch, C_NRM),spell1,CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM),spell2,CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM),spell3,CCNRM(ch, C_NRM) );
}

/** 
 * prints a delightfull description of the given object into the
 * given buffer of the given size, using ch for it's color and
 * showing or not showing spell names.
**/
void
sprintobj( char *buf, int bufsize, char_data *ch, 
		   obj_data *obj, int num, bool show_spell=false ) 
{
	char spellnames[256];
	if( show_spell ) {
		get_spell_names( ch, spellnames, obj );
	} else {
		spellnames[0] = '\0';
	}

	snprintf(buf,bufsize, "%3d. %s[%s%5d%s] %40s%s%s %s\r\n", num,
			     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_OBJ_VNUM(obj),
				 CCGRN(ch, C_NRM), obj->short_description, CCNRM(ch, C_NRM),
				 !OBJ_APPROVED(obj) ? " (!appr)" : "", spellnames);
}

void
do_show_objects( char_data *ch, char *value, char *arg ) {
	ObjectMatcherTable matcherTable;
	list<obj_data*> objects;
	Tokenizer tokens(value);
	tokens.append(arg);
	if(! tokens.hasNext() ) {
		char options[matcherTable.size() * 80];
		send_to_char(ch, "Show objects: utility to search the object prototype list.\r\n");
		send_to_char(ch, "Usage: show obj <option> <arg> [& <option> <arg> ...]\r\n");
		matcherTable.getOptionList(options);
		send_to_char(ch, "Options are: %s\r\n",options);
		return;
	}
	if(! matcherTable.init(ch,tokens) || !matcherTable.isReady() ) {
		return;
	}
	
	for (obj_data *obj = obj_proto; obj != NULL ; obj = obj->next) {
		for( int i = 0; i < matcherTable.size(); i++  ) {
			if( matcherTable.isMatch(obj) ) {
				objects.push_back(obj);
				break;
			}
		}
	}
	if( objects.size() <= 0 ) {
		send_to_char(ch, "No objects match your search criteria.\r\n");
		return;
	}
	send_to_char(ch, "Objects that match(%d): %s %s\r\n",
					 objects.size(),value,arg);

	list<obj_data*>::iterator it = objects.begin();
	bool show_spells = matcherTable.isUsed("spell");
	int objNum = 1;
	char buf[MAX_STRING_LENGTH];
	buf[0] = '\0';
	for( ; it != objects.end(); it++ ) {
		char linebuf[256];
		sprintobj(linebuf, 255, ch,*it,objNum++,show_spells);
		strncat(buf,linebuf,MAX_STRING_LENGTH - 80);
	}
	if( strlen(buf) >= MAX_STRING_LENGTH - 128 )
		strncat( buf, "OVERFLOW\r\n", MAX_STRING_LENGTH - 80);
	page_string(ch->desc, buf);
}
