//
// File: vehicle.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/****************************************************************************
	VEHICLE.C
****************************************************************************/

#define __vehicle_c__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "house.h"
#include "clan.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct obj_data *object_list;
extern int *has_key(struct char_data *ch, int key);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch,
		      int mode);

struct obj_data *cur_car = NULL;

const char *engine_state_bits[] = {
    "OPERATIONAL",
    "PARKED",
    "RUNNING",
    "",
    "\n"
};
const char *outside_weather[] = {
    "Clear Sky",
    "Cloudy",
    "Raining",
    "Lightning",
    "\n"
};

const char *car_flags[] = {
    "ATV",
    "Road",
    "Sky",
    "Boat",
    "Space",
    "\n"
};

ACMD(do_exits);

int has_car_key(struct char_data *ch, room_num car_room)
{
    struct obj_data *key = NULL;

    if (GET_LEVEL(ch) > LVL_GRGOD)
	return 1;

    for (key = ch->carrying; key; key = key->next_content)
	if (KEY_TO_CAR(key) == car_room)
	    return 1;

    if ((key = GET_EQ(ch, WEAR_HOLD)))
	if (KEY_TO_CAR(key) == car_room)
	    return 1;

    return 0;
}

void display_status(struct char_data *ch, struct obj_data *car,
		    struct char_data *driver, struct obj_data *engine)
{
    if (!ch || !engine || !car)
	return;

    send_to_char("You examine the instrument panel.\r\n", ch);
    if (!number(0, 5)) {
	sprintf(buf, "%sThe %sinstrument %spanel %sblinks %sfor %sa %smoment%s.",
		QGRN, QBLU, QMAG, QYEL, QCYN, QYEL, QBLU, QNRM);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
    }
    sprintf(buf,"\r\n%s<<<<<<<<%sSYSTEM STATUS UPDATE (%s)%s>>>>>>>>>%s\r\n",
	    QRED, QNRM, car->short_description, QRED, QNRM);
    send_to_char(buf, ch);
    sprintf(buf, "%s*******************************************************%s\r\n",
	    QBLU, QNRM);
    send_to_char(buf, ch);
    sprintbit(ENGINE_STATE(engine),engine_state_bits, buf2);
    sprintf(buf, 
	    "%sEngine State:%s    %s\r\n", QCYN, QNRM, buf2); 
    send_to_char(buf, ch);
    sprintbit(DOOR_STATE(car), container_bits, buf2);
    sprintf(buf, 
	    "%sDoor State:%s      %s\r\n", QCYN, QNRM, buf2);
    send_to_char(buf, ch);
    sprintf(buf,
	    "%sEnergy Status:%s   [%s%d / %d%s]%s\r\n"
	    "%sHeadlights are:%s  %s\r\n"
	    "%sDriver is:%s       %s\r\n"
	    "%sVhcl is located:%s %s\r\n"
	    "%sThe weather outside is:%s    %s\r\n",
	    QCYN, QRED, QNRM, CUR_ENERGY(engine), MAX_ENERGY(engine), QRED, QNRM,
	    QCYN, QNRM, (HEADLIGHTS_ON(engine) ? "ON" : "OFF"),
	    QCYN, QNRM, driver ? PERS(ch, driver) : "No-one",
	    QCYN, QNRM, car->in_room->name,
	    QCYN, QNRM, outside_weather[(int)car->in_room->zone->weather->sky]);
    send_to_char(buf, ch);
    if (LOW_ENERGY(engine)) {
	sprintf(buf, "A %sWARNING%s indicator lights up.", QRED, QNRM);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "%s***%sWARNING%s***%s (Low Energy Level).\r\n", QGRN, QRED,
		QGRN, QNRM);
	send_to_char(buf, ch);
    }

    sprintf(buf, "%s*******************************************************%s\r\n",
	    QBLU, QNRM);
    send_to_char(buf, ch);
}

void start_engine(struct char_data *ch, struct obj_data *car, 
		  struct obj_data *engine, struct obj_data *console)
{

    if (!IS_NPC(ch))
	V_CONSOLE_IDNUM(console) = GET_IDNUM(ch);
    else
	V_CONSOLE_IDNUM(console) = - (GET_MOB_VNUM(ch));

    if (ENGINE_ON(engine)) {
	act("The starter belt squeals as you try to crank $p!", 
	    FALSE, ch, car, 0, TO_CHAR);
	act("The starter belt squeals as $n tries to crank $p!", 
	    FALSE, ch, car, 0, TO_ROOM);
	if (car->in_room->people)
	    act("You hear a loud squealing from under the hood of $p.", 
		FALSE, 0, car, 0, TO_ROOM);
	return;
    }  
    if (CUR_ENERGY(engine) == 0) {
	act("$n tries to crank $p.", TRUE, ch, car, 0, TO_ROOM);
	act("$p turns over sluggishly, but doesn't start.", 
	    FALSE, ch, engine, 0, TO_ROOM);
	act("$p turns over and over sluggishly.", FALSE, ch, engine, 0, TO_CHAR);
	if (car->in_room->people)
	    act("$p's engine turns over and over sluggishly.",
		FALSE, 0, car, 0, TO_ROOM);
	return;
    }
    if (LOW_ENERGY(engine)) {
	act("$n tries to crank $p.", TRUE, ch, car, 0, TO_ROOM);
	act("$p turns over sluggishly and sputters to life.", 
	    FALSE, ch, engine, 0, TO_ROOM);
	act("$p turns over sluggishly and sputters to life.", 
	    FALSE, ch, engine, 0, TO_CHAR);
	if (car->in_room->people)
	    act("$p's engine turns over sluggishly and sputters to life.",
		FALSE, 0, car, 0, TO_ROOM);
	TOGGLE_BIT(ENGINE_STATE(engine), ENG_RUN);
	CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - USE_RATE(engine)*2);
	return;
    }
    act("$n tries to crank $p.", TRUE, ch, car, 0, TO_ROOM);
    act("$p turns over and roars to life.", FALSE, ch, engine, 0, TO_ROOM);
    act("$p turns over and roars to life.", FALSE, ch, engine, 0, TO_CHAR);
    if (car->in_room->people)
	act("$p's engine turns over and roars to life.",
	    FALSE, 0, car, 0, TO_ROOM);
    TOGGLE_BIT(ENGINE_STATE(engine), ENG_RUN);
    CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - USE_RATE(engine)*2);
    return;
}

int 
move_car(struct char_data *ch, struct obj_data *car, int dir)
{
    int energy_cost = 0;
    struct room_data *dest = NULL, *other_rm = NULL;
    struct char_data *pass = NULL;
    struct obj_data *engine = car->contains;
    char lbuf[256], abuf[256];

    cur_car = car;

    if (!EXIT(car, dir) || !(dest = EXIT(car, dir)->to_room))
	return ERR_NULL_DEST;
    if (IS_SET(EXIT(car, dir)->exit_info, EX_CLOSED))
	return ERR_CLOSED_EX;

    if (engine && !IS_ENGINE(engine))
	engine = NULL;

    if ((IS_ROADCAR(car) && (SECT_TYPE(dest) != SECT_CITY &&
			     SECT_TYPE(dest) != SECT_ROAD &&
			     SECT_TYPE(dest) != SECT_INSIDE &&
			     SECT_TYPE(dest) != SECT_FIELD)) || 
	(IS_SKYCAR(car) && (SECT_TYPE(dest) == SECT_UNDERWATER))) {
	return ERR_NODRIVE;
    }

    if ( ROOM_FLAGGED( dest, ROOM_HOUSE ) && !House_can_enter( ch, dest->number ) )
	return ERR_HOUSE;
    
    if ( ROOM_FLAGGED( dest, ROOM_CLAN_HOUSE ) &&
	 !clan_house_can_enter( ch, dest ) )
	return ERR_CLAN;

    if (engine) {
	energy_cost = ((movement_loss[car->in_room->sector_type] +
			movement_loss[dest->sector_type])* USE_RATE(engine));
	REMOVE_BIT(ENGINE_STATE(engine), ENG_PARK);

	CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - energy_cost);

	if (IS_ENGINE_SET(engine, ENG_PETROL)) {
	    sprintf(lbuf, "$p leaves noisily to the %s.",dirs[dir]);
	    sprintf(abuf, "$p arrives noisily from %s.", from_dirs[dir]);
	} else if (IS_ENGINE_SET(engine, ENG_ELECTRIC)) {
	    sprintf(lbuf, "$p hums off to the %s.",dirs[dir]);
	    sprintf(abuf, "$p hums in from %s.", from_dirs[dir]);
	} else {
	    sprintf(lbuf, "$p moves off to the %s.",dirs[dir]);
	    sprintf(abuf, "$p moves in from %s.", from_dirs[dir]);
	}
    } else if (IS_BOAT(car)) {
	sprintf(lbuf, "$p is rowed off to the %s.",dirs[dir]);
	sprintf(abuf, "$p is rowed in from %s.", from_dirs[dir]);
    } else {
	sprintf(lbuf, "$p moves off to the %s.",dirs[dir]);
	sprintf(abuf, "$p moves in from %s.", from_dirs[dir]);
    }
    if (IS_SKYCAR(car)) {
	sprintf(lbuf, "$p flies off to the %s.",dirs[dir]);
	sprintf(abuf, "$p flies in from %s.", from_dirs[dir]);
    } else if (IS_BOAT(car)) {
	sprintf(lbuf, "$p sails off to the %s.",dirs[dir]);
	sprintf(abuf, "$p sails in from %s.", from_dirs[dir]);
    }

    act(lbuf, FALSE, 0, car, 0, TO_ROOM | ACT_HIDECAR);

    obj_from_room(car);
    obj_to_room(car, dest);

    act(abuf, FALSE, 0, car, 0, TO_ROOM | ACT_HIDECAR);

    if (car->action_description && CAN_GO(car, dir) && 
	(other_rm = EXIT(car, dir)->to_room) && other_rm->people) {
	strcpy(buf, car->action_description);
	sprintf(buf, "%s %s.", buf, from_dirs[dir]);
	act(buf, FALSE, other_rm->people, car, 0, TO_ROOM);
	act(buf, FALSE, other_rm->people, car, 0, TO_CHAR);
    }

    if (ch) {
	sprintf(buf, "$n drives $p %s.", dirs[dir]);
	act(buf, FALSE, ch, car, 0, TO_ROOM);

	if (IS_SKYCAR(car))
	    send_to_room("You see as you fly up: \r\n", ch->in_room);
	else
	    send_to_room("You see as you drive up: \r\n", ch->in_room);
    
	for (pass = ch->in_room->people; pass; pass = pass->next_in_room) 
	    if (AWAKE(pass))
		look_at_room(pass, car->in_room, 0);
    } else if ((other_rm = real_room(ROOM_NUMBER(car))) && other_rm->people) {
	sprintf(buf, "$p travels %s.", to_dirs[dir]);
	act(buf, FALSE, other_rm->people, car, 0, TO_ROOM);
	act(buf, FALSE, other_rm->people, car, 0, TO_CHAR);
    }

    if (engine) {
	if (CUR_ENERGY(engine) == 0) {
	    if (ch) {
		act("$p has run out of energy.", FALSE, ch, car, 0, TO_CHAR);
		act("$p sputters and dies.", FALSE, ch, engine, 0, TO_CHAR);
		act("$p sputters and dies.", FALSE, ch, engine, 0, TO_ROOM);
	    }
	    if (car->in_room->people) {
		act("$p sputters and dies.", 
		    FALSE, car->in_room->people, engine, 0, TO_CHAR);
		act("$p sputters and dies.", 
		    FALSE, car->in_room->people, engine, 0, TO_ROOM);
	    }

	    if (ENGINE_ON(engine))
		REMOVE_BIT(ENGINE_STATE(engine), ENG_RUN);
	    if (HEADLIGHTS_ON(engine)) {
		REMOVE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
		car->in_room->light--;
	    }
	} else if (CUR_ENERGY(engine) < 20 && ch) {
	    send_to_room("A warning indicator lights up.\r\n", ch->in_room);
	    send_to_char("*WARNING*  Low Energy Level\r\n", ch);
	}  
	return ERR_NONE;
    }
    return ERR_NONE;
}

ACMD(do_install)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *car = NULL, *engine = NULL;
    struct room_data *room = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
	send_to_char("USAGE: install <engine> <car>\r\n", ch);
	return;
    }

    car = get_obj_in_list_vis(ch, arg2, ch->in_room->contents);

    if (!car) {
	send_to_char("No such car here.\r\n", ch);
	return;
    } 
    if (GET_OBJ_TYPE(car) != ITEM_VEHICLE) {
	act("$p is not a motorized vehicle.", FALSE, ch, car, 0, TO_CHAR);
	return;
    }

    engine = get_obj_in_list_vis(ch, arg1, ch->carrying);

    if (!engine) {
	send_to_char("You carry no such engine.\r\n", ch);
	return;
    }
    if (GET_OBJ_TYPE(engine) != ITEM_ENGINE) {
	act("$p is not an engine.", FALSE, ch, engine, 0, TO_CHAR);
	return;
    }
  
    if (car->contains && IS_ENGINE(car->contains)) {
	act("$p is already installed in that vehicle.", 
	    FALSE, ch, car->contains, 0, TO_CHAR);
	return;
    }
    obj_from_char(engine);
    obj_to_obj(engine, car);
    act("$n installs $p in $P.", FALSE, ch, engine, car, TO_ROOM);
    act("You install $p into $P.", FALSE, ch, engine, car, TO_CHAR);

    if ((room = real_room(ROOM_NUMBER(car))) && room->people) {
	act("$n opens the hood of the car.\r\n"
	    "$n installs $p and closes the hood.",
	    FALSE, ch, engine,room->people, TO_VICT);
	act("$n opens the hood of the car.\r\n"
	    "$n installs $p and closes the hood.",
	    FALSE, ch, engine,room->people, TO_NOTVICT);
    }
}
  
ACMD(do_uninstall)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *car = NULL, *engine = NULL;
    struct room_data *room;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
	send_to_char("USAGE: uninstall <engine> <car>\r\n", ch);
	return;
    }

    car = get_obj_in_list_vis(ch, arg2, ch->in_room->contents);

    if (!car) {
	send_to_char("No such car here.\r\n", ch);
	return;
    } 
    if (GET_OBJ_TYPE(car) != ITEM_VEHICLE) {
	act("$p is not a motorized vehicle.", FALSE, ch, car, 0, TO_CHAR);
	return;
    }

    engine = get_obj_in_list_vis(ch, arg1, car->contains);

    if (!engine) {
	act("$p is not even installed with an engine!", FALSE, ch, car, 0, TO_CHAR);
	return;
    }
    obj_from_obj(engine);
    obj_to_char(engine, ch);
    act("$n removes $p from $P.", FALSE, ch, engine, car, TO_ROOM);
    act("You remove $p from $P.", FALSE, ch, engine, car, TO_CHAR);

    if ((room = real_room(ROOM_NUMBER(car))) && room->people) {
	act("$n opens the hood of the car and removes $p.",
	    FALSE, ch, engine,room->people, TO_VICT);
	act("$n opens the hood of the car and removes $p.",
	    FALSE, ch, engine,room->people, TO_NOTVICT);
    }
}
   
/********************************************************
*	The Room Specials vl1				*
*********************************************************/
SPECIAL(vehicle_door)
{

    struct obj_data *v_door = (struct obj_data *) me;
    struct obj_data *vehicle = NULL;

    if (!(vehicle = find_vehicle(v_door)))
	return 0;

    skip_spaces(&argument);

    if (*argument && !isname(argument, vehicle->name) &&
	!isname(argument, v_door->name)) 
	return 0;
  
    if (CMD_IS("exit") || CMD_IS("leave")) {
    
	if (CAR_CLOSED(vehicle)) {
	    act("$p is currently closed--your head gets a nasty whack.",
		FALSE, ch, v_door, 0, TO_CHAR);
	    act("$n bangs $s head into $p.", TRUE, ch, v_door, 0, TO_ROOM);
	    return 1;
	}
    
	if ( ROOM_FLAGGED( vehicle->in_room, ROOM_HOUSE ) && !House_can_enter( ch, vehicle->in_room->number ) ) {
	    send_to_char( "That's private property -- you can't go there.\r\n", ch );
	    return 1;
	}
	
	if ( ROOM_FLAGGED( vehicle->in_room, ROOM_CLAN_HOUSE ) &&
	     !clan_house_can_enter( ch, vehicle->in_room ) ) {
	    send_to_char( "That is clan property -- you aren't allowed to go there.\r\n", ch );
	    return 1;
	}

	act("You climb out of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n climbs out of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	char_from_room(ch);
	char_to_room(ch, vehicle->in_room);
	look_at_room(ch, ch->in_room, 0);
	act("$n has climbed out of $p.", TRUE, ch, vehicle, 0, TO_ROOM);
	GET_POS(ch) = POS_STANDING;
	return 1;
    }
 
    if (CMD_IS("open")) {

	if (!CAR_CLOSED(vehicle)) {
	    send_to_char("It's already open.\r\n", ch);
	    return 1;
	} 
	if (CAR_LOCKED(vehicle)) {
	    REMOVE_BIT(DOOR_STATE(vehicle), CONT_LOCKED | CONT_CLOSED);
	    act("You unlock and open the door of $p.", 
		FALSE, ch, vehicle, 0, TO_CHAR);
	    act("$n unlocks and opens the door of $p.", 
		FALSE, ch, vehicle, 0, TO_ROOM);
	} 
	else {
	    REMOVE_BIT(DOOR_STATE(vehicle), CONT_CLOSED);
	    act("You open the door of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	    act("$n opens the door of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	}
	act("The door of $p swings open.", FALSE, 0, vehicle, 0, TO_ROOM);
	return 1;
    }

    if (CMD_IS("unlock")) {
	if (!CAR_CLOSED(vehicle)) {
	    send_to_char("It's already open.\r\n", ch);
	    return 1;
	} 
	if (!CAR_OPENABLE(vehicle)) {
	    send_to_char("This door is not lockable.\r\n", ch);
	    return 1;
	}
	if (!CAR_LOCKED(vehicle)) {
	    send_to_char("It's already unlocked.\r\n", ch);
	    return 1;
	}
	REMOVE_BIT(GET_OBJ_VAL(vehicle, 1), CONT_LOCKED);
	act("You unlock the door of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n unlocks the door of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	return 1;
    }
    if (CMD_IS("lock")) {
	if (!CAR_OPENABLE(vehicle)) {
	    send_to_char("This car is not lockable.\r\n", ch);
	    return 1;
	}
	if (CAR_LOCKED(vehicle)) {
	    send_to_char("It's already locked.\r\n", ch);
	    return 1;
	}
	if (!CAR_CLOSED(vehicle)) {
	    SET_BIT(GET_OBJ_VAL(vehicle, 1), CONT_CLOSED | CONT_LOCKED);
	    act("You close and lock the door of $p.", 
		FALSE, ch, vehicle, 0, TO_CHAR);
	    act("$n closes and locks the door of $p.", 
		FALSE, ch, vehicle, 0, TO_ROOM);
	    act("The door of $p is closed from the inside.", 
		FALSE, 0, vehicle, 0, TO_ROOM);
	    return 1;
	} else {
	    SET_BIT(GET_OBJ_VAL(vehicle, 1), CONT_LOCKED);
	    act("You lock the door of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	    act("$n locks the door of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	    return 1;
	}
	return 0;
    }
    if (CMD_IS("close")) {
	if (!CAR_OPENABLE(vehicle)) {
	    send_to_char("This car is not closeable.\r\n", ch);
	    return 1;
	}
	if (CAR_CLOSED(vehicle)) {
	    send_to_char("It's closed already.\r\n", ch);
	    return 1;
	} 

	TOGGLE_BIT(GET_OBJ_VAL(vehicle, 1), CONT_CLOSED);
	act("You close the door of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n closes the door of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	act("The door of $p is closed from the inside.", 
	    FALSE, 0, vehicle, 0, TO_ROOM);
    
	return 1;
    }
    return 0;
}

SPECIAL(vehicle_console)
{
  
    struct obj_data *console = (struct obj_data *) me;
    struct obj_data *vehicle = NULL, *engine = NULL;
    struct char_data *driver = NULL;
    int dir;

    if (!CMD_IS("drive") && !CMD_IS("fly") && !CMD_IS("status") &&
	!CMD_IS("crank") && !CMD_IS("hotwire") && !CMD_IS("park") &&
	!CMD_IS("shutoff") && !CMD_IS("deactivate") && !CMD_IS("activate") &&
	!CMD_IS("honk") && !CMD_IS("headlights") && !CMD_IS("listen") &&
	!CMD_IS("exits") && !CMD_IS("rev") && !CMD_IS("spinout"))
	return 0;

    for (vehicle = object_list; vehicle; vehicle = vehicle->next) {
	if (GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(console) && vehicle->in_room)
	    break;
	if (!vehicle->next)
	    return 0;
    }

    cur_car = vehicle;

    if (!(engine = vehicle->contains) || !IS_ENGINE(engine))
	engine = NULL;

    if (V_CONSOLE_IDNUM(console)) {
	for (driver = console->in_room->people; driver; 
	     driver = driver->next_in_room) {
	    if ((V_CONSOLE_IDNUM(console) > 0 &&
		 GET_IDNUM(driver) == V_CONSOLE_IDNUM(console)) ||
		(V_CONSOLE_IDNUM(console) < 0 &&
		 GET_MOB_VNUM(driver) == -V_CONSOLE_IDNUM(console)))
		break;
	    if (!driver->next) {
		driver = NULL;
		break;
	    }
	}
    } else
	driver = NULL;

    skip_spaces(&argument);

    if (CMD_IS("exits")) {
	send_to_char("These are the exits from the room the car is in:\r\n", ch);
	ch->in_room = vehicle->in_room;
	do_exits(ch, "", 0, 0);
	ch->in_room = console->in_room;
	return 1;
    }
    if (CMD_IS("status")) {
	if (!engine) 
	    return 0;
	if (*argument && !isname(argument, console->name))
	    return 0;

	display_status(ch, vehicle, driver, engine);
	act("$n checks the vehicle status.", TRUE, ch, 0, 0, TO_ROOM);
	return 1;
    }

    if (!engine) 
	return 0;

    if (CMD_IS("listen")) {
	if (ENGINE_ON(engine)) {
	    act("You hear the sound of $p running.\r\n", 
		FALSE, ch, engine, 0, TO_CHAR);
	    return 1;
	}
	return 0;
    }
  
    if (driver && driver != ch) {
	act("$N is driving $p right now.", FALSE, ch, vehicle, driver, TO_CHAR);
	return 1;
    }
  
    if (CMD_IS("rev")) {
    
	if (!ENGINE_ON(engine)) {
	    send_to_char("You should probably crank it first.\r\n", ch);
	    return 1;
	}
    
	act("You rev the engine of $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n revs the engine of $p.", TRUE, ch, vehicle, 0, TO_ROOM);
	act("The engine of $p revs loudly.",
	    FALSE, 0, vehicle, 0, TO_ROOM | ACT_HIDECAR);
	return 1;
    } 
    if (CMD_IS("spinout")) {

	if (!ENGINE_ON(engine)) {
	    send_to_char("The engine's not even running.\r\n", ch);
	    return 1;
	}

	act("The tires of $p scream as they spin on the pavement.", 
	    FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n stomps the gas--you hear the tires scream.", 
	    FALSE, ch, vehicle, 0, TO_ROOM);
	act("The tires of $p scream as they spin on the pavement.",
	    FALSE, 0, vehicle, 0, TO_ROOM | ACT_HIDECAR);
	return 1;
    } 

    if (CMD_IS("honk")) {
	send_to_char("You honk the horn.\r\n", ch);
	act("$n honks the horn of $p.", FALSE, ch, vehicle, 0, TO_ROOM);
	act("There is a loud honking sound from $p.\r\n", 
	    FALSE, 0, vehicle, 0, TO_ROOM);
	return 1;
    }

    if (CMD_IS("headlights")) {
    
	if (!*argument) {
	    if (HEADLIGHTS_ON(engine))
		send_to_char("Headlight status: ON\r\n", ch);
	    else
		send_to_char("Headlight status: OFF\r\n", ch);
	    return 1;
	}
	else if (!strncasecmp(argument, "on", 2)) {
	    if (HEADLIGHTS_ON(engine))
		send_to_char("The headlights are already on.\r\n", ch);
	    else {
		act("You activate the exterior lights of $p.", 
		    FALSE, ch, vehicle, 0, TO_CHAR);
		act("$n activates the exterior lights of $p.", 
		    FALSE, ch, vehicle, 0, TO_ROOM);
		TOGGLE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
		CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - 1);
		vehicle->in_room->light++;
	    }
	    return 1;
	}
	else if (!strncasecmp(argument, "off", 3)) {
	    if (!HEADLIGHTS_ON(engine))
		send_to_char("The headlights are already off.\r\n", ch);
	    else {
		act("You turn off the exterior lights of $p.", 
		    FALSE, ch, vehicle, 0, TO_CHAR);
		act("$n turns off the exterior lights of $p.", 
		    FALSE, ch, vehicle, 0, TO_ROOM);
		TOGGLE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
		vehicle->in_room->light--;
	    }
	    return 1;
	}
	send_to_char("end_of_headlights_error\r\n", ch);
	return 0;
    }

    if (CMD_IS("drive") || (CMD_IS("fly") && IS_SKYCAR(vehicle))) {
    
	if (!*argument) {
	    send_to_char("Travel in which direction?\r\n", ch);
	    return 1;
	}

	if (!ENGINE_ON(engine)) {
	    send_to_char("What, without the engine running?\r\n", ch);
	    act("$n pretends to be driving $p.", TRUE, ch, vehicle, 0, TO_ROOM);
	    return 1;
	}
	if ((dir = search_block (argument, dirs, FALSE)) < 0) {
	    send_to_char("That's not a direction!\r\n", ch);
	    return 1;
	}
	
	if (!vehicle->in_room->dir_option[dir] || 
	    !vehicle->in_room->dir_option[dir]->to_room ||
	    ROOM_FLAGGED(vehicle->in_room->dir_option[dir]->to_room, ROOM_DEATH) ||
	    IS_SET(vehicle->in_room->dir_option[dir]->exit_info, EX_CLOSED)) {
	    send_to_char("You can't drive that way.\r\n", ch);
	    return 1;
	}
	if (IS_SET(ABS_EXIT(vehicle->in_room, dir)->exit_info, EX_ISDOOR) &&
	    ROOM_FLAGGED(vehicle->in_room->dir_option[dir]->to_room, 
			 ROOM_INDOORS)) {
	    send_to_char("You can't go through there!\r\n", ch);
	    return 1;
	}
	
	switch ( move_car(ch, vehicle, dir) ) {
	case ERR_NULL_DEST:
	case ERR_CLOSED_EX:
	    send_to_char( "Sorry, you can't go that way.\r\n", ch );
	    break;
	case ERR_NODRIVE:
	    act( "You cannot drive $p there.", FALSE, ch, vehicle, 0, TO_CHAR );
	    break;
	case ERR_HOUSE:
	    send_to_char( "That's private property -- you can't go there.\r\n", ch );
	    break;
	case ERR_CLAN:
	    send_to_char( "That is clan property -- you aren't allowed to go there.\r\n", ch );
	    break;
	case ERR_NONE:
	default:
	    break;
	}
	return 1;
    }

    if (*argument && !isname(argument, vehicle->name) && 
	!isname(argument, console->name))
	return 0;

    if (CMD_IS("crank") || CMD_IS("activate")) {

	if (KEY_NUMBER(vehicle) != -1 && !has_car_key(ch, KEY_NUMBER(vehicle))) {
	    send_to_char("You need a key for that.\r\n", ch);
	    return 1;
	}

	start_engine(ch, vehicle, engine, console);
	return 1;

    }

    if (CMD_IS("hotwire")) {

	act("You attempt to hot-wire $p.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n attempts to hot-wire $p.", TRUE, ch, vehicle, 0, TO_ROOM);

	if (number(0, 101) > CHECK_SKILL(ch, SKILL_HOTWIRE)) {
	    send_to_char("You fail.\r\n", ch);
	    return 1;
	}

	start_engine(ch, vehicle, engine, console);
	return 1;
    }

    if (CMD_IS("shutoff") || CMD_IS("deactivate")) {

	if (!ENGINE_ON(engine)) {
	    send_to_char("It's not even running.\r\n", ch);
	    return 1;
	}  
	if (!IS_PARKED(engine)) {
	    send_to_char("You had better park the sucker first.\r\n", ch);
	    return 1;
	}

	act("You shut down $p.", FALSE, ch, engine, 0, TO_CHAR);
	act("$n shuts down $p.", FALSE, ch, engine, 0, TO_ROOM);
	act("$p's engine stops running and becomes quiet.",
	    FALSE, 0, vehicle, 0, TO_ROOM);
	REMOVE_BIT(ENGINE_STATE(engine), ENG_RUN);
	V_CONSOLE_IDNUM(console) = 0;

	return 1;
    }

    if (CMD_IS("park")) {

	V_CONSOLE_IDNUM(console) = 0;

	if (IS_PARKED(engine)) {
	    send_to_char("It's already parked.\r\n", ch);
	    return 1;
	}
	act("You put $p in park.", FALSE, ch, vehicle, 0, TO_CHAR);
	act("$n puts $p in park.", FALSE, ch, vehicle, 0, TO_ROOM);
	SET_BIT(ENGINE_STATE(engine), ENG_PARK);
	return 1;
    }

    return 0;
}

//
// find_vehicle only works for vehicles that are in_room
//

struct obj_data *
find_vehicle(struct obj_data *v_door)
{
    struct obj_data *vehicle = NULL;

    for (vehicle = object_list; vehicle; vehicle = vehicle->next) {
	if (ROOM_NUMBER(vehicle) == ROOM_NUMBER(v_door) &&
	    GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(v_door) && vehicle->in_room)
	    return vehicle;
    }
    return NULL;
}

#undef __vehicle_c__
