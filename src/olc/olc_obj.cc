//
// File: olc_obj.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <iomanip>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "security.h"
#include "olc.h"
#include "screen.h"
#include "spells.h"
#include "materials.h"
#include "specs.h"
#include "player_table.h"
#include "object_map.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern int olc_lock;
extern int *obj_index;
extern int top_of_objt;
extern char *olc_guide;
long asciiflag_conv(char *buf);

void num2str(char *str, int num);
void do_stat_object(struct Creature *ch, struct obj_data *obj);

int prototype_obj_value(struct obj_data *obj);
int set_maxdamage(struct obj_data *obj);
char *find_exdesc(char *word, struct extra_descr_data *list, int find_exact =
	0);

int add_path_to_vehicle(struct obj_data *obj, char *name);
int choose_material(struct obj_data *obj);

struct extra_descr_data *locate_exdesc(char *word,
	struct extra_descr_data *list);

const char *olc_oset_keys[] = {
	"alias",
	"name",
	"ldesc",
	"desc",
	"type",
	"extra1",		   /****** 5 ******/
	"extra2",
	"worn",
	"value",
	"material",
	"maxdamage",	   /****** 10 *****/
	"damage",
	"weight",
	"cost",
	"rent",
	"apply",		   /****** 15 *****/
	"affection",
	"action_desc",
	"special",
	"path",
	"soilage",					// 20
	"sigil",
	"extra3",
	"timer",
	"specparam",
	"owner",
	"\n"
};

int
write_obj_index(struct Creature *ch, struct zone_data *zone)
{
	int done = 0, i, j, found = 0, count = 0, *new_index;
	char fname[64];
	FILE *index;

	for (i = 0; obj_index[i] != -1; i++) {
		count++;
		if (obj_index[i] == zone->number) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		return (1);

	CREATE(new_index, int, count + 2);

	for (i = 0, j = 0;; i++) {
		if (obj_index[i] == -1) {
			if (done == 0) {
				new_index[j] = zone->number;
				new_index[j + 1] = -1;
			} else
				new_index[j] = -1;
			break;
		}
		if (obj_index[i] > zone->number && done != 1) {
			new_index[j] = zone->number;
			j++;
			new_index[j] = obj_index[i];
			done = 1;
		} else
			new_index[j] = obj_index[i];
		j++;
	}

	free(obj_index);

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	obj_index = new_index;

	sprintf(fname, "world/obj/index");
	if (!(index = fopen(fname, "w"))) {
		send_to_char(ch, "Could not open index file, object save aborted.\r\n");
		return (0);
	}

	for (i = 0; obj_index[i] != -1; i++)
		fprintf(index, "%d.obj\n", obj_index[i]);

	fprintf(index, "$\n");

	send_to_char(ch, "Object index file re-written.\r\n");

	fclose(index);

	return (1);
}


bool
save_objs(struct Creature *ch, struct zone_data *zone)
{
	size_t i, tmp;
	int aff_idx;
	room_num low = 0;
	room_num high = 0;
	char fname[64];
	char sbuf1[64], sbuf2[64], sbuf3[64], sbuf4[64];
	struct extra_descr_data *desc;
	struct obj_data *obj;
	FILE *file;
	FILE *realfile;

	if (!zone) {
		errlog("save_obj() called with NULL zone");
		return false;
	}

	sprintf(fname, "world/obj/%d.obj", zone->number);
	if ((access(fname, F_OK) >= 0) && (access(fname, W_OK) < 0)) {
		mudlog(0, BRF, true,
			"OLC: ERROR - Main object file for zone %d is read-only.",
			zone->number);
	}

	sprintf(fname, "world/obj/olc/%d.obj", zone->number);
	if (!(file = fopen(fname, "w")))
		return false;

	if ((write_obj_index(ch, zone)) != 1) {
		fclose(file);
		return false;
	}

	low = zone->number * 100;
	high = zone->top;

    ObjectMap::iterator oi;
    for (oi = objectPrototypes.lower_bound(low);
         oi != objectPrototypes.upper_bound(high); oi++) {
        obj = oi->second;
		fprintf(file, "#%d\n", obj->shared->vnum);
		if (obj->aliases)
			fprintf(file, "%s", obj->aliases);

		fprintf(file, "~\n");

		if (obj->name)
			fprintf(file, "%s", obj->name);

		fprintf(file, "~\n");

		if (obj->line_desc) {
			tmp = strlen(obj->line_desc);
			for (i = 0; i < tmp; i++)
				if (obj->line_desc[i] != '\r')
					fputc(obj->line_desc[i], file);
		}

		fprintf(file, "~\n");

		if (obj->action_desc) {
			tmp = strlen(obj->action_desc);
			for (i = 0; i < tmp; i++)
				if (obj->action_desc[i] != '\r')
					fputc(obj->action_desc[i], file);
		}
		fprintf(file, "~\n");

		num2str(sbuf1, obj->obj_flags.extra_flags);
		num2str(sbuf2, obj->obj_flags.extra2_flags);
		num2str(sbuf3, obj->obj_flags.wear_flags);
		num2str(sbuf4, obj->obj_flags.extra3_flags);

		//
		// TODO: decide how to handle this
		//

		fprintf(file, "%d %s %s %s %s\n", obj->obj_flags.type_flag,
			sbuf1, sbuf2, sbuf3, sbuf4);

		/*
		   fprintf(file,"%d %s %s %s %s\n", obj->obj_flags.type_flag,
		   sbuf1, sbuf2, sbuf3, sbuf4);
		 */

		for (i = 0; i < 4; i++) {
			fprintf(file, "%d", obj->obj_flags.value[i]);
			if (i < 3)
				fprintf(file, " ");
			else
				fprintf(file, "\n");
		}

		fprintf(file, "%d %d %d\n", obj->obj_flags.material,
			obj->obj_flags.max_dam, obj->obj_flags.damage);

		fprintf(file, "%d %d %d\n", obj->getWeight(),
			obj->shared->cost, obj->shared->cost_per_day);

		desc = obj->ex_description;
		while (desc != NULL) {
			if (!desc->keyword || !desc->description) {
				slog("OLCERROR: Extra Desc with no kywrd or desc, obj #%d.\n",
					obj->shared->vnum);
				send_to_char(ch,
					"I didn't save your bogus extra desc in obj %d.\r\n",
					obj->shared->vnum);
				desc = desc->next;
				continue;
			}
			fprintf(file, "E\n");
			fprintf(file, "%s~\n", desc->keyword);
			tmp = strlen(desc->description);
			for (i = 0; i < tmp; i++)
				if (desc->description[i] != '\r')
					fputc(desc->description[i], file);
			fprintf(file, "~\n");
			desc = desc->next;
		}

		for (aff_idx = 0; aff_idx < MAX_OBJ_AFFECT; aff_idx++) {
			if (obj->affected[aff_idx].location == APPLY_NONE)
				continue;
			fprintf(file, "A\n");
			fprintf(file, "%d %d\n", obj->affected[aff_idx].location,
				obj->affected[aff_idx].modifier);
		}


		for (i = 0; i < 3; i++) {
			if (obj->obj_flags.bitvector[i]) {
				num2str(sbuf1, obj->obj_flags.bitvector[i]);
				fprintf(file, "V\n");
				fprintf(file, "%d %s\n", (i + 1), sbuf1);
			}
		}

		if (GET_OBJ_PARAM(obj)) {
			char *str;

			str = tmp_gsub(GET_OBJ_PARAM(obj), "\r", "");
			str = tmp_gsub(str, "~", "!");
			fprintf(file, "P\n%s~\n", str);
		}
		// player id that owns the obj
		if( obj->shared->owner_id != 0 ) {
			fprintf(file, "O %ld \n", obj->shared->owner_id);
		}
		low++;
	}

	fprintf(file, "$\n");

	slog("OLC: %s osaved %d.", GET_NAME(ch), zone->number);

	sprintf(fname, "world/obj/%d.obj", zone->number);
	realfile = fopen(fname, "w");
	if (realfile) {
		fclose(file);
		sprintf(fname, "world/obj/olc/%d.obj", zone->number);
		if (!(file = fopen(fname, "r"))) {
			fclose(realfile);
			errlog("Failure to reopen olc obj file.");
			return false;
		}
		do {
			tmp = fread(buf, 1, 512, file);
			if (fwrite(buf, 1, tmp, realfile) != tmp) {
				fclose(realfile);
				fclose(file);
				errlog("Failure to duplicate olc obj file in the main wld dir.");
				return false;
			}
		} while (tmp == 512);
		fclose(realfile);
	}

	fclose(file);

	REMOVE_BIT(zone->flags, ZONE_OBJS_MODIFIED);
	return true;
}


struct obj_data *
do_create_obj(struct Creature *ch, int vnum)
{

	struct obj_data *obj = NULL, *new_obj = NULL;
	struct zone_data *zone = NULL;
	int i;

	if ((obj = real_object_proto(vnum))) {
		send_to_char(ch, "ERROR: Object already exists.\r\n");
		return NULL;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum >= zone->number * 100 && vnum <= zone->top)
			break;

	if (!zone) {
		send_to_char(ch, "ERROR: A zone must be defined for the object first.\r\n");
		return NULL;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Try creating objects in your own zone, luser.\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to CREATE obj %d.",
			GET_NAME(ch), vnum);
		return NULL;
	}

	if (!OLC_EDIT_OK(ch, zone, ZONE_OBJS_APPROVED)) {
		send_to_char(ch, "Object OLC is not approved for this zone.\r\n");
		return NULL;
	}

	CREATE(new_obj, struct obj_data, 1);
	new_obj->clear();
	CREATE(new_obj->shared, struct obj_shared_data, 1);
	new_obj->shared->vnum = vnum;
	new_obj->shared->number = 0;
	new_obj->shared->house_count = 0;
	new_obj->shared->func = NULL;
	new_obj->shared->proto = new_obj;

	new_obj->in_room = NULL;
	new_obj->cur_flow_pulse = 0;
	for (i = 0; i < 4; i++)
		new_obj->obj_flags.value[i] = 0;
	new_obj->obj_flags.type_flag = 0;
	new_obj->obj_flags.wear_flags = 0;
	new_obj->obj_flags.extra_flags = 0;
	new_obj->obj_flags.extra2_flags = 0;
	new_obj->setWeight(0);
	new_obj->shared->cost = 0;
	new_obj->shared->cost_per_day = 0;
	new_obj->obj_flags.timer = 0;
	new_obj->obj_flags.bitvector[0] = 0;
	new_obj->obj_flags.bitvector[1] = 0;
	new_obj->obj_flags.bitvector[2] = 0;
	new_obj->obj_flags.max_dam = 0;
	new_obj->obj_flags.damage = 0;

	for (i = 0; i < MAX_OBJ_AFFECT; i++) {
		new_obj->affected[i].location = APPLY_NONE;
		new_obj->affected[i].modifier = 0;
	}

	new_obj->aliases = str_dup("fresh blank object");
	new_obj->line_desc = str_dup("A fresh blank object is here.");
	new_obj->name = str_dup("a fresh blank object");
	new_obj->action_desc = NULL;
	new_obj->ex_description = NULL;

	new_obj->in_room = NULL;

    objectPrototypes.add(new_obj);
	top_of_objt++;

	return (new_obj);
}


int
do_destroy_object(struct Creature *ch, int vnum)
{

	struct zone_data *zone = NULL;
	struct obj_data *obj = NULL, *temp = NULL, *next_obj = NULL;
	struct extra_descr_data *desc = NULL;
	struct descriptor_data *d = NULL;

	if (!(obj = real_object_proto(vnum))) {
		send_to_char(ch, "ERROR: That object does not exist.\r\n");
		return 1;
	}

	for (zone = zone_table; zone; zone = zone->next)
		if (vnum < zone->top)
			break;

	if (!zone) {
		send_to_char(ch, "That object does not belong to any zone!!\r\n");
		errlog("object not in any zone.");
		return 1;
	}

	if (!CAN_EDIT_ZONE(ch, zone)) {
		send_to_char(ch, "Oh, no you don't!!!\r\n");
		mudlog(GET_INVIS_LVL(ch), BRF, true,
			"OLC: %s failed attempt to DESTROY object %d.",
			GET_NAME(ch), obj->shared->vnum);
		return 1;
	}

	for (temp = object_list; temp; temp = next_obj) {
		next_obj = temp->next;
		if (temp->shared->vnum == obj->shared->vnum)
			extract_obj(temp);
	}

    objectPrototypes.remove(obj);

	for (d = descriptor_list; d; d = d->next)
		if (d->creature && GET_OLC_OBJ(d->creature) == obj) {
			GET_OLC_OBJ(d->creature) = NULL;
			send_to_char(d->creature, "The object you were editing has been destroyed!\r\n");
			break;
		}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	if (obj->aliases)
		free(obj->aliases);
	if (obj->line_desc)
		free(obj->line_desc);
	if (obj->name)
		free(obj->name);
	if (obj->action_desc)
		free(obj->action_desc);


	while ((desc = obj->ex_description)) {
		obj->ex_description = desc->next;
		if (desc->keyword)
			free(desc->keyword);
		if (desc->description)
			free(desc->description);
		free(desc);
	}

	if (obj->shared)
		free(obj->shared);

	top_of_objt--;
	free(obj);

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	return 0;
}


void
perform_oset(struct Creature *ch, struct obj_data *obj_p,
	char *argument, byte subcmd)
{
	struct zone_data *zone = NULL;
	struct obj_data *proto = NULL, *tmp_obj = NULL;
	int i, j, k, oset_command;
	int tmp_flags = 0, state = 0, cur_flags = 0, flag = 0;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	struct extra_descr_data *desc = NULL, *ndesc = NULL;

	if (!*argument) {
		strcpy(buf, "Valid oset commands:\r\n");
		strcat(buf, CCYEL(ch, C_NRM));
		i = 0;
		while (*olc_oset_keys[i] != '\n') {
			strcat(buf, olc_oset_keys[i]);
			strcat(buf, "\r\n");
			i++;
		}
		strcat(buf, CCNRM(ch, C_NRM));
		page_string(ch->desc, buf);
		return;
	}
	// what zone is it a member of
	for (zone = zone_table; zone; zone = zone->next)
		if (GET_OBJ_VNUM(obj_p) >= zone->number * 100
			&& GET_OBJ_VNUM(obj_p) <= zone->top)
			break;
	if (!zone) {
		slog("OLC: ERROR finding zone for object %d.",
			GET_OBJ_VNUM(obj_p));
		send_to_char(ch, "Unable to match object with zone error..\r\n");
		return;
	}

	half_chop(argument, arg1, arg2);
	skip_spaces(&argument);

	if ((oset_command = search_block(arg1, olc_oset_keys, FALSE)) < 0) {
		send_to_char(ch, "Invalid oset command '%s'.\r\n", arg1);
		return;
	}
	if (oset_command != 3 && oset_command != 17 && oset_command != 24 && !*arg2) {
		send_to_char(ch, "Set %s to what??\r\n", olc_oset_keys[oset_command]);
		return;
	}

	proto = obj_p->shared->proto;

	switch (oset_command) {
	case 0:				/******** aliases *************/
		if ((subcmd == OLC_OSET || !proto || proto->aliases != obj_p->aliases) &&
			obj_p->aliases)
			free(obj_p->aliases);

		obj_p->aliases = strdup(arg2);
		if (subcmd == OLC_OSET)
			UPDATE_OBJLIST_NAMES(obj_p, tmp_obj,->aliases);
		send_to_char(ch, "Aliases set.\r\n");
		break;
	case 1:				/******** name ****************/
		if ((subcmd == OLC_OSET || !proto ||
				proto->name != obj_p->name) &&
			obj_p->name)
			free(obj_p->name);

		obj_p->name = str_dup(arg2);
		if (subcmd == OLC_OSET)
			UPDATE_OBJLIST_NAMES(obj_p, tmp_obj,->name);
		send_to_char(ch, "Object name set.\r\n");
		break;
	case 2:					// ldesc
		if ((subcmd == OLC_OSET || !proto ||
				proto->line_desc != obj_p->line_desc) &&
			obj_p->line_desc) {
			free(obj_p->line_desc);
		}
		if (arg2[0] == '~') {
			obj_p->line_desc = NULL;
		} else {
			obj_p->line_desc = str_dup(arg2);
		}
		if (subcmd == OLC_OSET) {
			UPDATE_OBJLIST(obj_p, tmp_obj,->line_desc);
		}
		send_to_char(ch, "Object L-desc set.\r\n");
		break;
	case 3:
		if (subcmd != OLC_OSET && proto &&
				proto->ex_description == obj_p->ex_description) {
			obj_p->ex_description = exdesc_list_dup(proto->ex_description);
		}

		desc = locate_exdesc(fname(obj_p->aliases), obj_p->ex_description);
		if (!desc) {
			CREATE(ndesc, struct extra_descr_data, 1);
			ndesc->keyword = str_dup(obj_p->aliases);
			ndesc->next = obj_p->ex_description;
			obj_p->ex_description = ndesc;
			desc = obj_p->ex_description;
		}
		start_text_editor(ch->desc, &desc->description, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);

		if (subcmd == OLC_OSET)
			UPDATE_OBJLIST(obj_p, tmp_obj,->ex_description);
		act("$n begins to write an object description.",
			TRUE, ch, 0, 0, TO_ROOM);
		break;

	case 4:		/************* obj type ******************/
		if (!is_number(arg2)) {
			if ((i = search_block(arg2, item_types, 0)) < 0) {
				send_to_char(ch, "Type olc help otypes for a valid list.\r\n");
				return;
			}
		} else
			i = atoi(arg2);

		if (i < 0 || i > NUM_ITEM_TYPES) {
			send_to_char(ch, "Object type out of range.\r\n");
			return;
		}
		obj_p->obj_flags.type_flag = i;
		send_to_char(ch, "Object %d type set to %s.\r\n",
			obj_p->shared->vnum, item_types[i]);

		break;

	case 5:	   /************* obj extra1 flags ***********/
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, "Usage: olc oset extra1 [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = obj_p->obj_flags.extra_flags;

		while (*arg1) {
			if ((flag = search_block(arg1, extra_names, FALSE)) == -1) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		obj_p->obj_flags.extra_flags = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char(ch, "Extra1 flags set\r\n");
		} else if (tmp_flags == 0)
			send_to_char(ch, "Extra1 flags not altered.\r\n");
		else {
			send_to_char(ch, "Extra1 flags set.\r\n");
		}
		break;

	case 6:	   /************* obj extra2 flags ***********/
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, "Usage: olc oset extra2 [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = obj_p->obj_flags.extra2_flags;

		while (*arg1) {
			if ((flag = search_block(arg1, extra2_names, FALSE)) == -1) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		obj_p->obj_flags.extra2_flags = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char(ch, "Extra2 flags set\r\n");
		} else if (tmp_flags == 0)
			send_to_char(ch, "Extra2 flags not altered.\r\n");
		else {
			send_to_char(ch, "Extra2 flags set.\r\n");
		}
		break;

	case 7:	   /************* obj wear flags ***********/
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, "Usage: olc oset worn [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = obj_p->obj_flags.wear_flags;

		while (*arg1) {
			if ((flag = search_block(arg1, wear_bits, FALSE)) == -1) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		obj_p->obj_flags.wear_flags = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char(ch, "Wear flags set\r\n");
		} else if (tmp_flags == 0)
			send_to_char(ch, "Wear flags not altered.\r\n");
		else {
			send_to_char(ch, "Wear flags set.\r\n");
		}
		break;

	case 8:	  /**************  values 0-3 *************/
		half_chop(arg2, arg1, arg2);
		if (!is_number(arg1)) {
			send_to_char(ch, "The argument must be a number.\r\n"
				"Type olc help values for the lowdown.\r\n");
			return;
		} else {
			i = atoi(arg1);
			if (i < 0 || i > 3) {
				send_to_char(ch, "Usage: olc oset value [0|1|2|3] <value>.\r\n");
				return;
			} else {
				if (!*arg2) {
					send_to_char(ch, "Set the value to what??\r\n");
					return;
				} else if (!is_number(arg2)) {
					send_to_char(ch, "The value argument must be a number.\r\n");
					return;
				} else {
					j = atoi(arg2);
					obj_p->obj_flags.value[i] = j;
					send_to_char(ch, "Value %d (%s) set to %d.\r\n", i,
						item_value_types[(int)obj_p->obj_flags.type_flag][i],
						j);

				}
			}
		}
		break;
	case 9:	  /******* material composition ********/
		if (!*arg2) {
			send_to_char(ch, "Set the material to what?\r\n");
			return;
		}
		if (!is_number(arg2)) {
			if ((i = search_block(arg2, material_names, 0)) < 0) {
				send_to_char(ch, "Type olc help material for a valid list.\r\n");
				return;
			}
		} else
			i = atoi(arg2);

		if (i < 0 || i > TOP_MATERIAL) {
			send_to_char(ch, "Object material out of range.\r\n");
			return;
		} else {
			obj_p->obj_flags.material = i;
			send_to_char(ch, "Object %d material set to %s (%d).\r\n",
				obj_p->shared->vnum, material_names[i], i);
		}
		break;

	case 10:	  /******* maxdamage *******************/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			obj_p->obj_flags.max_dam = i;
			send_to_char(ch, "Object %d maxdamage set to %d.\r\n",
				obj_p->shared->vnum, i);
		}
		break;

	case 11:	  /******* damage **********************/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			obj_p->obj_flags.damage = i;
			send_to_char(ch, "Object %d damage set to %d.\r\n",
				obj_p->shared->vnum, i);
		}
		break;

	case 12:	  /******** weight **********/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			if (i < 0) {
				send_to_char(ch, "Object weight out of range.\r\n");
				return;
			} else {
				obj_p->setWeight(i);
				send_to_char(ch, "Object %d weight set to %d.\r\n",
					obj_p->shared->vnum, i);
			}
		}
		break;
	case 13:	  /******** cost **********/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			if (i < 0) {
				send_to_char(ch, "Object weight out of range.\r\n");
				return;
			} else {
				obj_p->shared->cost = i;
				send_to_char(ch, "Object %5d cost set to %8d.\r\n"
					"Recommended cost:         %8d.\r\n",
					obj_p->shared->vnum, i, prototype_obj_value(obj_p));
			}
		}
		break;
	case 14:	  /******** rent **********/
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			if (i < 0) {
				send_to_char(ch, "Object rent out of range.\r\n");
				return;
			} else {
				obj_p->shared->cost_per_day = i;
				send_to_char(ch, "Object %d rent set to %d.\r\n",
					obj_p->shared->vnum, i);
			}
		}
		break;
	case 15:	 /******* apply *************/
		half_chop(arg2, arg1, arg2);
		if (!*arg2 || !*arg1) {
			send_to_char(ch, "Usage: olc oset apply <location> <value>.\r\n");
			return;
		}
		if (!is_number(arg1)) {
			if ((i = search_block(arg1, apply_types, 0)) < 0) {
				send_to_char(ch, "Unknown apply type.... type olc h apply.\r\n");
				return;
			}
		} else
			i = atoi(arg1);

		j = atoi(arg2);

		if (i < 0 || i > NUM_APPLIES) {
			send_to_char(ch, "Location out of range.  Try 'olc h apply'.\r\n");
			return;
		} else if (j < -125 || j > 125) {
			send_to_char(ch, "Modifier out of range. [-125, 125].\r\n");
			return;
		}

		for (k = 0; k < MAX_OBJ_AFFECT; k++) {
			if (j) {
				if (obj_p->affected[k].location == APPLY_NONE)
					obj_p->affected[k].location = i;
				if (obj_p->affected[k].location == i) {
					obj_p->affected[k].modifier = j;
					break;
				}
			} else {
				if (obj_p->affected[k].location == i) {
					obj_p->affected[k].location = APPLY_NONE;
					obj_p->affected[k].modifier = 0;
					break;
				}
			}
		}

		if (k >= MAX_OBJ_AFFECT) {
			if (j)
				send_to_char(ch, 
					"All apply slots are filled.  Set an existing apply mod. to zero to remove.\r\n");
			else
				send_to_char(ch, "Unable to find an apply of that type.\r\n");
			return;
		} else
			send_to_char(ch, "Done.\r\n");

		break;

	case 16:	 /******* affect ************/
		half_chop(arg2, arg1, arg2);
		if (!is_number(arg1) || !*arg2) {
			send_to_char(ch, "Usage: olc oset affect <index> <bit letter>.\r\n");
			return;
		}
		i = atoi(arg1);
		i -= 1;

		if (i < 0 || i > 2) {
			send_to_char(ch, "Index out of range [1, 3].\r\n");
			send_to_char(ch, "Usage: olc oset affect <index> <bit letter>.\r\n");
			return;
		}

		obj_p->obj_flags.bitvector[i] = asciiflag_conv(arg2);

		send_to_char(ch, "Bitvector set! Voila!\r\n");
		break;

	case 17:  /***** action_desc *****/
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		start_text_editor(ch->desc, &obj_p->action_desc, true);
		break;

	case 18: /** special **/
		if (subcmd != OLC_OSET) {
			send_to_char(ch, "This can only be set with olc.\r\n");
			return;
		}
		if (!*arg2 || (i = find_spec_index_arg(arg2)) < 0)
			send_to_char(ch, "That is not a valid special.\r\n"
				"Type show special obj to view a list.\r\n");
		else if (!IS_SET(spec_list[i].flags, SPEC_OBJ))
			send_to_char(ch, "This special is not for objects.\r\n");
		else if (IS_SET(spec_list[i].flags, SPEC_RES) && !OLCIMP(ch))
			send_to_char(ch, "This special is reserved.\r\n");
		else {

			obj_p->shared->func = spec_list[i].func;
			if( obj_p->shared->func_param != NULL ) 
				free(obj_p->shared->func_param);
			obj_p->shared->func_param = NULL;
			do_specassign_save(ch, SPEC_OBJ);
			send_to_char(ch, "Object special set, you trickster you.\r\n");
		}
		break;
	case 19:  /***** paths *****/
		if (subcmd == OLC_OSET) {
			send_to_char(ch, "Cannot set with OLC.\r\n");
			return;
		}
		if (add_path_to_vehicle(obj_p, arg2)) {
			send_to_char(ch, "%s now follows the path titled: %s.\r\n",
				obj_p->name, arg2);
		} else {
			send_to_char(ch, "Could not assign that path to vehicle.\r\n");
		}
		break;

	case 20: /** soilage **/
		if (subcmd == OLC_OSET) {
			send_to_char(ch, "Cannot set with OLC.\r\n");
			return;
		}

		OBJ_SOILAGE(obj_p) = atoi(arg2);
		send_to_char(ch, "Soilage set.\r\n");
		break;

	case 21:					// sigil
		if (subcmd == OLC_OSET) {
			send_to_char(ch, "Cannot set with OLC.\r\n");
			return;
		}

		argument = one_argument(arg2, arg1);

		skip_spaces(&argument);

		if (!*arg1 || !*argument) {
			send_to_char(ch, "Usage: oset <obj> sigil <idnum|level> <value>\r\n");
			return;
		}

		if (is_abbrev(arg1, "idnum"))
			GET_OBJ_SIGIL_IDNUM(obj_p) = atoi(argument);
		else if (is_abbrev(arg1, "level"))
			GET_OBJ_SIGIL_LEVEL(obj_p) = atoi(argument);
		else {
			send_to_char(ch,
				"Unknown argument '%s'.  You must set sigil idnum or level.\r\n", arg2);
			return;
		}

		send_to_char(ch, "Sigil set.\r\n");
		break;
	case 22:		/************* obj extra3 flags ***********/
		tmp_flags = 0;
		argument = one_argument(arg2, arg1);

		if (*arg1 == '+')
			state = 1;
		else if (*arg1 == '-')
			state = 2;
		else {
			send_to_char(ch, "Usage: olc oset extra3 [+/-] [FLAG, FLAG, ...]\r\n");
			return;
		}

		argument = one_argument(argument, arg1);

		cur_flags = obj_p->obj_flags.extra3_flags;

		while (*arg1) {
			if ((flag = search_block(arg1, extra3_names, FALSE)) == -1) {
				send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
			} else
				tmp_flags = tmp_flags | (1 << flag);

			argument = one_argument(argument, arg1);
		}

		if (state == 1)
			cur_flags = cur_flags | tmp_flags;
		else {
			tmp_flags = cur_flags & tmp_flags;
			cur_flags = cur_flags ^ tmp_flags;
		}

		obj_p->obj_flags.extra3_flags = cur_flags;

		if (tmp_flags == 0 && cur_flags == 0) {
			send_to_char(ch, "Extra3 flags set\r\n");
		} else if (tmp_flags == 0)
			send_to_char(ch, "Extra3 flags not altered.\r\n");
		else {
			send_to_char(ch, "Extra3 flags set.\r\n");
		}
		break;

	case 23:
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			i = atoi(arg2);
			GET_OBJ_TIMER(obj_p) = i;
			send_to_char(ch, "Object %d timer set to %d.\r\n",
				obj_p->shared->vnum, i);
		}
		break;
	case 24:
		// Make sure they have a obj special
		if (!GET_OBJ_SPEC(obj_p)) {
			send_to_char(ch, "You should set a special first!\r\n");
			break;
		}

		// Check to see that they can set the spec param
		i = find_spec_index_ptr(GET_OBJ_SPEC(obj_p));
		if (IS_SET(spec_list[i].flags, SPEC_RES) && !OLCIMP(ch)) {
			send_to_char(ch, "This special is reserved.\r\n");
			break;
		}

		// It's ok.  Let em set it.
		start_text_editor(ch->desc, &obj_p->shared->func_param, true);
		SET_BIT(PLR_FLAGS(ch), PLR_OLC);
		act("$n begins to write a object spec param.", TRUE, ch, 0, 0,
			TO_ROOM);
		break;
	case 25:
		if (!is_number(arg2)) {
			send_to_char(ch, "The argument must be a number.\r\n");
			return;
		} else {
			long id = atol(arg2);
			if( id == 0 ) {
				obj_p->shared->owner_id = 0;
				send_to_char(ch, "Owner removed.\r\n");
			} else if(! playerIndex.exists(id) ) {
				send_to_char(ch,"There is no player with id %ld.\r\n",id);
			} else {
				obj_p->shared->owner_id = id;
				send_to_char(ch, "Object %d owner set to %s[%ld].\r\n",
					obj_p->shared->vnum, playerIndex.getName(id), id);
			}
		}
		break;

	default:
		send_to_char(ch, "Unsupported olc oset option.\r\n");
		break;
	}

#ifdef DMALLOC
	dmalloc_verify(0);
#endif

	if (subcmd == OLC_OSET) {
		if (oset_command == 4 ||	/* type   */
			oset_command == 5 ||	/* extra1 */
			oset_command == 6 ||	/* extra2 */
			oset_command == 7 ||	/* worn   */
			oset_command == 8 ||	/* value */
			oset_command == 9 ||	/* material */
			oset_command == 10 ||	/* maxdam */
			oset_command == 11 ||	/* dam */
			oset_command == 12 ||	/* weight */
			oset_command == 15 ||	/* apply */
			oset_command == 16) {	/* affect */
			obj_p->shared->cost = prototype_obj_value(obj_p);
			i = obj_p->shared->cost % 10;
			obj_p->shared->cost -= i;
			obj_p->shared->cost_per_day = obj_p->shared->cost / 50;
		}
		if (oset_command == 4 ||	/* type   */
			oset_command == 5 ||	/* extra1 */
			oset_command == 6 ||	/* extra2 */
			oset_command == 8 ||	/* value */
			oset_command == 9 ||	/* material */
			oset_command == 12 ||	/* weight */
			oset_command == 16) {	/* affect */
			obj_p->obj_flags.max_dam = set_maxdamage(obj_p);
			obj_p->obj_flags.damage = obj_p->obj_flags.max_dam;

		}
		// auto-set the material type
		if (!obj_p->obj_flags.material && (oset_command == 0 ||	// alias
				oset_command == 1 ||	// name
				oset_command == 2))	// ldesc
			obj_p->obj_flags.material = choose_material(obj_p);

		if (!ZONE_FLAGGED(zone, ZONE_FULLCONTROL) && !OLCIMP(ch) )
			SET_BIT(obj_p->obj_flags.extra2_flags, ITEM2_UNAPPROVED);
	}
}

/** olc only! */
void
do_clear_olc_object(struct Creature *ch)
{
	struct obj_data *obj_p = GET_OLC_OBJ(ch), *tmp_obj = NULL;
	struct extra_descr_data *desc = NULL;
	int k;

	if (!obj_p) {
		send_to_char(ch, "You are not currently editing an object.\r\n");
		return;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	if (obj_p->aliases)
		free(obj_p->aliases);
	obj_p->aliases = str_dup("blank object");
	if (obj_p->name)
		free(obj_p->name);
	obj_p->name = str_dup("a blank object");
	if (obj_p->line_desc)
		free(obj_p->line_desc);
	obj_p->line_desc = str_dup("A blank object has been dropped here.");
	if (obj_p->action_desc)
		free(obj_p->action_desc);
	obj_p->action_desc = NULL;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	obj_p->obj_flags.type_flag = 0;
	obj_p->obj_flags.extra_flags = 0;
	obj_p->obj_flags.extra2_flags = 0;
	obj_p->obj_flags.extra3_flags = 0;
	obj_p->obj_flags.wear_flags = 0;
	obj_p->obj_flags.value[0] = 0;
	obj_p->obj_flags.value[1] = 0;
	obj_p->obj_flags.value[2] = 0;
	obj_p->obj_flags.value[3] = 0;
	obj_p->obj_flags.material = 0;
	obj_p->obj_flags.max_dam = 0;
	obj_p->obj_flags.damage = 0;
	obj_p->setWeight(0);
	obj_p->shared->cost = 0;
	obj_p->shared->cost_per_day = 0;

	SET_BIT(GET_OBJ_EXTRA2(obj_p), ITEM2_UNAPPROVED);

	for (k = 0; k < 3; k++)
		obj_p->obj_flags.bitvector[k] = 0;

	for (k = 0; k < MAX_OBJ_AFFECT; k++) {
		obj_p->affected[k].location = APPLY_NONE;
		obj_p->affected[k].modifier = 0;
	}
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	while ((desc = obj_p->ex_description)) {
		obj_p->ex_description = desc->next;
		if (desc->keyword)
			free(desc->keyword);
		if (desc->description)
			free(desc->description);
		free(desc);
	}
	obj_p->ex_description = NULL;
#ifdef DMALLOC
	dmalloc_verify(0);
#endif
	UPDATE_OBJLIST_FULL(obj_p, tmp_obj);

	send_to_char(ch, "Okay, object #%d fully cleared.\r\n", obj_p->shared->vnum);
}
