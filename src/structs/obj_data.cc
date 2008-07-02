#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "utils.h"
#include "handler.h"
#include "db.h"
#include "comm.h"

extern int no_plrtext;

struct extra_descr_data *locate_exdesc(char *word,
    struct extra_descr_data *list, int exact = 0);

/**
 * Stores this object and it's contents into the given file.
 */
void obj_data::clear() 
{
	memset((char *)this, 0, sizeof(struct obj_data));
	in_room = NULL;
	worn_on = -1;
}

const char*
get_worn_type( obj_data *obj ) 
{
    if (obj->worn_on == -1 || !obj->worn_by)
        return "none";
    else if( GET_EQ(obj->worn_by, obj->worn_on) == obj )
        return "equipped";
    else if( GET_IMPLANT(obj->worn_by, obj->worn_on) == obj )
        return "implanted";
    else if (GET_TATTOO(obj->worn_by, obj->worn_on) == obj)
        return "tattooed";

	return "unknown";
}


void
obj_data::saveToXML(FILE *ouf)
{
    struct tmp_obj_affect *af = NULL; 
    struct tmp_obj_affect *af_head = NULL;
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
			if (desc->keyword && desc->description)
				fprintf(ouf, "%s<extra_desc keywords=\"%s\">%s</extra_desc>\n", 
                        indent.c_str(),  xmlEncodeSpecialTmp(desc->keyword), 
                        xmlEncodeTmp(desc->description) );
	}

    s = action_desc;
    if( s != NULL && 
        ( proto == NULL || proto->action_desc == NULL || 
          strcmp(s, proto->action_desc) ) )
    {
        fprintf(ouf, "%s<action_desc>%s</action_desc>\n", indent.c_str(), 
                xmlEncodeTmp(tmp_gsub(tmp_gsub(s, "\r\n", "\n"), "\r", "")));
    }

    // Detach the list of temp affects from the object and remove them
    // without deleting them
    af_head = this->tmp_affects;
    for (af = af_head; af; af = af->next)
        this->affectModify(af, false);

	fprintf( ouf, "%s<points type=\"%d\" soilage=\"%d\" weight=\"%d\" material=\"%d\" timer=\"%d\"/>\n",
			  indent.c_str(), obj_flags.type_flag, soilage, 
			 getObjWeight(), obj_flags.material, obj_flags.timer );
	fprintf(ouf, "%s<tracking id=\"%ld\" method=\"%d\" creator=\"%ld\" time=\"%ld\"/>\n",
		indent.c_str(), unique_id, creation_method, creator, creation_time);
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
    // Write the temp affects out to the file
    for (af = af_head; af; af = af->next) {
        fprintf(ouf, "%s<tmpaffect level=\"%d\" type=\"%d\" duration=\"%d\" "
                "dam_mod=\"%d\" maxdam_mod=\"%d\" val_mod1=\"%d\" "
                "val_mod2=\"%d\" val_mod3=\"%d\" val_mod4=\"%d\" "
                "type_mod=\"%d\" old_type=\"%d\" worn_mod=\"%d\" "
                "extra_mod=\"%d\" extra_index=\"%d\" weight_mod=\"%d\" ", 
                indent.c_str(), af->level, af->type, af->duration,
                af->dam_mod, af->maxdam_mod, af->val_mod[0], af->val_mod[1],
                af->val_mod[2], af->val_mod[3], af->type_mod, af->old_type,
                af->worn_mod, af->extra_mod, af->extra_index, af->weight_mod);

        for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
            fprintf(ouf, "affect_loc%d=\"%d\" affect_mod%d=\"%d\" ",
                    i, af->affect_loc[i], i, af->affect_mod[i]);
        }

        fprintf(ouf, "/>\n");
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

    // Ok, we'll restore the affects to the object right here
    this->tmp_affects = af_head;
    for (af = af_head; af; af = af->next)
        this->affectModify(af, true);
    
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
	
    // Commenting out the code below may be a horrible idea, but I need
    // this function to handle corpses.

	// NOTE: This is bad, but since the object is already allocated, we have
	// to do it this way.  Functionality is copied from read_object(int)
    if (vnum > -1) {
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
    }
    else
        shared = null_obj_shared;

	for( xmlNodePtr cur = node->xmlChildrenNode; cur; cur = cur->next) {
		if(xmlMatches(cur->name, "name")) {
			str = (char *)xmlNodeGetContent(cur);
			name = strdup(tmp_gsub(str, "\n", "\r\n"));
			free(str);
		} else if(xmlMatches(cur->name, "aliases")) {
			aliases = (char*)xmlNodeGetContent(cur);
		} else if (xmlMatches(cur->name, "line_desc")) {
			str = (char *)xmlNodeGetContent(cur);
			line_desc = strdup(tmp_gsub(tmp_gsub(str, "\r", ""), "\n", "\r\n"));
			free(str);
		} else if (xmlMatches(cur->name, "extra_desc")) {
			struct extra_descr_data *desc;
			char *keyword;

			if (shared->proto
					&& ex_description == shared->proto->ex_description)
				ex_description = exdesc_list_dup(shared->proto->ex_description);
			
			keyword = xmlGetProp(cur, "keywords");
			desc = locate_exdesc(fname(keyword), ex_description, 1);
			if (!desc) {
				desc = new extra_descr_data;
				desc->keyword = keyword;
				desc->description = (char *)xmlNodeGetContent(cur);
			} else {
                free(keyword);
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
		} else if( xmlMatches( cur->name, "tracking" ) ) {
			unique_id = xmlGetIntProp( cur, "id" );
			creation_method = xmlGetIntProp( cur, "method" );
			creator = xmlGetIntProp( cur, "creator" );
			creation_time = xmlGetIntProp( cur, "time" );
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
					equip_char( victim, this, position, EQUIP_WORN );
				} else if( strcmp(type,"implanted") == 0 ) {
					equip_char( victim, this, position, EQUIP_IMPLANT );
				} else if( strcmp(type,"tattooed") == 0 ) {
					equip_char( victim, this, position, EQUIP_TATTOO );
				} else if (container) {
					obj_to_obj(this, container, false);
				} else if (victim) {
					obj_to_char(this, victim, false);
				} else if (room) {
					obj_to_room(this, room, false);
				} else {
					errlog("Don't know where to put object!");
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
		} else if (xmlMatches(cur->name, "tmpaffect")) {
            struct tmp_obj_affect af;
            char *prop;

            memset(&af, 0x0, sizeof(af));
            af.level = xmlGetIntProp(cur, "level");
            af.type = xmlGetIntProp(cur, "type");
            af.duration = xmlGetIntProp(cur, "duration");
            af.dam_mod = xmlGetIntProp(cur, "dam_mod");
            af.maxdam_mod = xmlGetIntProp(cur, "maxdam_mod");
            af.val_mod[0] = xmlGetIntProp(cur, "val_mod1");
            af.val_mod[1] = xmlGetIntProp(cur, "val_mod2");
            af.val_mod[2] = xmlGetIntProp(cur, "val_mod3");
            af.val_mod[3] = xmlGetIntProp(cur, "val_mod4");
            af.type_mod = xmlGetIntProp(cur, "type_mod");
            af.old_type = xmlGetIntProp(cur, "old_type");
            af.worn_mod = xmlGetIntProp(cur, "worn_mod");
            af.extra_mod = xmlGetIntProp(cur, "extra_mod");
            af.extra_index = xmlGetIntProp(cur, "extra_index");
            af.weight_mod = xmlGetIntProp(cur, "weight_mod");
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                prop = tmp_sprintf("affect_loc%d", i);
                af.affect_loc[i] = xmlGetIntProp(cur, prop);
                prop = tmp_sprintf("affect_mod%d", i);
                af.affect_mod[i] = xmlGetIntProp(cur, prop);
            }

            this->addAffect(&af);
        }

	}

    this->normalizeApplies();

	if (!OBJ_APPROVED(this)) {
		slog("Unapproved object %d being junked from %s's rent.", 
			 vnum, (victim && GET_NAME(victim)) ? GET_NAME(victim):"(none)");
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

	if (IS_OBJ_STAT(this, ITEM_NORENT)
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

room_data *
obj_data::find_room(void)
{
	if (worn_by)
		return worn_by->in_room;
	else if (carried_by)
		return carried_by->in_room;
	else if (in_obj)
		return in_obj->find_room();
	else if (in_room)
		return in_room;
	
	errlog("Object in limbo at %s:%d", __FILE__, __LINE__);
	return NULL;
}

void
obj_data::addAffect(struct tmp_obj_affect *af)
{
    struct tmp_obj_affect *new_aff;
    
    CREATE(new_aff, struct tmp_obj_affect, 1);

    memcpy(new_aff, af, sizeof(struct tmp_obj_affect));
    new_aff->next = this->tmp_affects;
    this->tmp_affects = new_aff;

    affectModify(new_aff, true);
}

void
obj_data::removeAffect(struct tmp_obj_affect *af)
{
    struct tmp_obj_affect *curr_aff;
    struct tmp_obj_affect *prev_aff = NULL;
    bool found = true;

    affectModify(af, false);

    curr_aff = this->tmp_affects;
    
    while(curr_aff != NULL) {
        found = false;
        if (curr_aff == af) {
            found = true;
            if (prev_aff != NULL)
                prev_aff->next = af->next;
            else
                this->tmp_affects = curr_aff->next;
            
            free(af);
            break;
        }
        else {
            prev_aff = curr_aff;
            curr_aff = curr_aff->next;
        }
    }

    if (!found) {
		errlog("Could not find matching temporary object affect to remove.");
    }
}

// NEVER call this function directly.  Use addAffect(), removeAffect(),
// or affectJoin() instead.
// add == true adds the affect, add == false deletes the affect
void
obj_data::affectModify(struct tmp_obj_affect *af, bool add)
{
    // Set or restore damage
    if (af->dam_mod && this->obj_flags.max_dam > 0) {
        if (add)
            this->obj_flags.damage += af->dam_mod;
        else
            this->obj_flags.damage -= af->dam_mod;
    }
    //Set or restore maxdam
    if (af->maxdam_mod && this->obj_flags.max_dam > 0) {
        if (add)
            this->obj_flags.max_dam += af->maxdam_mod;
        else
            this->obj_flags.max_dam -= af->maxdam_mod;
    }
    // Set or reset the value mods
    for (int i = 0; i < 4; i++) {
        if (af->val_mod[i] != 0) {
            if (add)
                this->obj_flags.value[i] += af->val_mod[i];
            else
                this->obj_flags.value[i] -= af->val_mod[i];
        }
    }
    // Set or restore type
    if (af->type_mod) {
        if (add) {
            af->old_type = this->obj_flags.type_flag;
            this->obj_flags.type_flag = af->type_mod;
        }
        else
            this->obj_flags.type_flag = af->old_type;
    }

    // Set or reset wear positions
    if (af->worn_mod) {
        if (add) {
            check_bits_32(this->obj_flags.wear_flags, &af->worn_mod);
            this->obj_flags.wear_flags |= af->worn_mod;
        }
        else
            this->obj_flags.wear_flags &= ~(af->worn_mod);
    }

    // set or reset extra flags
    if (af->extra_mod && af->extra_index) {
        int *oextra;
        if (af->extra_index == 1) {
            oextra = &this->obj_flags.extra_flags;
        }
        else if (af->extra_index == 2) {
            oextra = &this->obj_flags.extra2_flags;
        }
        else if (af->extra_index == 3) {
            oextra = &this->obj_flags.extra3_flags;
        }
        else {
			errlog("Invalid extra index (%d) in obj_data::affectModify().",
				af->extra_index);
            return;
        }

        if (add) {
            check_bits_32(*oextra, &af->extra_mod);
            *oextra |= af->extra_mod;
        }
        else
            *oextra &= ~(af->extra_mod);
    }

    // Set or reset weight
    if (af->weight_mod) {
        if (add)
            this->setWeight(this->getWeight() + af->weight_mod);
        else
            this->setWeight(this->getWeight() - af->weight_mod);
    }

    // Set or reset affections
    // I probably could have done this with less code but it would have been
    // much more difficult to understand
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (af->affect_mod[i] > 125)
            af->affect_mod[i] = 125;
        else if (af->affect_mod[i] < -125)
            af->affect_mod[i] = -125;

        if (af->affect_loc[i] != APPLY_NONE) {
            bool found = false;
            for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
                if (this->affected[j].location == af->affect_loc[i]) {
                    found = true;
                    if (add)
                        this->affected[j].modifier += af->affect_mod[i];
                    else
                        this->affected[j].modifier -= af->affect_mod[i];
                    break;
                }

                if (found) {
                    if (this->affected[j].modifier == 0) {
                        this->affected[j].location = APPLY_NONE;
                    }
                }
            }
            
            if (!found) {
                for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
                    if (this->affected[j].location == APPLY_NONE) {
                        found = true;
                        this->affected[j].location = af->affect_loc[i];
                        this->affected[j].modifier = af->affect_mod[i];
                        break;
                    }
                }
            }

            this->normalizeApplies();
            
            if (!found)
				errlog("No affect locations trying to alter object affect on obj vnum %d, id %ld", GET_OBJ_VNUM(this), unique_id);
        }
    }
}

void
obj_data::affectJoin(struct tmp_obj_affect *af, int dur_mode, int val_mode,
                     int aff_mode)
{
    struct tmp_obj_affect *cur_aff = this->tmp_affects;
    struct tmp_obj_affect tmp_aff;
    int j;
    bool found = false;

    for (; cur_aff != NULL; cur_aff = cur_aff->next) {
        if ((cur_aff->type == af->type) && 
            (cur_aff->extra_index == af->extra_index)) {
            memcpy(&tmp_aff, cur_aff, sizeof(struct tmp_obj_affect));
            if (dur_mode == AFF_ADD)
                tmp_aff.duration = MIN(666, af->duration + tmp_aff.duration);
            else if (dur_mode == AFF_AVG)
                tmp_aff.duration = (af->duration + tmp_aff.duration) / 2;

            for (int i = 0; i < 4; i++) {
                if (af->val_mod[i] != 0) {
                    if (val_mode == AFF_ADD)
                        tmp_aff.val_mod[i] += af->val_mod[i];
                    else if (val_mode == AFF_AVG)
                        tmp_aff.val_mod[i] = (af->val_mod[i] + 
                                               tmp_aff.val_mod[i]) / 2;
                }
            }

            // The next section for affects is really a cluster fuck
            // but i can't think of a better way to handle it
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (af->affect_loc[i] != APPLY_NONE) {
                    // Try to find a matching affect
                    for (int k = 0; k < MAX_OBJ_AFFECT; k++) {
                        if (af->affect_loc[i] == tmp_aff.affect_loc[k]) {
                            if (aff_mode == AFF_ADD)
                                tmp_aff.affect_mod[k] += af->affect_mod[i];
                            else if (aff_mode == AFF_AVG)
                                tmp_aff.affect_mod[i] = (tmp_aff.affect_mod[k] +
                                                         af->affect_mod[i]) / 2;
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                    // We didn't find a matching affect so just add it
                    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
                        if (tmp_aff.affect_loc[j] == APPLY_NONE) {
                            if (aff_mode != AFF_NOOP) {
                                tmp_aff.affect_loc[j] = af->affect_loc[i];
                                tmp_aff.affect_mod[j] = af->affect_mod[i];
                                break;
                            }
                        }
                    }

                    if (j == MAX_OBJ_AFFECT) {
                        errlog("Could not find free affect position in affectJoin().");
                    }
                }
            }

            if (af->weight_mod) {
                if (val_mode == AFF_ADD)
                    tmp_aff.weight_mod += af->weight_mod;
                else if (val_mode == AFF_AVG)
                    tmp_aff.weight_mod = (tmp_aff.weight_mod + af->weight_mod) / 2;
            }
            if (af->dam_mod) {
                if (val_mode == AFF_ADD)
                    tmp_aff.dam_mod += af->dam_mod;
                else if (val_mode == AFF_AVG)
                    tmp_aff.dam_mod = (tmp_aff.dam_mod + af->dam_mod) / 2;
            }

            if (af->maxdam_mod) {
                if (val_mode == AFF_ADD)
                    tmp_aff.maxdam_mod += af->maxdam_mod;
                else if (val_mode == AFF_AVG)
                    tmp_aff.maxdam_mod = (tmp_aff.maxdam_mod + af->maxdam_mod) / 2;
            }

            removeAffect(cur_aff);
            addAffect(&tmp_aff);
            return;
        }
    }

    this->addAffect(af);
}

void
obj_data::normalizeApplies(void)
{
    int i,j;

    for (i = 0, j = 0;i < MAX_OBJ_AFFECT;i++,j++) {
        while (j < MAX_OBJ_AFFECT
               && (this->affected[j].location == APPLY_NONE
                   || this->affected[j].modifier == 0))
            j++;
        if (j < MAX_OBJ_AFFECT) {
            this->affected[i] = this->affected[j];
        } else {
            this->affected[i].location = APPLY_NONE;
            this->affected[i].modifier = 0;
        }
            
    }
}

struct tmp_obj_affect *
obj_data::affectedBySpell(int spellnum)
{
    struct tmp_obj_affect *cur_aff = this->tmp_affects;

    for (; cur_aff != NULL; cur_aff = cur_aff->next) {
        if (cur_aff->type == spellnum)
            return cur_aff; 
    }
 
    return NULL;
}

int
obj_data::getEquipPos(void)
{
	int result;

	if (!worn_by)
		return -1;

	for (result = 0;result < NUM_WEARS;result++)
		if (GET_EQ(worn_by, result) == this)
			return result;
	
	return -1;
}

int
obj_data::getImplantPos(void)
{
	int result;

	if (!worn_by)
		return -1;

	for (result = 0;result < NUM_WEARS;result++)
		if (GET_IMPLANT(worn_by, result) == this)
			return result;
	
	return -1;
}
#undef __obj_data_cc__
