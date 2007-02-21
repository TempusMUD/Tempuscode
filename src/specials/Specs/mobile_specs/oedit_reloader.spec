#include <vector>

#include "actions.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "utils.h"
#include "object_map.h"

int retrieve_oedits( Creature *ch, list<obj_data*> &found );
int load_oedits( Creature *ch, list<obj_data*> &found );

SPECIAL(oedit_reloader)
{
	Creature *self = (Creature *)me;
	if(!cmd || ( !CMD_IS("help") && !CMD_IS("retrieve") ) ) {
		return false;
	}
	if (!can_see_creature(self, ch)) {
		perform_say(self, "say", "Who's there?  I can't see you.");
		return true;
	}
	
	if( CMD_IS("help") && !*argument ) {
		perform_say(self, "say", "If you want me to retrieve your property, just type 'retrieve'.");
	} else if (CMD_IS("retrieve")) {
		list<obj_data*> found;
		act("$n closes $s eyes in deep concentration.", TRUE, self, 0, FALSE, TO_ROOM);
		int retrieved = retrieve_oedits( ch, found );
		int existing = load_oedits( ch, found );

		if( existing == 0 ) {
            perform_say_to(self, ch, "You own nothing that can be retrieved.");
		} else if( retrieved == 0 && found.size() > 0 ) {
            perform_say_to(self, ch, "You already have all that could be retrieved.");
		} else {
            perform_say_to(self, ch, "That should be everything.");
		}
	} else {
		return false;
	}
	
	return true;
}

obj_data*
get_top_container( obj_data* obj )
{
	while( obj->in_obj ) {
		obj = obj->in_obj;
	}
	return obj;
}

bool
contains( list<obj_data*> &objects, int vnum )
{
	list<obj_data*>::iterator it;
	for( it = objects.begin(); it != objects.end(); ++it ) {
		if( (*it)->shared->vnum == vnum )
			return true;
	}
	return false;
}

int
load_oedits( Creature *ch, list<obj_data*> &found )
{
	int count = 0;
    obj_data *obj = NULL;
    ObjectMap::iterator oi = objectPrototypes.begin();
    for (; oi != objectPrototypes.end(); ++oi) {
        obj = oi->second;
		if( obj->shared->owner_id == GET_IDNUM(ch)  ) {
			++count;
			if( !contains( found, obj->shared->vnum ) ) { 
				obj_data* o = read_object( obj->shared->vnum );
				obj_to_char( o, ch, false );
				act("$p appears in your hands!", FALSE, ch, o, 0, TO_CHAR);
			}
		}
	}
	return count;
}

int
retrieve_oedits( Creature *ch, list<obj_data*> &found )
{	
	int count = 0;
	for( obj_data* obj = object_list; obj; obj = obj->next) {
		if( obj->shared->owner_id == GET_IDNUM(ch) ) {
			found.push_back(obj);
			// They already have it
			if( obj->worn_by == ch || obj->carried_by == ch ) {
				continue;
			}
			if( obj->in_obj ) {
				obj_data* tmp = get_top_container(obj);
				// They already have it
				if( tmp->carried_by == ch || tmp->worn_by == ch ){
					continue;
				}
			}
			// todo: check house
			if( obj->worn_by != NULL ) {
				act("$p disappears off of your body!", 
					FALSE, obj->worn_by, obj, 0, TO_CHAR);
				unequip_char(obj->worn_by, obj->worn_on, false);
			} else if( obj->carried_by != NULL ) {
				act("$p disappears out of your hands!", FALSE,
					obj->carried_by, obj, 0, TO_CHAR);
				obj_from_char( obj );
			} else if( obj->in_room != NULL ) {
				if( obj->in_room->people.size() > 0 )  {
					act("$p fades out of existence!", 
						FALSE, NULL, obj, NULL, TO_ROOM);
				}
				obj_from_room( obj );
			} else if( obj->in_obj != NULL ) {
				obj_from_obj( obj );
			}
			++count;
			obj_to_char( obj, ch, false );
			act("$p appears in your hands!", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
	return count;
}
