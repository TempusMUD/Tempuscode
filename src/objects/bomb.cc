//
// File: bomb.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define __bomb_c__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "bomb.h"
#include "fight.h"
#include "house.h"
#include "char_class.h"

extern struct room_data *world;
extern struct spell_info_type spell_info[];

const char *bomb_types[] = {
	"none",
	"concussion",
	"fragmentation",
	"incendiary",
	"disruption",
	"nuclear",
	"flash",
	"smoke",
	"\n"
};

const char *fuse_types[] = {
	"none",
	"burn",
	"electronic",
	"remote",
	"contact",
	"motion",
	"\n"
};
//
// bomb_radius_list and bomb_rooms are internally used in this file only
//

struct bomb_radius_list *bomb_rooms = NULL;

//
// dam_object is an important global var, used in damage() as well
//

struct obj_data *dam_object;

//
// sort_rooms sorts the global list bomb_radius_list *bomb_rooms
// in increasing order of power.
//

void
sort_rooms()
{
	struct bomb_radius_list
	*new_list = NULL, *i = NULL, *j = NULL, *k = NULL;

	// Sort to put damage rooms in increasing power order

	for (i = bomb_rooms; i; i = j) {
		j = i->next;

		if (!new_list || (new_list->power > i->power)) {
			i->next = new_list;
			new_list = i;
		} else {
			k = new_list;

			while (k->next) {
				if (k->next->power > i->power)
					break;
				else
					k = k->next;
			}

			i->next = k->next;
			k->next = i;
		}
	}
	bomb_rooms = new_list;
}

//
// add_bomb_room is a low level function for bomb radius calculations
//

void
add_bomb_room(struct room_data *room, int fromdir, int p_factor)
{

	register struct bomb_radius_list *rad_elem = NULL;
	int dir, new_factor;

	for (rad_elem = bomb_rooms; rad_elem; rad_elem = rad_elem->next)
		if (room == rad_elem->room) {
			if ((unsigned int)p_factor > rad_elem->power)
				rad_elem->power = p_factor;
			else
				return;
			break;
		}

	if (!rad_elem) {
		CREATE(rad_elem, struct bomb_radius_list, 1);
		rad_elem->room = room;
		rad_elem->power = p_factor;
		rad_elem->next = bomb_rooms;
		bomb_rooms = rad_elem;
	}

	if (p_factor > 0) {
		for (dir = 0; dir < NUM_DIRS; dir++) {
			if (room->dir_option[dir] && room->dir_option[dir]->to_room &&
				room->dir_option[dir]->to_room != room &&
				!IS_SET(room->dir_option[dir]->exit_info, EX_ONEWAY) &&
				(fromdir < 0 || rev_dir[fromdir] != dir)) {
				new_factor = p_factor - (p_factor / 10) -
					(movement_loss[room->sector_type] << 4) -
					(IS_SET(room->dir_option[dir]->exit_info,
						EX_ISDOOR) ? 5 : 0) -
					(IS_SET(room->dir_option[dir]->exit_info,
						EX_CLOSED) ? 10 : 0) -
					(IS_SET(room->dir_option[dir]->exit_info,
						EX_PICKPROOF) ? 10 : 0) -
					(IS_SET(room->dir_option[dir]->exit_info,
						EX_ONEWAY) ? p_factor : 0) -
					(IS_SET(room->dir_option[dir]->exit_info,
						EX_HEAVY_DOOR) ? 7 : 0);

				new_factor = MAX(0, new_factor);
				add_bomb_room(room->dir_option[dir]->to_room, dir, new_factor);
			}
		}
	}
}

//
// bomb_damage_room damages everyone in a room in various ways
// fairly low level
//

void
bomb_damage_room(char *bomb_name, int bomb_type, int bomb_power,
	struct room_data *room, int dir, int power,
	struct Creature *precious_vict)
{

	struct Creature *vict = NULL;
	struct room_affect_data rm_aff;
	struct affected_type af;
	int dam, damage_type = 0;
	char dname[128];

	if (power <= bomb_power * 0.25)
		dam = 0;
	else
		dam = dice(power, MAX(10, (power >> 1)));

	if (dir == BFS_ALREADY_THERE) {
		switch (bomb_type) {
		case BOMB_CONCUSSION:
			sprintf(buf, "A shockwave blasts you as %s explodes!!", bomb_name);
			damage_type = TYPE_CRUSH;
			dam >>= 1;
			break;
		case BOMB_INCENDIARY:
			sprintf(buf,
				"A roar fills the room as a blast of flame explodes from %s!!",
				bomb_name);
			damage_type = TYPE_ABLAZE;
			break;
		case SKILL_SELF_DESTRUCT:
			dam <<= 3;
			// fall through
		case BOMB_FRAGMENTATION:
			sprintf(buf,
				"%s explodes into thousands of deadly fragments!!", bomb_name);
			damage_type = TYPE_RIP;
			break;
		case BOMB_DISRUPTION:
			sprintf(buf, "A disruption wave explodes from %s!!", bomb_name);
			damage_type = SPELL_DISRUPTION;
			break;
		case BOMB_NUCLEAR:
			sprintf(buf,
				"You are engulfed by a blinding light as %s explodes!!",
				bomb_name);
			damage_type = SPELL_FISSION_BLAST;
			break;
		case BOMB_FLASH:
			sprintf(buf,
				"A blinding flash fills the room as %s explodes!!", bomb_name);
			damage_type = 0;
			dam = MIN(1, dam);
			break;
		case BOMB_SMOKE:
			sprintf(buf, "Clouds of smoke begin to billow from %s!!",
				bomb_name);
			dam = 0;
			damage_type = 0;
			break;
		case BOMB_ARTIFACT:
			sprintf(buf, "%s unfolds itself with a screaming roar!\nYou are engulfed in a bright blue light...", bomb_name);
			dam = (dam > 0) ? 30000:0;
			damage_type = 0;
			break;
		default:
			sprintf(buf, "%s explodes!!", bomb_name);
			damage_type = TYPE_CRUSH;
			break;
		}
		if (!(room->people.empty())) {
			act(buf, FALSE, *(room->people.begin()), 0, 0, TO_CHAR | TO_SLEEP);
			act(buf, FALSE, *(room->people.begin()), 0, 0, TO_ROOM | TO_SLEEP);
		}
	} else {

		if (dir >= 0 && room->dir_option[dir] &&
			IS_SET(room->dir_option[dir]->exit_info, EX_ISDOOR) &&
			IS_SET(room->dir_option[dir]->exit_info, EX_CLOSED)) {
			if (power > (IS_SET(room->dir_option[dir]->exit_info,
						EX_HEAVY_DOOR) ? 6 : 1 +
					IS_SET(room->dir_option[dir]->exit_info,
						EX_PICKPROOF) ? 10 : 1)) {
				if (room->dir_option[dir]->keyword)
					strcpy(dname, room->dir_option[dir]->keyword);
				else
					strcpy(dname, "door");

				if (room->dir_option[dir]->to_room &&
					room->dir_option[dir]->to_room->dir_option[rev_dir[dir]] &&
					room->dir_option[dir]->to_room->dir_option[rev_dir[dir]]->
					to_room == room
					&& IS_SET(room->dir_option[dir]->to_room->
						dir_option[rev_dir[dir]]
						->exit_info, EX_CLOSED))
					REMOVE_BIT(room->dir_option[dir]->to_room->
						dir_option[rev_dir[dir]]->exit_info, EX_CLOSED);
				REMOVE_BIT(room->dir_option[dir]->exit_info, EX_CLOSED);
				if (!(room->people.empty())) {
					sprintf(buf,
						"The %s %s blown open from the other side!\r\n", dname,
						ISARE(dname));
					send_to_room(buf, room);
				}
			}
		}

		if (power <= bomb_power * 0.10)
			strcpy(buf, "You hear an explosion");
		else if (power <= bomb_power * 0.20)
			strcpy(buf, "You hear a loud explosion");
		else if (power <= bomb_power * 0.25)
			strcpy(buf, "There is a deafening explosion");
		else {
			switch (bomb_type) {
			case BOMB_CONCUSSION:
				strcpy(buf, "You are rocked by a concussive blast");
				damage_type = TYPE_CRUSH;
				dam >>= 1;
				break;
			case BOMB_FRAGMENTATION:
				strcpy(buf, "You are ripped by a blast of shrapnel");
				damage_type = TYPE_RIP;
				break;
			case BOMB_INCENDIARY:
				strcpy(buf, "You are engulfed by a blast of flame");
				damage_type = TYPE_ABLAZE;
				break;
			case BOMB_DISRUPTION:
				strcpy(buf, "You are rocked by a disruption blast");
				damage_type = SPELL_DISRUPTION;
				break;
			case BOMB_NUCLEAR:
				strcpy(buf, "You are engulfed by a nuclear blast");
				damage_type = SPELL_FISSION_BLAST;
				break;
			case BOMB_FLASH:
				strcpy(buf, "There is a flash of light");
				damage_type = 0;
				dam = MIN(1, dam);
				break;
			case BOMB_SMOKE:
				strcpy(buf, "Clouds of smoke begin to billow in");
				dam = MAX(1, dam);
				damage_type = 0;
				break;
			case BOMB_ARTIFACT:	
				strcpy(buf, "You are engulfed by a blinding bright blue light");
				dam = (dam > 0) ? 30000:0;
				damage_type = 0;
				break;
			case SKILL_SELF_DESTRUCT:
				strcpy(buf, "You are showered with a rain of fiery debris!");
				damage_type = TYPE_RIP;
				break;

			default:
				strcpy(buf, "You are rocked by an explosion");
				damage_type = TYPE_CRUSH;
				break;
			}
		}

		if (dir >= 0) {
			strcpy(dname, from_dirs[rev_dir[dir]]);
			strcat(buf, " from ");
			strcat(buf, dname);
		}

		strcat(buf, ".\r\n");
		send_to_room(buf, room);
	}

	if (!dam)
		return;
	CreatureList::iterator it = room->people.begin();
	for (; it != room->people.end(); ++it) {
		vict = (*it);

		if (damage(NULL, vict, dam, damage_type, WEAR_RANDOM))
			continue;

		if (bomb_type == BOMB_INCENDIARY &&
			!CHAR_WITHSTANDS_FIRE(vict) && !AFF2_FLAGGED(vict, AFF2_ABLAZE))
			SET_BIT(AFF2_FLAGS(vict), AFF2_ABLAZE);

		if ((bomb_type == BOMB_ARTIFACT || bomb_type == BOMB_FLASH) &&
			AWAKE(vict) && GET_LEVEL(vict) < LVL_AMBASSADOR &&
			!mag_savingthrow(vict, 40, SAVING_PETRI) &&
			!AFF_FLAGGED(vict, AFF_BLIND) && !MOB_FLAGGED(vict, MOB_NOBLIND)) {
			af.type = SPELL_BLINDNESS;
			af.bitvector = AFF_BLIND;
			af.duration = MAX(1, (power >> 1));
			af.location = 0;
			af.modifier = 0;
			af.aff_index = 1;
			af.level = 30;
			affect_to_char(vict, &af);
		}

		if (CANNOT_DAMAGE(NULL, vict, NULL, damage_type))
			continue;

		if (GET_STR(vict) < number(3, power)) {
			send_to_char(vict, 
				"You are blown to the ground by the explosive blast!\r\n");
			vict->setPosition(POS_SITTING);
		} else if (vict->getPosition() > POS_STUNNED &&
			(bomb_type == BOMB_CONCUSSION || power > number(2, 12)) &&
			number(5, 5 + power) > GET_CON(vict)) {

			if ((dir >= 0 || (dir = number(0, NUM_DIRS - 1)) >= 0) &&
				room->dir_option[rev_dir[dir]] &&
				room->dir_option[rev_dir[dir]]->to_room &&
				!IS_SET(room->dir_option[rev_dir[dir]]->exit_info, EX_CLOSED)
				&& (power << 5) > number(0,
					GET_WEIGHT(vict) + IS_CARRYING_W(vict) +
					IS_WEARING_W(vict) + CAN_CARRY_W(vict))) {
				send_to_char(vict, 
					"You are blown out of the room by the blast!!\r\n");
				char_from_room(vict,false);
				char_to_room(vict, room->dir_option[rev_dir[dir]]->to_room,false);
				look_at_room(vict, vict->in_room, 0);

				sprintf(buf, "$n is blown in from %s!", from_dirs[dir]);
				act(buf, FALSE, vict, 0, 0, TO_ROOM);
			} else if (vict->getPosition() > POS_SITTING && (power << 5) >
				GET_WEIGHT(vict) + CAN_CARRY_W(vict)) {
				send_to_char(vict, "You are blown to the ground by the blast!!\r\n");
				vict->setPosition(POS_RESTING);
			}

			if (!mag_savingthrow(vict, 40, SAVING_ROD))
				vict->setPosition(POS_STUNNED);
		}
	}

	// Objects in an explosion should also be damaged
	obj_data *obj, *next_obj;
	for (obj = room->contents;obj; obj = next_obj) {
		next_obj = obj->next_content;

		if(!IS_BOMB(obj) // Do not damage lit bombs
		||  obj->contains == NULL
		|| !IS_FUSE(obj->contains)
		||  FUSE_STATE(obj->contains) == 0 ) 
		{
			damage_eq(NULL, obj, dam, damage_type);
		}
	}

	// room affects here

	if (bomb_type == BOMB_INCENDIARY &&
		room->sector_type != SECT_WATER_SWIM &&
		room->sector_type != SECT_WATER_NOSWIM &&
		room->sector_type != SECT_UNDERWATER &&
		room->sector_type != SECT_DEEP_OCEAN &&
		!ROOM_FLAGGED(room, ROOM_FLAME_FILLED)) {
		rm_aff.description =
			str_dup("   The room is ablaze with raging flames!\r\n");
		rm_aff.duration = 1 + number(power >> 3, power);
		rm_aff.level = MIN(power >> 1, LVL_AMBASSADOR);
		rm_aff.type = RM_AFF_FLAGS;
		rm_aff.flags = ROOM_FLAME_FILLED;
		affect_to_room(room, &rm_aff);
	} else if (bomb_type == BOMB_NUCLEAR &&
		!ROOM_FLAGGED(room, ROOM_RADIOACTIVE)) {
		rm_aff.description =
			str_dup("   You feel a warm glowing feeling.\r\n");
		rm_aff.duration = number(power << 1, power << 4);
		rm_aff.type = RM_AFF_FLAGS;
		rm_aff.level = MIN(power >> 1, LVL_AMBASSADOR);
		rm_aff.flags = ROOM_RADIOACTIVE;
		affect_to_room(room, &rm_aff);
	} else if (bomb_type == BOMB_SMOKE &&
		!ROOM_FLAGGED(room, ROOM_SMOKE_FILLED)) {
		rm_aff.description =
			str_dup
			("   The room is filled with thick smoke, hindering your vision.\r\n");
		rm_aff.duration = number(power, power << 1);
		rm_aff.type = RM_AFF_FLAGS;
		rm_aff.level = MIN(power >> 1, LVL_AMBASSADOR);
		rm_aff.flags = ROOM_SMOKE_FILLED | ROOM_NOTRACK;
		affect_to_room(room, &rm_aff);
	}
}

//
// detonate_bomb is a high-level function to detonate a bomb
//

struct obj_data *
detonate_bomb(struct obj_data *bomb)
{

	struct room_data *room = bomb->in_room;
	struct Creature *ch = bomb->carried_by;
	struct obj_data *cont = bomb->in_obj, *next_obj = NULL;
	struct bomb_radius_list *rad_elem = NULL, *next_elem = NULL;
	bool internal = false;

	if (!ch) {
		ch = bomb->worn_by;
		if (ch && GET_IMPLANT(ch, bomb->worn_on) &&
			GET_IMPLANT(ch, bomb->worn_on) == bomb)
			internal = true;
	}

	if (cont) {
		while (cont->in_obj)
			cont = cont->in_obj;

		ch = (cont->worn_by ? cont->worn_by : cont->carried_by);
		room = cont->in_room;
		obj_from_obj(bomb);
	}

	// Unequip the bomb before removing it.
	if( ch != NULL ) { 
		if( bomb->worn_by ) {
			unequip_char( ch, bomb->worn_on, 0 );
		}
		if (bomb->carried_by ) {
			obj_from_char(bomb);
		}
	}

	dam_object = bomb;

	if (ch) {
		sprintf(buf, "$p goes off in %s!!!", (internal ? "body!" :
				(cont ? "$P" : "your inventory")));
		act(buf, FALSE, ch, bomb, cont, TO_CHAR);
		sprintf(buf, "$p goes off $n's %s!!!",
			cont ? fname(cont->aliases) : "hands");
		act(buf, FALSE, ch, bomb, cont, TO_ROOM);
		room = ch->in_room;
		damage(NULL, ch,
			dice(MIN(100, BOMB_POWER(bomb) +
					(internal ? BOMB_POWER(bomb) : 0)),
				MIN(500, BOMB_POWER(bomb))), TYPE_BLAST, WEAR_HANDS);
	}

	if (cont)
		damage_eq(NULL, cont, dice(MIN(100, BOMB_POWER(bomb)),
				BOMB_POWER(bomb)));

	if ((cont || internal) && BOMB_IS_FLASH(bomb))
		add_bomb_room(room, -1, BOMB_POWER(bomb) >> 4);
	else if (internal)
		add_bomb_room(room, -1, MIN(500, BOMB_POWER(bomb) >> 2));
	else if (!bomb->in_room)
		add_bomb_room(room, -1, MIN(500, BOMB_POWER(bomb) >> 1));
	else
		add_bomb_room(room, -1, MIN(500, BOMB_POWER(bomb)));

	sort_rooms();

	for (rad_elem = bomb_rooms; rad_elem; rad_elem = next_elem) {
		next_elem = rad_elem->next;

		bomb_damage_room(bomb->name, BOMB_TYPE(bomb),
			BOMB_POWER(bomb), rad_elem->room, find_first_step(rad_elem->room,
				room, GOD_TRACK), rad_elem->power);
		free(rad_elem);
		bomb_rooms = next_elem;

	}
	next_obj = bomb->next;

	for (cont = bomb->contains; cont; cont = cont->next_content)
		if (next_obj == cont)
			next_obj = cont->next;

	extract_obj(bomb);
	dam_object = NULL;

	return (next_obj);
}

//
// engage_self_destruct is THE function that actually BLOWS UP somebody (assumed to be borg)
//

void
engage_self_destruct(struct Creature *ch)
{

	int level, i;
	struct obj_data *obj = NULL, *n_obj = NULL;
	struct room_data *room = NULL;
	struct bomb_radius_list *rad_elem = NULL, *next_elem = NULL;

	send_to_char(ch, 
		"Self-destruct point reached.  Stand by for termination...\r\n");

	level = (GET_LEVEL(ch) >> 3) + GET_REMORT_GEN(ch) + (GET_HIT(ch) >> 8);
	room = ch->in_room;

	// kill the cyborg first
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			obj_to_room(unequip_char(ch, i, MODE_EQ), ch->in_room);

	for (obj = ch->carrying; obj; obj = n_obj) {
		n_obj = obj->next_content;

		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
	}

	GET_HIT(ch) = 0;
	GET_MOVE(ch) = 0;

	// must have life points to do damage

	if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
		if (GET_LIFE_POINTS(ch) > 0) {
			GET_LIFE_POINTS(ch)--;
		} else {
			level = 0;
		}
	}
	// also must know the skill to do damage

	if (CHECK_SKILL(ch, SKILL_SELF_DESTRUCT) < 50) {
		level = 0;
	}

	gain_skill_prof(ch, SKILL_SELF_DESTRUCT);
	mudlog(GET_INVIS_LVL(ch), BRF, true,
		"%s self-destructed at room #%d, level %d.", GET_NAME(ch),
		ch->in_room->number, level);
	die(ch, ch, SKILL_SELF_DESTRUCT, FALSE);

	// now kill everyone else

	add_bomb_room(room, -1, level);

	sort_rooms();

	for (rad_elem = bomb_rooms; rad_elem; rad_elem = next_elem) {
		next_elem = rad_elem->next;

		bomb_damage_room(GET_NAME(ch),
			SKILL_SELF_DESTRUCT,
			level,
			rad_elem->room,
			find_first_step(rad_elem->room, room, GOD_TRACK), rad_elem->power);
		free(rad_elem);
		bomb_rooms = next_elem;

	}

}

//
// do_bomb is the wizcommand to take a look at potential bomb radii by bomb level
//

ACMD(do_bomb)
{
	struct bomb_radius_list *rad_elem = NULL, *next_elem = NULL;
	struct bomb_radius_list *i, *j, *k, *new_list;
	bool overflow = false;

	skip_spaces(&argument);

	if (atoi(argument) > (GET_LEVEL(ch) - 49) * 80) {
		send_to_char(ch, "Not so big... causes lag.\r\n");
		return;
	}
	bomb_rooms = NULL;
	add_bomb_room(ch->in_room, -1, atoi(argument));

	// Sort to put damage rooms in increasing power order
	// should sort_rooms() be used here?

	new_list = NULL;
	for (i = bomb_rooms; i; i = j) {
		j = i->next;
		if (!new_list || (new_list->power > i->power)) {
			i->next = new_list;
			new_list = i;
		} else {
			k = new_list;
			while (k->next)
				if (k->next->power > i->power)
					break;
				else
					k = k->next;
			i->next = k->next;
			k->next = i;
		}
	}
	bomb_rooms = new_list;


	strcpy(buf, "BOMB AFFECTS:\r\n");
	for (rad_elem = bomb_rooms; rad_elem; rad_elem = next_elem) {
		next_elem = rad_elem->next;
		if (!overflow) {
			sprintf(buf2, " %3d - [%5d] %s%s%s\r\n", rad_elem->power,
				rad_elem->room->number, CCCYN(ch, C_NRM),
				rad_elem->room->name, CCNRM(ch, C_NRM));
			if (strlen(buf) + strlen(buf2) > MAX_STRING_LENGTH - 128) {
				overflow = true;
				strcat(buf, "OVERFLOW\r\n");
			} else
				strcat(buf, buf2);
		}
		bomb_rooms = next_elem;
		free(rad_elem);
	}

	page_string(ch->desc, buf);
}

//
// do_defuse is the skill that allows one to stop a fuset (lit or unlit) bomb from blowing up
//

ACMD(do_defuse)
{

	struct obj_data *bomb = NULL, *fuse = NULL;

	skip_spaces(&argument);

	if (!(bomb = get_obj_in_list_vis(ch, argument, ch->carrying)) &&
		!(bomb = get_obj_in_list_vis(ch, argument, ch->in_room->contents)))
		send_to_char(ch, "Defuse what?\r\n");
	else if (!IS_BOMB(bomb))
		act("$p is not a bomb.", FALSE, ch, bomb, 0, TO_CHAR);
	else if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) < 20)
		send_to_char(ch, "You have no idea how.\r\n");
	else if (!(fuse = bomb->contains) || !IS_FUSE(fuse))
		act("$p is not fused.", FALSE, ch, bomb, 0, TO_CHAR);
	else {
		if (CHECK_SKILL(ch, SKILL_DEMOLITIONS) <
			number(0, 60 +
				(FUSE_STATE(fuse) ?
					(FUSE_IS_CONTACT(fuse) ? 45 : FUSE_IS_MOTION(fuse) ? 35 :
						25)
					: 20) + (IS_CONFUSED(ch) ? 30 : 0))) {
			send_to_char(ch, "You set it off!!\r\n");
			detonate_bomb(bomb);
			return;
		}

		FUSE_STATE(fuse) = 0;
		obj_from_obj(fuse);
		obj_to_char(fuse, ch);

		if (bomb->aux_obj && bomb == bomb->aux_obj->aux_obj) {
			bomb->aux_obj->aux_obj = NULL;
			bomb->aux_obj = NULL;
		}

		act("$n defuses $p.", TRUE, ch, bomb, 0, TO_ROOM);
		act("You remove $P from $p.", FALSE, ch, bomb, fuse, TO_CHAR);
		gain_skill_prof(ch, SKILL_DEMOLITIONS);

	}
}

//
// sound_gunshots is used for sending out radius-based sounds in general
// used for guns and spells, can be used in other places as well
//

#define LOUD (rad_elem->power > 0)
void
sound_gunshots(struct room_data *room, int type, int power, int num)
{
	struct bomb_radius_list *rad_elem = NULL;
	int dir;

	bomb_rooms = NULL;

	power = power / 6 + 2;
	power = MAX(8, MIN(21, power));

	add_bomb_room(room, -1, power);
	sort_rooms();

	while ((rad_elem = bomb_rooms)) {

		bomb_rooms = bomb_rooms->next;

		if (rad_elem->room && !(rad_elem->room->people.empty())
			&& rad_elem->room != room) {
			if ((dir = find_first_step(rad_elem->room, room, GOD_TRACK)) >= 0)
				dir = rev_dir[dir];

			if (dir < 0 || dir >= NUM_DIRS) {
				switch (type) {
				case SKILL_PROJ_WEAPONS:
					sprintf(buf, "You hear %s.\r\n",
						num > 1 ? "some gunshots" : "a gunshot");
					break;
				default:
					break;
				}
				continue;
			}

			switch (type) {
			case SKILL_BATTLE_CRY:
				sprintf(buf, "You hear a fearsome warcry from %s.\r\n",
					from_dirs[dir]);
				break;
			case SKILL_KIA:
				sprintf(buf, "You hear a fearsome KIA! from %s.\r\n",
					from_dirs[dir]);
				break;
			case SKILL_CRY_FROM_BEYOND:
				sprintf(buf,
					"You hear a blood-curdling warrior's cry from %s!\r\n",
					from_dirs[dir]);
				break;

			case SPELL_HELL_FIRE:
			case SPELL_FLAME_STRIKE:
			case SPELL_FIRE_BREATH:
				sprintf(buf, "You hear a %sfiery blast from %s.\r\n",
					LOUD ? "deafening " : "", from_dirs[dir]);
				break;
			case SPELL_HELL_FROST:
			case SPELL_CONE_COLD:
			case SPELL_ICY_BLAST:
			case SPELL_ICE_STORM:
				sprintf(buf, "You hear a%s icy blast from %s.\r\n",
					LOUD ? " deafening" : "n", from_dirs[dir]);
				break;
			case SPELL_LIGHTNING_BOLT:
			case SPELL_CHAIN_LIGHTNING:
			case SPELL_CALL_LIGHTNING:
			case SPELL_LIGHTNING_BREATH:
				sprintf(buf, "You hear a %sthunderclap from %s.\r\n",
					LOUD ? "loud " : "", from_dirs[dir]);
				break;
			case SPELL_COLOR_SPRAY:
			case SPELL_PRISMATIC_SPRAY:
				sprintf(buf, "You see a %sflash of light from %s.\r\n",
					LOUD ? "bright " : "", from_dirs[dir]);
				break;
			case SPELL_FIREBALL:
				sprintf(buf, "There is a %sfiery explosion from %s.\r\n",
					LOUD ? "deafening " : "", from_dirs[dir]);
				break;
			case SPELL_METEOR_STORM:
				sprintf(buf,
					"You hear the roar of a meteor storm from %s.\r\n",
					from_dirs[dir]);
				break;
			case SKILL_PROJ_WEAPONS:
				if (num > 1)
					sprintf(buf, "You hear %d %sgunshots from %s.\r\n",
						num, LOUD ? "loud " : "", from_dirs[dir]);
				else
					sprintf(buf, "You hear a %sgunshot from %s.\r\n",
						LOUD ? "loud " : "", from_dirs[dir]);
				break;
			default:
				sprintf(buf,
					"You heard a type %d sound from %s.  Please report this bug.\r\n",
					type, from_dirs[dir]);
				break;
			}
			send_to_room(buf, rad_elem->room);
		}
		free(rad_elem);
	}
}

#undef __bomb_c__
