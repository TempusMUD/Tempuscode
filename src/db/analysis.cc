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


#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <list>
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
#include "object_map.h"

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
            char *msg = tmp_strdup("");
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if( i > 0 )
                    msg = tmp_strcat( msg, ", ", NULL );
				msg = tmp_strcat(msg, table[i]->getKey(),NULL);
			}
            return msg;
		}
		/** returns the number of matchers in the table. **/
		int size() {
			return (int) table.size();
		}
		/** returns true if all matchers initialized properly. **/
		bool init(Creature *ch, Tokenizer &tokens) {
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
		const char* getAddedInfo( Creature *ch, obj_data *obj ) {
			const char* info = "";
			for( unsigned int i = 0; i < table.size(); i++  ) {
				if(! table[i]->isReady() )
					continue;
				const char* line = table[i]->getAddedInfo(ch,obj);
				if( *line != '\0' )
					info = tmp_strcat(info, line);
			}
			return info;
		}
};

/** 
 * prints a delightfull description of the given object into the
 * given buffer of the given size, using ch for it's color and
 * showing or not showing spell names.
**/
char*
sprintobj( Creature *ch, obj_data *obj, ObjectMatcherTable &table, int num ) 
{
	const char *info = table.getAddedInfo( ch, obj );
	return tmp_sprintf("%3d. %s[%s%5d%s] %35s%s %s%s\r\n", num,
					   CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_OBJ_VNUM(obj),
					   CCGRN(ch, C_NRM), obj->name, CCNRM(ch, C_NRM),
					   info, !OBJ_APPROVED(obj) ? "(!appr)" : "");
}

void
do_show_objects( Creature *ch, char *value, char *arg ) {
	ObjectMatcherTable matcherTable;
	list<obj_data*> objects;
	Tokenizer tokens(value);
    obj_data *obj = NULL;

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
	
//	for (obj_data *obj = obj_proto; obj != NULL ; obj = obj->next) {
    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;    
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
	char *msg = tmp_sprintf("Matched %zd objects with: '%s%s'\r\n",
                             objects.size(),value,arg);

	list<obj_data*>::iterator it = objects.begin();
	for( ; it != objects.end() && objNum <= 300; ++it ) {
        char *line = sprintobj( ch, *it, matcherTable, objNum++);
		msg = tmp_strcat( msg, line, NULL);
	}
	if( objNum > 300 )
		msg = tmp_strcat( msg, "**** List truncated to 300 objects. ****\r\n", NULL);

	page_string(ch->desc, msg);
}
