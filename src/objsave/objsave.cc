/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright ( C ) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright ( C ) 1990, 1991.               *
************************************************************************ */

//
// File: objsave.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "house.h"
#include "bomb.h"

/* these factors should be unique integers */
#define RENT_FACTOR         1
#define CRYO_FACTOR         4

extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern struct time_info_data time_info;
extern int top_of_p_table;
extern int min_rent_cost;
extern int no_plrtext;

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
void perform_tell(struct char_data *ch, struct char_data *vict, char *buf);
int write_plrtext(struct obj_data *obj, FILE * fl);

struct obj_data *
Obj_from_store(FILE * fl, bool allow_inroom)
{

	struct obj_data *obj, *tmpo;
	struct obj_file_elem object;
	int j, tmpsize;

	if ((tmpsize = fread(&object, sizeof(struct obj_file_elem), 1, fl)) != 1) {
		if (ferror(fl)) {
			sprintf(buf, "Error reading object in Obj_from_store: %s.",
				strerror(errno));
			slog(buf);
		}
		return NULL;
	}

	if (object.item_number < 0) {
		sprintf(buf,
			"Obj_from_store found object vnum %d in file.  short_desc=%s. name=%s",
			object.item_number, object.short_desc, object.name);
		slog(buf);
		return NULL;
	}

	obj = read_object(object.item_number);

	if (!obj) {
		sprintf(buf, "Object %d no longer in database.", object.item_number);
		slog(buf);
		return NULL;
	}

	if (!OBJ_APPROVED(obj)) {
		sprintf(buf, "Obj %s being junked from rent.", obj->short_description);
		extract_obj(obj);
		slog(buf);
		return NULL;
	}

	if (object.short_desc[0] != 0) {
		obj->short_description = str_dup(object.short_desc);
		sprintf(buf, "%s has been left here.", obj->short_description);
		obj->description = str_dup(CAP(buf));
	}
	if (object.name[0] != 0) {
		obj->name = str_dup(object.name);
	}


	GET_OBJ_VAL(obj, 0) = object.value[0];
	GET_OBJ_VAL(obj, 1) = object.value[1];
	GET_OBJ_VAL(obj, 2) = object.value[2];
	GET_OBJ_VAL(obj, 3) = object.value[3];
	GET_OBJ_EXTRA(obj) = object.extra_flags;
	GET_OBJ_EXTRA2(obj) = object.extra2_flags;
	GET_OBJ_EXTRA3(obj) = object.extra3_flags;
	GET_OBJ_TYPE(obj) = object.type;
	obj->soilage = object.soilage;

	GET_OBJ_SIGIL_IDNUM(obj) = object.sigil_idnum;
	GET_OBJ_SIGIL_LEVEL(obj) = object.sigil_level;

	obj->obj_flags.bitvector[0] = object.bitvector[0];
	obj->obj_flags.bitvector[1] = object.bitvector[1];
	obj->obj_flags.bitvector[2] = object.bitvector[2];

	obj->setWeight(object.weight);
	GET_OBJ_TIMER(obj) = object.timer;

	obj->worn_on = object.worn_on_position;

	obj->obj_flags.wear_flags = object.wear_flags;
	obj->obj_flags.max_dam = object.max_dam;
	obj->obj_flags.damage = object.damage;
	obj->obj_flags.material = object.material;

	if (allow_inroom == false) {
		if (object.in_room_vnum >= 0) {
			sprintf(buf,
				"Obj_from_store loading object %s found non-negative inroom %d!  JUNKING.",
				obj->short_description, object.in_room_vnum);
			slog(buf);
			extract_obj(obj);
			return NULL;
		}
		obj->in_room = NULL;
	} else {
		obj->in_room = real_room(object.in_room_vnum);
	}

	// read player text
	obj->plrtext_len = object.plrtext_len;

	if (obj->plrtext_len) {
		CREATE(obj->action_description, char, object.plrtext_len);

		if (!fread(obj->action_description, object.plrtext_len, 1, fl)) {
			obj->plrtext_len = 0;
			sprintf(buf, "Error reading Obj %s's plrtext.",
				obj->short_description);
			slog(buf);
			return NULL;
		}
	}

	for (j = 0; j < 3; j++)
		obj->obj_flags.bitvector[j] = object.bitvector[j];

	for (j = 0; j < MAX_OBJ_AFFECT; j++)
		obj->affected[j] = object.affected[j];

	obj->contains = NULL;

	for (j = 0; j < object.contains; j++) {
		if ((tmpo = Obj_from_store(fl, false)) == NULL) {
			perror("SYSERR: Error in obj file: contain field: ");
		} else {
			tmpo->in_obj = obj;
			tmpo->next_content = obj->contains;
			obj->contains = tmpo;
		}
	}

	return obj;
}

/**
 * Deletes the objects or implants file for the given character.
 *
 * First complains if it doesnt have r/w access, then unlinks.
 */
int
Crash_delete_file(char *name, int mode)
{
	char filename[50];
	FILE *fl;

	if (!get_filename(name, filename, mode))	/* 0-rent 1-text 2-implant */
		return 0;

	if (!(fl = fopen(filename, "rb"))) {
		if (errno != ENOENT) {	/* if it fails but NOT because of no file */
			sprintf(buf1, "SYSERR: deleting %s file %s ( 1 )",
				mode ? "implant" : "crash", filename);
			perror(buf1);
		}
		return 0;
	}
	fclose(fl);

	if (unlink(filename) < 0) {
		if (errno != ENOENT) {	/* if it fails, NOT because of no file */
			sprintf(buf1, "SYSERR: deleting %s file %s ( 2 )",
				mode ? "implant" : "crash", filename);
			perror(buf1);
		}
	}
	return (1);
}

/**
 * Deletes this player's .objs file if the rentcode is RENT_CRASH
 */
int
Crash_delete_crashfile(struct char_data *ch)
{
	char fname[MAX_INPUT_LENGTH];
	struct rent_info rent;
	FILE *fl;

	if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
		return 0;
	if (!(fl = fopen(fname, "rb"))) {
		if (errno != ENOENT) {	/* if it fails, NOT because of no file */
			sprintf(buf1, "SYSERR: checking for crash file %s ( 3 )", fname);
			perror(buf1);
		}
		return 0;
	}
	if (!feof(fl))
		fread(&rent, sizeof(struct rent_info), 1, fl);
	fclose(fl);

	if (rent.rentcode == RENT_CRASH)
		Crash_delete_file(GET_NAME(ch), CRASH_FILE);

	return 1;
}

/**
 * Deletes .objs file for the given character if it is too old.
 */
int
Crash_clean_file(char *name)
{
	char fname[MAX_STRING_LENGTH], filetype[20];
	struct rent_info rent;
	extern int rent_file_timeout, crash_file_timeout;
	FILE *fl;

	if (!get_filename(name, fname, CRASH_FILE))
		return 0;
	/*
	 * open for write so that permission problems will be flagged now, at boot
	 * time.
	 */
	if (!(fl = fopen(fname, "r+b"))) {
		if (errno != ENOENT) {	/* if it fails, NOT because of no file */
			sprintf(buf1, "SYSERR: OPENING OBJECT FILE %s ( 4 ) [%d: %s]",
				fname, errno, strerror(errno));
			perror(buf1);
		}
		return 0;
	}
	if (!feof(fl)) {
		fread(&rent, sizeof(struct rent_info), 1, fl);
	}

	fclose(fl);

	if ((rent.rentcode == RENT_CRASH) ||
		(rent.rentcode == RENT_FORCED) || (rent.rentcode == RENT_TIMEDOUT)) {
		if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
			Crash_delete_file(name, CRASH_FILE);
			switch (rent.rentcode) {
			case RENT_CRASH:
				strcpy(filetype, "crash");
				break;
			case RENT_FORCED:
				strcpy(filetype, "forced rent");
				break;
			case RENT_TIMEDOUT:
				strcpy(filetype, "idlesave");
				break;
			default:
				strcpy(filetype, "UNKNOWN!");
				break;
			}
			sprintf(buf, "    Deleting %s's %s file.", name, filetype);
			slog(buf);
			return 1;
		}
		// Must retrieve rented items w/in 30 days
	} else if (rent.rentcode == RENT_RENTED)
		if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
			Crash_delete_file(name, CRASH_FILE);
			sprintf(buf, "    Deleting %s's rent file.", name);
			slog(buf);
			return 1;
		}
	return (0);
}


/**
 * Deletes .objs files for all known players if they are
 * too old.  
 *
 * Calls Crash_clean_file for every character in the player table.
 *
**/
void
update_obj_file(void)
{
	int i;

	for (i = 0; i <= top_of_p_table; i++)
		Crash_clean_file((player_table + i)->name);
	return;
}


/**
 * prints the contents of 'name''s rent file into a buffer and pages
 * that buffer to 'ch'
**/
void
Crash_listrent(struct char_data *ch, char *name)
{
	FILE *fl;
	char fname[MAX_INPUT_LENGTH];
	char linebuf[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct obj_file_elem object;
	struct obj_data *obj;
	struct rent_info rent;
	int mode = 0;

	name = two_arguments(name, buf, buf2);
	if (*buf2 && is_abbrev(buf2, "implants"))
		mode = MODE_IMPLANT;

	if (!get_filename(buf, fname, (mode ? IMPLANT_FILE : CRASH_FILE)))
		return;
	if (!(fl = fopen(fname, "rb"))) {
		sprintf(buf, "%s has no %s file.\r\n", buf, mode ? "implant" : "rent");
		send_to_char(buf, ch);
		return;
	}
	sprintf(buf, "%s\r\n", fname);
	if (mode == MODE_EQ) {
		if (!feof(fl))
			fread(&rent, sizeof(struct rent_info), 1, fl);
		strcat(buf, CCRED(ch, C_NRM));
		strcat(buf, CCBLD(ch, C_CMP));
		switch (rent.rentcode) {
		case RENT_RENTED:
			strcat(buf, "Rent");
			break;
		case RENT_CRASH:
			strcat(buf, "Crash");
			break;
		case RENT_CRYO:
			strcat(buf, "Cryo");
			break;
		case RENT_TIMEDOUT:
		case RENT_FORCED:
			strcat(buf, "TimedOut");
			break;
		default:
			strcat(buf, "Undef");
			break;
		}
		strcat(buf, CCNRM(ch, C_CMP));
		sprintf(buf, "%s %2d items :: %s%7d%s/day  %s%8d%s gold.  %s%8d%s bank\r\n", buf, 0,	//rent.nitems,
			CCYEL(ch, C_NRM), rent.net_cost_per_diem, CCNRM(ch, C_NRM),
			CCYEL(ch, C_NRM), rent.gold, CCNRM(ch, C_NRM),
			CCYEL(ch, C_NRM), rent.account, CCNRM(ch, C_NRM));
	}
	while (fread(&object, sizeof(struct obj_file_elem), 1, fl)) {
		if (ferror(fl)) {
			fclose(fl);
			return;
		}
		if (object.plrtext_len)
			fseek(fl, object.plrtext_len, SEEK_CUR);

		if ((obj = real_object_proto(object.item_number))) {
			sprintf(linebuf,
				" %s[%s%5d%s]%s %s( %s%5dau%s )%s %s%-30s%s%s%s%s%s%s%s\r\n",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				object.item_number,
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
				GET_OBJ_RENT(obj),
				CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
				CCGRN(ch, C_NRM),
				strlen(object.short_desc) ? object.short_desc : obj->
				short_description, CCNRM(ch, C_NRM),
				IS_SET(object.extra2_flags, ITEM2_BROKEN) ? "<brk>" : "",
				CCRED(ch, C_NRM), isname("imm", object.name) ? " I_E" : "",
				object.name[0] ? " RaL" : "",
				object.short_desc[0] ? " ReN" : "", CCNRM(ch, C_NRM));
			if (strlen(buf) + strlen(linebuf) + 100 < MAX_STRING_LENGTH) {
				strcat(buf, linebuf);
			} else {
				if (strlen(buf) + 20 < MAX_STRING_LENGTH) {
					strcat(buf, "*** OVERFLOW ***");
				}
				break;
			}
		}
	}
	page_string(ch->desc, buf, 1);
	fclose(fl);
}


/**
 * Writes the given rent_info into the given FILE
 */
int
write_rentinfo(FILE * fl, struct rent_info *rent)
{
	if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1) {
		perror("Writing rent code.");
		return 0;
	}
	return 1;
}



int
Crash_load(struct char_data *ch)
/* return values:
        0 - successful load, keep char in rent room.
        1 - load failure or load of crash items -- put char in temple.
        2 - rented equipment lost ( no $ )
*/
{

	FILE *fl;
	char fname[MAX_STRING_LENGTH];
	struct obj_data *tmpo;
	struct rent_info rent;
	int cost = 0, orig_rent_code;
	float num_of_days;
	int tmpi;
	int cont = 0;
	int num_lost = 0;

	if (get_filename(GET_NAME(ch), fname, IMPLANT_FILE)) {
		if ((fl = fopen(fname, "r+b"))) {
			while (!feof(fl) || !cont) {
				tmpo = Obj_from_store(fl, false);
				if (tmpo && tmpo->worn_on >= 0) {
					tmpi = tmpo->worn_on;
					tmpo->worn_on = -1;
					equip_char(ch, tmpo, tmpi, MODE_IMPLANT);
				} else if (tmpo)
					obj_to_char(tmpo, ch);
				else
					cont = 1;

				if (ferror(fl)) {
					perror("Reading implant file: Crash_load.");
					fclose(fl);
					cont = 1;
				}
			}
			fclose(fl);
		}
	}

	if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
		return 1;

	if (!(fl = fopen(fname, "r+b"))) {
		if (errno != ENOENT) {	/* if it fails, NOT because of no file */
			sprintf(buf1, "SYSERR: READING OBJECT FILE %s ( 5 )", fname);
			perror(buf1);
			send_to_char
				("\r\n********************* NOTICE *********************\r\n"
				"There was a problem loading your objects from disk.\r\n"
				"Contact a God for assistance.\r\n", ch);
		}
		sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
		return 1;
	}
	if (!feof(fl))
		fread(&rent, sizeof(struct rent_info), 1, fl);

	if (rent.rentcode == RENT_RENTED ||
		rent.rentcode == RENT_TIMEDOUT || rent.rentcode == RENT_FORCED) {

		num_of_days = (float)(time(0) - rent.time) / SECS_PER_REAL_DAY;
		cost = (int)(rent.net_cost_per_diem * num_of_days);

		// immortals dont have to pay rent
		if (GET_LEVEL(ch) < LVL_IMMORT) {
			// costs credits
			if (rent.currency == TIME_ELECTRO) {
				if (cost < GET_CASH(ch) + GET_ECONET(ch)) {
					GET_ECONET(ch) -= MAX(cost - GET_CASH(ch), 0);
					GET_CASH(ch) = MAX(GET_CASH(ch) - cost, 0);
					cost = 0;
					save_char(ch, NULL);
				} else {
					sprintf(buf,
						"%s entering game, some rented eq lost ( no creds ). -- %d/day, %f days",
						GET_NAME(ch), rent.net_cost_per_diem, num_of_days);
					mudlog(buf, BRF, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)),
						TRUE);
				}
			} else {			// default costs gold
				if (cost < GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
					GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
					GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
					save_char(ch, NULL);
					cost = 0;
				} else {
					sprintf(buf,
						"%s entering game, some rented eq lost ( no gold ). -- %d/day, %f days",
						GET_NAME(ch), rent.net_cost_per_diem, num_of_days);
					mudlog(buf, BRF, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)),
						TRUE);
				}
			}					// Currency
		}
	}
	// set buf in the switch and log it at the end
	switch (orig_rent_code = rent.rentcode) {
	case RENT_RENTED:
		if (!cost)				// rent is paid in full, normal un-rent status
			sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
		else
			*buf = 0;
		break;
	case RENT_CRASH:
		sprintf(buf, "%s retrieving crash-saved items and entering game.",
			GET_NAME(ch));
		GET_HIT(ch) = GET_MAX_HIT(ch);	//  if it crashes, give them 
		GET_MOVE(ch) = GET_MAX_MOVE(ch);	//  a break.
		GET_MANA(ch) = GET_MAX_MANA(ch);
		break;
	case RENT_CRYO:
		sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
		break;
	case RENT_FORCED:
	case RENT_TIMEDOUT:
		sprintf(buf,
			"%sWARNING:%s Failure to rent before disconnecting has tripled your rent.\r\n",
			CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
		sprintf(buf, "%s retrieving force-saved items and entering game.",
			GET_NAME(ch));
		break;
	default:
		sprintf(buf, "WARNING: %s entering game with undefined rent code.",
			GET_NAME(ch));
		break;

	}
	if (*buf)
		mudlog(buf, BRF, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);

	// load objects from file
	while (!feof(fl)) {
		tmpo = Obj_from_store(fl, false);

		if (tmpo) {

			//  if we still owe money, take the object as payment, if possible
			if (cost > 0 && !IS_OBJ_STAT2(tmpo, ITEM2_NOREMOVE)
				&& !IS_OBJ_STAT(tmpo, ITEM_NODROP)) {

				// see if we're paying with credits
				if (rent.currency == TIME_ELECTRO) {
					sprintf(buf, "remobj( no creds ):%s, cost:%d, value:%d",
						tmpo->short_description,
						cost, recurs_obj_cost(tmpo, 1, NULL));
					slog(buf);
					cost -= (recurs_obj_cost(tmpo, true, NULL));
					extract_obj(tmpo);
					num_lost++;
				} else {		// default to gold
					sprintf(buf, "remobj( no gold ):%s, cost:%d, value:%d",
						tmpo->short_description,
						cost, recurs_obj_cost(tmpo, true, NULL));

					slog(buf);
					cost -= (recurs_obj_cost(tmpo, true, NULL));
					extract_obj(tmpo);
					num_lost++;
				}
			} else if (tmpo->worn_on >= 0) {	// object was stored as equipment, equip it on the char
				tmpi = tmpo->worn_on;
				tmpo->worn_on = -1;
				equip_char(ch, tmpo, tmpi, MODE_EQ);
			} else {			// object was stored as inventory, give it to the char
				obj_to_char(tmpo, ch);
			}
		}

		if (ferror(fl)) {
			perror("Reading crash file: Crash_load.");
			fclose(fl);
			return 1;
		}
	}
	fclose(fl);

	save_char(ch, NULL);

	if (num_lost)
		return 2;
	else if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
		return 0;
	else
		return 1;
}

/*
 * Stores the given object list into the given file
 * recursively via obj->next_content
 */
int
store_obj_list(struct obj_data *obj, FILE * fp)
{
	int result;

	if (obj) {
		store_obj_list(obj->next_content, fp);
		result = obj->save(fp);

		if (!result)
			return 0;
	}
	return TRUE;
}


void
Crash_restore_weight(struct obj_data *obj)
{
	if (obj) {
		Crash_restore_weight(obj->contains);
		Crash_restore_weight(obj->next_content);
		if (obj->in_obj)
			obj->in_obj->modifyWeight(obj->getWeight());
	}
}

/**
 * recursively extracts all the objects in 
 * this list and thier contents.
 */
void
extract_object_list(obj_data * head)
{
	if (!head)
		return;
	extract_object_list(head->contains);
	extract_object_list(head->next_content);
	extract_obj(head);
}


void
Crash_extract_norents(struct obj_data *obj)
{
	if (obj) {
		Crash_extract_norents(obj->contains);
		Crash_extract_norents(obj->next_content);
		if (obj->isUnrentable() &&
			(!obj->worn_by || !IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) &&
			!IS_OBJ_STAT(obj, ITEM_NODROP))
			extract_obj(obj);
	}
}

void
Crash_calculate_rent(struct obj_data *obj, int *cost)
{
	if (obj) {
		*cost += MAX(0, GET_OBJ_RENT(obj));
		Crash_calculate_rent(obj->contains, cost);
		Crash_calculate_rent(obj->next_content, cost);
	}
}

/*
 * Stores implants into the player's IMPLANT_FILE and extracts them.
 * if extract is true, extract implants after saving them.
 */
void
Crash_save_implants(struct char_data *ch, bool extract = true)
{

	FILE *fp = 0;

	if (!get_filename(GET_NAME(ch), buf, IMPLANT_FILE))
		return;
	if (!(fp = fopen(buf, "w")))
		return;
	for (int j = 0; j < NUM_WEARS; j++)
		if (GET_IMPLANT(ch, j)) {
			if (store_obj_list(GET_IMPLANT(ch, j), fp)) {
				if (extract)
					extract_object_list(GET_IMPLANT(ch, j));
			} else {
				fclose(fp);
				return;
			}
		}
	fclose(fp);
}

/** 
 * Saves the given character and it's equipment to a file. 
 * Does not extract any eq or implants. 
 **/
void
Crash_crashsave(struct char_data *ch)
{
	char buf[MAX_INPUT_LENGTH];
	struct rent_info rent;
	int j;
	FILE *fp;

	if (IS_NPC(ch))
		return;

	if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
		return;
	if (!(fp = fopen(buf, "wb")))
		return;

	rent.rentcode = RENT_CRASH;
	rent.time = time(0);
	rent.currency = ch->in_room->zone->time_frame;

	if (!write_rentinfo(fp, &rent)) {
		fclose(fp);
		return;
	}
	if (!store_obj_list(ch->carrying, fp)) {
		fclose(fp);
		return;
	}

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j)) {
			if (!store_obj_list(GET_EQ(ch, j), fp)) {
				fclose(fp);
				return;
			}
		}
	fclose(fp);

	Crash_save_implants(ch, false);

	REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


/** 
 * Extracts unrentables.
 * Saves and extracts all eq & implants
 * sets cost per day to cost
 * Sets rent code.
**/
void
Crash_rentsave(struct char_data *ch, int cost, int rentcode)
{
	char buf[MAX_INPUT_LENGTH];
	struct rent_info rent;
	int j;
	FILE *fp;

	if (IS_NPC(ch))
		return;

	if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
		return;
	if (!(fp = fopen(buf, "wb")))
		return;

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			Crash_extract_norents(GET_EQ(ch, j));

	Crash_extract_norents(ch->carrying);

	rent.net_cost_per_diem = cost;
	rent.rentcode = rentcode;
	rent.time = time(0);
	rent.gold = GET_GOLD(ch);
	rent.account = GET_BANK_GOLD(ch);
	rent.currency = ch->in_room->zone->time_frame;
	if (!write_rentinfo(fp, &rent)) {
		fclose(fp);
		return;
	}

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			if (store_obj_list(GET_EQ(ch, j), fp))
				extract_object_list(GET_EQ(ch, j));

	if (store_obj_list(ch->carrying, fp))
		extract_object_list(ch->carrying);

	fclose(fp);

	Crash_save_implants(ch);
}

void
Crash_idlesave(struct char_data *ch)
{
	int cost = 0;
	Crash_calculate_rent(ch->carrying, &cost);
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		cost = 0;
	}
	// INCREASE
	Crash_rentsave(ch, (cost * 3), RENT_FORCED);
}

/**
 *  Saves the char into it's rent file, extracting all cursed eq in the
 *  process.
**/
void
Crash_cursesave(struct char_data *ch)
{
	char buf[MAX_INPUT_LENGTH];
	struct rent_info rent;
	int j;
	FILE *fp;
	struct obj_data *obj, *next_obj;

	if (IS_NPC(ch))
		return;

	if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
		return;
	if (!(fp = fopen(buf, "wb")))
		return;

	rent.net_cost_per_diem = 0;
	rent.rentcode = RENT_RENTED;
	rent.time = time(0);
	rent.gold = GET_GOLD(ch);
	rent.account = GET_BANK_GOLD(ch);
	rent.currency = ch->in_room->zone->time_frame;

	if (!write_rentinfo(fp, &rent)) {
		fclose(fp);
		return;
	}

	for (j = 0; j < NUM_WEARS; j++) {
		if (GET_EQ(ch, j)) {
			if (IS_OBJ_STAT(GET_EQ(ch, j), ITEM_NODROP) ||
				IS_OBJ_STAT2(GET_EQ(ch, j), ITEM2_NOREMOVE)) {

				// the item is cursed, but its contents cannot be (normally)
				while (GET_EQ(ch, j)->contains)
					extract_object_list(GET_EQ(ch, j)->contains);

				if (store_obj_list(GET_EQ(ch, j), fp))
					extract_object_list(GET_EQ(ch, j));
			}
		}
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (IS_OBJ_STAT(obj, ITEM_NODROP)) {

			// the item is cursed, but its contents cannot be (normally)
			while (obj->contains)
				extract_object_list(obj->contains);

			if (obj->save(fp))
				extract_obj(obj);
		}

	}

	fclose(fp);

	// save implants
	Crash_save_implants(ch);
	REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);

}


void
Crash_cryosave(struct char_data *ch, int cost)
{
	char buf[MAX_INPUT_LENGTH];
	struct rent_info rent;
	int j;
	FILE *fp;

	if (IS_NPC(ch))
		return;

	if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
		return;
	if (!(fp = fopen(buf, "wb")))
		return;

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			Crash_extract_norents(GET_EQ(ch, j));

	Crash_extract_norents(ch->carrying);

	if (ch->in_room->zone->time_frame == TIME_ELECTRO)
		GET_CASH(ch) = MAX(0, GET_CASH(ch) - cost);
	else
		GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

	rent.rentcode = RENT_CRYO;
	rent.time = time(0);
	rent.gold = GET_GOLD(ch);
	rent.account = GET_BANK_GOLD(ch);
	rent.net_cost_per_diem = 0;
	rent.currency = ch->in_room->zone->time_frame;

	if (!write_rentinfo(fp, &rent)) {
		fclose(fp);
		return;
	}

	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j))
			if (store_obj_list(GET_EQ(ch, j), fp))
				extract_object_list(GET_EQ(ch, j));

	if (!store_obj_list(ch->carrying, fp)) {
		fclose(fp);
		return;
	}
	fclose(fp);

	extract_object_list(ch->carrying);

	Crash_save_implants(ch);

	SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist                                          *
************************************************************************* */

void
Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost)
{
	long rent_deadline;

	if (!cost)
		return;

	if (ch->in_room->zone->time_frame == TIME_ELECTRO)
		rent_deadline = ((GET_CASH(ch) + GET_ECONET(ch)) / cost);
	else
		rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);

	sprintf(buf2,
		"You can rent for %ld day%s with the money you have on hand and in the bank.",
		rent_deadline, (rent_deadline > 1) ? "s" : "");
	if (recep)
		perform_tell(recep, ch, buf2);
	else {
		send_to_char(buf2, ch);
		send_to_char("\r\n", ch);
	}
}

int
Crash_report_unrentables(struct char_data *ch, struct char_data *recep,
	struct obj_data *obj)
{
	int has_norents = 0;

	if (obj) {
		if (obj->isUnrentable()) {
			has_norents = 1;
			if ((GET_OBJ_TYPE(obj) == ITEM_CIGARETTE ||
					GET_OBJ_TYPE(obj) == ITEM_PIPE)
				&& GET_OBJ_VAL(obj, 3))
				sprintf(buf2, "No smoking in the rooms... %s is still lit!",
					OBJS(obj, ch));
			else if (IS_BOMB(obj) && obj->contains && IS_FUSE(obj->contains) &&
				FUSE_STATE(obj->contains))
				sprintf(buf2, "You cannot store an active bomb ( %s ) here!!",
					obj->short_description);
			else
				sprintf(buf2, "You cannot store %s.", OBJS(obj, ch));

			if (recep)
				perform_tell(recep, ch, buf2);
			else
				send_to_char(strcat(buf2, "\r\n"), ch);
		}
		has_norents += Crash_report_unrentables(ch, recep, obj->contains);
		has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
	}
	return (has_norents);
}



void
Crash_report_rent(struct char_data *ch, struct char_data *recep,
	struct obj_data *obj, long *cost, long *nitems, int display, int factor)
{

	char curr[64];
	if (ch->in_room->zone->time_frame == TIME_ELECTRO)
		strcpy(curr, "credits");
	else
		strcpy(curr, "coins");

	if (obj) {
		if (!(obj->isUnrentable())) {
			(*nitems)++;
			*cost += (GET_OBJ_RENT(obj) * factor);
			if (display) {
				if (recep) {
					sprintf(buf2, "%5d %s for %s...",
						(GET_OBJ_RENT(obj) * factor), curr, OBJS(obj, ch));
					perform_tell(recep, ch, buf2);
				} else {
					sprintf(buf2, "Rent cost: %5d %s for %s.\r\n",
						(GET_OBJ_RENT(obj) * factor), curr, OBJS(obj, ch));
					send_to_char(buf2, ch);
				}
			}
		}
		Crash_report_rent(ch, recep, obj->contains, cost, nitems, display,
			factor);
		Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display,
			factor);
	}
}


int
Crash_rentcost(struct char_data *ch, int display, int factor)
{
//  extern int max_obj_save;        /* change in config.c */
	int i;
	long totalcost = 0, numitems = 0, norent = 0;
	char curr[64];

	if (ch->in_room->zone->time_frame == TIME_ELECTRO)
		strcpy(curr, "credits");
	else
		strcpy(curr, "coins");

	norent = Crash_report_unrentables(ch, NULL, ch->carrying);
	for (i = 0; i < NUM_WEARS; i++)
		norent += Crash_report_unrentables(ch, NULL, GET_EQ(ch, i));

	Crash_report_rent(ch, NULL, ch->carrying, &totalcost, &numitems,
		display, factor);

	for (i = 0; i < NUM_WEARS; i++)
		Crash_report_rent(ch, NULL, GET_EQ(ch, i), &totalcost, &numitems,
			display, factor);

/* 
  if ( !norent &&
       numitems > ( max_obj_save << 1 ) + GET_LEVEL( ch ) + ( GET_REMORT_GEN( ch ) << 3 ) ) {
      sprintf( buf, "Sorry, you cannot store more than %d items.\r\n"
              "You are carrying %ld.\r\n",
              ( max_obj_save << 1 ) + GET_LEVEL( ch ) + ( GET_REMORT_GEN( ch ) << 3 ),
              numitems );
      send_to_char( buf2, ch );
      norent++;
  }
*/
	totalcost = (int)(totalcost * (0.30) * ((10 + GET_LEVEL(ch)) / 10));

	if (!norent) {
		sprintf(buf, "It will cost you %ld %s per day to rent.\r\n",
			totalcost, curr);
		send_to_char(buf, ch);
		Crash_rent_deadline(ch, NULL, totalcost);
	}

	return (totalcost * (norent ? -1 : 1));
}


int
Crash_offer_rent(struct char_data *ch, struct char_data *receptionist,
	int display, int factor)
{
	extern int max_obj_save;	/* change in config.c */
	char buf[MAX_INPUT_LENGTH];
	int i;
	long totalcost = 0, numitems = 0, norent = 0;
	char curr[64];

	if (receptionist->in_room->zone->time_frame == TIME_ELECTRO)
		strcpy(curr, "credits");
	else
		strcpy(curr, "coins");


	norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
	for (i = 0; i < NUM_WEARS; i++)
		norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

	if (norent)
		return 0;
	totalcost = 0;

	Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems,
		display, factor);

	for (i = 0; i < NUM_WEARS; i++)
		Crash_report_rent(ch, receptionist, GET_EQ(ch, i), &totalcost,
			&numitems, display, factor);

	totalcost = (int)(totalcost * (0.30) * ((10 + GET_LEVEL(ch)) / 10));

	/* adds in the receptionist's fee =  level*10     */
	totalcost += (min_rent_cost * GET_LEVEL(ch)) * factor;

	totalcost += (totalcost * time_info.day) / 1000;
	totalcost += (totalcost * time_info.month) / 200;

	if (!numitems) {
		perform_tell(receptionist, ch,
			"But you are not carrying anything!  Just quit!");
		return (0);
	}
	if (numitems > max_obj_save + GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3)) {
		sprintf(buf2, "Sorry, but I cannot store more than %d items.",
			max_obj_save + GET_LEVEL(ch) + (GET_REMORT_GEN(ch) << 3));
		perform_tell(receptionist, ch, buf2);
		sprintf(buf2, "You are carrying %ld.", numitems);
		perform_tell(receptionist, ch, buf2);
		return (0);
	}
	if (display) {
		sprintf(buf2, "Plus, my %d %s insurance fee..",
			(min_rent_cost * GET_LEVEL(ch)) * factor, curr);
		perform_tell(receptionist, ch, buf2);
		strcpy(buf, CCRED(ch, C_NRM));
		sprintf(buf2,
			"%s tells you,%s%s 'I'll cut you a deal for %ld %s%s.'%s\r\n",
			PERS(receptionist, ch), CCGRN(ch, C_NRM), CCBLD(ch, C_SPR),
			totalcost, curr, (factor == RENT_FACTOR ? " per day" : ""),
			CCNRM(ch, C_SPR));
		strcat(buf, CAP(buf2));
		send_to_char(buf, ch);

		if ((receptionist->in_room->zone->time_frame == TIME_ELECTRO &&
				totalcost > GET_CASH(ch)) ||
			(receptionist->in_room->zone->time_frame != TIME_ELECTRO &&
				totalcost > GET_GOLD(ch))) {
			perform_tell(receptionist, ch, "...which I see you can't afford.");
			return (0);
		} else if (factor == RENT_FACTOR)
			Crash_rent_deadline(ch, receptionist, totalcost);
	}
	return (totalcost);
}

int
gen_receptionist(struct char_data *ch, struct char_data *recep,
	int cmd, char *arg, int mode)
{
	int cost = 0;
	extern int free_rent;
	struct room_data *save_room;
	char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
		"cough", "fart", "twiddle", "yawn"
	};
	char curr[64];

	ACMD(do_action);

	if (recep->in_room->zone->time_frame == TIME_ELECTRO)
		strcpy(curr, "credits");
	else
		strcpy(curr, "coins");

	if (!ch->desc || IS_NPC(ch))
		return FALSE;

	if (!cmd && !number(0, 5)) {
		do_action(recep, "", find_command(action_table[number(0, 8)]), 0);
		return FALSE;
	}
	if (!CMD_IS("offer") && !CMD_IS("rent"))
		return FALSE;
	if (!AWAKE(recep)) {
		act("$E is unable to talk to you...", FALSE, ch, 0, recep, TO_CHAR);
		return TRUE;
	}
	if (!CAN_SEE(recep, ch) && GET_LEVEL(ch) <= LVL_AMBASSADOR) {
		act("$n says, 'I don't deal with people I can't see!'", FALSE, recep,
			0, 0, TO_ROOM);
		return TRUE;
	}
	if (free_rent) {
		perform_tell(recep, ch,
			"Rent is free here.  Just quit, and your objects will be saved!");
		return 1;
	}
	if (PLR_FLAGGED(ch, PLR_KILLER) || PLR_FLAGGED(ch, PLR_THIEF)) {
		perform_tell(recep, ch, "I don't deal with KILLERS and THIEVES.");
		return 1;
	}
	if (PLR_FLAGGED(ch, PLR_QUESTOR)) {
		send_to_char("Please remove your questor flag first.\r\n", ch);
		return 1;
	}
	if (CMD_IS("rent")) {
		if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
			return TRUE;
		if (mode == RENT_FACTOR)
			sprintf(buf2, "Rent will cost you %d %s per day.", cost, curr);
		else if (mode == CRYO_FACTOR)
			sprintf(buf2, "It will cost you %d %s to be frozen.", cost, curr);
		perform_tell(recep, ch, buf2);
		if ((recep->in_room->zone->time_frame == TIME_ELECTRO &&
				cost > GET_CASH(ch)) ||
			(recep->in_room->zone->time_frame != TIME_ELECTRO &&
				cost > GET_GOLD(ch))) {
			perform_tell(recep, ch, "...which I see you can't afford.");
			return TRUE;
		}
		if (cost && (mode == RENT_FACTOR))
			Crash_rent_deadline(ch, recep, cost);

		if (mode == RENT_FACTOR) {
			act("$n stores your belongings and helps you into your private chamber.", FALSE, recep, 0, ch, TO_VICT);
			Crash_rentsave(ch, cost, RENT_RENTED);
			sprintf(buf, "%s has rented ( %d/day, %d tot. )", GET_NAME(ch),
				cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
		} else {				/* cryo */
			act("$n stores your belongings and helps you into your private chamber.\r\n" "A white mist appears in the room, chilling you to the bone...\r\n" "You begin to lose consciousness...", FALSE, recep, 0, ch, TO_VICT);
			Crash_cryosave(ch, cost);
			sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
			SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
		}

		mudlog(buf, NRM, MAX(LVL_AMBASSADOR, GET_INVIS_LEV(ch)), TRUE);
		act("$n helps $N into $S private chamber.",
			FALSE, recep, 0, ch, TO_NOTVICT);
		save_room = ch->in_room;
		save_char(ch, save_room);
		ch->extract(true, false, CON_MENU);
	} else {
		Crash_offer_rent(ch, recep, TRUE, mode);
		act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
	}
	return TRUE;
}


SPECIAL(receptionist)
{
	if (spec_mode == SPECIAL_DEATH)
		return 0;
	return (gen_receptionist(ch, (struct char_data *)me, cmd, argument,
			RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
	if (spec_mode == SPECIAL_DEATH)
		return 0;
	return (gen_receptionist(ch, (struct char_data *)me, cmd, argument,
			CRYO_FACTOR));
}


void
Crash_save_all(void)
{
	struct descriptor_data *d;
	for (d = descriptor_list; d; d = d->next) {
		if ((IS_PLAYING(d)) && !IS_NPC(d->character)) {
			if (PLR_FLAGGED(d->character, PLR_CRASH)) {
				Crash_crashsave(d->character);
				save_char(d->character, NULL);
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
			}
		}
	}
}
