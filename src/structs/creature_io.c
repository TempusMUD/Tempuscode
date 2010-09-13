#ifdef HAS_CONFIG_H
#include "config.h"
#endif

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
#include "security.h"

void add_alias(struct creature *ch, struct alias_data *a);
void affect_to_char(struct creature *ch, struct affected_type *af);
void extract_object_list(struct obj_data * head);
bool save_player_objects(struct creature *ch);



struct obj_data *
findCostliestObj(struct creature *ch)
{
 	struct obj_data *cur_obj, *result;

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
		return false;

	result = NULL;

	cur_obj = ch->carrying;
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

// struct creature_payRent will pay the player's rent, selling off items to meet the
// bill, if necessary.
int
pay_player_rent(struct creature *ch, time_t last_time, int code, int currency)
{
	float day_count;
	int factor;
	long cost;

	// Immortals don't pay rent
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
		return 0;
    if (is_authorized(ch, TESTER, NULL))
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
	cost = (int)(calc_daily_rent(ch, factor, NULL, false) * day_count);
	slog("Charging %ld for %.2f days of rent", cost, day_count);

	// First we get as much as we can out of their hand
	if (currency == TIME_ELECTRO) {
		if (cost < GET_CASH(ch) + GET_FUTURE_BANK(ch)) {
			withdraw_future_bank(ch->account, cost - GET_CASH(ch));
			GET_CASH(ch) = MAX(GET_CASH(ch) - cost, 0);
			cost = 0;
		} else {
			cost -= GET_CASH(ch) + GET_FUTURE_BANK(ch);
			set_future_bank(ch->account, 0);
			GET_CASH(ch) = 0;
		}
	} else {
		if (cost < GET_GOLD(ch) + GET_PAST_BANK(ch)) {
			withdraw_past_bank(ch->account, cost - GET_GOLD(ch));
			GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
			cost = 0;
		} else {
			cost -= GET_GOLD(ch) + GET_PAST_BANK(ch);
			set_past_bank(ch->account, 0);
			GET_GOLD(ch) = 0;
		}
	}

	// If they didn't have enough, try the cross-time money
	if (cost > 0) {
		if (currency == TIME_ELECTRO) {
			if (cost < GET_GOLD(ch) + GET_PAST_BANK(ch)) {
				withdraw_past_bank(ch->account, cost - GET_GOLD(ch));
				GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
				cost = 0;
			} else {
				cost -= GET_GOLD(ch) + GET_PAST_BANK(ch);
				set_past_bank(ch->account, 0);
				GET_GOLD(ch) = 0;
			}
		} else {
			if (cost < GET_CASH(ch) + GET_FUTURE_BANK(ch)) {
				withdraw_future_bank(ch->account, cost - GET_CASH(ch));
				GET_CASH(ch) = MAX(GET_CASH(ch) - cost, 0);
				cost = 0;
			} else {
				cost -= GET_CASH(ch) + GET_FUTURE_BANK(ch);
				set_future_bank(ch->account, 0);
				GET_CASH(ch) = 0;
			}
		}
	}

	// If they still don't have enough, start selling stuff
	if (cost > 0) {
		struct obj_data *doomed_obj, *tmp_obj;

		while (cost > 0) {
			doomed_obj = findCostliestObj(ch);
			if (!doomed_obj)
				break;

			slog("%s (%lu) sold for %d %s to cover %s's rent",
				tmp_capitalize(doomed_obj->name),
				doomed_obj->unique_id,
				GET_OBJ_COST(doomed_obj),
				(currency == TIME_ELECTRO) ? "creds":"gold",
				GET_NAME(ch));
			send_to_char(ch,
				"%s has been sold to cover the cost of your rent.\r\n",
				tmp_capitalize(doomed_obj->name));

			// Credit player with value of object
			cost -= GET_OBJ_COST(doomed_obj);

			// Remove objects within doomed object, if any
			while (doomed_obj->contains) {
				tmp_obj = doomed_obj->contains;
				obj_from_obj(tmp_obj);
				obj_to_char(tmp_obj, ch);
			}

			// Remove doomed object
			extract_obj(doomed_obj);
		}

		if (cost < 0) {
			// If there's any money left over, give em a consolation prize
			if (currency == TIME_ELECTRO)
				GET_CASH(ch) -= cost;
			else
				GET_GOLD(ch) -= cost;
		}

		return (cost > 0) ? 2:3;
	}
	return 0;
}

bool
reportUnrentables(struct creature *ch, struct obj_data *obj_list, const char *pos)
{
	bool same_obj(struct obj_data *obj1, struct obj_data *obj2);

 	struct obj_data *cur_obj, *last_obj;
	bool result = false;

	last_obj = NULL;
	cur_obj = obj_list;
	while (cur_obj) {
		if (!last_obj || !same_obj(last_obj, cur_obj)) {
			if (isUnrentable(cur_obj)) {
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
displayUnrentables(struct creature *ch)
{
	struct obj_data *cur_obj;
	int pos;
	bool result = false;

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
		return false;

	for (pos = 0;pos < NUM_WEARS;pos++) {
		cur_obj = GET_EQ(ch, pos);
		if (cur_obj)
			result = result || reportUnrentables(ch, cur_obj, "wearing");
		cur_obj = GET_IMPLANT(ch, pos);
		if (cur_obj)
			result = result || reportUnrentables(ch, cur_obj, "implanted with");
	}

	result = result || reportUnrentables(ch, ch->carrying, "carrying");

	return result;
}

bool
save_player_objects(struct creature *ch)
{
	FILE *ouf;
	char *path, *tmp_path;
	int idx;

    path = get_equipment_file_path(GET_IDNUM(ch));
    tmp_path = tmp_sprintf("%s.new", path);
	ouf = fopen(tmp_path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML equipment file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return false;
	}
	fprintf( ouf, "<objects>\n" );
	// Save the inventory
	for( struct obj_data *obj = ch->carrying; obj != NULL; obj = obj->next_content ) {
		save_object_to_xml(obj, ouf);
	}
	// Save the equipment
	for( idx = 0; idx < NUM_WEARS; idx++ ) {
		if( GET_EQ(ch, idx) )
			save_object_to_xml(GET_EQ(ch,idx), ouf);
		if( GET_IMPLANT(ch, idx) )
			save_object_to_xml(GET_IMPLANT(ch,idx), ouf);
		if( GET_TATTOO(ch, idx) )
			save_object_to_xml(GET_TATTOO(ch,idx), ouf);
	}
	fprintf( ouf, "</objects>\n" );
	fclose(ouf);

    // on success, move the new object file onto the old one
    rename(tmp_path, path);

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
unrent(struct creature *ch)
{
  int err;

  err = load_player_objects();
  if (err)
    return 0;

  return pay_player_rent(ch,
                         ch->player.time.logon,
                         ch->player_specials->rentcode,
                         ch->player_specials->rent_currency);
}

int
load_player_objects(struct creature *ch)
{

    char *path = get_equipment_file_path( GET_IDNUM(ch) );
	int axs = access(path, W_OK);

	if( axs != 0 ) {
		if( errno != ENOENT ) {
			errlog("Unable to open xml equipment file '%s': %s",
				 path, strerror(errno) );
			return -1;
		} else {
			return 1; // normal no eq file
		}
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return -1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return 1;
    }

	for ( xmlNodePtr node = root->xmlChildrenNode; node; node = node->next ) {
        if ( xmlMatches(node->name, "object") )
			(void)load_object_from_xml(NULL,ch,NULL,node);
	}

    xmlFreeDoc(doc);

    return 0;
}

bool
checkLoadCorpse(struct creature *ch)
{
    char *path = get_corpse_file_path(GET_IDNUM(ch));
    int axs = access(path, W_OK);
    struct stat file_stat;
    extern time_t boot_time;

    if (axs != 0) {
        if (errno != ENOENT) {
            errlog("Unable to open xml corpse file '%s' : %s",
                 path, strerror(errno));
            return false;
        }
        else {
            return false;
        }
    }

    stat(path, &file_stat);

    if (file_stat.st_ctime < boot_time)
        return true;

    return false;
}

int
loadCorpse(struct creature *ch)
{

    char *path = get_corpse_file_path( GET_IDNUM(ch) );
	int axs = access(path, W_OK);
    struct obj_data *corpse_obj;

	if( axs != 0 ) {
		if( errno != ENOENT ) {
			errlog("Unable to open xml corpse file '%s': %s",
				 path, strerror(errno) );
			return -1;
		} else {
			return 1; // normal no eq file
		}
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return -1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
        return 1;
    }

    xmlNodePtr node = root->xmlChildrenNode;
    while (!xmlMatches(node->name, "object")) {
        node = node->next;
        if (node == NULL) {
            xmlFreeDoc(doc);
            errlog("First child in XML file (%s) not an object", path);
            return 1;
        }
    }

    corpse_obj = load_object_from_xml(NULL, ch, NULL, node);
    if (!corpse_obj) {
        xmlFreeDoc(doc);
        extract_obj(corpse_obj);
        errlog("Could not create corpse object from file %s", path);
        return 1;
    }

    if (!IS_CORPSE(corpse_obj)) {
        xmlFreeDoc(doc);
        extract_obj(corpse_obj);
        errlog("First object in corpse file %s not a corpse", path);
        return 1;
    }

    xmlFreeDoc(doc);

    remove(path);
	return 0;
}

void
save_player_to_xml(struct creature *ch)
{
    void expire_old_grievances(struct creature *);
	// Save vital statistics
	struct obj_data *saved_eq[NUM_WEARS];
	struct obj_data *saved_impl[NUM_WEARS];
	struct obj_data *saved_tattoo[NUM_WEARS];
	struct affected_type *saved_affs, *cur_aff;
	FILE *ouf;
	char *path, *tmp_path;
	struct alias_data *cur_alias;
	int idx, pos;
	int hit = GET_HIT(ch), mana = GET_MANA(ch), move = GET_MOVE(ch);

	if (GET_IDNUM(ch) == 0) {
		slog("Attempt to save creature with idnum==0");
		raise(SIGSEGV);
	}

    path = get_player_file_path(GET_IDNUM(ch));
    tmp_path = tmp_sprintf("%s.tmp", path);
	ouf = fopen(tmp_path, "w");

	if(!ouf) {
		fprintf(stderr, "Unable to open XML player file for save.[%s] (%s)\n",
			path, strerror(errno) );
		return;
	}

	// Remove all spell affects without deleting them
	saved_affs = ch->affected;
	ch->affected = NULL;

	for (cur_aff = saved_affs;cur_aff;cur_aff = cur_aff->next)
		affect_modify(ch, cur_aff->location, cur_aff->modifier,
			cur_aff->bitvector, cur_aff->aff_index, false);

	// Before we save everything, every piece of eq, and every affect must
	// be removed and stored - otherwise all the stats get screwed up when
	// we restore the eq and affects
	for (pos = 0;pos < NUM_WEARS;pos++) {
		if (GET_EQ(ch, pos))
			saved_eq[pos] = raw_unequip_char(ch, pos, EQUIP_WORN);
		else
			saved_eq[pos] = NULL;
		if (GET_IMPLANT(ch, pos))
			saved_impl[pos] = raw_unequip_char(ch, pos, EQUIP_IMPLANT);
		else
			saved_impl[pos] = NULL;
		if (GET_TATTOO(ch, pos))
			saved_tattoo[pos] = raw_unequip_char(ch, pos, EQUIP_TATTOO);
		else
			saved_tattoo[pos] = NULL;
	}

	// we need to update time played every time we save...
	ch->player.time.played += time(0) - ch->player.time.logon;
	ch->player.time.logon = time(0);

    expire_old_grievances(ch);

	fprintf(ouf, "<creature name=\"%s\" idnum=\"%ld\">\n",
		GET_NAME(ch), ch->char_specials.saved.idnum);

	fprintf(ouf, "<points hit=\"%d\" mana=\"%d\" move=\"%d\" maxhit=\"%d\" maxmana=\"%d\" maxmove=\"%d\"/>\n",
		ch->points.hit, ch->points.mana, ch->points.move,
		ch->points.max_hit, ch->points.max_mana, ch->points.max_move);

    fprintf(ouf, "<money gold=\"%lld\" cash=\"%lld\" xp=\"%d\"/>\n",
        ch->points.gold, ch->points.cash,  ch->points.exp);

	fprintf(ouf, "<stats level=\"%d\" sex=\"%s\" race=\"%s\" height=\"%d\" weight=\"%d\" align=\"%d\"/>\n",
		GET_LEVEL(ch), genders[(int)GET_SEX(ch)], player_race[(int)GET_RACE(ch)],
		GET_HEIGHT(ch), GET_WEIGHT(ch), GET_ALIGNMENT(ch));

	fprintf(ouf, "<class name=\"%s\"", class_names[GET_CLASS(ch)]);
	if( IS_REMORT(ch) ) {
		fprintf(ouf, " remort=\"%s\" gen=\"%d\"",
			class_names[GET_REMORT_CLASS(ch)], GET_REMORT_GEN(ch));
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

	fprintf(ouf, "<carnage pkills=\"%d\" akills=\"%d\" mkills=\"%d\" deaths=\"%d\" reputation=\"%d\"",
		GET_PKILLS(ch), GET_ARENAKILLS(ch), GET_MOBKILLS(ch), GET_PC_DEATHS(ch),
		ch->player_specials->saved.reputation);
	if (PLR_FLAGGED(ch, PLR_KILLER | PLR_THIEF))
		fprintf(ouf, " severity=\"%d\"", GET_SEVERITY(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<attr str=\"%d\" int=\"%d\" wis=\"%d\" dex=\"%d\" con=\"%d\" cha=\"%d\" stradd=\"%d\"/>\n",
            ch->real_abils.str,
            ch->real_abils.intel,
            ch->real_abils.wis,
            ch->real_abils.dex,
            ch->real_abils.con,
            ch->real_abils.cha,
            ch->real_abils.str_add);

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
	if( GET_IMMORT_QP(ch) != 0 )
		fprintf(ouf, " points=\"%d\"", GET_IMMORT_QP(ch));
	fprintf(ouf, "/>\n");

	fprintf(ouf, "<bits flag1=\"%lx\" flag2=\"%x\"/>\n",
		ch->char_specials.saved.act, ch->player_specials->saved.plr2_bits);
	if (PLR_FLAGGED(ch, PLR_FROZEN)) {
        fprintf(ouf, "<frozen thaw_time=\"%d\" freezer_id=\"%d\"/>\n",
                ch->player_specials->thaw_time, ch->player_specials->freezer_id);
    }

	fprintf(ouf, "<prefs flag1=\"%lx\" flag2=\"%lx\" tongue=\"%s\"/>\n",
            ch->player_specials->saved.pref, ch->player_specials->saved.pref2,
            tongue_name(GET_TONGUE(ch)));

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

	if (GET_LEVEL(ch) >= 50) {
		fprintf(ouf, "<immort badge=\"%s\" qlog=\"%d\" invis=\"%d\"/>\n",
			xmlEncodeSpecialTmp(BADGE(ch)), GET_QLOG_LEVEL(ch),
			GET_INVIS_LVL(ch));
		if (POOFIN(ch) && *POOFIN(ch))
			fprintf(ouf, "<poofin>%s</poofin>\n", xmlEncodeTmp(POOFIN(ch)));
		if (POOFOUT(ch) && *POOFOUT(ch))
			fprintf(ouf, "<poofout>%s</poofout>\n", xmlEncodeTmp(POOFOUT(ch)));
	}
	if (ch->player.description && *ch->player.description) {
		fprintf(ouf, "<description>%s</description>\n",  xmlEncodeTmp(tmp_gsub(tmp_gsub(ch->player.description, "\r\n", "\n"), "\r", "")));
	}
	for (cur_alias = ch->player_specials->aliases; cur_alias; cur_alias = cur_alias->next)
		fprintf(ouf, "<alias type=\"%d\" alias=\"%s\" replace=\"%s\"/>\n", cur_alias->type,
                xmlEncodeSpecialTmp(cur_alias->alias),
                xmlEncodeSpecialTmp(cur_alias->replacement) );
	for (cur_aff = saved_affs;cur_aff; cur_aff = cur_aff->next)
		fprintf(ouf, "<affect type=\"%d\" duration=\"%d\" modifier=\"%d\" location=\"%d\" level=\"%d\" instant=\"%s\" affbits=\"%lx\" index=\"%d\" owner=\"%ld\"/>\n",
			cur_aff->type, cur_aff->duration, cur_aff->modifier,
			cur_aff->location, cur_aff->level,
			(cur_aff->is_instant) ? "yes":"no",
			cur_aff->bitvector,
			cur_aff->aff_index, cur_aff->owner );

	if (!IS_IMMORT(ch)) {
		for (idx = 0;idx < MAX_SKILLS;idx++)
			if (ch->player_specials->saved.skills[idx] > 0)
				fprintf(ouf, "<skill name=\"%s\" level=\"%d\"/>\n",
					spell_to_str(idx), GET_SKILL(ch, idx));
        write_tongue_xml(ch, ouf);
        if (!GET_RECENT_KILLS(ch).empty()) {
            std_list<KillRecord>::iterator it;

            for (it = GET_RECENT_KILLS(ch).begin();
                 it != GET_RECENT_KILLS(ch).end();++it)
                fprintf(ouf, "<recentkill vnum=\"%d\" times=\"%d\"/>\n",
                        it->_vnum,
                        it->_times);
        }
        if (!GET_GRIEVANCES(ch).empty()) {
            std_list<Grievance>::iterator it;

            for (it = GET_GRIEVANCES(ch).begin();
                 it != GET_GRIEVANCES(ch).end();++it)
                fprintf(ouf, "<grievance time=\"%lu\" player=\"%d\" reputation=\"%d\" kind=\"%s\"/>\n",
                        (long unsigned)it->_time,
                        it->_player_id,
                        it->_rep,
                        Grievance_kind_descs[it->_grievance]);
        }
	}

	fprintf(ouf, "</creature>\n");
	fclose(ouf);

    // on success, move temp file on top of the old file
    rename(tmp_path, path);

	// Now we get to put all that eq back on and reinstate the spell affects
	for (pos = 0;pos < NUM_WEARS;pos++) {
		if (saved_eq[pos])
			equip_char(ch, saved_eq[pos], pos, EQUIP_WORN);
		if (saved_impl[pos])
			equip_char(ch, saved_impl[pos], pos, EQUIP_IMPLANT);
		if (saved_tattoo[pos])
			equip_char(ch, saved_tattoo[pos], pos, EQUIP_TATTOO);
	}

	for (cur_aff = saved_affs;cur_aff;cur_aff = cur_aff->next)
		affect_modify(ch, cur_aff->location, cur_aff->modifier,
			cur_aff->bitvector, cur_aff->aff_index, true);
	affected = saved_affs;
	affect_total(ch);

	GET_HIT(ch) = MIN(GET_MAX_HIT(ch), hit);
	GET_MANA(ch) = MIN(GET_MAX_MANA(ch), mana);
	GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), move);
}// Saves the given characters equipment to a file. Intended for use while
// the character is still in the game.
bool
crashSave(struct creature *ch)
{
    ch->player_specials->rentcode = RENT_CRASH;
	ch->player_specials->rent_currency = ch->in_room->zone->time_frame;

    if(!save_player_objects(ch) )
        return false;

    REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
    save_player_to_xml(ch);
    return true;
}

bool
loadFromXML(struct creature *ch,  long id )
{
    return loadFromXML(get_player_file_path(id));
}
/* copy data from the file structure to a struct creature */
bool
loadFromXML(struct creature *ch,  const char *path )
{
	char *txt;
	int idx;

	if( access(path, W_OK) ) {
		errlog("Unable to open xml player file '%s': %s", path, strerror(errno) );
		return false;
	}
    xmlDocPtr doc = xmlParseFile(path);
    if (!doc) {
        errlog("XML parse error while loading %s", path);
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        xmlFreeDoc(doc);
        errlog("XML file %s is empty", path);
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
        } else if ( xmlMatches(node->name, "stats") ) {
            player.level = xmlGetIntProp(node, "level");
            player.height = xmlGetIntProp(node, "height");
            player.weight = xmlGetIntProp(node, "weight");
            GET_ALIGNMENT(ch) = xmlGetIntProp(node, "align");

            GET_SEX(ch) = 0;
            char *sex = xmlGetProp(node, "sex");
            if( sex != NULL )
                GET_SEX(ch) = search_block(sex, genders, false);
			free(sex);

            GET_RACE(ch) = 0;
            char *race = xmlGetProp(node, "race");
            if( race != NULL )
                GET_RACE(ch) = search_block(race, player_race, false);
			free(race);

        } else if ( xmlMatches(node->name, "class") ) {
			GET_OLD_CLASS(ch) = GET_REMORT_CLASS(ch) = GET_CLASS(ch) = -1;

            char *trade = xmlGetProp(node, "name");
            if( trade != NULL ) {
                GET_CLASS(ch) = search_block(trade, class_names, false);
				free(trade);
			}

            trade = xmlGetProp(node, "remort");
            if( trade != NULL ) {
                GET_REMORT_CLASS(ch) = search_block(trade, class_names, false);
				free(trade);
			}

			if( IS_CYBORG(ch) ) {
				char *subclass = xmlGetProp( node, "subclass" );
				if( subclass != NULL ) {
					GET_OLD_CLASS(ch) = search_block( subclass,
														borg_subchar_class_names,
														false);
					free(subclass);
				}
			}

			if (GET_CLASS(ch) == CLASS_MAGE ) {
				player_specials->saved.mana_shield_low =
					xmlGetLongProp(node, "manash_low");
				player_specials->saved.mana_shield_pct =
					xmlGetLongProp(node, "manash_pct");
			}

			GET_REMORT_GEN(ch) = xmlGetIntProp( node, "gen" );
			GET_TOT_DAM(ch) = xmlGetIntProp( node, "total_dam" );

        } else if ( xmlMatches(node->name, "time") ) {
            player.time.birth = xmlGetLongProp(node, "birth");
            player.time.death = xmlGetLongProp(node, "death");
            player.time.played = xmlGetLongProp(node, "played");
            player.time.logon = xmlGetLongProp(node, "last");
        } else if ( xmlMatches(node->name, "carnage") ) {
            GET_PKILLS(ch) = xmlGetIntProp(node, "pkills");
            GET_ARENAKILLS(ch) = xmlGetIntProp(node, "akills");
            GET_MOBKILLS(ch) = xmlGetIntProp(node, "mkills");
            GET_PC_DEATHS(ch) = xmlGetIntProp(node, "deaths");
			ch->set_reputation(xmlGetIntProp(node, "reputation"));
			GET_SEVERITY(ch) = xmlGetIntProp(node, "severity");
        } else if ( xmlMatches(node->name, "attr") ) {
            aff_abils.str = real_abils.str = xmlGetIntProp(node, "str");
            aff_abils.str_add = real_abils.str_add = xmlGetIntProp(node, "stradd");
            aff_abils.intel = real_abils.intel = xmlGetIntProp(node, "int");
            aff_abils.wis = real_abils.wis = xmlGetIntProp(node, "wis");
            aff_abils.dex = real_abils.dex = xmlGetIntProp(node, "dex");
            aff_abils.con = real_abils.con = xmlGetIntProp(node, "con");
            aff_abils.cha = real_abils.cha = xmlGetIntProp(node, "cha");
        } else if ( xmlMatches(node->name, "condition") ) {
			GET_COND(ch, THIRST) = xmlGetIntProp(node, "thirst");
			GET_COND(ch, FULL) = xmlGetIntProp(node, "hunger");
			GET_COND(ch, DRUNK) = xmlGetIntProp(node, "drunk");
       	} else if ( xmlMatches(node->name, "player") ) {
			GET_WIMP_LEV(ch) = xmlGetIntProp(node, "wimpy");
			GET_LIFE_POINTS(ch) = xmlGetIntProp(node, "lp");
			GET_CLAN(ch) = xmlGetIntProp(node, "clan");
        } else if ( xmlMatches(node->name, "home") ) {
			GET_HOME(ch) = xmlGetIntProp(node, "town");
			GET_HOMEROOM(ch) = xmlGetIntProp(node, "homeroom");
			GET_LOADROOM(ch) = xmlGetIntProp(node, "loadroom");
        } else if ( xmlMatches(node->name, "quest") ) {
			GET_QUEST(ch) = xmlGetIntProp(node, "current");
			GET_IMMORT_QP(ch) = xmlGetIntProp(node, "points");
			GET_QUEST_ALLOWANCE(ch) = xmlGetIntProp(node, "allowance");
        } else if ( xmlMatches(node->name, "bits") ) {
			char* flag = xmlGetProp( node, "flag1" );
			char_specials.saved.act = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.plr2_bits = hex2dec(flag);
			free(flag);
        } else if (xmlMatches(node->name, "frozen")) {
            ch->player_specials->thaw_time = xmlGetIntProp(node, "thaw_time");
            ch->player_specials->freezer_id = xmlGetIntProp(node, "freezer_id");
        } else if ( xmlMatches(node->name, "prefs") ) {
			char* flag = xmlGetProp( node, "flag1" );
			player_specials->saved.pref = hex2dec(flag);
			free(flag);

			flag = xmlGetProp( node, "flag2" );
			player_specials->saved.pref2 = hex2dec(flag);
			free(flag);

            flag = xmlGetProp(node, "tongue");
            if (flag)
                GET_TONGUE(ch) = find_tongue_idx_by_name(flag);
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
			set_title(ch, txt);
            free(txt);
        } else if ( xmlMatches(node->name, "affect") ) {
			affected_type af;
            memset(&af, 0x0, sizeof(affected_type));
			af.type = xmlGetIntProp( node, "type" );
			af.duration = xmlGetIntProp( node, "duration" );
			af.modifier = xmlGetIntProp( node, "modifier" );
			af.location = xmlGetIntProp( node, "location" );
			af.level = xmlGetIntProp( node, "level" );
			af.aff_index = xmlGetIntProp( node, "index" );
            af.owner = xmlGetIntProp(node, "owner");
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

			affect_to_char(ch, &af);

        } else if ( xmlMatches(node->name, "affects") ) {
			// PCs shouldn't have ANY perm affects
			if (IS_NPC(ch)) {
				char* flag = xmlGetProp( node, "flag1" );
				AFF_FLAGS(ch) = hex2dec(flag);
				free(flag);

				flag = xmlGetProp( node, "flag2" );
				AFF2_FLAGS(ch) = hex2dec(flag);
				free(flag);

				flag = xmlGetProp( node, "flag3" );
				AFF3_FLAGS(ch) = hex2dec(flag);
				free(flag);
			} else {
				AFF_FLAGS(ch) = 0;
				AFF2_FLAGS(ch) = 0;
				AFF3_FLAGS(ch) = 0;
			}

        } else if ( xmlMatches(node->name, "skill") ) {
			char *spellName = xmlGetProp( node, "name" );
			int index = str_to_spell( spellName );
			if( index >= 0 ) {
				GET_SKILL( ch, index ) = xmlGetIntProp( node, "level" );
			}
			free(spellName);
        } else if ( xmlMatches(node->name, "tongue") ) {
			char *tongue = xmlGetProp( node, "name" );
			int index = find_tongue_idx_by_name(tongue);
			if( index >= 0 )
                SET_TONGUE(ch, index,
                           MIN(100, xmlGetIntProp(node, "level")));
			free(tongue);
        } else if ( xmlMatches(node->name, "alias") ) {
			alias_data *alias;
			CREATE(alias, struct alias_data, 1);
			alias->type = xmlGetIntProp(node, "type");
			alias->alias = xmlGetProp(node, "alias");
			alias->replacement = xmlGetProp(node, "replace");
			if( alias->alias == NULL || alias->replacement == NULL ) {
				free(alias);
			} else {
				add_alias(ch,alias);
			}
		} else if ( xmlMatches(node->name, "description" ) ) {
			txt = (char *)xmlNodeGetContent(node);
			player.description = strdup(tmp_gsub(txt, "\n", "\r\n"));
			free(txt);
        } else if ( xmlMatches(node->name, "poofin") ) {
			POOFIN(ch) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "poofout") ) {
			POOFOUT(ch) = (char*)xmlNodeGetContent( node );
        } else if ( xmlMatches(node->name, "immort") ) {
			txt = xmlGetProp(node, "badge");
			strncpy(BADGE(ch), txt, 7);
			BADGE(ch)[7] = '\0';
			free(txt);

			GET_QLOG_LEVEL(ch) = xmlGetIntProp(node, "qlog");
			GET_INVIS_LVL(ch) = xmlGetIntProp(node, "invis");
        } else if (xmlMatches(node->name, "rent")) {
			char *txt;

			player_specials->rentcode = xmlGetIntProp(node, "code");
			player_specials->rent_per_day = xmlGetIntProp(node, "perdiem");
			player_specials->rent_currency = xmlGetIntProp(node, "currency");
			txt = (char *)xmlGetProp(node, "state");
			if (txt)
                player_specials->desc_mode = (cxn_state)search_block(txt, desc_modes, false);
            free(txt);
        } else if (xmlMatches(node->name, "recentkill")) {
            KillRecord kill;

            kill.set(xmlGetIntProp(node, "vnum"), xmlGetIntProp(node, "times"));
            GET_RECENT_KILLS(ch).push_back(kill);
        } else if (xmlMatches(node->name, "grievance")) {
            Grievance grievance;

            txt = (char *)xmlGetProp(node, "kind");
            if (txt) {
                grievance.set(xmlGetIntProp(node, "time"),
                              xmlGetIntProp(node, "player"),
                              xmlGetIntProp(node, "reputation"),
                              (Grievance_kind)search_block(txt, Grievance::kind_descs, false));
                GET_GRIEVANCES(ch).push_back(grievance);
            }
            free(txt);
		}
    }

    xmlFreeDoc(doc);

	// reset all imprint rooms
	for( int i = 0; i < MAX_IMPRINT_ROOMS; i++ )
		GET_IMPRINT_ROOM(ch, i) = -1;

    // Make sure the NPC flag isn't set
    if( IS_SET(char_specials.saved.act, MOB_ISNPC) ) {
		REMOVE_BIT(char_specials.saved.act, MOB_ISNPC);
		errlog("loadFromXML %s loaded with MOB_ISNPC bit set!",
			GET_NAME(ch));
	}

    // Check for freezer expiration
    if (PLR_FLAGGED(ch, PLR_FROZEN)
        && time(NULL) >= player_specials->thaw_time) {
        REMOVE_BIT(PLR_FLAGS(ch), PLR_FROZEN);
    }

	// If you're not poisioned and you've been away for more than an hour,
	// we'll set your HMV back to full
	if (!IS_POISONED(ch) && (((long)(time(0) - player.time.logon)) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(ch) = GET_MAX_HIT(ch);
		GET_MOVE(ch) = GET_MAX_MOVE(ch);
		GET_MANA(ch) = GET_MAX_MANA(ch);
	}

	if (GET_LEVEL(ch) >= 50) {
		for (idx = 0;idx < MAX_SKILLS;idx++)
            player_specials->saved.skills[idx] = 100;
		for (idx = 0;idx < MAX_TONGUES;idx++)
            language_data->tongues[idx] = 100;
	}

    return true;
}

void
set(struct creature *ch, const char *key, const char *val)
{
	if (!strcmp(key, "race"))
		GET_RACE(ch) = atoi(val);
	else if (!strcmp(key, "class"))
		GET_CLASS(ch) = atoi(val);
	else if (!strcmp(key, "remort"))
		GET_REMORT_CLASS(ch) = atoi(val);
	else if (!strcmp(key, "name"))
		GET_NAME(ch) = strdup(val);
	else if (!strcmp(key, "title"))
		GET_TITLE(ch) = strdup(val);
	else if (!strcmp(key, "poofin"))
		POOFIN(ch) = strdup(val);
	else if (!strcmp(key, "poofout"))
		POOFOUT(ch) = strdup(val);
	else if (!strcmp(key, "immbadge"))
		strcpy(BADGE(ch), val);
	else if (!strcmp(key, "sex"))
		GET_SEX(ch) = atoi(val);
	else if (!strcmp(key, "hitp"))
		GET_HIT(ch) = atoi(val);
	else if (!strcmp(key, "mana"))
		GET_MANA(ch) = atoi(val);
	else if (!strcmp(key, "move"))
		GET_MOVE(ch) = atoi(val);
	else if (!strcmp(key, "maxhitp"))
		GET_MAX_HIT(ch) = atoi(val);
	else if (!strcmp(key, "maxmana"))
		GET_MAX_MANA(ch) = atoi(val);
	else if (!strcmp(key, "maxmove"))
		GET_MAX_MOVE(ch) = atoi(val);
	else if (!strcmp(key, "gold"))
		GET_GOLD(ch) = atol(val);
	else if (!strcmp(key, "cash"))
		GET_CASH(ch) = atol(val);
	else if (!strcmp(key, "exp"))
		GET_EXP(ch) = atol(val);
	else if (!strcmp(key, "level"))
		GET_LEVEL(ch) = atol(val);
	else if (!strcmp(key, "height"))
		GET_HEIGHT(ch) = atol(val);
	else if (!strcmp(key, "weight"))
		GET_WEIGHT(ch) = atol(val);
	else if (!strcmp(key, "align"))
		GET_ALIGNMENT(ch) = atoi(val);
	else if (!strcmp(key, "gen"))
		GET_REMORT_GEN(ch) = atoi(val);
	else if (!strcmp(key, "birth_time"))
		player.time.birth = atol(val);
	else if (!strcmp(key, "death_time"))
		player.time.death = atol(val);
	else if (!strcmp(key, "played_time"))
		player.time.played = atol(val);
	else if (!strcmp(key, "login_time"))
		player.time.logon = atol(val);
	else if (!strcmp(key, "pkills"))
		GET_PKILLS(ch) = atol(val);
	else if (!strcmp(key, "mkills"))
		GET_MOBKILLS(ch) = atol(val);
	else if (!strcmp(key, "akills"))
		GET_ARENAKILLS(ch) = atol(val);
	else if (!strcmp(key, "deaths"))
		GET_PC_DEATHS(ch) = atol(val);
	else if (!strcmp(key, "reputation"))
		ch->set_reputation(atoi(val));
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
		GET_COND(ch, FULL) = atoi(val);
	else if (!strcmp(key, "thirst"))
		GET_COND(ch, THIRST) = atoi(val);
	else if (!strcmp(key, "drunk"))
		GET_COND(ch, DRUNK) = atoi(val);
	else
		slog("Invalid player field %s set to %s", key, val);
}
