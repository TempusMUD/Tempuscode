#include <dirent.h>
#include <string.h>
#include "constants.h"
#include "utils.h"
#include "xml_utils.h"
#include "db.h"
#include "shop.h"
#include "tmpstr.h"
#include "comm.h"



/* copy data from the file structure to a Creature */
bool
Creature::loadFromXML( long id )
{
    char *path = getPlayerfilePath( id );
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
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, struct player_special_data, 1);


    ch->player.name = (char*)xmlGetProp(node, "NAME");
    char_specials.saved.idnum = xmlGetIntProp(node, "IDNUM");
    
    // Read in the subnodes
	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "POINTS") || ) {
            points.mana = xmlGetIntProp(node, "MANA");
            points.max_mana = xmlGetIntProp(node, "MAXMANA");
            points.hit = xmlGetIntProp(node, "HIT");
            points.max_hit = xmlGetIntProp(node, "MAXHIT");
            points.move = xmlGetIntProp(node, "MOVE");
            points.max_move = xmlGetIntProp(node, "MAXMOVE");
        } else if ( xmlMatches(node->name, "MONEY") ) {
            points.gold = xmlGetIntProp(node, "GOLD");
            points.bank_gold = xmlGetIntProp(node, "BANK");
            points.cash = xmlGetIntProp(node, "CASH");
            points.credits = xmlGetIntProp(node, "CREDITS");
            points.exp = xmlGetIntProp(node, "XP");
        } else if ( xmlMatches(node->name, "STATS") ) {
            ch->player.level = xmlGetIntProp(node, "LEVEL");
            ch->player.height = xmlGetIntProp(node, "HEIGHT");
            ch->player.weight = xmlGetIntProp(node, "WEIGHT");
            GET_ALIGNMENT(ch) = xmlGetIntProp(node, "ALIGN");
            
            GET_SEX(ch) = 0;
            char *sex = xmlGetProp(node, "SEX");
            if( sex != NULL )
                GET_SEX(ch) = search_block(sex, genders, FALSE);
            GET_RACE(ch) = 0;
            char *race = xmlGetProp(node, "RACE");
            if( race != NULL )
                GET_RACE(ch) = search_block(race, player_race, FALSE);

        } else if ( xmlMatches(node->name, "CLASS") ) {
            GET_CLASS(ch) = 0;
            char *trade = xmlGetProp(node, "NAME");
            if( trade != NULL )
                GET_CLASS(ch) = search_block(trade, pc_char_class_types, FALSE);
            
        } else if ( xmlMatches(node->name, "TIME") ) {
            ch->player.time.birth = xmlGetLongProp(node, "BIRTH");
            ch->player.time.death = xmlGetLongProp(node, "DEATH");
            ch->player.time.played = xmlGetIntProp(node, "PLAYED");
        } else if ( xmlMatches(node->name, "LASTLOGIN") ) {
            ch->player.time.logon = xmlGetLongProp(node, "TIME");
            ch->desc->host = xmlGetProp(node, "HOST");
        } else if ( xmlMatches(node->name, "CARNAGE") ) {
            GET_PKILLS(ch) = xmlGetIntProp(node, "PKILLS");
            GET_MOBKILLS(ch) = xmlGetIntProp(node, "MKILLS");
            GET_PC_DEATHS(ch) = xmlGetIntProp(node, "DEATHS");
        } else if ( xmlMatches(node->name, "ATTR") ) {
            real_abils.str = xmlGetIntProp(node, "STR");
            real_abils.str_add = xmlGetIntProp(node, "STRADD");
            real_abils.intel = xmlGetIntProp(node, "INT");
            real_abils.wis = xmlGetIntProp(node, "WIS");
            real_abils.dex = xmlGetIntProp(node, "DEX");
            real_abils.con = xmlGetIntProp(node, "CON");
            real_abils.cha = xmlGetIntProp(node, "CHA");
        } else if ( xmlMatches(node->name, "CONDITION") ) {
        } else if ( xmlMatches(node->name, "PLAYER") ) {
        } else if ( xmlMatches(node->name, "HOME") ) {
        } else if ( xmlMatches(node->name, "QUEST") ) {
        } else if ( xmlMatches(node->name, "ACCOUNT") ) {
        } else if ( xmlMatches(node->name, "PREFS") ) {
        } else if ( xmlMatches(node->name, "TERMINAL") ) {
        } else if ( xmlMatches(node->name, "WEAPONSPEC") ) {
        } else if ( xmlMatches(node->name, "TITLE") ) {
        } else if ( xmlMatches(node->name, "AFFECT") ) {
        } else if ( xmlMatches(node->name, "AFFECTS") ) {
        } else if ( xmlMatches(node->name, "SKILL") ) {
        } else if ( xmlMatches(node->name, "aliases") ) {
        }

    }


    xmlFreeDoc(doc);



	
	GET_SEX(ch) = st->sex;
	GET_CLASS(ch) = st->char_class;
	GET_REMORT_CLASS(ch) = st->remort_char_class;
	GET_RACE(ch) = st->race;
	GET_LEVEL(ch) = st->level;

	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;
	set_title(ch, st->title);
	ch->player.description = str_dup(st->description);
	ch->player.hometown = st->hometown;
	ch->player.time.birth = st->birth;
	ch->player.time.death = st->death;
	ch->player.time.played = st->played;
	ch->player.time.logon = time(0);

	ch->player.weight = st->weight;
	ch->player.height = st->height;

	ch->real_abils = st->abilities;
	ch->aff_abils = st->abilities;
	ch->points = st->points;
	ch->char_specials.saved = st->char_specials_saved;

	

	ch->player_specials->saved = st->player_specials_saved;

	if (ch->points.max_mana < 100)
		ch->points.max_mana = 100;

	ch->char_specials.carry_weight = 0;
	ch->char_specials.carry_items = 0;
	ch->char_specials.worn_weight = 0;
	ch->points.armor = 100;
	ch->points.hitroll = 0;
	ch->points.damroll = 0;
	ch->setSpeed(0);


	st->pwd[MAX_PWD_LENGTH] = '\0';
	strcpy(ch->player.passwd, st->pwd);

	if (*st->poofin) {
		CREATE(POOFIN(ch), char, strlen(st->poofin) + 1);
		strcpy(POOFIN(ch), st->poofin);
	} else
		POOFIN(ch) = NULL;

	if (*st->poofout) {
		CREATE(POOFOUT(ch), char, strlen(st->poofout) + 1);
		strcpy(POOFOUT(ch), st->poofout);
	} else
		POOFOUT(ch) = NULL;

	// reset all imprint rooms
	for (i = 0; i < MAX_IMPRINT_ROOMS; i++)
		GET_IMPRINT_ROOM(ch, i) = -1;

	/* Add all spell effects */
	for (i = 0; i < MAX_AFFECT; i++) {
		if (st->affected[i].type)
			affect_to_char(ch, &st->affected[i]);
	}


    // Make sure the NPC flag isn't set
    if (IS_SET(ch->char_specials.saved.act, MOB_ISNPC)) {
		REMOVE_BIT(ch->char_specials.saved.act, MOB_ISNPC);
		slog("SYSERR: loadFromXML %s loaded with MOB_ISNPC bit set!",
			GET_NAME(ch));
	}

	// If you're not poisioned and you've been away for more than an hour,
	// we'll set your HMV back to full
	if (!IS_POISONED(ch) &&
		(((long)(time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(ch) = GET_MAX_HIT(ch);
		GET_MOVE(ch) = GET_MAX_MOVE(ch);
		GET_MANA(ch) = GET_MAX_MANA(ch);
	}

	read_alias(ch);
}



void
load_xml_mobile(xmlNodePtr node)
{
	struct Creature *mob;

	// Initialize structures
	CREATE(mob, struct Creature, 1);
	clear_char(mob);
	CREATE(mob->mob_specials.shared, struct mob_shared_data, 1);

	// Read in initial properties
	mob->player.short_descr = xmlGetProp(node, "name");
	MOB_SHARED(mob)->vnum = xmlGetIntProp(node, "vnum");
	MOB_SHARED(mob)->lair = xmlGetIntProp(node, "lair", -1);
	MOB_SHARED(mob)->leader = xmlGetIntProp(node, "leader", -1);
	MOB_SHARED(mob)->svnum = xmlGetIntProp(node, "iscript");

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