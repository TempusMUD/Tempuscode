#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include "constants.h"
#include "utils.h"
#include "xml_utils.h"
#include "db.h"
#include "shop.h"
#include "tmpstr.h"
#include "comm.h"
#include "creature.h"
#include "char_class.h"
#include "handler.h"



void obj_to_room(struct obj_data *object, struct room_data *room);
void add_alias(struct Creature *ch, struct alias_data *a);
void affect_to_char(struct Creature *ch, struct affected_type *af);
void extract_object_list(obj_data * head);
void Crash_calculate_rent(struct obj_data *obj, int *cost);

// Saves the given characters equipment to a file. Intended for use while 
// the character is still in the game. 
bool
Creature::crashSave()
{
    rent_info rent;
    rent.rentcode = RENT_CRASH;
	rent.time = time(0);
	rent.currency = in_room->zone->time_frame;

    if(! saveObjects(rent) ) {
        return false;
    }

    REMOVE_BIT(PLR_FLAGS(this), PLR_CRASH);
    //saveToXML(); // This should probably be here.
    return true;
}

bool
Creature::rentSave(int cost, int rentcode)//= RENT_RENTED
{
    rent_info rent;

    rent.net_cost_per_diem = cost;
	rent.rentcode = rentcode;
	rent.time = time(0);
	rent.gold = GET_GOLD(this);
	rent.account = GET_BANK_GOLD(this);
	rent.currency = in_room->zone->time_frame;

    extractUnrentables();

    if(! saveObjects(rent) ) {
        return false;
    }
    
    // THIS SHOULD NOT BE HERE. 
    // Extract should be called properly instead.
    // Left here temporarily to get the xml code working first. --jr 6-30-02
    for (int j = 0; j < NUM_WEARS; j++) {
		if( GET_EQ(this, j) )
            extract_object_list(GET_EQ(this, j));
        if( GET_IMPLANT(this, j) )
            extract_object_list(GET_IMPLANT(this, j));
    }

    extract_object_list(carrying);

    return true;
}

bool
Creature::idleSave()
{
    int cost = 0;
	if (GET_LEVEL(this) < LVL_AMBASSADOR) {
		Crash_calculate_rent(carrying, &cost);
	}
	// INCREASE
	return rentSave( (cost * 3), RENT_FORCED );
}

// Drops all !cursed eq to the floor, breaking implants, then calls rentSave(0)
// Used for 'quit yes'
bool
Creature::curseSave()
{
    for (int j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(this, j)) {
			if (IS_OBJ_STAT(GET_EQ(this, j), ITEM_NODROP) ||
				IS_OBJ_STAT2(GET_EQ(this, j), ITEM2_NOREMOVE)) {

				// the item is cursed, but its contents cannot be (normally)
				while (GET_EQ(this, j)->contains)
					extract_object_list(GET_EQ(this, j)->contains);
			}
		}
		if (GET_IMPLANT(this, j)) {
			// Break implants and put them on the floor
			obj_data *obj = unequip_char(this, j, MODE_IMPLANT);
			GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj) >> 3 - 1;
			SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_BROKEN);
			obj_to_room(obj, in_room);
		}

	}
    obj_data* obj;
    obj_data* next_obj;
	for( obj = carrying; obj; obj = next_obj ) {
		next_obj = obj->next_content;
		if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
			// the item is cursed, but its contents cannot be (normally)
			while (obj->contains)
				extract_object_list(obj->contains);
		}
	}


    if(! rentSave( 0 ) )
        return false;
    
    REMOVE_BIT(PLR_FLAGS(this), PLR_CRASH);
    return true;
}

bool
Creature::cryoSave(int cost)
{
    
    if (in_room->zone->time_frame == TIME_ELECTRO)
		GET_CASH(this) = MAX(0, GET_CASH(this) - cost);
	else
		GET_GOLD(this) = MAX(0, GET_GOLD(this) - cost);

    rent_info rent;
    rent.rentcode = RENT_CRYO;
	rent.time = time(0);
	rent.gold = GET_GOLD(this);
	rent.account = GET_BANK_GOLD(this);
	rent.net_cost_per_diem = 0;
	rent.currency = in_room->zone->time_frame;
    extractUnrentables();

    if(! saveObjects(rent) )
        return false;

	SET_BIT(PLR_FLAGS(this), PLR_CRYO);
    return true;
}

bool
Creature::saveObjects( const rent_info &rent ) 
{
	FILE *ouf;
	char *path;
	int idx;

    path = get_equipment_file_path(GET_IDNUM(this));
	ouf = fopen(path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML equipment file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return false;
	}
	fprintf( ouf, "<objects>\n" );
    rent.saveToXML( ouf );
	// Save the inventory
	for( obj_data *obj = carrying; obj != NULL; obj = obj->next_content ) {
		obj->saveToXML(ouf);
	}
	// Save the equipment
	for( idx = 0; idx < NUM_WEARS; idx++ ) {
		if( GET_EQ(this, idx) )
			(GET_EQ(this,idx))->saveToXML(ouf);
		if( GET_IMPLANT(this, idx) )
			(GET_IMPLANT(this,idx))->saveToXML(ouf);
	}
	fprintf( ouf, "</objects>\n" );
	fclose(ouf);
    return true;
}

/**
 * return values:
 * -1 - dangerous failure - don't allow char to enter
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple. (or noeq)
 *  2 - rented equipment lost ( no $ )
**/
int
Creature::loadObjects()
{

    char *path = get_equipment_file_path( GET_IDNUM(this) );
	int axs = access(path, W_OK);
	if( axs != 0 ) {
		if( axs != ENOENT ) {
			slog("SYSERR: Unable to open xml equipment file '%s': %s", 
				 path, strerror(axs) );
			return -1;
		} else {
			return 1; // normal no eq file
		}
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        slog("SYSERR: XML parse error while loading %s", path);
        return -1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        slog("SYSERR: XML file %s is empty", path);
        return 1;
    }

	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "object") ) {
			obj_data *obj;
			CREATE(obj, obj_data, 1);
			obj->clear();
			if(! obj->loadFromXML(NULL,this,node) ) {
				extract_obj(obj);
			}
			//obj_to_room(obj, in_room);
		}
	}
	return 0;
}


void
Creature::saveToXML() 
{
	// Save vital statistics
	FILE *ouf;
	char *path;
	struct alias_data *cur_alias;
	struct affected_type *cur_aff;
	int idx;
    Creature *ch = this;

    path = get_player_file_path(GET_IDNUM(ch));
	ouf = fopen(path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML player file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return;
	}

	fprintf(ouf, "<creature name=\"%s\" idnum=\"%ld\">\n",
		GET_NAME(ch), ch->char_specials.saved.idnum);

	fprintf(ouf, "<points hit=\"%d\" mana=\"%d\" move=\"%d\" maxhit=\"%d\" maxmana=\"%d\" maxmove=\"%d\"/>\n",
		ch->points.hit, ch->points.mana, ch->points.move,
		ch->points.max_hit, ch->points.max_mana, ch->points.max_move);

    fprintf(ouf, "<money gold=\"%d\" bank=\"%d\" cash=\"%d\" credits=\"%d\" xp=\"%d\"/>\n",
        ch->points.gold, ch->points.bank_gold, 
		ch->points.cash, ch->points.credits, 
		ch->points.exp);

	fprintf(ouf, "<stats level=\"%d\" sex=\"%s\" race=\"%s\" height=\"%d\" weight=\"%d\" align=\"%d\"/>\n",
		GET_LEVEL(ch), genders[GET_SEX(ch)], player_race[GET_RACE(ch)],
		GET_HEIGHT(ch), GET_WEIGHT(ch), GET_ALIGNMENT(ch));
	
	fprintf(ouf, "<class name=\"%s\"", pc_char_class_types[GET_CLASS(ch)]);
	if (GET_REMORT_GEN(ch))
		fprintf(ouf, " remort=\"%s\" gen=\"%d\"",
			pc_char_class_types[GET_REMORT_CLASS(ch)], GET_REMORT_GEN(ch));
	if (IS_CYBORG(ch)) {
		if (GET_OLD_CLASS(ch) != -1)
			fprintf(ouf, " subclass=\"%s\"",
				borg_subchar_class_names[GET_OLD_CLASS(ch)]);
		if (GET_TOT_DAM(ch))
			fprintf(ouf, " total_dam=\"%d\"", GET_TOT_DAM(ch));
		if (GET_BROKE(ch))
			fprintf(ouf, " broken=\"%d\"", GET_BROKE(ch));
	} else if (GET_CLASS(ch) == CLASS_MAGE &&
			GET_SKILL(ch, SPELL_MANA_SHIELD) > 0) {
		fprintf(ouf, " manash_low=\"%ld\" manash_pct=\"%ld\"",	
			ch->player_specials->saved.mana_shield_low,
			ch->player_specials->saved.mana_shield_pct);
	}
	fprintf(ouf, "/>\n");


	fprintf(ouf, "<time birth=\"%ld\" death=\"%ld\" played=\"%d\"/>\n",
		ch->player.time.birth,
		ch->player.time.death,
		ch->player.time.played);

	char *host = "";
	if( desc != NULL ) {
		host = xmlEncodeTmp( desc->host );
	}
	fprintf(ouf, "<lastlogin time=\"%ld\" host=\"%s\"/>\n",
		(long int)ch->player.time.logon, host );

	fprintf(ouf, "<carnage pkills=\"%d\" mkills=\"%d\" deaths=\"%d\"/>\n",
		GET_PKILLS(ch), GET_MOBKILLS(ch), GET_PC_DEATHS(ch));

	fprintf(ouf, "<attr str=\"%d\" int=\"%d\" wis=\"%d\" dex=\"%d\" con=\"%d\" cha=\"%d\" stradd=\"%d\"/>\n",
		GET_STR(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), GET_CON(ch),
		GET_CHA(ch), GET_ADD(ch));
    /*  Calculated from eq and affects.
	fprintf(ouf, "<save para=\"%d\" rod=\"%d\" petri=\"%d\" breath=\"%d\" spell=\"%d\" chem=\"%d\" psi=\"%d\" phy=\"%d\"/>\n",
		GET_SAVE(ch, SAVING_PARA), GET_SAVE(ch, SAVING_ROD),
		GET_SAVE(ch, SAVING_PETRI), GET_SAVE(ch, SAVING_BREATH),
		GET_SAVE(ch, SAVING_SPELL), GET_SAVE(ch, SAVING_CHEM),
		GET_SAVE(ch, SAVING_PSI), GET_SAVE(ch, SAVING_PHY));
    */
	fprintf(ouf, "<condition hunger=\"%d\" thirst=\"%d\" drunk=\"%d\"/>\n",
		ch->player_specials->saved.conditions[0],
		ch->player_specials->saved.conditions[1],
		ch->player_specials->saved.conditions[2]);

	fprintf(ouf, "<player invis=\"%d\" wimpy=\"%d\" pracs=\"%d\" lp=\"%d\" clan=\"%d\"/>\n",
		GET_INVIS_LVL(ch), GET_WIMP_LEV(ch), GET_PRACTICES(ch),
		GET_LIFE_POINTS(ch), GET_CLAN(ch));

	fprintf(ouf, "<home town=\"%d\" loadroom=\"%d\" held_town=\"%d\" held_loadroom=\"%d\"/>\n",
		GET_HOME(ch), GET_LOADROOM(ch), GET_HOLD_HOME(ch),
		GET_HOLD_LOADROOM(ch));

	fprintf(ouf, "<quest");
	if (GET_QUEST(ch))
		fprintf(ouf, " current=\"%d\"", GET_QUEST(ch));
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		fprintf(ouf, " allowance=\"%d\"", GET_QUEST_ALLOWANCE(ch));
    if( GET_QUEST_POINTS(ch) != 0 )
        fprintf(ouf, " points=\"%d\"", GET_QUEST_POINTS(ch));
    
	fprintf(ouf, "/>\n");
		
	fprintf(ouf, "<account flag1=\"%lx\" flag2=\"%x\" password=\"%s\" bad_pws=\"%d\"",
		ch->char_specials.saved.act, ch->player_specials->saved.plr2_bits,
		ch->player.passwd, ch->player_specials->saved.bad_pws);
	if (PLR_FLAGGED(ch, PLR_FROZEN))
		fprintf(ouf, " frozen_lvl=\"%d\"", GET_FREEZE_LEV(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<prefs flag1=\"%lx\" flag2=\"%lx\"/>\n",
		ch->player_specials->saved.pref, ch->player_specials->saved.pref2);

	fprintf(ouf, "<terminal rows=\"%d\" columns=\"%d\"/>\n",
		GET_PAGE_LENGTH(ch), GET_COLS(ch));

	fprintf(ouf, "<affects flag1=\"%lx\" flag2=\"%lx\" flag3=\"%lx\"/>\n",
		ch->char_specials.saved.affected_by,
		ch->char_specials.saved.affected2_by,
		ch->char_specials.saved.affected3_by);

	for (idx = 0;idx < MAX_WEAPON_SPEC;idx++) {
		if (ch->player_specials->saved.weap_spec[idx].level)
			fprintf(ouf, "<weaponspec vnum=\"%d\" level=\"%d\"/>\n",
				ch->player_specials->saved.weap_spec[idx].vnum,
				ch->player_specials->saved.weap_spec[idx].level);
	}

	if (GET_TITLE(ch) && *GET_TITLE(ch)) {
		fprintf(ouf, "<title>%s</title>\n", xmlEncodeTmp(GET_TITLE(ch)) );
	}

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		fprintf(ouf, "<immort badge=\"%d\"/>\n",
			ch->player_specials->saved.occupation);
		if (POOFIN(ch) && *POOFIN(ch))
			fprintf(ouf, "<poofin>%s</poofin>\n", xmlEncodeTmp(POOFIN(ch)));
		if (POOFOUT(ch) && *POOFOUT(ch))
			fprintf(ouf, "<poofout>%s</poofout>\n", xmlEncodeTmp(POOFOUT(ch)));
	}
	if (ch->player.description && *ch->player.description) {
		fprintf(ouf, "<description>%s</description>\n",  xmlEncodeTmp(ch->player.description) );
	}
	for (cur_alias = ch->player_specials->aliases; cur_alias; cur_alias = cur_alias->next)
		fprintf(ouf, "<alias type=\"%d\" alias=\"%s\" replace=\"%s\"/>\n",
			cur_alias->type, cur_alias->alias, cur_alias->replacement);
	for (cur_aff = ch->affected;cur_aff; cur_aff = cur_aff->next)
		fprintf(ouf, "<affect type=\"%d\" duration=\"%d\" modifier=\"%d\" location=\"%d\" level=\"%d\" instant=\"%s\" affbits=\"%lx\" index=\"%d\" />\n",
			cur_aff->type, cur_aff->duration, cur_aff->modifier,
			cur_aff->location, cur_aff->level,
			(cur_aff->is_instant) ? "yes":"no", 
			cur_aff->bitvector,
			cur_aff->aff_index );

	for (idx = 0;idx < MAX_SKILLS;idx++)
		if (ch->player_specials->saved.skills[idx] > 0)
			fprintf(ouf, "<skill name=\"%s\" level=\"%d\"/>\n",
				spell_to_str(idx), GET_SKILL(ch, idx));

	fprintf(ouf, "</creature>\n");
	fclose(ouf);
}

/* copy data from the file structure to a Creature */
bool
Creature::loadFromXML( long id )
{
    char *path = get_player_file_path( id );
	if( access(path, W_OK) ) {
		slog("SYSERR: Unable to open xml player file '%s': %s", path, strerror(errno) );
		return false;
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        slog("SYSERR: XML parse error while loading %s", path);
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        slog("SYSERR: XML file %s is empty", path);
        return false;
    }


    /* to save memory, only PC's -- not MOB's -- have player_specials */
	if (player_specials == NULL)
		CREATE(player_specials, struct player_special_data, 1);

    player.name = xmlGetProp(root, "name");
    char_specials.saved.idnum = xmlGetIntProp(root, "idnum");
    
    // Read in the subnodes
	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "points") ) {
            points.mana = xmlGetIntProp(node, "mana");
            points.max_mana = xmlGetIntProp(node, "maxmana");
            points.hit = xmlGetIntProp(node, "hit");
            points.max_hit = xmlGetIntProp(node, "maxhit");
            points.move = xmlGetIntProp(node, "move");
            points.max_move = xmlGetIntProp(node, "maxmove");
        } else if ( xmlMatches(node->name, "money") ) {
            points.gold = xmlGetIntProp(node, "gold");
            points.bank_gold = xmlGetIntProp(node, "bank");
            points.cash = xmlGetIntProp(node, "cash");
            points.credits = xmlGetIntProp(node, "credits");
            points.exp = xmlGetIntProp(node, "xp");
        } else if ( xmlMatches(node->name, "stats") ) {
            player.level = xmlGetIntProp(node, "level");
            player.height = xmlGetIntProp(node, "height");
            player.weight = xmlGetIntProp(node, "weight");
            GET_ALIGNMENT(this) = xmlGetIntProp(node, "align");
            
            GET_SEX(this) = 0;
            char *sex = xmlGetProp(node, "sex");
            if( sex != NULL )
                GET_SEX(this) = search_block(sex, genders, FALSE);
			free(sex);

            GET_RACE(this) = 0;
            char *race = xmlGetProp(node, "race");
            if( race != NULL )
                GET_RACE(this) = search_block(race, player_race, FALSE);
			free(race);

        } else if ( xmlMatches(node->name, "class") ) {
			GET_OLD_CLASS(this) = GET_REMORT_CLASS(this) = GET_CLASS(this) = 0;

            char *trade = xmlGetProp(node, "name");
            if( trade != NULL ) {
                GET_CLASS(this) = search_block(trade, pc_char_class_types, FALSE);
				free(trade);
			}
			
            trade = xmlGetProp(node, "remort");
            if( trade != NULL ) {
                GET_REMORT_CLASS(this) = search_block(trade, pc_char_class_types, FALSE);
				free(trade);
			}

			if( IS_CYBORG(this) ) {
				char *subclass = xmlGetProp( node, "subclass" );
				if( subclass != NULL ) {
					GET_OLD_CLASS(this) = search_block( subclass, 
														borg_subchar_class_names, 
														FALSE);
					free(subclass);
				}
			}

			GET_REMORT_GEN(this) = xmlGetIntProp( node, "gen" );
			GET_TOT_DAM(this) = xmlGetIntProp( node, "total_dam" );

        } else if ( xmlMatches(node->name, "time") ) {
            player.time.birth = xmlGetLongProp(node, "birth");
            player.time.death = xmlGetLongProp(node, "death");
            player.time.played = xmlGetIntProp(node, "played");
            player.time.logon = xmlGetLongProp(node, "time");
            //char *h = xmlGetProp(node, "host");
            //strncpy( desc->host, h, HOST_LENGTH );
			//desc->host[HOST_LENGTH] = '\0';
            //free(h);
        } else if ( xmlMatches(node->name, "carnage") ) {
            GET_PKILLS(this) = xmlGetIntProp(node, "pkills");
            GET_MOBKILLS(this) = xmlGetIntProp(node, "mkills");
            GET_PC_DEATHS(this) = xmlGetIntProp(node, "deaths");
        } else if ( xmlMatches(node->name, "attr") ) {
            aff_abils.str = real_abils.str = xmlGetIntProp(node, "str");
            aff_abils.str_add = real_abils.str_add = xmlGetIntProp(node, "stradd");
            aff_abils.intel = real_abils.intel = xmlGetIntProp(node, "int");
            aff_abils.wis = real_abils.wis = xmlGetIntProp(node, "wis");
            aff_abils.dex = real_abils.dex = xmlGetIntProp(node, "dex");
            aff_abils.con = real_abils.con = xmlGetIntProp(node, "con");
            aff_abils.cha = real_abils.cha = xmlGetIntProp(node, "cha");
        } else if ( xmlMatches(node->name, "condition") ) {
			GET_COND(this, THIRST) = xmlGetIntProp(node, "thirst");
			GET_COND(this, FULL) = xmlGetIntProp(node, "hunger");
			GET_COND(this, DRUNK) = xmlGetIntProp(node, "drunk");
       	} else if ( xmlMatches(node->name, "player") ) {
			GET_WIMP_LEV(this) = xmlGetIntProp(node, "wimpy");
			GET_PRACTICES(this) = xmlGetIntProp(node, "wimpy");
			GET_LIFE_POINTS(this) = xmlGetIntProp(node, "lp");
			GET_INVIS_LVL(this) = xmlGetIntProp(node, "invis");
			GET_CLAN(this) = xmlGetIntProp(node, "clan");
        } else if ( xmlMatches(node->name, "home") ) {
			GET_HOME(this) = xmlGetIntProp(node, "town");
			GET_HOLD_HOME(this) = xmlGetIntProp(node, "held_town");
			GET_LOADROOM(this) = xmlGetIntProp(node, "loadroom");
			GET_HOLD_LOADROOM(this) = xmlGetIntProp(node, "held_loadroom");
        } else if ( xmlMatches(node->name, "quest") ) {
			GET_QUEST(this) = xmlGetIntProp(node, "current");
			GET_QUEST_POINTS(this) = xmlGetIntProp(node, "points");
			GET_QUEST_ALLOWANCE(this) = xmlGetIntProp(node, "allowance");
        } else if ( xmlMatches(node->name, "account") ) {
			char* flag = xmlGetProp( node, "flag1" );
			char_specials.saved.act = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.plr2_bits = hex2dec(flag);
			free(flag);

			char *pw = xmlGetProp( node, "password" );
			strncpy( GET_PASSWD( this ), pw, MAX_PWD_LENGTH );
			GET_PASSWD(this)[MAX_PWD_LENGTH] = '\0';
			player_specials->saved.bad_pws = xmlGetIntProp( node, "bad_pws" );
        } else if ( xmlMatches(node->name, "prefs") ) {
			char* flag = xmlGetProp( node, "flag1" );
			player_specials->saved.pref = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.pref2 = hex2dec(flag);
			free(flag);
        } else if ( xmlMatches(node->name, "terminal") ) {
			GET_PAGE_LENGTH(this) = xmlGetIntProp( node, "rows" );
			//GET_PAGE_WIDTH(this) = xmlGetIntProp( node, "columns" );
        } else if ( xmlMatches(node->name, "weaponspec") ) {
			int vnum = xmlGetIntProp( node, "vnum" );
			int level = xmlGetIntProp( node, "level" );
			if( vnum > 0 && level > 0 ) {
				for( int i = 0; i < MAX_WEAPON_SPEC; i++ ) {
					if( player_specials->saved.weap_spec[i].vnum == 0 ) {
						player_specials->saved.weap_spec[i].vnum = vnum;
						player_specials->saved.weap_spec[i].level = level;
						break;
					}
				}
			}
        } else if ( xmlMatches(node->name, "title") ) {
			GET_TITLE(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "affect") ) {
			affected_type af;
			af.type = xmlGetIntProp( node, "type" );
			af.duration = xmlGetIntProp( node, "duration" );
			af.modifier = xmlGetIntProp( node, "modifier" );
			af.location = xmlGetIntProp( node, "location" );
			af.level = xmlGetIntProp( node, "level" );
			af.aff_index = xmlGetIntProp( node, "index" );
			af.next = NULL;
			char* instant = xmlGetProp( node, "instant" );
			if( instant != NULL && strcmp( instant, "yes" ) == 0 ) {
				af.is_instant = 1;
			} else {
				af.is_instant = 0;
			}
			free(instant);

			char* bits = xmlGetProp( node, "affbits" );
			af.bitvector = hex2dec(bits);
			free(bits);

			affect_to_char(this, &af);

        } else if ( xmlMatches(node->name, "affects") ) {
			char* flag = xmlGetProp( node, "flag1" );
			AFF_FLAGS(this) = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			AFF2_FLAGS(this) = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag3" );
			AFF3_FLAGS(this) = hex2dec(flag);
			free(flag);

        } else if ( xmlMatches(node->name, "skill") ) {
			char *spellName = xmlGetProp( node, "name" );
			int index = str_to_spell( spellName );
			if( index >= 0 ) {
				GET_SKILL( this, index ) = xmlGetIntProp( node, "level" );
			}
			free(spellName);
        } else if ( xmlMatches(node->name, "alias") ) {
			alias_data *alias;
			CREATE(alias, struct alias_data, 1);
			alias->type = xmlGetIntProp(node, "type");
			alias->alias = xmlGetProp(node, "alias");
			alias->replacement = xmlGetProp(node, "replace");
			if( alias->alias == NULL || alias->replacement == NULL ) {
				free(alias);
			} else {
				add_alias(this,alias);
			}
		} else if ( xmlMatches(node->name, "description" ) ) {
			player.description = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "poofin") ) {
			POOFIN(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "poofout") ) {
			POOFOUT(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "immort") ) {
			player_specials->saved.occupation = xmlGetIntProp(node,"badge");
        } else if ( xmlMatches(node->name, "lastlogin") ) {
            player.time.logon = xmlGetIntProp(node, "time");
            if( desc != NULL ) {
                char *host = xmlGetProp(node,"host");
                strcpy( desc->host, host );
                free(host);
            }
        }
    }

    xmlFreeDoc(doc);

	player.short_descr = NULL;
	player.long_descr = NULL;

	if (points.max_mana < 100)
		points.max_mana = 100;

	char_specials.carry_weight = 0;
	char_specials.carry_items = 0;
	char_specials.worn_weight = 0;
	points.armor = 100;
	points.hitroll = 0;
	points.damroll = 0;
	setSpeed(0);

	// reset all imprint rooms
	for( int i = 0; i < MAX_IMPRINT_ROOMS; i++ )
		GET_IMPRINT_ROOM(this, i) = -1;

    // Make sure the NPC flag isn't set
    if( IS_SET(char_specials.saved.act, MOB_ISNPC) ) {
		REMOVE_BIT(char_specials.saved.act, MOB_ISNPC);
		slog("SYSERR: loadFromXML %s loaded with MOB_ISNPC bit set!",
			GET_NAME(this));
	}

	// If you're not poisioned and you've been away for more than an hour,
	// we'll set your HMV back to full
	if (!IS_POISONED(this) && (((long)(time(0) - player.time.logon)) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
		GET_MANA(this) = GET_MAX_MANA(this);
	}

	//read_alias(ch);
    return true;
}
