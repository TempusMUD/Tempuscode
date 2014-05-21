// File: hell_hunter.spec                     -- Part of TempusMUD
//
// DataFile: lib/etc/hell_hunter_data
//
// Copyright 1998 by John Watson, all rights reserved.
// Hacked to use classes and XML John Rothe 2001
//
#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "spells.h"
#include "fight.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "specs.h"
#include "strutil.h"
#include "hell_hunter_spec.h"

/*   external vars  */
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];

/* extern functions */
void add_follower(struct creature *ch, struct creature *leader);
void do_auto_exits(struct creature *ch, room_num room);
int get_check_money(struct creature *ch, struct obj_data **obj, int display);

ACMD(do_echo);
ACMD(do_gen_comm);
ACMD(do_rescue);
ACMD(do_steal);
ACMD(do_get);

GList *devils = NULL;
GList *targets = NULL;
GList *hunters = NULL;
GList *blind_spots = NULL;

struct devil *
load_devil(xmlNodePtr n)
{
    struct devil *devil;

    CREATE(devil, struct devil, 1);
    devil->vnum = xmlGetIntProp(n, "ID", 0);
    xmlChar *p = xmlGetProp(n, (const xmlChar *)"Name");

    if (p) {
        devil->name = strdup("Invalid");
    } else {
        devil->name = (char *)p;
    }
    return devil;
}

struct target *
load_target(xmlNodePtr n)
{
    struct target *target;

    CREATE(target, struct target, 1);
    target->o_vnum = xmlGetIntProp(n, "ID", 0);
    target->level = xmlGetIntProp(n, "Level", 0);

    return target;
}

struct devil *
devil_by_name(char *name)
{
    for (GList * dit = devils; dit; dit = dit->next) {
        struct devil *devil = dit->data;
        if (!strcmp(devil->name, name))
            return devil;
    }
    return NULL;
}

struct hunter *
load_hunter(xmlNodePtr n)
{
    struct hunter *hunter;

    CREATE(hunter, struct hunter, 1);

    hunter->weapon = xmlGetIntProp(n, "Weapon", 0);
    hunter->prob = xmlGetIntProp(n, "Probability", 0);

    xmlChar *c = xmlGetProp(n, (const xmlChar *)"Name");
    struct devil *devil = devil_by_name((char *)c);
    if (devil)
        hunter->m_vnum = devil->vnum;
    free(c);

    return hunter;
}

struct hunt_group *
load_hunt_group(xmlNodePtr n)
{
    struct hunt_group *hunt_group;

    CREATE(hunt_group, struct hunt_group, 1);
    hunt_group->level = xmlGetIntProp(n, "Level", 0);
    for (xmlNodePtr node = n->xmlChildrenNode; n; n = n->next) {
        if (xmlMatches(node->name, "HUNTER")) {
            struct hunter *hunter = load_hunter(node);
            hunt_group->hunters = g_list_prepend(hunt_group->hunters, hunter);
        }
    }

    return hunt_group;
}

void
acc_print_hunter(struct hunter *hunter)
{
    acc_sprintf("[%d,%d,%d]", hunter->m_vnum, hunter->weapon, hunter->prob);
}

void
acc_print_hunt_group(struct hunt_group *hunt_group)
{
    int x = 0;

    acc_strcat("{", NULL);
    for (GList * it = hunt_group->hunters; it; it = it->next) {
        struct hunter *hunter = it->data;
        acc_print_hunter(hunter);
        if (x++ % 5 == 0)
            acc_strcat("\r\n", NULL);
    }
    acc_strcat("}", NULL);
}

void
acc_print_hunt_groups(GList * hunt_groups)
{
    acc_strcat("{ ", NULL);
    for (GList * it = hunt_groups; it; it = it->next) {
        struct hunt_group *hunt_group = it->data;
        acc_print_hunt_group(hunt_group);
    }
    acc_strcat(" }", NULL);
}

void
acc_print_devil(struct devil *devil)
{
    acc_sprintf("[%s,%d]", devil->name, devil->vnum);
}

void
acc_print_devils(GList * devils)
{
    acc_strcat("{ ", NULL);
    for (GList * it = devils; it; it = it->next) {
        struct devil *devil = it->data;
        acc_print_devil(devil);
    }
    acc_strcat(" }", NULL);
}

void
acc_print_target(struct target *target)
{
    acc_sprintf("[%d,%d]", target->o_vnum, target->level);
}

void
acc_print_targets(GList * targets)
{
    acc_strcat("{ ", NULL);
    for (GList * it = targets; it; it = it->next) {
        struct target *target = it->data;
        acc_print_target(target);
    }
    acc_strcat(" }", NULL);
}

bool
load_hunter_data(void)
{
    g_list_foreach(devils, (GFunc) free, NULL);
    g_list_foreach(targets, (GFunc) free, NULL);
    g_list_foreach(hunters, (GFunc) free, NULL);

    xmlDocPtr doc = xmlParseFile("etc/hell_hunter_data.xml");
    if (doc == NULL) {
        errlog("Hell Hunter Brain failed to load hunter data.");
        return false;
    }
    // discard root node
    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xmlFreeDoc(doc);
        return false;
    }
    freq = xmlGetIntProp(cur, "Frequency", 0);
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((xmlMatches(cur->name, "TARGET"))) {
            targets = g_list_prepend(targets, load_target(cur));
        }
        if ((xmlMatches(cur->name, "DEVIL"))) {
            devils = g_list_prepend(devils, load_devil(cur));
        }
        if ((xmlMatches(cur->name, "HUNTGROUP"))) {
            hunters = g_list_prepend(hunters, load_hunt_group(cur));
        }
        if ((xmlMatches(cur->name, "BLINDSPOT"))) {
            blind_spots = g_list_prepend(blind_spots,
                GINT_TO_POINTER(xmlGetIntProp(cur, "ID", 0)));
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return true;
}

bool
isBlindSpot(struct zone_data * zone)
{
    return g_list_find(blind_spots, GINT_TO_POINTER(zone->number));
}

SPECIAL(hell_hunter_brain)
{
    static bool data_loaded = false;
    static int counter = 1;
    struct obj_data *obj = NULL, *weap = NULL;
    struct creature *mob = NULL, *vict = NULL;
    unsigned int i;
    int num_devils = 0, regulator = 0;

    if (spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK)
        return 0;

    if (!data_loaded) {
        if (!load_hunter_data())
            return 0;
        data_loaded = true;
        slog("Hell Hunter Brain data successfully loaded.");
    }

    if (cmd) {
        if (CMD_IS("reload")) {
            if (!load_hunter_data()) {
                data_loaded = false;
                send_to_char(ch,
                    "Hell Hunter Brain failed to load hunter data.\r\n");
            } else {
                data_loaded = true;
                send_to_char(ch,
                    "Hell Hunter Brain data successfully reloaded.\r\n");
            }
            return 1;
        } else if (CMD_IS("status")) {
            send_to_char(ch, "Counter is at %d, freq %d.\r\n", counter, freq);
            snprintf(buf, sizeof(buf), "     [vnum] %30s exist/housed\r\n", "Object Name");
            i = 1;
            for (GList * it = targets; it; it = it->next, i++) {
                struct target *target = it->data;
                if (!(obj = real_object_proto(target->o_vnum)))
                    continue;
                send_to_char(ch, "%3d. [%5d] %s%30s%s %3d/%3d\r\n",
                    i, target->o_vnum, CCGRN(ch, NRM), obj->name, CCNRM(ch,
                        NRM), obj->shared->number, obj->shared->house_count);
            }
            send_to_char(ch, "Hunter blind spots:\r\n");
            int idx = 0;
            for (GList * it = blind_spots; it; it = it->next, i++) {
                struct zone_data *zone = real_zone(GPOINTER_TO_INT(it->data));
                send_to_char(ch, "%3d. [%3d] %s%s%s\r\n", idx,
                    GPOINTER_TO_INT(it->data), CCCYN(ch, NRM),
                    (zone) ? zone->name : "<no such zone>", CCNRM(ch, NRM));
            }

            return 1;
        } else if (CMD_IS("activate")) {
            skip_spaces(&argument);

            if (!*argument) {
                return 0;
            }

            if (isdigit(*argument)) {
                freq = atoi(argument);
                send_to_char(ch, "Frequency set to %d.\n", freq);
                counter = freq;
                return 1;
            }

            if (strcmp(argument, "now") == 0) {
                snprintf(buf, sizeof(buf), "Counter set to 1.\r\n");
                counter = 1;
                return 1;
            }
        }
        return 0;
    }

    if (--counter > 0) {
        return 0;
    }

    counter = freq;

    for (obj = object_list; obj; vict = NULL, obj = obj->next) {
        if (!IS_OBJ_STAT3(obj, ITEM3_HUNTED)) {
            continue;
        }

        if (obj->in_room == NULL) {
            vict = obj->carried_by;
            if (vict == NULL) {
                vict = obj->worn_by;
            }
            if (vict == NULL) {
                continue;
            }
        }
        if (vict != NULL) {
            if (IS_SOULLESS(vict)) {
                send_to_char(ch, "You feel the eyes of hell look down upon you.\r\n");
                continue;
            }

            if (IS_NPC(vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
                // some rooms are safe
                ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) ||
                // no players in quest
                GET_QUEST(vict) ||
                // can't go to isolated zones
                ZONE_FLAGGED(vict->in_room->zone, ZONE_ISOLATED) ||
                // ignore shopkeepers
                (IS_NPC(vict) && vendor == GET_NPC_SPEC(vict)) ||
                // don't go to heaven
                isBlindSpot(vict->in_room->zone)) {
                continue;
            }
            
        }

        if (obj->in_room &&
            (ROOM_FLAGGED(obj->in_room, SAFE_ROOM_BITS) ||
                ZONE_FLAGGED(obj->in_room->zone, ZONE_ISOLATED))) {
            continue;
        }

        GList *it;
        struct target *target = NULL;
        for (it = targets; it; it = it->next) {
            target = it->data;
            if (target->o_vnum == GET_OBJ_VNUM(obj)
                && IS_OBJ_STAT3(obj, ITEM3_HUNTED)) {
                break;
            }
        }
        if (target == NULL)
            continue;

        // try to skip the first person sometimes
        if (vict && !number(0, 2)) {
            continue;
        }

        struct hunt_group *hunt_group =
            g_list_nth_data(hunters, target->level);
        for (GList * hit = hunt_group->hunters; hit; hit = hit->next) {
            struct hunter *hunter = hit->data;
            if (number(0, 100) <= hunter->prob) {   // probability
                if (!(mob = read_mobile(hunter->m_vnum))) {
                    errlog("Unable to load mob in hell_hunter_brain()");
                    return 0;
                }
                if (hunter->weapon >= 0 &&
                    (weap = read_object(hunter->weapon))) {
                    if (equip_char(mob, weap, WEAR_WIELD, EQUIP_WORN)) {    // mob equipped
                        errlog("(non-critical) Hell Hunter killed by eq.");
                        return 1;   // return if equip killed mob
                    }
                }

                num_devils++;

                if (vict) {
                    start_hunting(mob, vict);
                    SET_BIT(NPC_FLAGS(mob), NPC_SPIRIT_TRACKER);
                }

                char_to_room(mob, vict ? vict->in_room : obj->in_room, false);
                act("$n steps suddenly out of an infernal conduit from the outer planes!", false, mob, NULL, NULL, TO_ROOM);
            }
        }

        if (vict && !IS_NPC(vict) && GET_REMORT_GEN(vict)
            && number(0, GET_REMORT_GEN(vict)) > 1) {

            mob = read_mobile(H_REGULATOR);
            if (mob == NULL) {
                errlog("Unable to load hell hunter regulator in hell_hunter_brain.");
                return 1;
            }

            regulator = 1;
            start_hunting(mob, vict);
            char_to_room(mob, vict->in_room, false);
            act("$n materializes suddenly from a stream of hellish energy!", false, mob, NULL, NULL, TO_ROOM);
        }

        mudlog(vict ? GET_INVIS_LVL(vict) : 0, CMP, true,
            "HELL: %d Devils%s sent after obj %s (%s@%d)",
            num_devils,
            regulator ? " (+reg)" : "",
            obj->name,
            vict ? GET_NAME(vict) : "Nobody",
            obj->in_room ? obj->in_room->number :
            (vict && vict->in_room) ? vict->in_room->number : -1);
        snprintf(buf, sizeof(buf), "%d Devils%s sent after obj %s (%s@%d)",
            num_devils,
            regulator ? " (+reg)" : "",
            obj->name, vict ? "$N" : "Nobody",
            obj->in_room ? obj->in_room->number :
            (vict && vict->in_room) ? vict->in_room->number : -1);

        act(buf, false, ch, NULL, vict, TO_ROOM);
        act(buf, false, ch, NULL, vict, TO_CHAR);

        return 1;
    }

    // we fell through, lets check again sooner than freq
    counter = freq / 8;

    return 0;
}

SPECIAL(hell_hunter)
{

    if (spec_mode != SPECIAL_TICK)
        return 0;
    struct obj_data *obj = NULL, *t_obj = NULL;
    struct creature *devil = NULL;

    if (cmd)
        return 0;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
        if (IS_CORPSE(obj)) {
            for (t_obj = obj->contains; t_obj; t_obj = t_obj->next_content) {
                for (GList * it = targets; it; it = it->next) {
                    struct target *target = it->data;
                    if (target->o_vnum == GET_OBJ_VNUM(t_obj)) {
                        act("$n takes $p from $P.", false, ch, t_obj, obj,
                            TO_ROOM);
                        mudlog(0, CMP, true,
                            "HELL: %s looted %s at %d.", GET_NAME(ch),
                            t_obj->name, ch->in_room->number);
                        extract_obj(t_obj);
                        return 1;
                    }
                }
            }
            continue;
        }
        for (GList * it = targets; it; it = it->next) {
            struct target *target = it->data;
            if (target->o_vnum == GET_OBJ_VNUM(obj)) {
                act("$n takes $p.", false, ch, obj, t_obj, TO_ROOM);
                mudlog(0, CMP, true,
                    "HELL: %s retrieved %s at %d.", GET_NAME(ch),
                    obj->name, ch->in_room->number);
                extract_obj(obj);
                return 1;
            }
        }
    }

    if (!is_fighting(ch) && !NPC_HUNTING(ch) && !AFF_FLAGGED(ch, AFF_CHARM)) {
        act("$n vanishes into the mouth of an interplanar conduit.",
            false, ch, NULL, NULL, TO_ROOM);
        creature_purge(ch, true);
        return 1;
    }

    if (GET_NPC_VNUM(ch) == H_REGULATOR) {

        if (GET_MANA(ch) < 100) {
            act("$n vanishes into the mouth of an interplanar conduit.",
                false, ch, NULL, NULL, TO_ROOM);
            creature_purge(ch, true);
            return 1;
        }

        for (GList * cit = ch->in_room->people; cit; cit = cit->next) {
            struct creature *vict = cit->data;

            if (vict == ch)
                continue;

            // REGULATOR doesn't want anyone attacking him
            if (!IS_DEVIL(vict) && g_list_find(vict->fighting, ch)) {

                if (!(devil = read_mobile(H_SPINED))) {
                    errlog
                        ("HH REGULATOR failed to load H_SPINED for defense.");
                    // set mana to zero so he will go away on the next loop
                    GET_MANA(ch) = 0;
                    return 1;
                }

                char_to_room(devil, ch->in_room, false);
                act("$n gestures... A glowing conduit flashes into existence!",
                    false, ch, NULL, vict, TO_ROOM);
                act("...$n leaps out and attacks $N!", false, devil, NULL, vict,
                    TO_NOTVICT);
                act("...$n leaps out and attacks you!", false, devil, NULL, vict,
                    TO_VICT);

                remove_all_combat(ch);
                hit(devil, vict, TYPE_UNDEFINED);
                WAIT_STATE(vict, 1 RL_SEC);

                return 1;
            }

            if (IS_DEVIL(vict) && IS_NPC(vict) &&
                GET_HIT(vict) < (GET_MAX_HIT(vict) - 500)) {

                act("$n opens a conduit of streaming energy to $N!\r\n"
                    "...$N's wounds appear to regenerate!", false, ch, NULL, vict,
                    TO_ROOM);

                GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + 500);
                GET_MANA(ch) -= 100;
                return 1;
            }

        }

    }

    return 0;
}

#define BLADE_VNUM 16203
#define ARIOCH_LAIR 16264
#define PENTAGRAM_ROOM 15437
#define ARIOCH_LEAVE_MSG "A glowing portal spins into existence behind $n, who is then drawn backwards into the mouth of the conduit, and out of this plane.  The portal then spins down to a singular point and disappears."
#define ARIOCH_ARRIVE_MSG "A glowing portal spins into existence before you, and you see a dark figure approaching from the depths.  $n steps suddenly from the mouth of the conduit, which snaps shut behind $m."

SPECIAL(arioch)
{
    struct obj_data *blade = NULL, *obj = NULL;
    struct room_data *rm = NULL;
    struct creature *vict = NULL;

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (ch->in_room->zone->number != 162) {

        if (!NPC_HUNTING(ch) && !is_fighting(ch)) {

            for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
                if (IS_CORPSE(obj)) {
                    for (blade = obj->contains; blade;
                        blade = blade->next_content) {
                        if (BLADE_VNUM == GET_OBJ_VNUM(blade)) {
                            act("$n takes $p from $P.", false, ch, blade,
                                obj, TO_ROOM);
                            mudlog(0, CMP, true,
                                "HELL: %s looted %s at %d.",
                                GET_NAME(ch), blade->name,
                                ch->in_room->number);
                            if (!GET_EQ(ch, WEAR_WIELD)) {
                                obj_from_obj(blade);
                                obj_to_char(blade, ch);
                            } else
                                extract_obj(blade);
                            return 1;
                        }
                    }
                }
                if (BLADE_VNUM == GET_OBJ_VNUM(obj)) {
                    act("$n takes $p.", false, ch, obj, obj, TO_ROOM);
                    mudlog(0, CMP, true,
                        "HELL: %s retrieved %s at %d.", GET_NAME(ch),
                        obj->name, ch->in_room->number);
                    if (!GET_EQ(ch, WEAR_WIELD)) {
                        obj_from_room(obj);
                        obj_to_char(obj, ch);
                    } else
                        extract_obj(obj);
                    return 1;
                }
            }
            act(ARIOCH_LEAVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            char_from_room(ch, false);
            char_to_room(ch, real_room(ARIOCH_LAIR), false);
            act(ARIOCH_ARRIVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            return 1;
        }
        if (GET_HIT(ch) < 800) {
            act(ARIOCH_LEAVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            char_from_room(ch, false);
            char_to_room(ch, real_room(ARIOCH_LAIR), false);
            act(ARIOCH_ARRIVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            return 1;
        }
        return 0;
    }

    if (GET_EQ(ch, WEAR_WIELD))
        return 0;

    for (blade = object_list; blade; blade = blade->next) {
        if (GET_OBJ_VNUM(blade) == BLADE_VNUM &&
            (((rm = blade->in_room) && !ROOM_FLAGGED(rm, SAFE_ROOM_BITS)) ||
                (((vict = blade->carried_by) || (vict = blade->worn_by)) &&
                    !ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) &&
                    (!IS_NPC(vict) || vendor != GET_NPC_SPEC(vict)) &&
                    !PRF_FLAGGED(vict, PRF_NOHASSLE)
                    && can_see_creature(ch, vict)))) {
            if (vict) {
                start_hunting(ch, vict);
                rm = vict->in_room;
            }
            act(ARIOCH_LEAVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            char_from_room(ch, false);
            char_to_room(ch, rm, false);
            act(ARIOCH_ARRIVE_MSG, false, ch, NULL, NULL, TO_ROOM);
            mudlog(0, CMP, true,
                "HELL: Arioch ported to %s@%d",
                vict ? GET_NAME(vict) : "Nobody", rm->number);
            return 1;
        }
    }
    return 0;
}

#undef H_SPINED
#undef H_REGULATOR
