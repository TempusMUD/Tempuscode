//
// File: guns.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define __guns_c__

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
#include "char_class.h"
#include "guns.h"

extern const char *gun_types[] = {
	"none",
	".22 cal",
	".223 cal",
	".25 cal",
	".30 cal",
	".357 cal",
	".38 cal",
	".40 cal",
	".44 cal",
	".45 cal",
	".50 cal",
	".555 cal",
	"nailgun",
	".410",
	"20 gauge",
	"16 gauge",
	"12 gauge",
	"10 gauge",
	"7 mm",
	"7.62 mm",
	"9 mm",
	"10 mm",
	"7mm magnum",
	"unused - 23",
	"unused - 24",
	"bow",
	"x-bow",
	"unused - 27",
	"unused - 28",
	"unused - 29",
	"flamethrower",
	"\n"
};

extern const char *egun_types[] = {
	"none",
	"lightning",
	"plasma",
	"ion",
	"laser",
	"disruption",
	"phaser",
	"microwave",
	"gamma",
	"\n"
};

extern const int gun_damage[][2] = {
	{0, 0},						/* none  */
	{14, 7},					/* .22   */
	{24, 11},					/* .223  */
	{18, 9},					/* .25   */
	{22, 11},					/* .30   */
	{26, 13},					/* .357  */
	{22, 9},					/* .38   */
	{24, 11},					/* .40   */
	{30, 13},					/* .44   */
	{24, 13},					/* .45   */
	{32, 15},					/* .50   */
	{34, 15},					/* .555  */
	{20, 10},					/* nail  */
	{30, 11},					/* .410  */
	{36, 15},					/* 20 ga */
	{40, 19},					/* 16 ga */
	{40, 27},					/* 12 ga */
	{40, 31},					/* 10 ga */
	{24, 11},					/* 7 mm  */
	{24, 11},					/* 7.62 mm */
	{20, 11},					/* 9 mm  */
	{24, 13},					/* 10 mm */
	{30, 12},					/*7mm magnum */
	{0, 0},
	{0, 0},
	{8, 15},					/* bow */
	{12, 15},					/* xbow */
	{0, 0},						// 27
	{0, 0},						// 28
	{0, 0},						// 29
	{15, 25},					// flamethrower
	{0, 0}						/* trailer */
};

void
show_gun_status(struct Creature *ch, struct obj_data *gun)
{
	struct obj_data *bullet = NULL;
	int count = 0;

	if (IS_ENERGY_GUN(gun)) {
		if (gun->contains && IS_ENERGY_CELL(gun->contains)) {
			sprintf(buf,
				"%s is loaded with %s.\r\n"
				"The potential energy of %s is:  %s[%3d/%3d]%s units.\r\n",
				gun->name, gun->contains->name,
				gun->contains->name,
				QGRN, CUR_ENERGY(gun->contains), MAX_ENERGY(gun->contains),
				QNRM);
			CAP(buf);
		} else
			strcpy(buf, "");

		sprintf(buf,
			"%sIt is set to discharge at rate:    %s[%2d]%s units/shot.\r\n",
			buf, QGRN, GUN_DISCHARGE(gun), QNRM);
		if (MAX_R_O_F(gun) > 1)
			sprintf(buf,
				"%sThe Rate of Fire is set to:        %s[%d/%d]%s.\r\n", buf,
				QGRN, CUR_R_O_F(gun), MAX_R_O_F(gun), QNRM);

		send_to_char(ch, "%s", buf);
	}

	else if (IS_GUN(gun)) {

		if (MAX_LOAD(gun)) {
			if (gun->contains) {
				for (bullet = gun->contains, count = 0; bullet;
					count++, bullet = bullet->next_content);
				sprintf(buf, "$p is loaded with %s[%d/%d]%s cartridge%s",
					QGRN, count, MAX_LOAD(gun), QNRM, count == 1 ? "" : "s");
			} else
				strcpy(buf, "$p is not loaded.");
		} else if (!gun->contains)
			strcpy(buf, "$p is not loaded.");
		else {
			for (bullet = gun->contains->contains, count = 0; bullet;
				count++, bullet = bullet->next_content);
			sprintf(buf, "$p is loaded with $P,\r\n"
				"which contains %s[%d/%d]%s cartridge%s",
				QGRN, count, MAX_LOAD(gun->contains), QNRM,
				count == 1 ? "" : "s");
		}
		act(buf, FALSE, ch, gun, gun->contains, TO_CHAR);
		if (MAX_R_O_F(gun) > 1) {
			send_to_char(ch, "The Rate of Fire is set to:        %s[%d/%d]%s.\r\n",
				QGRN, CUR_R_O_F(gun), MAX_R_O_F(gun), QNRM);
		}
	} else
		send_to_char(ch, "Unsupported gun type.\r\n");
}


#define GUNSET_RATE      1
#define GUNSET_DISCHARGE 2

ACMD(do_gunset)
{

	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	struct obj_data *gun;
	int number, i = 0;
	int mode = 0;

	argument = one_argument(argument, arg1);

	if (!*arg1) {
		send_to_char(ch, "Usage: gunset [internal] <gun> <rate> <value>\r\n");
		return;
	}

	if (!strncmp(arg1, "internal", 8)) {

		argument = one_argument(argument, arg1);

		if (!*arg1) {
			send_to_char(ch, "Gunset which implant?\r\n");
			return;
		}

		if (!(gun = get_object_in_equip_vis(ch, arg1, ch->implants, &i))) {
			send_to_char(ch, "You are not implanted with %s '%s'.\r\n", AN(arg1),
				arg1);
			return;
		}

	} else if ((!(gun = GET_EQ(ch, WEAR_WIELD)) || !isname(arg1, gun->aliases)) &&
		(!(gun = GET_EQ(ch, WEAR_WIELD_2)) || !isname(arg1, gun->aliases))) {
		send_to_char(ch, "You are not wielding %s '%s'.\r\n", AN(arg1), arg1);
		return;
	}

	if (!IS_ENERGY_GUN(gun) && !IS_GUN(gun)) {
		act("$p is not a gun.", FALSE, ch, gun, 0, TO_CHAR);
		return;
	}

	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		show_gun_status(ch, gun);
		return;
	}

	if (is_abbrev(arg1, "rate") || is_abbrev(arg1, "rof"))
		mode = GUNSET_RATE;
	else {
		send_to_char(ch, "usage: gunset <rate> <value>\r\n");
		return;
	}

	if (!*arg2) {
		send_to_char(ch, "Set the rate of fire to what?\r\n");
		return;
	}
	if (!is_number(arg2)) {
		send_to_char(ch, "The value must be numeric.\r\n");
		return;
	}
	if ((number = atoi(arg2)) < 0) {
		send_to_char(ch, "A NEGATIVE value?  Consider the implications...\r\n");
		return;
	}

	if (mode == GUNSET_RATE) {
		if (number > MAX_R_O_F(gun)) {
			send_to_char(ch, "The maximum rate of fire of %s is %d.\r\n",
				gun->name, MAX_R_O_F(gun));
		} else if (!number) {
			send_to_char(ch, "A zero rate of fire doesnt make much sense.\r\n");
		} else {
			act("$n adjusts the configuration of $p.", TRUE, ch, gun, 0,
				TO_ROOM);
			CUR_R_O_F(gun) = number;

			send_to_char(ch, "The rate of fire of %s set to %d/%d.\r\n",
				gun->name, CUR_R_O_F(gun), MAX_R_O_F(gun));
		}
		return;
	}
	//send_to_char("Gunset discharge error.\r\n", ch);
	return;
}

#undef __guns_c__
