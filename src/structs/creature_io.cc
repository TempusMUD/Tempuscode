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


void add_alias(struct Creature *ch, struct alias_data *a);
void affect_to_char(struct Creature *ch, struct affected_type *af);

void
Creature::saveToXML() {
	// Save vital statistics
	FILE *ouf;
	char *path;
	struct alias_data *cur_alias;
	struct affected_type *cur_aff;
	int idx;
    Creature *ch = this;

    path = getPlayerfilePath(GET_IDNUM(ch));
	ouf = fopen(path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML player file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return;
	}

	fprintf(ouf, "<CREATURE NAME=\"%s\" IDNUM=\"%ld\">\n",
		GET_NAME(ch), ch->char_specials.saved.idnum);

	fprintf(ouf, "<POINTS HIT=\"%d\" MANA=\"%d\" MOVE=\"%d\" MAXHIT=\"%d\" MAXMANA=\"%d\" MAXMOVE=\"%d\"/>\n",
		ch->points.hit, ch->points.mana, ch->points.move,
		ch->points.max_hit, ch->points.max_mana, ch->points.max_move);

    fprintf(ouf, "<MONEY GOLD=\"%d\" BANK=\"%d\" CASH=\"%d\" CREDITS=\"%d\" XP=\"%d\"/>\n",
        ch->points.gold, ch->points.bank_gold, 
		ch->points.cash, ch->points.credits, 
		ch->points.exp);

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

	char *host = xmlEncodeEntities(ch->desc->host);
	fprintf(ouf, "<LASTLOGIN TIME=\"%ld\" HOST=\"%s\"/>\n",
		(long int)ch->player.time.logon, host);
	free(host);

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
		fprintf(ouf, " CURRENT=\"%d\"", GET_QUEST(ch));
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		fprintf(ouf, " ALLOWANCE=\"%d\"", GET_QUEST_ALLOWANCE(ch));
    if( GET_QUEST_POINTS(ch) != 0 )
        fprintf(ouf, " POINTS=\"%d\"", GET_QUEST_POINTS(ch));
    
	fprintf(ouf, "/>\n");
		
	fprintf(ouf, "<ACCOUNT FLAG1=\"%lx\" FLAG2=\"%x\" PASSWORD=\"%s\" BAD_PWS=\"%d\"",
		ch->char_specials.saved.act, ch->player_specials->saved.plr2_bits,
		ch->player.passwd, ch->player_specials->saved.bad_pws);
	if (PLR_FLAGGED(ch, PLR_FROZEN))
		fprintf(ouf, " FROZEN_LVL=\"%d\"", GET_FREEZE_LEV(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<PREFS FLAG1=\"%lx\" FLAG2=\"%lx\"/>\n",
		ch->player_specials->saved.pref, ch->player_specials->saved.pref2);

	fprintf(ouf, "<TERMINAL ROWS=\"%d\" COLUMNS=\"%d\"/>\n",
		GET_PAGE_LENGTH(ch), GET_COLS(ch));

	fprintf(ouf, "<AFFECTS FLAG1=\"%lx\" FLAG2=\"%lx\" FLAG3=\"%lx\"/>\n",
		ch->char_specials.saved.affected_by,
		ch->char_specials.saved.affected2_by,
		ch->char_specials.saved.affected3_by);

	for (idx = 0;idx < MAX_WEAPON_SPEC;idx++) {
		if (ch->player_specials->saved.weap_spec[idx].level)
			fprintf(ouf, "<WEAPONSPEC VNUM=\"%d\" LEVEL=\"%d\"/>\n",
				ch->player_specials->saved.weap_spec[idx].vnum,
				ch->player_specials->saved.weap_spec[idx].level);
	}

	if (GET_TITLE(ch) && *GET_TITLE(ch)) {
		char* title = xmlEncodeEntities(GET_TITLE(ch));
		fprintf(ouf, "<TITLE>%s</TITLE>\n", title );
		free(title);
	}

	if (GET_LEVEL(ch) >= LVL_IMMORT) {
		fprintf(ouf, "<IMMORT BADGE=\"%d\"/>\n",
			ch->player_specials->saved.occupation);
		if (POOFIN(ch) && *POOFIN(ch))
			fprintf(ouf, "<POOFIN>%s</POOFIN>\n", POOFIN(ch));
		if (POOFOUT(ch) && *POOFOUT(ch))
			fprintf(ouf, "<POOFOUT>%s</POOFOUT>\n", POOFOUT(ch));
	}
	if (ch->player.description && *ch->player.description) {
		char *desc = xmlEncodeEntities(ch->player.description);
		fprintf(ouf, "<DESCRIPTION>%s</DESCRIPTION>\n",  desc );
		free(desc);
	}
	for (cur_alias = ch->player_specials->aliases; cur_alias; cur_alias = cur_alias->next)
		fprintf(ouf, "<ALIAS TYPE=\"%d\" ALIAS=\"%s\" REPLACE=\"%s\"/>\n",
			cur_alias->type, cur_alias->alias, cur_alias->replacement);
	for (cur_aff = ch->affected;cur_aff; cur_aff = cur_aff->next)
		fprintf(ouf, "<AFFECT TYPE=\"%d\" DURATION=\"%d\" MODIFIER=\"%d\" LOCATION=\"%d\" LEVEL=\"%d\" INSTANT=\"%s\" AFFBITS=\"%lx\" INDEX=\"%d\" />\n",
			cur_aff->type, cur_aff->duration, cur_aff->modifier,
			cur_aff->location, cur_aff->level,
			(cur_aff->is_instant) ? "yes":"no", 
			cur_aff->bitvector,
			cur_aff->aff_index );

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
			free(sex);

            GET_RACE(this) = 0;
            char *race = xmlGetProp(node, "RACE");
            if( race != NULL )
                GET_RACE(this) = search_block(race, player_race, FALSE);
			free(race);

        } else if ( xmlMatches(node->name, "CLASS") ) {
			GET_OLC_CLASS(this) = GET_REMORT_CLASS(this) = GET_CLASS(this) = 0;

            char *trade = xmlGetProp(node, "NAME");
            if( trade != NULL ) {
                GET_CLASS(this) = search_block(trade, pc_char_class_types, FALSE);
				free(trade);
			}
			
            trade = xmlGetProp(node, "REMORT");
            if( trade != NULL ) {
                GET_REMORT_CLASS(this) = search_block(trade, pc_char_class_types, FALSE);
				free(trade);
			}

			if( IS_CYBORG(this) ) {
				char *subclass = xmlGetProp( node, "SUBCLASS" );
				if( subclass != NULL ) {
					GET_OLD_CLASS(this) = search_block( subclass, 
														borg_subchar_class_names, 
														FALSE);
					free(subclass);
				}
			}

			GET_REMORT_GEN(this) = xmlGetIntProp( node, "GEN" );
			GET_TOT_DAM(this) = xmlGetIntProp( node, "TOTAL_DAM" );

        } else if ( xmlMatches(node->name, "TIME") ) {
            player.time.birth = xmlGetLongProp(node, "BIRTH");
            player.time.death = xmlGetLongProp(node, "DEATH");
            player.time.played = xmlGetIntProp(node, "PLAYED");
            player.time.logon = xmlGetLongProp(node, "TIME");
            //char *h = xmlGetProp(node, "HOST");
            //strncpy( desc->host, h, HOST_LENGTH );
			//desc->host[HOST_LENGTH] = '\0';
            //free(h);
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
			GET_COND(this, THIRST) = xmlGetIntProp(node, "THIRST");
			GET_COND(this, FULL) = xmlGetIntProp(node, "HUNGER");
			GET_COND(this, DRUNK) = xmlGetIntProp(node, "DRUNK");
       	} else if ( xmlMatches(node->name, "PLAYER") ) {
			GET_WIMP_LEV(this) = xmlGetIntProp(node, "WIMPY");
			GET_PRACTICES(this) = xmlGetIntProp(node, "WIMPY");
			GET_LIFE_POINTS(this) = xmlGetIntProp(node, "LP");
			GET_INVIS_LVL(this) = xmlGetIntProp(node, "INVIS");
			GET_CLAN(this) = xmlGetIntProp(node, "CLAN");
        } else if ( xmlMatches(node->name, "HOME") ) {
			GET_HOME(this) = xmlGetIntProp(node, "TOWN");
			GET_HOLD_HOME(this) = xmlGetIntProp(node, "HELD_TOWN");
			GET_LOADROOM(this) = xmlGetIntProp(node, "LOADROOM");
			GET_HOLD_LOADROOM(this) = xmlGetIntProp(node, "HELD_LOADROOM");
        } else if ( xmlMatches(node->name, "QUEST") ) {
			GET_QUEST(this) = xmlGetIntProp(node, "CURRENT");
			GET_QUEST_POINTS(this) = xmlGetIntProp(node, "POINTS");
			GET_QUEST_ALLOWANCE(this) = xmlGetIntProp(node, "ALLOWANCE");
        } else if ( xmlMatches(node->name, "ACCOUNT") ) {
			char* flag = xmlGetProp( node, "FLAG1" );
			char_specials.saved.act = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "FLAG2" );
			player_specials->saved.plr2_bits = hex2dec(flag);
			free(flag);

			char *pw = xmlGetProp( node, "PASSWORD" );
			strncpy( GET_PASSWD( this ), pw, MAX_PWD_LENGTH );
			GET_PASSWD(this)[MAX_PWD_LENGTH] = '\0';
			player_specials->saved.bad_pws = xmlGetIntProp( node, "BAD_PWS" );
        } else if ( xmlMatches(node->name, "PREFS") ) {
			char* flag = xmlGetProp( node, "FLAG1" );
			player_specials->saved.pref = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "FLAG2" );
			player_specials->saved.pref2 = hex2dec(flag);
			free(flag);
        } else if ( xmlMatches(node->name, "TERMINAL") ) {
			GET_PAGE_LENGTH(this) = xmlGetIntProp( node, "ROWS" );
			//GET_PAGE_WIDTH(this) = xmlGetIntProp( node, "COLUMNS" );
        } else if ( xmlMatches(node->name, "WEAPONSPEC") ) {
			int vnum = xmlGetIntProp( node, "VNUM" );
			int level = xmlGetIntProp( node, "LEVEL" );
			if( vnum > 0 && level > 0 ) {
				for( int i = 0; i < MAX_WEAPON_SPEC; i++ ) {
					if( player_specials->saved.weap_spec[i].vnum == 0 ) {
						player_specials->saved.weap_spec[i].vnum = vnum;
						player_specials->saved.weap_spec[i].level = level;
						break;
					}
				}
			}
        } else if ( xmlMatches(node->name, "TITLE") ) {
			GET_TITLE(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "AFFECT") ) {
			affected_type af;
			af.type = xmlGetIntProp( node, "TYPE" );
			af.duration = xmlGetIntProp( node, "DURATION" );
			af.modifier = xmlGetIntProp( node, "MODIFIER" );
			af.location = xmlGetIntProp( node, "LOCATION" );
			af.level = xmlGetIntProp( node, "LEVEL" );
			af.aff_index = xmlGetIntProp( node, "INDEX" );
			af.next = NULL;
			char* instant = xmlGetProp( node, "INSTANT" );
			if( instant != NULL && strcmp( instant, "yes" ) == 0 ) {
				af.is_instant = 1;
			}
			free(instant);

			char* bits = xmlGetProp( node, "AFFBITS" );
			af.bitvector = hex2dec(bits);
			free(bits);

			affect_to_char(this, &af);

        } else if ( xmlMatches(node->name, "AFFECTS") ) {
			char* flag = xmlGetProp( node, "FLAG1" );
			AFF_FLAGS(this) = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "FLAG2" );
			AFF2_FLAGS(this) = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "FLAG3" );
			AFF3_FLAGS(this) = hex2dec(flag);
			free(flag);

        } else if ( xmlMatches(node->name, "SKILL") ) {
			char *spellName = xmlGetProp( node, "NAME" );
			int index = str_to_spell( spellName );
			if( index >= 0 ) {
				GET_SKILL( this, index ) = xmlGetIntProp( node, "LEVEL" );
			}
			free(spellName);
        } else if ( xmlMatches(node->name, "ALIAS") ) {
			alias_data *alias;
			CREATE(alias, struct alias_data, 1);
			alias->type = xmlGetIntProp(node, "TYPE");
			alias->alias = xmlGetProp(node, "ALIAS");
			alias->replacement = xmlGetProp(node, "REPLACE");
			if( alias->alias == NULL || alias->replacement == NULL ) {
				free(alias);
			} else {
				add_alias(this,alias);
			}
        } else if ( xmlMatches(node->name, "POOFIN") ) {
			POOFIN(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "POOFOUT") ) {
			POOFOUT(this) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "IMMORT") ) {
			player_specials->saved.occupation = xmlGetIntProp(node,"BADGE");
        }

    }

    xmlFreeDoc(doc);

	/*
	ch->player.short_descr = NULL;
	ch->player.long_descr = NULL;
	set_title(ch, st->title);
	ch->player.description = str_dup(st->description);
	ch->player.hometown = st->hometown;

	st->pwd[MAX_PWD_LENGTH] = '\0';
	strcpy(ch->player.passwd, st->pwd);

	// Add all spell effects 
	for (i = 0; i < MAX_AFFECT; i++) {
		if (st->affected[i].type)
			affect_to_char(ch, &st->affected[i]);
	}
    */

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
