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

void
Creature::saveToXML() {
	// Save vital statistics
	FILE *ouf;
	char *path;
	struct alias_data *cur_alias;
	struct affected_type *cur_aff;
	int idx;
    Creature *ch = this;
	//path = tmp_sprintf("plrxml/%d.dat", GET_PFILEPOS(ch));
    path = getPlayerfilePath( GET_IDNUM(ch) );
	ouf = fopen(path, "w");

	if( ouf == NULL ) {
		fprintf(stderr, "Unable to open XML player file for save.[%s](%s)\n",
				path,strerror(errno) );
		return;
	}
	fprintf(ouf, "<CREATURE NAME=\"%s\" IDNUM=\"%ld\">\n",
		GET_NAME(ch), ch->char_specials.saved.idnum);

	fprintf(ouf, "<POINTS HIT=\"%d\" MANA=\"%d\" MOVE=\"%d\" MAXHIT=\"%d\" MAXMANA=\"%d\" MAXMOVE=\"%d\"/>\n",
		ch->points.hit, ch->points.mana, ch->points.move,
		ch->points.max_hit, ch->points.max_mana, ch->points.max_move);

    fprintf(ouf, "<MONEY GOLD=\"%d\" BANK=\"%d\" CASH=\"%d\" CREDITS=\"%d\" />\n",
        ch->points.gold, ch->points.bank_gold, ch->points.cash, ch->points.credits );

	fprintf(ouf, "<STATS LEVEL=\"%d\" SEX=\"%s\" RACE=\"%s\" HEIGHT=\"%d\" WEIGHT=\"%d\" ALIGN=\"%d\"/>\n",
		GET_LEVEL(ch), genders[GET_SEX(ch)], player_race[GET_RACE(ch)],
		GET_HEIGHT(ch), GET_WEIGHT(ch), GET_ALIGNMENT(ch));
	
	fprintf(ouf, "<CLASS NAME=\"%s\"", pc_char_class_types[GET_CLASS(ch)]);
	if (GET_REMORT_GEN(ch))
		fprintf(ouf, " REMORT=\"%s\" GEN=\"%d\"",
			pc_char_class_types[GET_REMORT_CLASS(ch)], GET_REMORT_GEN(ch));
	if (IS_CYBORG(ch)) {
		if (GET_OLD_CLASS(ch) != -1)
			fprintf(ouf, " SUBCLASS=\"%s\"",
				borg_subchar_class_names[GET_OLD_CLASS(ch)]);
		if (GET_TOT_DAM(ch))
			fprintf(ouf, " TOTAL_DAM=\"%d\"", GET_TOT_DAM(ch));
		if (GET_BROKE(ch))
			fprintf(ouf, " BROKEN=\"%d\"", GET_BROKE(ch));
	} else if (GET_CLASS(ch) == CLASS_MAGE &&
			GET_SKILL(ch, SPELL_MANA_SHIELD) > 0) {
		fprintf(ouf, " MANASH_LOW=\"%ld\" MANASH_PCT=\"%ld\"",	
			ch->player_specials->saved.mana_shield_low,
			ch->player_specials->saved.mana_shield_pct);
	}
	fprintf(ouf, "/>\n");


	fprintf(ouf, "<TIME BIRTH=\"%ld\" DEATH=\"%ld\" PLAYED=\"%d\"/>\n",
		ch->player.time.birth,
		ch->player.time.death,
		ch->player.time.played);

	fprintf(ouf, "<LASTLOGIN TIME=\"%ld\" HOST=\"%s\"/>\n",
		(long int)ch->player.time.logon, ch->desc->host);

	fprintf(ouf, "<CARNAGE PKILLS=\"%d\" MKILLS=\"%d\" DEATHS=\"%d\"/>\n",
		GET_PKILLS(ch), GET_MOBKILLS(ch), GET_PC_DEATHS(ch));

	fprintf(ouf, "<ATTR STR=\"%d\" INT=\"%d\" WIS=\"%d\" DEX=\"%d\" CON=\"%d\" CHA=\"%d\" STRADD=\"%d\"/>\n",
		GET_STR(ch), GET_INT(ch), GET_WIS(ch), GET_DEX(ch), GET_CON(ch),
		GET_CHA(ch), GET_ADD(ch));
    /*  Calculated from eq and affects.
	fprintf(ouf, "<SAVE PARA=\"%d\" ROD=\"%d\" PETRI=\"%d\" BREATH=\"%d\" SPELL=\"%d\" CHEM=\"%d\" PSI=\"%d\" PHY=\"%d\"/>\n",
		GET_SAVE(ch, SAVING_PARA), GET_SAVE(ch, SAVING_ROD),
		GET_SAVE(ch, SAVING_PETRI), GET_SAVE(ch, SAVING_BREATH),
		GET_SAVE(ch, SAVING_SPELL), GET_SAVE(ch, SAVING_CHEM),
		GET_SAVE(ch, SAVING_PSI), GET_SAVE(ch, SAVING_PHY));
    */
	fprintf(ouf, "<CONDITION HUNGER=\"%d\" THIRST=\"%d\" DRUNK=\"%d\"/>\n",
		ch->player_specials->saved.conditions[0],
		ch->player_specials->saved.conditions[1],
		ch->player_specials->saved.conditions[2]);

	fprintf(ouf, "<PLAYER INVIS=\"%d\" WIMPY=\"%d\" PRACS=\"%d\" LP=\"%d\" CLAN=\"%d\"/>\n",
		GET_INVIS_LVL(ch), GET_WIMP_LEV(ch), GET_PRACTICES(ch),
		GET_LIFE_POINTS(ch), GET_CLAN(ch));

	fprintf(ouf, "<HOME TOWN=\"%d\" LOADROOM=\"%d\" HELD_TOWN=\"%d\" HELD_LOADROOM=\"%d\"/>\n",
		GET_HOME(ch), GET_LOADROOM(ch), GET_HOLD_HOME(ch),
		GET_HOLD_LOADROOM(ch));

	fprintf(ouf, "<QUEST");
	if (GET_QUEST(ch))
		fprintf(ouf, " JOINED=\"%d\"", GET_QUEST(ch));
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		fprintf(ouf, " ALLOWANCE=\"%d\"", GET_QUEST_ALLOWANCE(ch));
    if( GET_QUEST_POINTS(ch) != 0 )
        fprintf(ouf, " POINTS=\"%d\"", GET_QUEST_POINTS(ch));
    
	fprintf(ouf, "/>\n");
		
	fprintf(ouf, "<ACCOUNT FLAG1=\"%ld\" FLAG2=\"%d\" PASSWORD=\"%s\" BAD_PWS=\"%d\"",
		ch->char_specials.saved.act, ch->player_specials->saved.plr2_bits,
		ch->player.passwd, ch->player_specials->saved.bad_pws);
	if (PLR_FLAGGED(ch, PLR_FROZEN))
		fprintf(ouf, " FROZEN_LVL=\"%d\"", GET_FREEZE_LEV(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<PREFS FLAG1=\"%ld\" FLAG2=\"%ld\" LOADROOM=\"%d\" PAGE_LEN=\"%d\"/>\n",
		ch->player_specials->saved.pref, ch->player_specials->saved.pref2,
		ch->player_specials->saved.load_room,
		ch->player_specials->saved.page_length);

	fprintf(ouf, "<TERMINAL PAGE_LENGTH=\"%d\" COLUMNS=\"%d\"/>\n",
		GET_PAGE_LENGTH(ch), GET_COLS(ch));

	fprintf(ouf, "<AFFECTS FLAG1=\"%ld\" FLAG2=\"%ld\" FLAG3=\"%ld\"/>\n",
		ch->char_specials.saved.affected_by,
		ch->char_specials.saved.affected2_by,
		ch->char_specials.saved.affected3_by);

	for (idx = 0;idx < MAX_WEAPON_SPEC;idx++) {
		if (ch->player_specials->saved.weap_spec[idx].level)
			fprintf(ouf, "<WEAPONSPEC VNUM=\"%d\" LEVEL=\"%d\"/>\n",
				ch->player_specials->saved.weap_spec[idx].vnum,
				ch->player_specials->saved.weap_spec[idx].level);
	}

	if (GET_TITLE(ch) && *GET_TITLE(ch))
		fprintf(ouf, "<TITLE>%s</TITLE>\n", GET_TITLE(ch));

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		fprintf(ouf, "<IMMORT BADGE=\"%d\"/>\n",
			ch->player_specials->saved.occupation);
		if (POOFIN(ch) && *POOFIN(ch))
			fprintf(ouf, "<POOFIN>%s</POOFIN>\n", POOFIN(ch));
		if (POOFOUT(ch) && *POOFOUT(ch))
			fprintf(ouf, "<POOFOUT>%s</POOFOUT>\n", POOFOUT(ch));
	}
	if (ch->player.description && *ch->player.description)
		fprintf(ouf, "<DESCRIPTION>%s</DESCRIPTION>\n", ch->player.description);
	for (cur_alias = ch->player_specials->aliases; cur_alias; cur_alias = cur_alias->next)
		fprintf(ouf, "<ALIAS TYPE=\"%d\" ALIAS=\"%s\" REPLACE=\"%s\"/>\n",
			cur_alias->type, cur_alias->alias, cur_alias->replacement);
	for (cur_aff = ch->affected;cur_aff; cur_aff = cur_aff->next)
		fprintf(ouf, "<AFFECT TYPE=\"%d\" DURATION=\"%d\" MODIFIER=\"%d\" LOCATION=\"%d\" LEVEL=\"%d\" INSTANT=\"%s\" AFFBITS=\"%ld\"/>\n",
			cur_aff->type, cur_aff->duration, cur_aff->modifier,
			cur_aff->location, cur_aff->level,
			(cur_aff->is_instant) ? "yes":"no", cur_aff->bitvector);

	for (idx = 0;idx < MAX_SKILLS;idx++)
		if (ch->player_specials->saved.skills[idx] > 0)
			fprintf(ouf, "<SKILL NAME=\"%s\" LEVEL=\"%d\"/>\n",
				spell_to_str(idx), GET_SKILL(ch, idx));

	fprintf(ouf, "</CREATURE>\n");
	fclose(ouf);
}

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
	if (player_specials == NULL)
		CREATE(player_specials, struct player_special_data, 1);

    player.name = xmlGetProp(root, "NAME");
    char_specials.saved.idnum = xmlGetIntProp(root, "IDNUM");
    
    // Read in the subnodes
	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "POINTS") ) {
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
            player.level = xmlGetIntProp(node, "LEVEL");
            player.height = xmlGetIntProp(node, "HEIGHT");
            player.weight = xmlGetIntProp(node, "WEIGHT");
            GET_ALIGNMENT(this) = xmlGetIntProp(node, "ALIGN");
            
            GET_SEX(this) = 0;
            char *sex = xmlGetProp(node, "SEX");
            if( sex != NULL )
                GET_SEX(this) = search_block(sex, genders, FALSE);
            GET_RACE(this) = 0;
            char *race = xmlGetProp(node, "RACE");
            if( race != NULL )
                GET_RACE(this) = search_block(race, player_race, FALSE);

        } else if ( xmlMatches(node->name, "CLASS") ) {
            GET_CLASS(this) = 0;
            char *trade = xmlGetProp(node, "NAME");
            if( trade != NULL )
                GET_CLASS(this) = search_block(trade, pc_char_class_types, FALSE);
            
        } else if ( xmlMatches(node->name, "TIME") ) {
            player.time.birth = xmlGetLongProp(node, "BIRTH");
            player.time.death = xmlGetLongProp(node, "DEATH");
            player.time.played = xmlGetIntProp(node, "PLAYED");
        } else if ( xmlMatches(node->name, "LASTLOGIN") ) {
            player.time.logon = xmlGetLongProp(node, "TIME");
            char *h = xmlGetProp(node, "HOST");
            strcpy( desc->host, h );
            free(h);
        } else if ( xmlMatches(node->name, "CARNAGE") ) {
            GET_PKILLS(this) = xmlGetIntProp(node, "PKILLS");
            GET_MOBKILLS(this) = xmlGetIntProp(node, "MKILLS");
            GET_PC_DEATHS(this) = xmlGetIntProp(node, "DEATHS");
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



	/*
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

	// Add all spell effects 
	for (i = 0; i < MAX_AFFECT; i++) {
		if (st->affected[i].type)
			affect_to_char(ch, &st->affected[i]);
	}
    */

    // Make sure the NPC flag isn't set
    if (IS_SET(char_specials.saved.act, MOB_ISNPC)) {
		REMOVE_BIT(char_specials.saved.act, MOB_ISNPC);
		slog("SYSERR: loadFromXML %s loaded with MOB_ISNPC bit set!",
			GET_NAME(this));
	}

	// If you're not poisioned and you've been away for more than an hour,
	/* we'll set your HMV back to full
	if (!IS_POISONED(this) && (((long)(time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
		GET_MANA(this) = GET_MAX_MANA(this);
	}*/

	//read_alias(ch);
    return true;
}
