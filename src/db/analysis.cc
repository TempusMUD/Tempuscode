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


#include <slist>
#include <vector>
using namespace std;

#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "screen.h"
#include "tokenizer.h"
#include "obj_matcher.h"
#include "tmpstr.h"

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
            table.push_back(new ObjectCostMatcher());
			_ready = false;
		}

		~ObjectMatcherTable() {
			for( unsigned int i = 0; i < table.size(); i++  ) {
				delete table[i];
			}
		}
		char *getOptionList() {
            char *msg = tmp_sprintf("");
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( i > 0 ) msg = tmp_strcat( msg, ", ");
				msg = tmp_strcat(msg, table[i]->getKey());
			}
            return msg;
		}
		/** returns the number of matchers in the table. **/
		int size() {
			return (int) table.size();
		}
		/** returns true if all matchers initialized properly. **/
		bool init(char_data *ch, Tokenizer &tokens) {
			char arg[256];
			while( tokens.next(arg) ) {
                bool used = false;
				for( unsigned int i = 0; i < table.size(); i++  ) {
					if( table[i]->isKey(arg) ) {
                        used = true;
						if( table[i]->init(ch, tokens) ) 
							break;
					}
				}
                if(! used ) {
                    send_to_char(ch,"Invalid option: %s\r\n",arg);
                    send_to_char(ch, "Options are: %s\r\n",getOptionList());
                    return false;
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
char*
get_spell_names( char_data *ch, obj_data *obj ) 
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
	return tmp_sprintf("[%s%s%s,%s%s%s,%s%s%s]",
		CCCYN(ch, C_NRM),spell1,CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM),spell2,CCNRM(ch, C_NRM),
		CCCYN(ch, C_NRM),spell3,CCNRM(ch, C_NRM) );
}

/** 
 * prints a delightfull description of the given object into the
 * given buffer of the given size, using ch for it's color and
 * showing or not showing spell names.
**/
char*
sprintobj( char_data *ch, obj_data *obj, int num, bool show_spell=false ) 
{
	return tmp_sprintf("%3d. %s[%s%5d%s] %40s%s%s %s\r\n", num,
			     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_OBJ_VNUM(obj),
				 CCGRN(ch, C_NRM), obj->short_description, CCNRM(ch, C_NRM),
				 !OBJ_APPROVED(obj) ? " (!appr)" : "", 
                 show_spell ? get_spell_names( ch, obj ) : "" );
}

void
do_show_objects( char_data *ch, char *value, char *arg ) {
	ObjectMatcherTable matcherTable;
	list<obj_data*> objects;
	Tokenizer tokens(value);
	tokens.append(arg);
	if(! tokens.hasNext() ) {
		send_to_char(ch, "Show objects: utility to search the object prototype list.\r\n");
		send_to_char(ch, "Usage: show obj <option> <arg> [& <option> <arg> ...]\r\n");
		send_to_char(ch, "Options are: %s\r\n",matcherTable.getOptionList());
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

	int objNum = 1;
	char *msg = tmp_sprintf("Matched %d objects with: '%s%s'\r\n",
                             objects.size(),value,arg);

	list<obj_data*>::iterator it = objects.begin();
	for( ; it != objects.end() && objNum <= 300; ++it ) {
        char *line = sprintobj( ch, *it, objNum++, matcherTable.isUsed("spell") );
		msg = tmp_strcat( msg, line);
	}
	if( objNum > 300 )
		msg = tmp_strcat( msg, "**** List truncated to 300 objects. ****\r\n");

	page_string(ch->desc, msg);
}
