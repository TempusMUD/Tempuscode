#define __obj_data_cc__

#include "structs.h"
#include "utils.h"
#include "handler.h"

extern int no_plrtext;
struct obj_data *real_object_proto(int vnum);

/**
 * Stores this object and it's contents into the given file.
 */
int
obj_data::save(FILE * fl)
{
	int j, i;
	struct obj_data *tmpo;
	struct obj_file_elem object;

	object.item_number = GET_OBJ_VNUM(this);

	object.short_desc[0] = 0;
	object.name[0] = 0;

	if (shared->proto) {
		if (name != shared->proto->name)
			strncpy(object.name, name, EXDSCR_LENGTH - 1);
		if (short_description != shared->proto->short_description)
			strncpy(object.short_desc, short_description, EXDSCR_LENGTH - 1);
	}

	object.in_room_vnum = in_room ? in_room->number : -1;
	object.wear_flags = obj_flags.wear_flags;
	object.type = GET_OBJ_TYPE(this);
	object.damage = GET_OBJ_DAM(this);
	object.max_dam = obj_flags.max_dam;
	object.material = obj_flags.material;
	object.plrtext_len = plrtext_len;
	object.sparebyte1 = 0;
	object.sigil_level = GET_OBJ_SIGIL_LEVEL(this);
	object.soilage = this->soilage;
	object.sigil_idnum = GET_OBJ_SIGIL_IDNUM(this);
	object.spareint4 = 0;
	object.value[0] = GET_OBJ_VAL(this, 0);
	object.value[1] = GET_OBJ_VAL(this, 1);
	object.value[2] = GET_OBJ_VAL(this, 2);
	object.value[3] = GET_OBJ_VAL(this, 3);
	object.bitvector[0] = obj_flags.bitvector[0];
	object.bitvector[1] = obj_flags.bitvector[1];
	object.bitvector[2] = obj_flags.bitvector[2];
	object.extra_flags = GET_OBJ_EXTRA(this);
	object.extra2_flags = GET_OBJ_EXTRA2(this);
	object.extra3_flags = GET_OBJ_EXTRA3(this);
	object.weight = getWeight();
	object.timer = GET_OBJ_TIMER(this);
	object.worn_on_position = this->worn_on;

	if (no_plrtext)
		object.plrtext_len = 0;

	for (j = 0; j < 3; j++)
		object.bitvector[j] = this->obj_flags.bitvector[j];

	for (j = 0; j < MAX_OBJ_AFFECT; j++)
		object.affected[j] = this->affected[j];

	for (tmpo = contains, i = 0; tmpo; tmpo = tmpo->next_content, i++);
	object.contains = i;

	if (fwrite(&object, sizeof(obj_file_elem), 1, fl) < 1) {
		perror("Error writing object in obj_data::save()");
		return 0;
	}
	if (object.plrtext_len && !fwrite(action_description, plrtext_len, 1, fl)) {
		perror("Error writing player text in obj file.");
		return 0;
	}

	for (tmpo = contains; tmpo; tmpo = tmpo->next_content)
		tmpo->save(fl);

	return 1;
}

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


    char *s = short_description;
    if( s != NULL && 
        ( proto == NULL || 
          proto->short_description == NULL || 
          strcmp(s, proto->short_description) ) )
    {
        fprintf( ouf, "%s<short_desc>%s</short_desc>\n",
                 indent.c_str(), xmlEncodeTmp(s) );
    }

    s = name;
    if( s != NULL && 
        ( proto == NULL ||   proto->name == NULL ||  strcmp(s, proto->name) ) )
    {
        fprintf( ouf, "%s<name>%s</name>\n",  indent.c_str(), xmlEncodeTmp(s) );
    }

    s = description;
    if( s != NULL && 
        ( proto == NULL ||   proto->description == NULL ||  strcmp(s, proto->description) ) )
    {
        fprintf( ouf, "%s<long_desc>%s</long_desc>\n", indent.c_str(),  xmlEncodeTmp(s) );
    }


    s = action_description;
    if( s != NULL && 
        ( proto == NULL || proto->action_description == NULL || 
          strcmp(s, proto->action_description) ) )
    {
        fprintf( ouf, "%s<action_desc>%s</action_desc>\n", indent.c_str(), xmlEncodeTmp(s));
    }


	fprintf( ouf, "%s<points type=\"%d\" soilage=\"%d\" weight=\"%d\" material=\"%d\" timer=\"%d\"/>\n",
			  indent.c_str(), obj_flags.type_flag, soilage, 
			 obj_flags.getWeight(), obj_flags.material, obj_flags.timer );
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
		if( affected[i].location > 0 && affected[i].modifier > 0 ) {
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
obj_data::loadFromXML(obj_data *container, Creature *victim, xmlNodePtr node)
{

	clear();
	int vnum = xmlGetIntProp(node, "vnum");
	
	if( vnum < 0 ) {
		slog("obj_data->loadFromXML found vnum %d in %s's file. Junking.",
			  vnum, GET_NAME(victim) );
		return false;
	}

	obj_data* prototype = real_object_proto( vnum );
	if( prototype == NULL ) {
		slog("Object %d no longer in database.", vnum );
		return false;
	} 

	shared = prototype->shared;

	short_description = shared->proto->short_description;
	name = shared->proto->name;
	description = shared->proto->description;
	action_description  = shared->proto->action_description;

	for( xmlNodePtr cur = node->xmlChildrenNode; cur; cur = cur->next) {
		if( xmlMatches( cur->name, "name" ) ) {
			name = (char*)xmlNodeGetContent( cur );
		} else if( xmlMatches( cur->name, "short_desc" ) ) {
			short_description = (char*)xmlNodeGetContent( cur );
		} else if( xmlMatches( cur->name, "long_desc" ) ) {
			description = (char*)xmlNodeGetContent( cur );
		} else if( xmlMatches( cur->name, "action_desc" ) ) {
			action_description = (char*)xmlNodeGetContent( cur );
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
				} else if( victim != NULL && container == NULL ) {
					obj_to_char(this,victim);
				}
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
			CREATE(obj, struct obj_data, 1);
			obj->clear();
			obj->loadFromXML(this,victim,cur);
		} 
	}
	if (!OBJ_APPROVED(this)) {
		slog("Unapproved object %d being junked from %s's rent.", 
			 vnum, GET_NAME(victim) );
		return false;
	}

	if( container ) {
		obj_to_obj( this, container );
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

#undef __obj_data_cc__
