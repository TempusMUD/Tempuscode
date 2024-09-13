//
// File: vehicle.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/****************************************************************************
        VEHICLE.C
****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "sector.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "tmpstr.h"
#include "spells.h"
#include "vehicle.h"
#include "obj_data.h"
#include "strutil.h"
#include "weather.h"

/*   external vars  */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct obj_data *object_list;
extern int *has_key(struct creature *ch, int key);
void show_obj_to_char(struct obj_data *obj, struct creature *ch, int mode);

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

int
has_car_key(struct creature *ch, room_num car_room)
{
    struct obj_data *key = NULL;

    if (GET_LEVEL(ch) > LVL_GRGOD) {
        return 1;
    }

    for (key = ch->carrying; key; key = key->next_content) {
        if (KEY_TO_CAR(key) == car_room) {
            return 1;
        }
    }

    if ((key = GET_EQ(ch, WEAR_HOLD))) {
        if (KEY_TO_CAR(key) == car_room) {
            return 1;
        }
    }

    return 0;
}

void
display_status(struct creature *ch, struct obj_data *car,
               struct creature *driver, struct obj_data *engine)
{
    if (!ch || !engine || !car) {
        return;
    }

    send_to_char(ch, "You examine the instrument panel.\r\n");
    if (!number(0, 5)) {
        snprintf(buf, sizeof(buf),
                 "%sThe %sinstrument %spanel %sblinks %sfor %sa %smoment%s.", CCGRN(ch, C_SPR),
                 CCBLU(ch, C_SPR), CCMAG(ch, C_SPR), CCYEL(ch, C_SPR), CCCYN(ch, C_SPR), CCYEL(ch, C_SPR), CCBLU(ch, C_SPR), CCNRM(ch, C_SPR));
        act(buf, false, ch, NULL, NULL, TO_ROOM);
    }
    send_to_char(ch,
                 "\r\n%s<<<<<<<<%sSYSTEM STATUS UPDATE (%s)%s>>>>>>>>>%s\r\n", CCRED(ch, C_SPR),
                 CCNRM(ch, C_SPR), car->name, CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
    snprintf(buf, sizeof(buf),
             "%s*******************************************************%s\r\n",
             CCBLU(ch, C_SPR), CCNRM(ch, C_SPR));
    send_to_char(ch, "%s", buf);
    sprintbit(ENGINE_STATE(engine), engine_state_bits, buf2, sizeof(buf2));
    send_to_char(ch, "%sEngine State:%s    %s\r\n", CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), buf2);
    sprintbit(DOOR_STATE(car), container_bits, buf2, sizeof(buf2));
    send_to_char(ch, "%sDoor State:%s      %s\r\n", CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), buf2);
    snprintf(buf, sizeof(buf),
             "%sEnergy Status:%s   [%s%d / %d%s]%s\r\n"
             "%sHeadlights are:%s  %s\r\n"
             "%sDriver is:%s       %s\r\n"
             "%sVhcl is located:%s %s\r\n"
             "%sThe weather outside is:%s    %s\r\n",
             CCCYN(ch, C_SPR), CCRED(ch, C_SPR), CCNRM(ch, C_SPR), CUR_ENERGY(engine), MAX_ENERGY(engine), CCRED(ch, C_SPR), CCNRM(ch, C_SPR),
             CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), (HEADLIGHTS_ON(engine) ? "ON" : "OFF"),
             CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), driver ? PERS(ch, driver) : "No-one",
             CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), car->in_room->name,
             CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), outside_weather[(int)car->in_room->zone->weather->sky]);
    send_to_char(ch, "%s", buf);
    if (LOW_ENERGY(engine)) {
        send_to_char(ch, "A %sWARNING%s indicator lights up.", CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
        act(buf, false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch, "%s***%sWARNING%s***%s (Low Energy Level).\r\n", CCGRN(ch, C_SPR),
                     CCRED(ch, C_SPR), CCGRN(ch, C_SPR), CCNRM(ch, C_SPR));
    }

    send_to_char(ch,
                 "%s*******************************************************%s\r\n",
                 CCBLU(ch, C_SPR), CCNRM(ch, C_SPR));
}

void
start_engine(struct creature *ch, struct obj_data *car,
             struct obj_data *engine, struct obj_data *console)
{

    if (!IS_NPC(ch)) {
        V_CONSOLE_IDNUM(console) = GET_IDNUM(ch);
    } else {
        V_CONSOLE_IDNUM(console) = -(GET_NPC_VNUM(ch));
    }

    if (ENGINE_ON(engine)) {
        act("The starter squeals as you try to crank $p!",
            false, ch, car, NULL, TO_CHAR);
        act("The starter squeals as $n tries to crank $p!",
            false, ch, car, NULL, TO_ROOM);
        if (car->in_room->people) {
            act("You hear a loud squealing from under the hood of $p.",
                false, NULL, car, NULL, TO_ROOM);
        }
        return;
    }
    if (CUR_ENERGY(engine) == 0) {
        act("$n tries to crank $p.", true, ch, car, NULL, TO_ROOM);
        act("$p turns over sluggishly, but doesn't start.",
            false, ch, engine, NULL, TO_ROOM);
        act("$p turns over and over sluggishly.", false, ch, engine, NULL,
            TO_CHAR);
        if (car->in_room->people) {
            act("$p's engine turns over and over sluggishly.",
                false, NULL, car, NULL, TO_ROOM);
        }
        return;
    }
    if (LOW_ENERGY(engine)) {
        act("$n tries to crank $p.", true, ch, car, NULL, TO_ROOM);
        act("$p turns over sluggishly and sputters to life.",
            false, ch, engine, NULL, TO_ROOM);
        act("$p turns over sluggishly and sputters to life.",
            false, ch, engine, NULL, TO_CHAR);
        if (car->in_room->people) {
            act("$p's engine turns over sluggishly and sputters to life.",
                false, NULL, car, NULL, TO_ROOM);
        }
        TOGGLE_BIT(ENGINE_STATE(engine), ENG_RUN);
        CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - USE_RATE(engine) * 2);
        return;
    }
    act("$n tries to crank $p.", true, ch, car, NULL, TO_ROOM);
    act("$p turns over and roars to life.", false, ch, engine, NULL, TO_ROOM);
    act("$p turns over and roars to life.", false, ch, engine, NULL, TO_CHAR);
    if (car->in_room->people) {
        act("$p's engine turns over and roars to life.",
            false, NULL, car, NULL, TO_ROOM);
    }
    TOGGLE_BIT(ENGINE_STATE(engine), ENG_RUN);
    CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - USE_RATE(engine) * 2);
}

int
move_car(struct creature *ch, struct obj_data *car, int dir)
{
    int energy_cost = 0;
    struct room_data *dest = NULL, *other_rm = NULL;
    struct obj_data *engine = car->contains;
    char lbuf[256], abuf[256];

    cur_car = car;

    if (!OEXIT(car, dir) || !(dest = OEXIT(car, dir)->to_room)) {
        return ERR_NULL_DEST;
    }
    if (IS_SET(OEXIT(car, dir)->exit_info, EX_CLOSED)) {
        return ERR_CLOSED_EX;
    }

    if (engine && !IS_ENGINE(engine)) {
        engine = NULL;
    }

    if ((IS_ROADCAR(car) && (SECT_TYPE(dest) != SECT_CITY &&
                             SECT_TYPE(dest) != SECT_ROAD &&
                             SECT_TYPE(dest) != SECT_INSIDE &&
                             SECT_TYPE(dest) != SECT_FIELD)) ||
        (IS_SKYCAR(car) && room_is_underwater(dest))) {
        return ERR_NODRIVE;
    }

    if (ROOM_FLAGGED(dest, ROOM_HOUSE) && ch
        && !can_enter_house(ch, dest->number)) {
        return ERR_HOUSE;
    }

    if (ROOM_FLAGGED(dest, ROOM_CLAN_HOUSE) && ch &&
        !clan_house_can_enter(ch, dest)) {
        return ERR_CLAN;
    }

    if (engine) {
        energy_cost = ((sector_by_idnum(car->in_room->sector_type)->moveloss +
                        sector_by_idnum(dest->sector_type)->moveloss) * USE_RATE(engine));
        REMOVE_BIT(ENGINE_STATE(engine), ENG_PARK);

        CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - energy_cost);

        if (IS_ENGINE_SET(engine, ENG_PETROL)) {
            snprintf(lbuf, sizeof(lbuf), "$p leaves noisily to the %s.", dirs[dir]);
            snprintf(abuf, sizeof(abuf), "$p arrives noisily from %s.", from_dirs[dir]);
        } else if (IS_ENGINE_SET(engine, ENG_ELECTRIC)) {
            snprintf(lbuf, sizeof(lbuf), "$p hums off to the %s.", dirs[dir]);
            snprintf(abuf, sizeof(abuf), "$p hums in from %s.", from_dirs[dir]);
        } else {
            snprintf(lbuf, sizeof(lbuf), "$p moves off to the %s.", dirs[dir]);
            snprintf(abuf, sizeof(abuf), "$p moves in from %s.", from_dirs[dir]);
        }
    } else if (IS_BOAT(car)) {
        snprintf(lbuf, sizeof(lbuf), "$p is rowed off to the %s.", dirs[dir]);
        snprintf(abuf, sizeof(abuf), "$p is rowed in from %s.", from_dirs[dir]);
    } else {
        snprintf(lbuf, sizeof(lbuf), "$p moves off to the %s.", dirs[dir]);
        snprintf(abuf, sizeof(abuf), "$p moves in from %s.", from_dirs[dir]);
    }
    if (IS_SKYCAR(car)) {
        snprintf(lbuf, sizeof(lbuf), "$p flies off to the %s.", dirs[dir]);
        snprintf(abuf, sizeof(abuf), "$p flies in from %s.", from_dirs[dir]);
    } else if (IS_BOAT(car)) {
        snprintf(lbuf, sizeof(lbuf), "$p sails off to the %s.", dirs[dir]);
        snprintf(abuf, sizeof(abuf), "$p sails in from %s.", from_dirs[dir]);
    }

    act(lbuf, false, NULL, car, NULL, TO_ROOM | ACT_HIDECAR);

    obj_from_room(car);
    obj_to_room(car, dest);

    act(abuf, false, NULL, car, NULL, TO_ROOM | ACT_HIDECAR);

    if (car->action_desc && OCAN_GO(car, dir) &&
        (other_rm = OEXIT(car, dir)->to_room) && other_rm->people) {
        snprintf(buf, sizeof(buf), "%s %s.", car->action_desc, from_dirs[dir]);
        act(buf, false, other_rm->people->data, car, NULL, TO_ROOM);
        act(buf, false, other_rm->people->data, car, NULL, TO_CHAR);
    }

    if (ch) {
        snprintf(buf, sizeof(buf), "$n drives $p %s.", dirs[dir]);
        act(buf, false, ch, car, NULL, TO_ROOM);

        if (IS_SKYCAR(car)) {
            send_to_room("You see as you fly up: \r\n", ch->in_room);
        } else {
            send_to_room("You see as you drive up: \r\n", ch->in_room);
        }

        for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
            struct creature *tch = it->data;
            if (AWAKE(tch)) {
                look_at_room(tch, car->in_room, 0);
            }
        }
    } else if ((other_rm = real_room(ROOM_NUMBER(car))) && other_rm->people) {
        snprintf(buf, sizeof(buf), "$p travels %s.", to_dirs[dir]);
        act(buf, false, other_rm->people->data, car, NULL, TO_ROOM);
        act(buf, false, other_rm->people->data, car, NULL, TO_CHAR);
    }

    if (engine) {
        if (CUR_ENERGY(engine) == 0) {
            if (ch) {
                act("$p has run out of energy.", false, ch, car, NULL, TO_CHAR);
                act("$p sputters and dies.", false, ch, engine, NULL, TO_CHAR);
                act("$p sputters and dies.", false, ch, engine, NULL, TO_ROOM);
            }
            if (car->in_room) {
                act("$p sputters and dies.", false, NULL, car, NULL, TO_CHAR);
                act("$p sputters and dies.", false, NULL, car, NULL, TO_ROOM);
            }

            if (ENGINE_ON(engine)) {
                REMOVE_BIT(ENGINE_STATE(engine), ENG_RUN);
            }
            if (HEADLIGHTS_ON(engine)) {
                REMOVE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
                car->in_room->light--;
            }
        } else if (CUR_ENERGY(engine) < 20 && ch) {
            send_to_room("A warning indicator lights up.\r\n", ch->in_room);
            send_to_char(ch, "*WARNING*  Low Energy Level\r\n");
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
        send_to_char(ch, "USAGE: install <engine> <car>\r\n");
        return;
    }

    car = get_obj_in_list_vis(ch, arg2, ch->in_room->contents);

    if (!car) {
        send_to_char(ch, "No such car here.\r\n");
        return;
    }
    if (GET_OBJ_TYPE(car) != ITEM_VEHICLE) {
        act("$p is not a motorized vehicle.", false, ch, car, NULL, TO_CHAR);
        return;
    }

    engine = get_obj_in_list_vis(ch, arg1, ch->carrying);

    if (!engine) {
        send_to_char(ch, "You carry no such engine.\r\n");
        return;
    }
    if (GET_OBJ_TYPE(engine) != ITEM_ENGINE) {
        act("$p is not an engine.", false, ch, engine, NULL, TO_CHAR);
        return;
    }

    if (car->contains && IS_ENGINE(car->contains)) {
        act("$p is already installed in that vehicle.",
            false, ch, car->contains, NULL, TO_CHAR);
        return;
    }
    obj_from_char(engine);
    obj_to_obj(engine, car);
    act("$n installs $p in $P.", false, ch, engine, car, TO_ROOM);
    act("You install $p into $P.", false, ch, engine, car, TO_CHAR);

    if ((room = real_room(ROOM_NUMBER(car))) && room->people) {
        act("$n opens the hood of the car.\r\n"
            "$n installs $p and closes the hood.",
            false, ch, engine, room->people->data, TO_VICT);
        act("$n opens the hood of the car.\r\n"
            "$n installs $p and closes the hood.",
            false, ch, engine, room->people->data, TO_NOTVICT);
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
        send_to_char(ch, "USAGE: uninstall <engine> <car>\r\n");
        return;
    }

    car = get_obj_in_list_vis(ch, arg2, ch->in_room->contents);

    if (!car) {
        send_to_char(ch, "No such car here.\r\n");
        return;
    }
    if (GET_OBJ_TYPE(car) != ITEM_VEHICLE) {
        act("$p is not a motorized vehicle.", false, ch, car, NULL, TO_CHAR);
        return;
    }

    engine = get_obj_in_list_vis(ch, arg1, car->contains);

    if (!engine) {
        act("$p is not even installed with an engine!", false, ch, car, NULL,
            TO_CHAR);
        return;
    }
    obj_from_obj(engine);
    obj_to_char(engine, ch);
    act("$n removes $p from $P.", false, ch, engine, car, TO_ROOM);
    act("You remove $p from $P.", false, ch, engine, car, TO_CHAR);

    if ((room = real_room(ROOM_NUMBER(car))) && room->people) {
        act("$n opens the hood of the car and removes $p.",
            false, ch, engine, room->people->data, TO_VICT);
        act("$n opens the hood of the car and removes $p.",
            false, ch, engine, room->people->data, TO_NOTVICT);
    }
}

/********************************************************
 *        The Room Specials vl1                                *
 *********************************************************/
SPECIAL(vehicle_door)
{

    struct obj_data *v_door = (struct obj_data *)me;
    struct obj_data *vehicle = NULL;

    if (spec_mode != SPECIAL_CMD) {
        return 0;
    }

    if (!(vehicle = find_vehicle(v_door))) {
        return 0;
    }

    skip_spaces(&argument);

    if (*argument && !isname(argument, vehicle->aliases) &&
        !isname(argument, v_door->aliases)) {
        return 0;
    }

    if (CMD_IS("exit") || CMD_IS("leave")) {

        if (CAR_CLOSED(vehicle)) {
            act("$p is currently closed--your head gets a nasty whack.",
                false, ch, v_door, NULL, TO_CHAR);
            act("$n bangs $s head into $p.", true, ch, v_door, NULL, TO_ROOM);
            return 1;
        }

        if (ROOM_FLAGGED(vehicle->in_room, ROOM_HOUSE)
            && !can_enter_house(ch, vehicle->in_room->number)) {
            send_to_char(ch,
                         "That's private property -- you can't go there.\r\n");
            return 1;
        }

        if (ROOM_FLAGGED(vehicle->in_room, ROOM_CLAN_HOUSE) &&
            !clan_house_can_enter(ch, vehicle->in_room)) {
            send_to_char(ch,
                         "That is clan property -- you aren't allowed to go there.\r\n");
            return 1;
        }

        act("You climb out of $p.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n climbs out of $p.", false, ch, vehicle, NULL, TO_ROOM);
        char_from_room(ch, true);
        char_to_room(ch, vehicle->in_room, true);
        look_at_room(ch, ch->in_room, 0);
        act("$n has climbed out of $p.", true, ch, vehicle, NULL, TO_ROOM);
        GET_POSITION(ch) = POS_STANDING;
        return 1;
    }

    if (CMD_IS("open")) {

        if (!CAR_CLOSED(vehicle)) {
            send_to_char(ch, "It's already open.\r\n");
            return 1;
        }
        if (CAR_LOCKED(vehicle)) {
            REMOVE_BIT(DOOR_STATE(vehicle), CONT_LOCKED | CONT_CLOSED);
            act("You unlock and open the door of $p.",
                false, ch, vehicle, NULL, TO_CHAR);
            act("$n unlocks and opens the door of $p.",
                false, ch, vehicle, NULL, TO_ROOM);
        } else {
            REMOVE_BIT(DOOR_STATE(vehicle), CONT_CLOSED);
            act("You open the door of $p.", false, ch, vehicle, NULL, TO_CHAR);
            act("$n opens the door of $p.", false, ch, vehicle, NULL, TO_ROOM);
        }
        act("The door of $p swings open.", false, NULL, vehicle, NULL, TO_ROOM);
        return 1;
    }

    if (CMD_IS("unlock")) {
        if (!CAR_CLOSED(vehicle)) {
            send_to_char(ch, "It's already open.\r\n");
            return 1;
        }
        if (!CAR_OPENABLE(vehicle)) {
            send_to_char(ch, "This door is not lockable.\r\n");
            return 1;
        }
        if (!CAR_LOCKED(vehicle)) {
            send_to_char(ch, "It's already unlocked.\r\n");
            return 1;
        }
        REMOVE_BIT(GET_OBJ_VAL(vehicle, 1), CONT_LOCKED);
        act("You unlock the door of $p.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n unlocks the door of $p.", false, ch, vehicle, NULL, TO_ROOM);
        return 1;
    }
    if (CMD_IS("lock")) {
        if (!CAR_OPENABLE(vehicle)) {
            send_to_char(ch, "This car is not lockable.\r\n");
            return 1;
        }
        if (CAR_LOCKED(vehicle)) {
            send_to_char(ch, "It's already locked.\r\n");
            return 1;
        }
        if (!CAR_CLOSED(vehicle)) {
            SET_BIT(GET_OBJ_VAL(vehicle, 1), CONT_CLOSED | CONT_LOCKED);
            act("You close and lock the door of $p.",
                false, ch, vehicle, NULL, TO_CHAR);
            act("$n closes and locks the door of $p.",
                false, ch, vehicle, NULL, TO_ROOM);
            act("The door of $p is closed from the inside.",
                false, NULL, vehicle, NULL, TO_ROOM);
            return 1;
        } else {
            SET_BIT(GET_OBJ_VAL(vehicle, 1), CONT_LOCKED);
            act("You lock the door of $p.", false, ch, vehicle, NULL, TO_CHAR);
            act("$n locks the door of $p.", false, ch, vehicle, NULL, TO_ROOM);
            return 1;
        }
        return 0;
    }
    if (CMD_IS("close")) {
        if (!CAR_OPENABLE(vehicle)) {
            send_to_char(ch, "This car is not closeable.\r\n");
            return 1;
        }
        if (CAR_CLOSED(vehicle)) {
            send_to_char(ch, "It's closed already.\r\n");
            return 1;
        }

        TOGGLE_BIT(GET_OBJ_VAL(vehicle, 1), CONT_CLOSED);
        act("You close the door of $p.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n closes the door of $p.", false, ch, vehicle, NULL, TO_ROOM);
        act("The door of $p is closed from the inside.",
            false, NULL, vehicle, NULL, TO_ROOM);

        return 1;
    }
    return 0;
}

static struct creature *
find_driver(struct obj_data *console)
{
    struct creature *driver = NULL;

    for (GList *it = first_living(console->in_room->people); it; it = next_living(it)) {
        driver = it->data;
        if ((V_CONSOLE_IDNUM(console) > 0 &&
             GET_IDNUM(driver) == V_CONSOLE_IDNUM(console)) ||
            (V_CONSOLE_IDNUM(console) < 0 &&
             GET_NPC_VNUM(driver) == -V_CONSOLE_IDNUM(console))) {
            return driver;
        }
    }
    return NULL;
}

SPECIAL(vehicle_console)
{

    struct obj_data *console = (struct obj_data *)me;
    struct obj_data *vehicle = NULL, *engine = NULL;
    struct creature *driver = NULL;
    int dir;

    if (!CMD_IS("drive") && !CMD_IS("fly") && !CMD_IS("status") &&
        !CMD_IS("crank") && !CMD_IS("hotwire") && !CMD_IS("park") &&
        !CMD_IS("shutoff") && !CMD_IS("deactivate") && !CMD_IS("activate") &&
        !CMD_IS("honk") && !CMD_IS("headlights") && !CMD_IS("listen") &&
        !CMD_IS("exits") && !CMD_IS("rev") && !CMD_IS("spinout")) {
        return 0;
    }

    for (vehicle = object_list; vehicle; vehicle = vehicle->next) {
        if (GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(console) && vehicle->in_room) {
            break;
        }
    }

    if (!vehicle) {
        return 0;
    }

    cur_car = vehicle;

    engine = vehicle->contains;
    if (engine && !IS_ENGINE(engine)) {
        engine = NULL;
    }

    if (V_CONSOLE_IDNUM(console)) {
        if (!console->in_room) {
            send_to_char(ch,
                         "You have to put the console IN the vehicle to use it.\r\n");
            return 1;
        }
        driver = find_driver(console);
    }

    skip_spaces(&argument);

    if (CMD_IS("exits")) {
        send_to_char(ch,
                     "These are the exits from the room the car is in:\r\n");
        ch->in_room = vehicle->in_room;
        do_exits(ch, tmp_strdup(""), 0, 0);
        ch->in_room = console->in_room;
        return 1;
    }
    if (CMD_IS("status")) {
        if (!engine) {
            return 0;
        }
        if (*argument && !isname(argument, console->aliases)) {
            return 0;
        }

        display_status(ch, vehicle, driver, engine);
        act("$n checks the vehicle status.", true, ch, NULL, NULL, TO_ROOM);
        return 1;
    }

    if (!engine) {
        return 0;
    }

    if (CMD_IS("listen")) {
        if (ENGINE_ON(engine)) {
            act("You hear the sound of $p running.\r\n",
                false, ch, engine, NULL, TO_CHAR);
            return 1;
        }
        return 0;
    }

    if (driver && driver != ch) {
        act("$N is driving $p right now.", false, ch, vehicle, driver,
            TO_CHAR);
        return 1;
    }

    if (CMD_IS("rev")) {

        if (!ENGINE_ON(engine)) {
            send_to_char(ch, "You should probably crank it first.\r\n");
            return 1;
        }

        act("You rev the engine of $p.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n revs the engine of $p.", true, ch, vehicle, NULL, TO_ROOM);
        act("The engine of $p revs loudly.",
            false, NULL, vehicle, NULL, TO_ROOM | ACT_HIDECAR);
        return 1;
    }
    if (CMD_IS("spinout")) {

        if (!ENGINE_ON(engine)) {
            send_to_char(ch, "The engine's not even running.\r\n");
            return 1;
        }

        act("The tires of $p scream as they spin on the pavement.",
            false, ch, vehicle, NULL, TO_CHAR);
        act("$n stomps the gas--you hear the tires scream.",
            false, ch, vehicle, NULL, TO_ROOM);
        act("The tires of $p scream as they spin on the pavement.",
            false, NULL, vehicle, NULL, TO_ROOM | ACT_HIDECAR);
        return 1;
    }

    if (CMD_IS("honk")) {
        send_to_char(ch, "You honk the horn.\r\n");
        act("$n honks the horn of $p.", false, ch, vehicle, NULL, TO_ROOM);
        act("There is a loud honking sound from $p.\r\n",
            false, NULL, vehicle, NULL, TO_ROOM);
        return 1;
    }

    if (CMD_IS("headlights")) {

        if (!*argument) {
            if (HEADLIGHTS_ON(engine)) {
                send_to_char(ch, "Headlight status: ON\r\n");
            } else {
                send_to_char(ch, "Headlight status: OFF\r\n");
            }
            return 1;
        } else if (!strncasecmp(argument, "on", 2)) {
            if (HEADLIGHTS_ON(engine)) {
                send_to_char(ch, "The headlights are already on.\r\n");
            } else {
                act("You activate the exterior lights of $p.",
                    false, ch, vehicle, NULL, TO_CHAR);
                act("$n activates the exterior lights of $p.",
                    false, ch, vehicle, NULL, TO_ROOM);
                TOGGLE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
                CUR_ENERGY(engine) = MAX(0, CUR_ENERGY(engine) - 1);
                vehicle->in_room->light++;
            }
            return 1;
        } else if (!strncasecmp(argument, "off", 3)) {
            if (!HEADLIGHTS_ON(engine)) {
                send_to_char(ch, "The headlights are already off.\r\n");
            } else {
                act("You turn off the exterior lights of $p.",
                    false, ch, vehicle, NULL, TO_CHAR);
                act("$n turns off the exterior lights of $p.",
                    false, ch, vehicle, NULL, TO_ROOM);
                TOGGLE_BIT(ENGINE_STATE(engine), ENG_LIGHTS);
                vehicle->in_room->light--;
            }
            return 1;
        }
        send_to_char(ch, "end_of_headlights_error\r\n");
        return 0;
    }

    if (CMD_IS("drive") || (CMD_IS("fly") && IS_SKYCAR(vehicle))) {

        if (!*argument) {
            send_to_char(ch, "Travel in which direction?\r\n");
            return 1;
        }

        if (!ENGINE_ON(engine)) {
            send_to_char(ch, "What, without the engine running?\r\n");
            act("$n pretends to be driving $p.", true, ch, vehicle, NULL,
                TO_ROOM);
            return 1;
        }
        if ((dir = search_block(argument, dirs, false)) < 0) {
            send_to_char(ch, "That's not a direction!\r\n");
            return 1;
        }

        if (!vehicle->in_room->dir_option[dir] ||
            !vehicle->in_room->dir_option[dir]->to_room ||
            ROOM_FLAGGED(vehicle->in_room->dir_option[dir]->to_room,
                         ROOM_DEATH)
            || IS_SET(vehicle->in_room->dir_option[dir]->exit_info, EX_CLOSED)) {
            send_to_char(ch, "You can't drive that way.\r\n");
            return 1;
        }
        if (IS_SET(ABS_EXIT(vehicle->in_room, dir)->exit_info, EX_ISDOOR) &&
            ROOM_FLAGGED(vehicle->in_room->dir_option[dir]->to_room,
                         ROOM_INDOORS)) {
            send_to_char(ch, "You can't go through there!\r\n");
            return 1;
        }

        switch (move_car(ch, vehicle, dir)) {
        case ERR_NULL_DEST:
        case ERR_CLOSED_EX:
            send_to_char(ch, "Sorry, you can't go that way.\r\n");
            break;
        case ERR_NODRIVE:
            act("You cannot drive $p there.", false, ch, vehicle, NULL, TO_CHAR);
            break;
        case ERR_HOUSE:
            send_to_char(ch,
                         "That's private property -- you can't go there.\r\n");
            break;
        case ERR_CLAN:
            send_to_char(ch,
                         "That is clan property -- you aren't allowed to go there.\r\n");
            break;
        case ERR_NONE:
        default:
            break;
        }
        return 1;
    }

    if (*argument && !isname(argument, vehicle->aliases) &&
        !isname(argument, console->aliases)) {
        return 0;
    }

    if (CMD_IS("crank") || CMD_IS("activate")) {

        if (KEY_NUMBER(vehicle) != -1 && !has_car_key(ch, KEY_NUMBER(vehicle))) {
            send_to_char(ch, "You need a key for that.\r\n");
            return 1;
        }

        start_engine(ch, vehicle, engine, console);
        return 1;

    }

    if (CMD_IS("hotwire")) {

        act("You attempt to hot-wire $p.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n attempts to hot-wire $p.", true, ch, vehicle, NULL, TO_ROOM);

        if (number(0, 101) > CHECK_SKILL(ch, SKILL_HOTWIRE)) {
            send_to_char(ch, "You fail.\r\n");
            return 1;
        }

        start_engine(ch, vehicle, engine, console);
        return 1;
    }

    if (CMD_IS("shutoff") || CMD_IS("deactivate")) {

        if (!ENGINE_ON(engine)) {
            send_to_char(ch, "It's not even running.\r\n");
            return 1;
        }
        if (!IS_PARKED(engine)) {
            send_to_char(ch, "You had better park the sucker first.\r\n");
            return 1;
        }

        act("You shut down $p.", false, ch, engine, NULL, TO_CHAR);
        act("$n shuts down $p.", false, ch, engine, NULL, TO_ROOM);
        act("$p's engine stops running and becomes quiet.",
            false, NULL, vehicle, NULL, TO_ROOM);
        REMOVE_BIT(ENGINE_STATE(engine), ENG_RUN);
        V_CONSOLE_IDNUM(console) = 0;

        return 1;
    }

    if (CMD_IS("park")) {

        V_CONSOLE_IDNUM(console) = 0;

        if (IS_PARKED(engine)) {
            send_to_char(ch, "It's already parked.\r\n");
            return 1;
        }
        act("You put $p in park.", false, ch, vehicle, NULL, TO_CHAR);
        act("$n puts $p in park.", false, ch, vehicle, NULL, TO_ROOM);
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
            GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(v_door) && vehicle->in_room) {
            return vehicle;
        }
    }
    return NULL;
}

#undef __vehicle_c__
