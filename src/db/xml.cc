#include <dirent.h>
#include <string.h>
#include "constants.h"
#include "utils.h"
#include "xml_utils.h"
#include "db.h"
#include "vendor.h"
#include "tmpstr.h"
#include "comm.h"

void load_xml_object(xmlNodePtr node);
void load_xml_mobile(xmlNodePtr node);
void load_xml_zone(xmlNodePtr node);
void load_xml_room(xmlNodePtr node);

// Needed from db.cc
extern int top_of_objt;
extern int top_of_mobt;
extern player_special_data dummy_mob;
void set_physical_attribs(struct Creature *ch);

void
xml_boot(void)
{
	DIR *dir;
	dirent *file;
	xmlDocPtr doc;
	xmlNodePtr node;
	char path[256];
	int file_count = 0;

	dir = opendir(XML_PREFIX);
	if (!dir) {
		slog("SYSERR: XML directory does not exist");
		return;
	}

	while ((file = readdir(dir)) != NULL) {
		if (!rindex(file->d_name, '.'))
			continue;
		if (strcmp(rindex(file->d_name, '.'), ".xml"))
			continue;

		snprintf(path, 255, "%s/%s", XML_PREFIX, file->d_name);
		doc = xmlParseFile(path);
		if (!doc) {
			slog("SYSERR: XML parse error while loading %s", path);
			continue;
		}

		node = xmlDocGetRootElement(doc);
		if (!node) {
			xmlFreeDoc(doc);
			slog("SYSERR: XML file %s is empty", path);
			continue;
		}

		while (node) {
			// Parse different nodes here.
			if (xmlMatches(node->name, "craftshop"))
				load_craft_shop(node);
			else if (xmlMatches(node->name, "object"))
				load_xml_object(node);
			else if (xmlMatches(node->name, "mobile"))
				load_xml_mobile(node);
			else if (xmlMatches(node->name, "zone"))
				load_xml_zone(node);
			else if (xmlMatches(node->name, "room"))
				load_xml_room(node);
			else
				slog("SYSERR: Invalid XML object '%s' in %s", node->name, path);
			node = node->next;
		}

		xmlFreeDoc(doc);
	file_count++;
	}
	closedir(dir);

	if (file_count)
		slog("%d xml file%s loaded", file_count, (file_count == 1) ? "":"s");
	else
		slog("No xml files loaded");
}



void
xml_reload( Creature *ch = NULL )
{
	DIR *dir;
	dirent *file;
	xmlDocPtr doc;
	xmlNodePtr node;
	char path[256];
	int file_count = 0;

    if( ch != NULL ) {
        mudlog( GET_INVIS_LVL(ch), NRM, false,
                "%s Reloading XML data files.", 
                GET_NAME(ch) );
    } else {
        slog("Reloading XML data files.");
    }

	dir = opendir(XML_PREFIX);
	if (!dir) {
		slog("SYSERR: XML directory does not exist");
		return;
	}

	while ((file = readdir(dir)) != NULL) {
        bool found = false;

		if (!rindex(file->d_name, '.'))
			continue;
		if (strcmp(rindex(file->d_name, '.'), ".xml"))
			continue;

		snprintf(path, 255, "%s/%s", XML_PREFIX, file->d_name);
		doc = xmlParseFile(path);
		if (!doc) {
			slog("SYSERR: XML parse error while loading %s", path);
			continue;
		}

		node = xmlDocGetRootElement(doc);
		if (!node) {
			xmlFreeDoc(doc);
			slog("SYSERR: XML file %s is empty", path);
			continue;
		}

		while (node) {
			// Parse different nodes here.
			if (xmlMatches(node->name, "craftshop")) {
				load_craft_shop(node);
                found = true;
            }
			node = node->next;
		}

		xmlFreeDoc(doc);
        if( found )
            file_count++;
	}
	closedir(dir);

    char *msg;
	if (file_count)
		msg = tmp_sprintf("%d xml file%s reloaded", file_count, (file_count == 1) ? "":"s");
	else
		msg = tmp_sprintf("No xml files reloaded");

    slog(msg);

    if( ch != NULL ) {
        msg = tmp_strcat(msg,"\r\n");
        send_to_char( ch, msg );
    }
}


void
load_xml_object(xmlNodePtr node)
{
	struct obj_data *obj, *tmp_obj;

	CREATE(obj, struct obj_data, 1);
	obj->clear();
	CREATE(obj->shared, struct obj_shared_data, 1);

	obj->shared->vnum = xmlGetIntProp(node, "vnum");
	obj->shared->number = 0;
	obj->shared->house_count = 0;
	obj->shared->func = 0;
	obj->shared->proto = obj;
	obj->shared->cost = xmlGetIntProp(node, "cost");
	obj->shared->cost_per_day = xmlGetIntProp(node, "rent");

/*	obj->obj_flags.type_flag = xmlGetEnumProp(node, "type", item_types);

	obj->obj_flags.material = xmlGetEnumProp(node, "material", materials);
*/	obj->obj_flags.max_dam = xmlGetIntProp(node, "maxdamage");
	obj->obj_flags.damage = xmlGetIntProp(node, "damage");

	obj->setWeight(xmlGetIntProp(node, "weight"));
	obj->in_room = 0;

	obj->name = xmlGetProp(node, "name");

/*
	for (cur_node = node->xmlChildrenNode; cur_node; cur_node = cur_node->next) {
		if (!xmlStrcmp(cur_node->name, (const xmlChar *)"aliases"))
			obj->name = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"linedesc"))
			obj->description = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"action"))
			obj->action_desc = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"extradesc")) {
			CREATE(new_desc, struct extra_descr_data, 1);
			new_desc->keyword = xmlGetProp(cur_node, "keyword");
			new_desc->description = xmlNodeListGetString(doc, cur_node, 1);
			new_desc->next = obj->ex_description;
			obj->ex_description = new_desc;
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"apply")) {
			obj->affected[xmlGetEnumProp(cur_node, "attr", applies)] =
				xmlGetIntProp(cur_node, "value");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"value")) {
			obj->value[xmlGetIntProp(cur_node, "num")] =
				xmlGetIntProp(cur_node, "value");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"objflags")) {
			obj->obj_flags.extra_flags = xmlGetFlags(cur_node, extra1_enum);
			obj->obj_flags.extra2_flags = xmlGetFlags(cur_node, extra2_enum);
			obj->obj_flags.extra3_flags = xmlGetFlags(cur_node, extra3_enum);
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"wearpos")) {
			obj->obj_flags.wear_flags = xmlGetFlags(cur_node, wearpos_enum);
		} else {
			slog("SYSERR: Invalid xml node in <object>");
		}
	}
*/

	top_of_objt++;
	obj->next = NULL;
	if (obj_proto) {
		tmp_obj = obj_proto;
		while (tmp_obj->next)
			tmp_obj = tmp_obj->next;
		tmp_obj->next = obj;
	} else {
		obj_proto = obj;
	}
}

void
load_xml_mobile(xmlNodePtr node)
{
	struct Creature *mob;

	// Initialize structures
	mob = new Creature(false);
	CREATE(mob->mob_specials.shared, struct mob_shared_data, 1);

	// Read in initial properties
	mob->player.short_descr = xmlGetProp(node, "name");
	MOB_SHARED(mob)->vnum = xmlGetIntProp(node, "vnum");
	MOB_SHARED(mob)->lair = xmlGetIntProp(node, "lair", -1);
	MOB_SHARED(mob)->leader = xmlGetIntProp(node, "leader", -1);

	mob->player_specials = &dummy_mob;

/*
	// Read in the subnodes
	for (cur_node = node->xmlChildrenNode; cur_node; cur_node = cur_node->next) {
		if (!xmlStrcmp(cur_node->name, (const xmlChar *)"aliases"))
			mob->player.name = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"linedesc"))
			mob->player.description = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"movedesc"))
			MOB_SHARED(mob)->move_buf = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"description"))
			mob->player.long_descr = xmlNodeListGetString(doc, cur_node, 1);
		else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"attr")) {
			if (!sscanf(xmlGetProp(cur_node, "strength"), "%d/%d",
					&mob->real_abils.str, &mob->real_abils.str_add))
				mob->real_abils.str = xmlGetIntProp(cur_node, "strength", 11);
			mob->real_abils.intel =
				xmlGetIntProp(cur_node, "intelligence", 11);
			mob->real_abils.wis = xmlGetIntProp(cur_node, "wisdom", 11);
			mob->real_abils.dex = xmlGetIntProp(cur_node, "dexterity", 11);
			mob->real_abils.con = xmlGetIntProp(cur_node, "constitution", 11);
			mob->real_abils.cha = xmlGetIntProp(cur_node, "charisma", 11);
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"points")) {
			mob->points.hit = xmlGetIntProp(cur_node, "hit");
			mob->points.mana = xmlGetIntProp(cur_node, "mana");
			mob->points.move = xmlGetIntProp(cur_node, "move");
			mob->points.max_hit = xmlGetIntProp(cur_node, "maxhit");
			mob->points.max_mana = xmlGetIntProp(cur_node, "maxmana");
			mob->points.max_move = xmlGetIntProp(cur_node, "maxmove");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"stats")) {
			GET_LEVEL(mob) = xmlGetIntProp(node, "level");
			GET_SEX(mob) = xmlGetEnumProp(node, "sex", sex_enum);
			mob->player.char_class =
				xmlGetIntProp(cur_node, "class", CLASS_NORMAL);
			mob->player.race = xmlGetIntProp(cur_node, "race", RACE_MOBILE);
			mob->player.remort_char_class =
				xmlGetIntProp(cur_node, "remort", -1);
			mob->player.weight = xmlGetIntProp(cur_node, "weight", 200);
			mob->player.height = xmlGetIntProp(cur_node, "height", 198);
			GET_ALIGNMENT(mob) = xmlGetIntProp(node, "align");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"money")) {
			GET_EXP(mob) = xmlGetIntProp(cur_node, "exp");
			GET_GOLD(mob) = xmlGetIntProp(cur_node, "gold");
			GET_CASH(mob) = xmlGetIntProp(cur_node, "cash");
			GET_CREDITS(mob) = xmlGetIntProp(cur_node, "econet");
			GET_BANK(mob) = xmlGetIntProp(cur_node, "bank");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"pos")) {
			mob->mob_specials.shared->pos = xmlGetIntProp(cur_node, "current");
			mob->mob_specials.shared->default_pos =
				xmlGetIntProp(cur_node, "default");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"combat")) {
			sscanf(xmlGetProp(cur_node, "damage"),
				"%dd%d+%d",
				&mob->mob_specials.shared->damnodice,
				&mob->mob_specials.shared->damsizedice,
				&mob->mob_specials.shared->damroll);
			mob->points.damroll = xmlGetIntProp(cur_node, "damroll");
			mob->points.hitroll = xmlGetIntProp(cur_node, "hitroll");
			mob->points.armor = xmlGetIntProp(cur_node, "armor");
			MOB_SHARED(mob)->attack_type = xmlGetIntProp(cur_node, "attack");
			MOB_SHARED(mob)->morale = xmlGetIntProp(cur_node, "morale");
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"affects")) {
			AFF_FLAGS(mob) = xmlGetFlags(node, aff1_enum);
			AFF2_FLAGS(mob) = xmlGetFlags(node, aff2_enum);
			AFF3_FLAGS(mob) = xmlGetFlags(node, aff3_enum);
		} else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"actions")) {
			MOB_FLAGS(mob) = xmlGetFlags(node, action1_enum);
			MOB2_FLAGS(mob) = xmlGetFlags(node, action2_enum);
		} else {
			slog("SYSERR: Invalid xml node in <mobile>");
		}
	}

*/

	// Now set constant and data-dependent variables
	MOB_SHARED(mob)->proto = mob;

	mob->player.title = NULL;
	GET_COND(mob, 0) = GET_COND(mob, 1) = GET_COND(mob, 2) = -1;
	REMOVE_BIT(MOB2_FLAGS(mob), MOB2_RENAMED);
	SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
	if (!mob->mob_specials.shared->morale)
		mob->mob_specials.shared->morale =
			MOB_FLAGGED(mob, MOB_WIMPY) ? MAX(30, GET_LEVEL(mob)) : 100;
	set_physical_attribs(mob);
	mob->aff_abils = mob->real_abils;

	mobilePrototypes.add(mob);
	top_of_mobt++;
}

void
load_xml_room(xmlNodePtr node)
{
	int vnum;
	room_data *room;
	zone_data *zone;

	vnum = xmlGetIntProp(node, "vnum");
	for (zone = zone_table; vnum > zone->top; zone = zone->next)
		if (!zone || vnum < (zone->number * 100)) {
			fprintf(stderr, "Room %d is outside of any zone.\n", vnum);
			safe_exit(1);
		}

	room = new room_data(vnum, zone);
	room->name = xmlGetProp(node, "name");
//	room->sector = xmlGetEnumProp(node, "sector", sector_enum);
	room->max_occupancy = xmlGetIntProp(node, "size");

/*
	for (cur_node = node->xmlChildrenNode; cur_node; cur_node = cur_node->next) {
		if (!xmlStrcmp(cur_node->name, "description")) {
			room->description = xmlNodeListGetString(doc, cur_node, 1);
		} else if (!xmlStrcmp(cur_node->name, "extradesc")) {
			CREATE(new_descr, struct extra_descr_data, 1);
			new_descr->keyword = xmlGetProp(cur_node, "keywords");
			new_descr->description = xmlNodeListGetString(doc, cur_node, 1);
			new_descr->next = NULL;

			// Add the extra description to the end of the room's linked list
			if (!room->ex_description)
				room->ex_description = new_descr;
			else {
				t_exdesc = room->ex_description;
				while (t_exdesc->next)
					t_exdesc = t_exdesc->next;
				t_exdesc->next = new_descr;
			}
		} else if (!xmlStrcmp(cur_node->name, "exit")) {
			dir = xmlGetEnumProp(cur_node, "dir", direction_enum);
			CREATE(room->dir_option[dir], struct room_direction_data, 1);
			room->dir_option[dir]->keyword = xmlGetProp(cur_node, "keywords");
			room->dir_option[dir]->key = xmlGetIntProp(cur_node, "key");
			room->dir_option[dir]->to_room = xmlGetIntProp(cur_node, "dest");
			for (sub_node = cur_node->xmlChildrenNode; sub_node;
				sub_node = sub_node->next) {
				if (!xmlStrcmp(sub_node->name, "desc")) {
					room->dir_option[dir]->general_description =
						xmlNodeListGetString(doc, sub_node, 1);
				} else if (!xmlStrcmp(sub_node->name, "doorflags")) {
					room->dir_option[dir]->exitinfo =
						xmlGetFlags(sub_node, door_enum);
				} else {
					slog("SYSERR: Invalid xml node in <exit>");
				}
			}
		} else if (!xmlStrcmp(cur_node->name, "flow")) {
			FLOW_DIR(room) = xmlGetIntProp(cur_node, "dir");
			FLOW_SPEED(room) = xmlGetIntProp(cur_node, "speed");
			FLOW_TYPE(room) = xmlGetIntProp(cur_node, "type");
		} else if (!xmlStrcmp(cur_node->name, "sound")) {
			room->sounds = xmlNodeListGetString(doc, cur_node, 1);
		} else if (!xmlStrcmp(cur_node->name, "search")) {
			CREATE(new_search, struct special_search_data, 1);
			new_search->command_keys = xmlGetProp(cur_node, "cmd");
			new_search->keywords = xmlGetProp(cur_node, "keywords");
			new_search->command = xmlGetProp(cur_node, "type");
			new_search->arg[0] = xmlGetProp(cur_node, "arg0");
			new_search->arg[1] = xmlGetProp(cur_node, "arg1");
			new_search->arg[2] = xmlGetProp(cur_node, "arg2");
			for (sub_node = cur_node->xmlChildrenNode; sub_node;
				sub_node = sub_node->next) {
				if (!xmlStrcmp(sub_node->name, "searchflags")) {
					new_search->flags = xmlGetFlags(sub_node, search_enum);
				} else if (!xmlStrcmp(sub_node->name, "tovict")) {
					new_search->to_vict =
						xmlNodeListGetString(doc, sub_node, 1);
				} else if (!xmlStrcmp(sub_node->name, "toroom")) {
					new_search->to_room =
						xmlNodeListGetString(doc, sub_node, 1);
				} else if (!xmlStrcmp(sub_node->name, "toremote")) {
					new_search->to_remote =
						xmlNodeListGetString(doc, sub_node, 1);
				} else {
					slog("SYSERR: Invalid xml node in <search>");
				}
			}

			// Add the search to the end of the room's linked list
			new_search->next = NULL;
			if (!room->search)
				room->search = new_search;
			else {
				t_srch = room->search;
				while (t_srch->next)
					t_srch = t_srch->next;
				t_srch->next = new_search;
			}
		} else {
			slog("SYSERR: Invalid xml node in <room>");
		}
	}

*/
	if (IS_SET(room->room_flags, ROOM_TUNNEL) && room->max_occupancy > 5) {
		room->max_occupancy = 2;
		REMOVE_BIT(room->room_flags, ROOM_TUNNEL);
	}
}

void
load_xml_zone(xmlNodePtr node)
{
}
