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
#include "tmpstr.h"
#include "comm.h"
#include "creature.h"
#include "char_class.h"
#include "handler.h"
#include "language.h"

void obj_to_room(struct obj_data *object, struct room_data *room);
void add_alias(struct Creature *ch, struct alias_data *a);
void affect_to_char(struct Creature *ch, struct affected_type *af);
void extract_object_list(obj_data * head);

// Saves the given characters equipment to a file. Intended for use while 
// the character is still in the game. 
bool
Creature::crashSave()
{
    player_specials->rentcode = RENT_CRASH;
	player_specials->rent_currency = in_room->zone->time_frame;

    if(!saveObjects() ) {
        return false;
    }

    REMOVE_BIT(PLR_FLAGS(this), PLR_CRASH);
    saveToXML(); // This should probably be here.
    return true;
}

obj_data *
Creature::findCostliestObj(void)
{
 	obj_data *cur_obj, *result;

	if (GET_LEVEL(this) >= LVL_AMBASSADOR)
		return false;

	result = NULL;

	cur_obj = carrying;
	while (cur_obj) {
		if (cur_obj &&
				(!result || GET_OBJ_COST(result) < GET_OBJ_COST(cur_obj)))
			result = cur_obj;

		if (cur_obj->contains)
			cur_obj = cur_obj->contains;	// descend into obj
		else if (!cur_obj->next_content && cur_obj->in_obj)
			cur_obj = cur_obj->in_obj->next_content; // ascend out of obj
		else
			cur_obj = cur_obj->next_content; // go to next obj
	}

	return result;
}

// Creature::payRent will pay the player's rent, selling off items to meet the
// bill, if necessary.
int
Creature::payRent(time_t last_time, int code, int currency)
{
	float day_count;
	int factor;
	long cost;

	// Immortals don't pay rent
	if (GET_LEVEL(this) >= LVL_AMBASSADOR)
		return 0;
    if (isTester())
        return 0;

	// Cryoed chars already paid their rent, quit chars don't have any rent
	if (code == RENT_NEW_CHAR || code == RENT_CRYO || code == RENT_QUIT)
		return 0;

	// Calculate total cost
	day_count = (float)(time(NULL) - last_time) / SECS_PER_REAL_DAY;
	if (code == RENT_FORCED)
		factor = 3;
	else
		factor = 1;
	cost = (int)(calc_daily_rent(this, factor, NULL, NULL) * day_count);
	slog("Charging %ld for %.2f days of rent", cost, day_count);

	// First we get as much as we can out of their hand
	if (currency == TIME_ELECTRO) {
		if (cost < GET_CASH(this) + GET_FUTURE_BANK(this)) {
			this->account->withdraw_future_bank(cost - GET_CASH(this));
			GET_CASH(this) = MAX(GET_CASH(this) - cost, 0);
			cost = 0;
		} else {
			cost -= GET_CASH(this) + GET_FUTURE_BANK(this);
			this->account->set_future_bank(0);
			GET_CASH(this) = 0;
		}
	} else {
		if (cost < GET_GOLD(this) + GET_PAST_BANK(this)) {
			this->account->withdraw_past_bank(cost - GET_GOLD(this));
			GET_GOLD(this) = MAX(GET_GOLD(this) - cost, 0);
			cost = 0;
		} else {
			cost -= GET_GOLD(this) + GET_PAST_BANK(this);
			this->account->set_past_bank(0);
			GET_GOLD(this) = 0;
		}
	}

	// If they didn't have enough, try the cross-time money
	if (cost > 0) {
		if (currency == TIME_ELECTRO) {
			if (cost < GET_GOLD(this) + GET_PAST_BANK(this)) {
				this->account->withdraw_past_bank(cost - GET_GOLD(this));
				GET_GOLD(this) = MAX(GET_GOLD(this) - cost, 0);
				cost = 0;
			} else {
				cost -= GET_GOLD(this) + GET_PAST_BANK(this);
				this->account->set_past_bank(0);
				GET_GOLD(this) = 0;
			}
		} else {
			if (cost < GET_CASH(this) + GET_FUTURE_BANK(this)) {
				this->account->withdraw_future_bank(cost - GET_CASH(this));
				GET_CASH(this) = MAX(GET_CASH(this) - cost, 0);
				cost = 0;
			} else {
				cost -= GET_CASH(this) + GET_FUTURE_BANK(this);
				this->account->set_future_bank(0);
				GET_CASH(this) = 0;
			}
		}
	}

	// If they still don't have enough, start selling stuff
	if (cost > 0) {
		obj_data *doomed_obj, *tmp_obj;
		
		while (cost > 0) {
			doomed_obj = findCostliestObj();
			if (!doomed_obj)
				break;

			slog("%s sold for %d %s to cover %s's rent",
				tmp_capitalize(doomed_obj->name),
				GET_OBJ_COST(doomed_obj),
				(currency == TIME_ELECTRO) ? "creds":"gold",
				GET_NAME(this));
			send_to_char(this,
				"%s has been sold to cover the cost of your rent.\r\n",
				tmp_capitalize(doomed_obj->name));

			// Credit player with value of object
			cost -= GET_OBJ_COST(doomed_obj);

			// Remove objects within doomed object, if any
			while (doomed_obj->contains) {
				tmp_obj = doomed_obj->contains;
				obj_from_obj(tmp_obj);
				obj_to_char(tmp_obj, this);
			}

			// Remove doomed object
			extract_obj(doomed_obj);
		}

		if (cost < 0) {
			// If there's any money left over, give em a consolation prize
			if (currency == TIME_ELECTRO)
				GET_CASH(this) -= cost;
			else
				GET_GOLD(this) -= cost;
		}

		return (cost > 0) ? 2:3;
	}
	return 0;
}

bool
reportUnrentables(Creature *ch, obj_data *obj_list, const char *pos)
{
	bool same_obj(struct obj_data *obj1, struct obj_data *obj2);

 	obj_data *cur_obj, *last_obj;
	bool result = false;

	last_obj = NULL;
	cur_obj = obj_list;
	while (cur_obj) {
		if (!last_obj || !same_obj(last_obj, cur_obj)) {
			if (cur_obj->isUnrentable()) {
				act(tmp_sprintf("You cannot rent while %s $p!", pos),
					true, ch, cur_obj, 0, TO_CHAR);
				result = true;
			}
		}
		last_obj = cur_obj;

		pos = "carrying";
		if (cur_obj->contains)
			cur_obj = cur_obj->contains;	// descend into obj
		else if (!cur_obj->next_content && cur_obj->in_obj)
			cur_obj = cur_obj->in_obj->next_content; // ascend out of obj
		else
			cur_obj = cur_obj->next_content; // go to next obj
	}

	return result;
}

// Displays all unrentable items and returns true if any are found
bool
Creature::displayUnrentables(void)
{
	obj_data *cur_obj;
	int pos;
	bool result = false;

	if (GET_LEVEL(this) >= LVL_AMBASSADOR)
		return false;

	for (pos = 0;pos < NUM_WEARS;pos++) {
		cur_obj = GET_EQ(this, pos);
		if (cur_obj)
			result = result || reportUnrentables(this, cur_obj, "wearing");
		cur_obj = GET_IMPLANT(this, pos);
		if (cur_obj)
			result = result || reportUnrentables(this, cur_obj, "implanted with");
	}

	result = result || reportUnrentables(this, carrying, "carrying");

	return result;
}

bool
Creature::saveObjects(void) 
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
	obj_data *obj;

	if( axs != 0 ) {
		if( errno != ENOENT ) {
			slog("SYSERR: Unable to open xml equipment file '%s': %s", 
				 path, strerror(errno) );
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

			obj = create_obj();
			if(!obj->loadFromXML(NULL,this,NULL,node) ) {
				extract_obj(obj);
			}
			//obj_to_room(obj, in_room);
		}
	}

    xmlFreeDoc(doc);

	return payRent(player.time.logon, player_specials->rentcode, player_specials->rent_currency);
}

int
Creature::loadCorpse()
{

    char *path = get_corpse_file_path( GET_IDNUM(this) );
	int axs = access(path, W_OK);
    obj_data *corpse_obj;

	if( axs != 0 ) {
		if( errno != ENOENT ) {
			slog("SYSERR: Unable to open xml equipment file '%s': %s", 
				 path, strerror(errno) );
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
    
    xmlNodePtr node = root->xmlChildrenNode;
    while (!xmlMatches(node->name, "object")) {
        node = node->next;
        if (node == NULL) {
            xmlFreeDoc(doc);
            slog("SYSERR: First child in XML file (%s) not an object", path);
            return 1;
        }
    }
    
    corpse_obj = create_obj();
    corpse_obj->shared = null_obj_shared;
    if (!corpse_obj->loadFromXML(NULL, this, NULL, node)) {
        xmlFreeDoc(doc);
        extract_obj(corpse_obj);
        slog("SYSERR: Could not create corpse object from file %s", path);
        return 1;
    }

    if (!IS_CORPSE(corpse_obj)) {
        xmlFreeDoc(doc);
        extract_obj(corpse_obj);
        slog("SYSERR: First object in corpse file %s not a corpse", path);
        return 1;
    }
    
    xmlFreeDoc(doc);

    remove(path);
	return 0; 
}

void
Creature::saveToXML() 
{
	// Save vital statistics
	obj_data *saved_eq[NUM_WEARS];
	obj_data *saved_impl[NUM_WEARS];
	affected_type *saved_affs, *cur_aff;
	FILE *ouf;
	char *path;
	struct alias_data *cur_alias;
	int idx, pos;
	int hit = GET_HIT(this), mana = GET_MANA(this), move = GET_MOVE(this);
    Creature *ch = this;

	if (GET_IDNUM(ch) == 0) {
		slog("Attempt to save creature with idnum==0");
		raise(SIGSEGV);
	}

    path = get_player_file_path(GET_IDNUM(ch));
	ouf = fopen(path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML player file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return;
	}


	// Remove all spell affects without deleting them
	saved_affs = affected;
	affected = NULL;

	for (cur_aff = saved_affs;cur_aff;cur_aff = cur_aff->next)
		affect_modify(this, cur_aff->location, cur_aff->modifier,
			cur_aff->bitvector, cur_aff->aff_index, false);

	// Before we save everything, every piece of eq, and every affect must
	// be removed and stored - otherwise all the stats get screwed up when
	// we restore the eq and affects
	for (pos = 0;pos < NUM_WEARS;pos++) {
		if (GET_EQ(this, pos))
			saved_eq[pos] = unequip_char(this, pos, MODE_EQ, true);
		else
			saved_eq[pos] = NULL;
		if (GET_IMPLANT(this, pos))
			saved_impl[pos] = unequip_char(this, pos, MODE_IMPLANT, true);
		else
			saved_impl[pos] = NULL;
	}

	// we need to update time played every time we save...
	player.time.played += time(0) - player.time.logon;
	player.time.logon = time(0);

	fprintf(ouf, "<creature name=\"%s\" idnum=\"%ld\">\n",
		GET_NAME(ch), ch->char_specials.saved.idnum);

	fprintf(ouf, "<points hit=\"%d\" mana=\"%d\" move=\"%d\" maxhit=\"%d\" maxmana=\"%d\" maxmove=\"%d\"/>\n",
		ch->points.hit, ch->points.mana, ch->points.move,
		ch->points.max_hit, ch->points.max_mana, ch->points.max_move);

    fprintf(ouf, "<money gold=\"%d\" cash=\"%d\" xp=\"%d\"/>\n",
        ch->points.gold, ch->points.cash,  ch->points.exp);

	fprintf(ouf, "<stats level=\"%d\" sex=\"%s\" race=\"%s\" height=\"%d\" weight=\"%d\" align=\"%d\"/>\n",
		GET_LEVEL(ch), genders[GET_SEX(ch)], player_race[GET_RACE(ch)],
		GET_HEIGHT(ch), GET_WEIGHT(ch), GET_ALIGNMENT(ch));
	
    fprintf(ouf, "<languages known=\"%lld\" current=\"%d\"/>\n",
            KNOWN_LANGUAGES(ch), GET_LANGUAGE(ch));
	fprintf(ouf, "<class name=\"%s\"", pc_char_class_types[GET_CLASS(ch)]);
	if( IS_REMORT(ch) ) {
		fprintf(ouf, " remort=\"%s\" gen=\"%d\"",
			pc_char_class_types[GET_REMORT_CLASS(ch)], GET_REMORT_GEN(ch));
    }
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

	fprintf(ouf, "<time birth=\"%ld\" death=\"%ld\" played=\"%ld\" last=\"%ld\"/>\n",
		ch->player.time.birth, ch->player.time.death, ch->player.time.played,
		ch->player.time.logon);

	char *host = "";
	if( desc != NULL ) {
		host = xmlEncodeTmp( desc->host );
	}
	fprintf(ouf, "<carnage pkills=\"%d\" akills=\"%d\" mkills=\"%d\" deaths=\"%d\" reputation=\"%d\"",
		GET_PKILLS(ch), GET_ARENAKILLS(ch), GET_MOBKILLS(ch), GET_PC_DEATHS(ch),
		GET_REPUTATION(ch));
	if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF))
		fprintf(ouf, " severity=\"%d\"", GET_SEVERITY(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<attr str=\"%d\" int=\"%d\" wis=\"%d\" dex=\"%d\" con=\"%d\" cha=\"%d\" stradd=\"%d\"/>\n",
		real_abils.str, real_abils.intel, real_abils.wis, real_abils.dex,
		real_abils.con, real_abils.cha, real_abils.str_add);

	fprintf(ouf, "<condition hunger=\"%d\" thirst=\"%d\" drunk=\"%d\"/>\n",
		GET_COND(ch, FULL), GET_COND(ch, THIRST), GET_COND(ch, DRUNK));

	fprintf(ouf, "<player wimpy=\"%d\" lp=\"%d\" clan=\"%d\"/>\n",
		GET_WIMP_LEV(ch), GET_LIFE_POINTS(ch), GET_CLAN(ch));

	if (ch->desc)
		ch->player_specials->desc_mode = ch->desc->input_mode;
	if (ch->player_specials->rentcode == RENT_CREATING ||
			ch->player_specials->rentcode == RENT_REMORTING) {
		fprintf(ouf, "<rent code=\"%d\" perdiem=\"%d\" "
			"currency=\"%d\" state=\"%s\"/>\n",
			ch->player_specials->rentcode, ch->player_specials->rent_per_day, ch->player_specials->rent_currency,
			desc_modes[(int)ch->player_specials->desc_mode]);
	} else {
		fprintf(ouf, "<rent code=\"%d\" perdiem=\"%d\" currency=\"%d\"/>\n",
			ch->player_specials->rentcode, ch->player_specials->rent_per_day, ch->player_specials->rent_currency);
	}
	fprintf(ouf, "<home town=\"%d\" homeroom=\"%d\" loadroom=\"%d\"/>\n",
		GET_HOME(ch), GET_HOMEROOM(ch), GET_LOADROOM(ch));

	fprintf(ouf, "<quest");
	if (GET_QUEST(ch))
		fprintf(ouf, " current=\"%d\"", GET_QUEST(ch));
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		fprintf(ouf, " allowance=\"%d\"", GET_QUEST_ALLOWANCE(ch));
	if( GET_QUEST_POINTS(ch) != 0 )
		fprintf(ouf, " points=\"%d\"", GET_QUEST_POINTS(ch));
	fprintf(ouf, "/>\n");
		
	fprintf(ouf, "<bits flag1=\"%lx\" flag2=\"%x\"/>\n",
		ch->char_specials.saved.act, ch->player_specials->saved.plr2_bits);
	if (PLR_FLAGGED(ch, PLR_FROZEN)) {
        fprintf(ouf, "<frozen thaw_time=\"%d\" freezer_id=\"%d\"/>\n", 
                ch->player_specials->thaw_time, ch->player_specials->freezer_id);
    }

	fprintf(ouf, "<prefs flag1=\"%lx\" flag2=\"%lx\"/>\n",
		ch->player_specials->saved.pref, ch->player_specials->saved.pref2);

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
		fprintf(ouf, "<immort badge=\"%s\" qlog=\"%d\" invis=\"%d\"/>\n",
			xmlEncodeTmp(BADGE(ch)), GET_QLOG_LEVEL(ch),
			GET_INVIS_LVL(ch));
		if (POOFIN(ch) && *POOFIN(ch))
			fprintf(ouf, "<poofin>%s</poofin>\n", xmlEncodeTmp(POOFIN(ch)));
		if (POOFOUT(ch) && *POOFOUT(ch))
			fprintf(ouf, "<poofout>%s</poofout>\n", xmlEncodeTmp(POOFOUT(ch)));
	}
	if (ch->player.description && *ch->player.description) {
		fprintf(ouf, "<description>%s</description>\n",  xmlEncodeTmp(tmp_gsub(ch->player.description, "\r\n", "\n")));
	}
	for (cur_alias = ch->player_specials->aliases; cur_alias; cur_alias = cur_alias->next)
		fprintf(ouf, "<alias type=\"%d\" alias=\"%s\" replace=\"%s\"/>\n",
			cur_alias->type, xmlEncodeTmp(cur_alias->alias), xmlEncodeTmp(cur_alias->replacement) );
	for (cur_aff = saved_affs;cur_aff; cur_aff = cur_aff->next)
		fprintf(ouf, "<affect type=\"%d\" duration=\"%d\" modifier=\"%d\" location=\"%d\" level=\"%d\" instant=\"%s\" affbits=\"%lx\" index=\"%d\" />\n",
			cur_aff->type, cur_aff->duration, cur_aff->modifier,
			cur_aff->location, cur_aff->level,
			(cur_aff->is_instant) ? "yes":"no", 
			cur_aff->bitvector,
			cur_aff->aff_index );

	if (GET_LEVEL(ch) < 50) {
		for (idx = 0;idx < MAX_SKILLS;idx++)
			if (ch->player_specials->saved.skills[idx] > 0)
				fprintf(ouf, "<skill name=\"%s\" level=\"%d\"/>\n",
					spell_to_str(idx), GET_SKILL(ch, idx));
	}

	fprintf(ouf, "</creature>\n");
	fclose(ouf);


	// Now we get to put all that eq back on and reinstate the spell affects
	for (pos = 0;pos < NUM_WEARS;pos++) {
		if (saved_eq[pos])
			equip_char(this, saved_eq[pos], pos, MODE_EQ);
		if (saved_impl[pos])
			equip_char(this, saved_impl[pos], pos, MODE_IMPLANT);
	}

	for (cur_aff = saved_affs;cur_aff;cur_aff = cur_aff->next)
		affect_modify(this, cur_aff->location, cur_aff->modifier,
			cur_aff->bitvector, cur_aff->aff_index, true);
	affected = saved_affs;
	affect_total(this);

	GET_HIT(this) = MIN(GET_MAX_HIT(ch), hit);
	GET_MANA(this) = MIN(GET_MAX_MANA(ch), mana);
	GET_MOVE(this) = MIN(GET_MAX_MOVE(ch), move);
}

bool 
Creature::loadFromXML( long id )
{
    char *path = get_player_file_path( id );
    return loadFromXML( path );
}
/* copy data from the file structure to a Creature */
bool
Creature::loadFromXML( const char *path )
{
	char *txt; 
	int idx;
    
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
            points.cash = xmlGetIntProp(node, "cash");
            points.exp = xmlGetIntProp(node, "xp");
        } else if (xmlMatches(node->name, "languages")) {
            GET_LANGUAGE(this) = xmlGetLongLongProp(node, "current");
            KNOWN_LANGUAGES(this) = xmlGetLongLongProp(node, "known");

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
			GET_OLD_CLASS(this) = GET_REMORT_CLASS(this) = GET_CLASS(this) = -1;

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

			if (GET_CLASS(this) == CLASS_MAGE ) {
				player_specials->saved.mana_shield_low =
					xmlGetLongProp(node, "manash_low");
				player_specials->saved.mana_shield_pct =
					xmlGetLongProp(node, "manash_pct");
			}

			GET_REMORT_GEN(this) = xmlGetIntProp( node, "gen" );
			GET_TOT_DAM(this) = xmlGetIntProp( node, "total_dam" );

        } else if ( xmlMatches(node->name, "time") ) {
            player.time.birth = xmlGetLongProp(node, "birth");
            player.time.death = xmlGetLongProp(node, "death");
            player.time.played = xmlGetLongProp(node, "played");
            player.time.logon = xmlGetLongProp(node, "last");
        } else if ( xmlMatches(node->name, "carnage") ) {
            GET_PKILLS(this) = xmlGetIntProp(node, "pkills");
            GET_ARENAKILLS(this) = xmlGetIntProp(node, "akills");
            GET_MOBKILLS(this) = xmlGetIntProp(node, "mkills");
            GET_PC_DEATHS(this) = xmlGetIntProp(node, "deaths");
			GET_REPUTATION(this) = xmlGetIntProp(node, "reputation");
			GET_SEVERITY(this) = xmlGetIntProp(node, "severity");
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
			GET_LIFE_POINTS(this) = xmlGetIntProp(node, "lp");
			GET_CLAN(this) = xmlGetIntProp(node, "clan");
        } else if ( xmlMatches(node->name, "home") ) {
			GET_HOME(this) = xmlGetIntProp(node, "town");
			GET_HOMEROOM(this) = xmlGetIntProp(node, "homeroom");
			GET_LOADROOM(this) = xmlGetIntProp(node, "loadroom");
        } else if ( xmlMatches(node->name, "quest") ) {
			GET_QUEST(this) = xmlGetIntProp(node, "current");
			GET_QUEST_POINTS(this) = xmlGetIntProp(node, "points");
			GET_QUEST_ALLOWANCE(this) = xmlGetIntProp(node, "allowance");
        } else if ( xmlMatches(node->name, "bits") ) {
			char* flag = xmlGetProp( node, "flag1" );
			char_specials.saved.act = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.plr2_bits = hex2dec(flag);
			free(flag);
        } else if (xmlMatches(node->name, "frozen")) {
            this->player_specials->thaw_time = xmlGetIntProp(node, "thaw_time");
            this->player_specials->freezer_id = xmlGetIntProp(node, "freezer_id");
        } else if ( xmlMatches(node->name, "prefs") ) {
			char* flag = xmlGetProp( node, "flag1" );
			player_specials->saved.pref = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.pref2 = hex2dec(flag);
			free(flag);
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
			char *txt;
			
			txt = (char*)xmlNodeGetContent( node );
			set_title(this, txt);
			if (txt)
				free(txt);
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
			// PCs shouldn't have ANY perm affects
			if (IS_NPC(this)) {
				char* flag = xmlGetProp( node, "flag1" );
				AFF_FLAGS(this) = hex2dec(flag);
				free(flag);

				flag = xmlGetProp( node, "flag2" );
				AFF2_FLAGS(this) = hex2dec(flag);
				free(flag);

				flag = xmlGetProp( node, "flag3" );
				AFF3_FLAGS(this) = hex2dec(flag);
				free(flag);
			} else {
				AFF_FLAGS(this) = 0;
				AFF2_FLAGS(this) = 0;
				AFF3_FLAGS(this) = 0;
			}

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
			txt = (char *)xmlNodeGetContent(node);
			player.description = strdup(tmp_gsub(txt, "\n", "\r\n"));
			free(txt);
        } else if ( xmlMatches(node->name, "poofin") ) {
			POOFIN(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "poofout") ) {
			POOFOUT(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "immort") ) {
			txt = xmlGetProp(node, "badge");
			strncpy(BADGE(this), txt, 7);
			BADGE(this)[7] = '\0';
			free(txt);

			GET_QLOG_LEVEL(this) = xmlGetIntProp(node, "qlog");
			GET_INVIS_LVL(this) = xmlGetIntProp(node, "invis");
        } else if (xmlMatches(node->name, "rent")) {
			char *txt;

			player_specials->rentcode = xmlGetIntProp(node, "code");
			player_specials->rent_per_day = xmlGetIntProp(node, "perdiem");
			player_specials->rent_currency = xmlGetIntProp(node, "currency");
			txt = (char *)xmlGetProp(node, "state");
			if (txt)
                player_specials->desc_mode = (cxn_state)search_block(txt, desc_modes, FALSE);
		} else if( xmlMatches(node->name, "account") ) { // Legacy for old char database
            char *pw = xmlGetProp( node, "password" );
            strcpy( GET_PASSWD(this), pw );
            if( pw != NULL )
                free(pw);
        }
    }

    xmlFreeDoc(doc);

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

	if (GET_LEVEL(this) >= 50) {
		for (idx = 0;idx < MAX_SKILLS;idx++)
			if (player_specials->saved.skills[idx] < 100)
				player_specials->saved.skills[idx] = 100;
	}
	//read_alias(ch);
    return true;
}

void
Creature::set(const char *key, const char *val)
{
	if (!strcmp(key, "race"))
		GET_RACE(this) = atoi(val);
	else if (!strcmp(key, "class"))
		GET_CLASS(this) = atoi(val);
	else if (!strcmp(key, "remort"))
		GET_REMORT_CLASS(this) = atoi(val);
	else if (!strcmp(key, "name"))
		GET_NAME(this) = strdup(val);
	else if (!strcmp(key, "title"))
		GET_TITLE(this) = strdup(val);
	else if (!strcmp(key, "poofin"))
		POOFIN(this) = strdup(val);
	else if (!strcmp(key, "poofout"))
		POOFOUT(this) = strdup(val);
	else if (!strcmp(key, "immbadge"))
		strcpy(BADGE(this), val);
	else if (!strcmp(key, "sex"))
		GET_SEX(this) = atoi(val);
	else if (!strcmp(key, "hitp"))
		GET_HIT(this) = atoi(val);
	else if (!strcmp(key, "mana"))
		GET_MANA(this) = atoi(val);
	else if (!strcmp(key, "move"))
		GET_MOVE(this) = atoi(val);
	else if (!strcmp(key, "maxhitp"))
		GET_MAX_HIT(this) = atoi(val);
	else if (!strcmp(key, "maxmana"))
		GET_MAX_MANA(this) = atoi(val);
	else if (!strcmp(key, "maxmove"))
		GET_MAX_MOVE(this) = atoi(val);
	else if (!strcmp(key, "gold"))
		GET_GOLD(this) = atol(val);
	else if (!strcmp(key, "cash"))
		GET_CASH(this) = atol(val);
	else if (!strcmp(key, "exp"))
		GET_EXP(this) = atol(val);
	else if (!strcmp(key, "level"))
		GET_LEVEL(this) = atol(val);
	else if (!strcmp(key, "height"))
		GET_HEIGHT(this) = atol(val);
	else if (!strcmp(key, "weight"))
		GET_WEIGHT(this) = atol(val);
	else if (!strcmp(key, "align"))
		GET_ALIGNMENT(this) = atoi(val);
	else if (!strcmp(key, "gen"))
		GET_REMORT_GEN(this) = atoi(val);
	else if (!strcmp(key, "birth_time"))
		player.time.birth = atol(val);
	else if (!strcmp(key, "death_time"))
		player.time.death = atol(val);
	else if (!strcmp(key, "played_time"))
		player.time.played = atol(val);
	else if (!strcmp(key, "login_time"))
		player.time.logon = atol(val);
	else if (!strcmp(key, "pkills"))
		GET_PKILLS(this) = atol(val);
	else if (!strcmp(key, "mkills"))
		GET_MOBKILLS(this) = atol(val);
	else if (!strcmp(key, "akills"))
		GET_ARENAKILLS(this) = atol(val);
	else if (!strcmp(key, "deaths"))
		GET_PC_DEATHS(this) = atol(val);
	else if (!strcmp(key, "reputation"))
		GET_REPUTATION(this) = atoi(val);
	else if (!strcmp(key, "str"))
		aff_abils.str = real_abils.str = atoi(val);
	else if (!strcmp(key, "int"))
		aff_abils.intel = real_abils.intel = atoi(val);
	else if (!strcmp(key, "wis"))
		aff_abils.wis = real_abils.wis = atoi(val);
	else if (!strcmp(key, "dex"))
		aff_abils.dex = real_abils.dex = atoi(val);
	else if (!strcmp(key, "con"))
		aff_abils.con = real_abils.con = atoi(val);
	else if (!strcmp(key, "cha"))
		aff_abils.cha = real_abils.cha = atoi(val);
	else if (!strcmp(key, "hunger"))
		GET_COND(this, FULL) = atoi(val);
	else if (!strcmp(key, "thirst"))
		GET_COND(this, THIRST) = atoi(val);
	else if (!strcmp(key, "drunk"))
		GET_COND(this, DRUNK) = atoi(val);
	else
		slog("Invalid player field %s set to %s", key, val);
}

bool
Creature::loadFromDB(long idnum)
{
	const char **fields;
	PGresult *res;
	int res_count, acct_count, field_count, field_idx;

	res = sql_query("select race, class, remort, name, title, poofin, poofout, imm_badge, sex, hitp, mana, move, maxhitp, maxmana, maxmove, gold, cash, exp, level, height, weight, align, gen, birth_time, death_time, played_time, login_time, pkills, mkills, akills, deaths, reputation, flag_severity, str, int, wis, dex, con, cha, hunger, thirst, drunk, invis_lvl, wimpy, lifepoints, rent_kind, rent_per_day, currency, cxn_mode, home_town, home_room, load_room, prefs_1, prefs_2, affects_1, affects_2, affects_3, descrip from accounts where idnum=%ld", idnum);
	res_count = PQntuples(res);
	if (res_count > 1) {
		slog("SYSERR: search for player %ld returned more than one match", idnum);
		return false;
	}
	
	if (acct_count < 1)
		return false;
	
	GET_IDNUM(this) = idnum;

	field_count = PQnfields(res);
	fields = new (const char *)[field_count];
	for (field_idx = 0;field_idx < field_count;field_idx++)
		fields[field_idx] = PQfname(res, field_idx);
	
	for (field_idx = 0;field_idx < field_count;field_idx++)
		this->set(fields[field_idx], PQgetvalue(res, 0, field_idx));
	delete [] fields;
	return true;
}

