#define __obj_data_cc__

#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "db.h"
#include "comm.h"

extern int no_plrtext;

struct extra_descr_data *locate_exdesc(char *word,
    struct extra_descr_data *list);

/**
 * Stores this object and it's contents into the given file.
 */
void obj_data::clear() 
{
	memset((char *)this, 0, sizeof(struct obj_data));
	in_room = NULL;
	worn_on = -1;
}

char*
get_worn_type( obj_data *obj ) 
{
	if( obj->worn_on == -1 ) {
		return "none";
	} else if( obj->worn_by != NULL ) {
		if( GET_EQ(obj->worn_by, obj->worn_on) == obj ) {
			return "equipped";
		} else if( GET_IMPLANT(obj->worn_by, obj->worn_on) == obj ) {
			return "implanted";
		}
	} 
	return "unknown";
}


void
obj_data::saveToXML(FILE *ouf)
{
	static string indent = "\t";
	fprintf( ouf, "%s<object vnum=\"%d\">\n", 
			indent.c_str(), shared->vnum );
	indent += "\t";

    obj_data *proto = shared->proto;


    char *s = name;
    if( s != NULL && 
        ( proto == NULL || 
          proto->name == NULL || 
          strcmp(s, proto->name) ) )
    {
        fprintf( ouf, "%s<name>%s</name>\n",
                 indent.c_str(), xmlEncodeTmp(s) );
    }

    s = aliases;
    if( s != NULL && 
        ( proto == NULL ||   proto->aliases == NULL ||  strcmp(s, proto->aliases) ) )
    {
        fprintf( ouf, "%s<aliases>%s</aliases>\n",  indent.c_str(), xmlEncodeTmp(s) );
    }

    s = line_desc;
    if( s != NULL && 
        ( proto == NULL ||  proto->line_desc == NULL ||  strcmp(s, proto->line_desc ) ) )
    {
        fprintf( ouf, "%s<line_desc>%s</line_desc>\n", indent.c_str(),  xmlEncodeTmp(s) );
    }

	if (!proto || ex_description != proto->ex_description) {
		extra_descr_data *desc;

		for (desc = ex_description;desc;desc = desc->next)
			fprintf(ouf, "%s<extra_desc keywords=\"%s\">%s</extra_desc>\n", indent.c_str(),  xmlEncodeTmp(desc->keyword), xmlEncodeTmp(desc->description) );
	}

    s = action_desc;
    if( s != NULL && 
        ( proto == NULL || proto->action_desc == NULL || 
          strcmp(s, proto->action_desc) ) )
    {
        fprintf( ouf, "%s<action_desc>%s</action_desc>\n", indent.c_str(), xmlEncodeTmp(s));
    }


	fprintf( ouf, "%s<points type=\"%d\" soilage=\"%d\" weight=\"%d\" material=\"%d\" timer=\"%d\"/>\n",
			  indent.c_str(), obj_flags.type_flag, soilage, 
			 getObjWeight(), obj_flags.material, obj_flags.timer );
	fprintf( ouf, "%s<damage current=\"%d\" max=\"%d\" sigil_id=\"%d\" sigil_level=\"%d\" />\n",
			 indent.c_str(), obj_flags.damage, obj_flags.max_dam, 
			obj_flags.sigil_idnum, obj_flags.sigil_level );
	fprintf( ouf, "%s<flags extra=\"%x\" extra2=\"%x\" extra3=\"%x\" />\n",
			 indent.c_str(), obj_flags.extra_flags, 
			obj_flags.extra2_flags, obj_flags.extra3_flags );
	fprintf( ouf, "%s<values v0=\"%d\" v1=\"%d\" v2=\"%d\" v3=\"%d\" />\n",
			 indent.c_str(), obj_flags.value[0],obj_flags.value[1],
			obj_flags.value[2],obj_flags.value[3] );

	fprintf( ouf, "%s<affectbits aff1=\"%lx\" aff2=\"%lx\" aff3=\"%lx\" />\n",
			 indent.c_str(), obj_flags.bitvector[0], 
			obj_flags.bitvector[1], obj_flags.bitvector[2] );

	for( int i = 0; i < MAX_OBJ_AFFECT; i++ ) {
		if( affected[i].location > 0) {
			fprintf( ouf, "%s<affect modifier=\"%d\" location=\"%d\" />\n",
					  indent.c_str(), affected[i].modifier, affected[i].location );
		}
	}
	// Contained objects
	indent += "\t";
	for( obj_data *obj = contains; obj != NULL; obj = obj->next_content ) {
		obj->saveToXML(ouf);
	}
	indent.erase(indent.size() - 1);
	// Intentionally done last since reading this property in loadFromXML 
    // causes the eq to be worn on the character.
	fprintf( ouf, "%s<worn possible=\"%x\" pos=\"%d\" type=\"%s\"/>\n", 
			 indent.c_str(), obj_flags.wear_flags, worn_on, get_worn_type(this) );


	indent.erase(indent.size() - 1);
	fprintf( ouf, "%s</object>\n", indent.c_str() );
	return;
}

bool
obj_data::loadFromXML(obj_data *container, Creature *victim, room_data* room, xmlNodePtr node)
{
	int vnum = xmlGetIntProp(node, "vnum");
	bool placed;
	char *str;

	placed = false;
	
	if( vnum < 0 ) {
		slog("obj_data->loadFromXML found vnum %d in %s's file. Junking.",
			  vnum, victim ? GET_NAME(victim):"(null)");
		return false;
	}

	// NOTE: This is bad, but since the object is already allocated, we have
	// to do it this way.  Functionality is copied from read_object(int)
	obj_data* prototype = real_object_proto(vnum);
	if(!prototype) {
		slog("Object #%d being removed from %s's rent",
			vnum, victim ? GET_NAME(victim):"(null)");
		return false;
	}

	shared = prototype->shared;
	shared->number++;

	name = shared->proto->name;
	aliases = shared->proto->aliases;
	line_desc = shared->proto->line_desc;
	action_desc  = shared->proto->action_desc;
	ex_description = shared->proto->ex_description;

	for( xmlNodePtr cur = node->xmlChildrenNode; cur; cur = cur->next) {
		if(xmlMatches(cur->name, "name")) {
			str = (char *)xmlNodeGetContent(cur);
			name = strdup(tmp_gsub(str, "\n", "\r\n"));
			free(str);
		} else if(xmlMatches(cur->name, "aliases")) {
			aliases = (char*)xmlNodeGetContent(cur);
		} else if (xmlMatches(cur->name, "line_desc")) {
			str = (char *)xmlNodeGetContent(cur);
			line_desc = strdup(tmp_gsub(str, "\n", "\r\n"));
			free(str);
		} else if (xmlMatches(cur->name, "extra_desc")) {
			struct extra_descr_data *desc;
			char *keyword;

			if (ex_description == shared->proto->ex_description)
				ex_description = exdesc_list_dup(shared->proto->ex_description);
			
			keyword = xmlGetProp(cur, "keywords");
			desc = locate_exdesc(fname(keyword), ex_description);
			if (!desc) {
				desc = new extra_descr_data;
				desc->keyword = keyword;
				desc->description = (char *)xmlNodeGetContent(cur);
			} else {
				free(desc->description);
				desc->description = (char *)xmlNodeGetContent(cur);
			}

		} else if( xmlMatches( cur->name, "action_desc" ) ) {
			str = (char *)xmlNodeGetContent(cur);
			action_desc = strdup(tmp_gsub(str, "\n", "\r\n"));
			free(str);
		} else if( xmlMatches( cur->name, "points" ) ) {
			 obj_flags.type_flag = xmlGetIntProp( cur, "type");
			 soilage = xmlGetIntProp( cur, "soilage");
			 obj_flags.setWeight(xmlGetIntProp( cur, "weight"));
			 obj_flags.material = xmlGetIntProp( cur, "material");
			 obj_flags.timer = xmlGetIntProp( cur, "timer");
		} else if( xmlMatches( cur->name, "damage" ) ) {
			obj_flags.damage = xmlGetIntProp( cur, "current");
			obj_flags.max_dam = xmlGetIntProp( cur, "max");
			obj_flags.sigil_idnum = xmlGetIntProp( cur, "sigil_id");
			obj_flags.sigil_level = xmlGetIntProp( cur, "sigil_level");
		} else if( xmlMatches( cur->name, "flags" ) ) {
			char* flag = xmlGetProp(cur,"extra");
			obj_flags.extra_flags = hex2dec(flag);
			free(flag);
			flag = xmlGetProp(cur,"extra2");
			obj_flags.extra2_flags = hex2dec(flag);
			free(flag);
			flag = xmlGetProp(cur,"extra3");
			obj_flags.extra3_flags = hex2dec(flag);
			free(flag);
		} else if( xmlMatches( cur->name, "worn" ) ) {
			char* flag = xmlGetProp(cur,"possible");
			obj_flags.wear_flags = hex2dec(flag);
			free(flag);
			int position = xmlGetIntProp( cur, "pos" );

			char *type = xmlGetProp(cur, "type");
			if( type != NULL && victim != NULL ) {
				if( strcmp(type,"equipped") == 0 ) {
					equip_char( victim, this, position, MODE_EQ );
				} else if( strcmp(type,"implanted") == 0 ) {
					equip_char( victim, this, position, MODE_IMPLANT );
				} else if (container) {
					obj_to_obj(this, container, false);
				} else if (victim) {
					obj_to_char(this, victim, false);
				} else if (room) {
					obj_to_room(this, room, false);
				} else {
					slog("SYSERR: Don't know where to put object!");
					return false;
				}
				placed = true;
			}
			free(type);
		} else if( xmlMatches( cur->name, "values" ) ) {
			obj_flags.value[0] = xmlGetIntProp( cur, "v0" );
			obj_flags.value[1] = xmlGetIntProp( cur, "v1" );
			obj_flags.value[2] = xmlGetIntProp( cur, "v2" );
			obj_flags.value[3] = xmlGetIntProp( cur, "v3" );
		} else if( xmlMatches( cur->name, "affectbits" ) ) {
			char* aff = xmlGetProp(cur,"aff1");
			obj_flags.bitvector[0] = hex2dec(aff);
			free(aff);

			aff = xmlGetProp(cur,"aff2");
			obj_flags.bitvector[1] = hex2dec(aff);
			free(aff);

			aff = xmlGetProp(cur,"aff3");
			obj_flags.bitvector[2] = hex2dec(aff);
			free(aff);

		} else if( xmlMatches( cur->name, "affect" ) ) {
			for( int i = 0; i < MAX_OBJ_AFFECT; i++ ) {
				if( affected[i].location == 0 && affected[i].modifier == 0 ) {
					 affected[i].modifier = xmlGetIntProp( cur, "modifier");
					 affected[i].location = xmlGetIntProp( cur, "location");
					 break;
				}
			}
		} else if( xmlMatches( cur->name, "object" ) ) {
			obj_data *obj;
			obj = create_obj();
			if(! obj->loadFromXML(this,victim,room,cur) ) {
				extract_obj(obj);
			}
		} 
	}

	if (!OBJ_APPROVED(this)) {
		slog("Unapproved object %d being junked from %s's rent.", 
			 vnum, GET_NAME(victim) );
		return false;
	}
	if (!placed) {
		if(victim) {
			obj_to_char(this, victim, false);
		} else if(container) {
			obj_to_obj(this, container, false);
		} else if( room != NULL ) {
			obj_to_room(this, room, false);
		}
	}
	return true;
}

int
obj_data::modifyWeight(int mod_weight)
{

	// if object is inside another object, recursively
	// go up and modify it
	if (in_obj)
		in_obj->modifyWeight(mod_weight);

	else if (worn_by) {
		// implant, increase character weight
		if (GET_IMPLANT(worn_by, worn_on) == this)
			worn_by->modifyWeight(mod_weight);
		// simply worn, increase character worn weight
		else
			worn_by->modifyWornWeight(mod_weight);
	}

	else if (carried_by)
		carried_by->modifyCarriedWeight(mod_weight);

	return (obj_flags.setWeight(getWeight() + mod_weight));
}

bool
obj_data::isUnrentable()
{

	if (IS_OBJ_STAT(this, ITEM_NORENT) || GET_OBJ_RENT(this) < 0
		|| !OBJ_APPROVED(this) || GET_OBJ_VNUM(this) <= NOTHING
		|| (GET_OBJ_TYPE(this) == ITEM_KEY && GET_OBJ_VAL(this, 1) == 0)
		|| (GET_OBJ_TYPE(this) == ITEM_CIGARETTE && GET_OBJ_VAL(this, 3))) {
		return true;
	}

	if (no_plrtext && plrtext_len)
		return true;
	return false;
}

int
obj_data::setWeight(int new_weight)
{

	return (modifyWeight(new_weight - getWeight()));
}

int
obj_flag_data::setWeight(int new_weight)
{
	return ((weight = new_weight));
}

void
obj_data::display_rent(Creature *ch, const char *currency_str)
{
	send_to_char(ch, "%10d %s for %s\r\n", GET_OBJ_RENT(this), currency_str,
		name);
}


#undef __obj_data_cc__
