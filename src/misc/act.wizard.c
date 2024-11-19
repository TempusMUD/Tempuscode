/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.wizard.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define _GNU_SOURCE
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <execinfo.h>
#include <inttypes.h>
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
#include "sector.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "str_builder.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "flow_room.h"
#include "bomb.h"
#include "fight.h"
#include "obj_data.h"
#include "specs.h"
#include "strutil.h"
#include "actions.h"
#include "guns.h"
#include "language.h"
#include "weather.h"
#include "search.h"
#include "prog.h"
#include "quest.h"
#include "paths.h"
#include "voice.h"
#include "olc.h"
#include "editor.h"
#include "boards.h"
#include "smokes.h"
#include "ban.h"

/*   external vars  */
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct creature *mob_proto;
// extern struct obj_data *obj_proto;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int top_of_world;
extern int log_cmds;
extern int olc_lock;
extern int lunar_day;
extern int quest_status;
extern int restrict_logins;
extern struct creature *combat_list;    /* head of list of fighting chars */
extern int shutdown_count;
extern int shutdown_mode;
extern int mini_mud;
extern int current_mob_idnum;
extern struct last_command_data last_cmd[NUM_SAVE_CMDS];
extern const char *language_names[];
extern const char *instrument_types[];
extern const char *zone_pk_flags[];
extern const char *logtypes[];

char *how_good(int percent);
extern char *prac_types[];
int skill_sort_info[MAX_SKILLS + 1];
bool has_mail(long idnum);
int prototype_obj_value(struct obj_data *obj);
int choose_material(struct obj_data *obj);
int set_maxdamage(struct obj_data *obj);

long asciiflag_conv(char *buf);
void build_player_table(void);
void show_social_messages(struct creature *ch, char *arg);
void autosave_zones(int SAVE_TYPE);
void perform_oset(struct creature *ch, struct obj_data *obj_p,
                  char *argument, int8_t subcmd);
void do_show_objects(struct creature *ch, char *value, char *arg);
void do_show_mobiles(struct creature *ch, char *value, char *argument);
void show_searches(struct creature *ch, char *value, char *arg);
static void show_rooms(struct creature *ch, char *value, char *arg);
void do_zone_cmdlist(struct creature *ch, struct zone_data *zone, char *arg);
const char *stristr(const char *haystack, const char *needle);

int parse_char_class(char *arg);
void retire_trails(void);
float prac_gain(struct creature *ch, int mode);
int skill_gain(struct creature *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct creature *ch, int mode,
                      bool show, struct str_builder *sb);
void save_quests();             // quests.cc - saves quest data
void save_all_players();
static int do_freeze_char(char *argument, struct creature *vict,
                          struct creature *ch);
void verify_tempus_integrity(struct creature *ch);
static void build_stat_obj_tmp_affs(struct creature *ch, struct obj_data *obj, struct str_builder *sb);

ACMD(do_equipment);

void
show_char_class_skills(struct creature *ch, int con, int rcon, int bits)
{
    bool check_class_abilities(struct creature *ch, int con, int rcon, int bits);
    bool skill_matches_bits(int skl, int bits);
    int lvl, skl;
    bool found;
    char *class_name = tmp_tolower(strlist_aref(con, class_names));
    char *tmp;

    if (!check_class_abilities(ch, con, rcon, bits)) {
        return;
    }

    struct str_builder sb = str_builder_default;

    if (rcon < 0) {
        sb_strcat(&sb, "The ", class_name, " class can learn the following ", NULL);
    } else {
        sb_strcat(&sb, "You can learn the following ", NULL);
    }

    sb_strcat(&sb,
              IS_SET(bits, SPELL_BIT) ? "spells" :
              IS_SET(bits, TRIG_BIT) ? "triggers" :
              IS_SET(bits, ZEN_BIT) ? "zens" :
              IS_SET(bits, ALTER_BIT) ? "alterations" :
              IS_SET(bits, SONG_BIT) ? "songs" :
              IS_SET(bits, PROGRAM_BIT) ? "programs" :"skills",
              ":\r\n", NULL);

    if (IS_IMMORT(ch)) {
        sb_strcat(&sb, "Lvl  Number   Skill           Mana: Max  Min  Chn  Flags\r\n",
            NULL);
    } else {
        sb_strcat(&sb, "Lvl    Skill\r\n", NULL);
    }

    // Small optimization
    int start_skill = 1;
    if (bits == 0) {
        start_skill = MAX_SPELLS;
    }
    for (lvl = 1; lvl < LVL_AMBASSADOR; lvl++) {
        found = false;
        for (skl = start_skill; skl < MAX_SKILLS; skl++) {
            if (spell_info[skl].min_level[con] != lvl
                && (rcon < 0 || spell_info[skl].min_level[rcon] != lvl)) {
                continue;
            }
            if (!skill_matches_bits(skl, bits)) {
                continue;
            }
            if (!found) {
                sb_sprintf(&sb, " %-2d", lvl);
            } else {
                sb_strcat(&sb, "   ", NULL);
            }

            if (GET_LEVEL(ch) < LVL_IMMORT) {
                sb_strcat(&sb, "    ", NULL);
            } else {
                sb_sprintf(&sb, " - %3d. ", skl);
            }

            if (spell_info[skl].gen[con]) {
                // This is wasteful, but it looks a lot better to have
                // the gen after the spell.  The trick is that we want it
                // to be yellow, but printf doesn't recognize the existence
                // of escape codes for purposes of padding.
                size_t len;

                tmp = tmp_sprintf("%s (gen %d)", spell_to_str(skl),
                                  spell_info[skl].gen[con]);
                len = strlen(tmp);
                if (len > 33) {
                    len = 33;
                }
                sb_sprintf(&sb, "%s%s %s(gen %d)%s%s",
                           CCGRN(ch, C_NRM), spell_to_str(skl), CCYEL(ch, C_NRM),
                           spell_info[skl].gen[con], CCNRM(ch, C_NRM),
                           tmp_pad(' ', 33 - len));
            } else {
                sb_sprintf(&sb, "%s%-33s%s",
                           CCGRN(ch, C_NRM), spell_to_str(skl), CCNRM(ch, C_NRM));
            }

            if (IS_IMMORT(ch)) {
                sb_sprintf(&sb, "%3d  %3d  %2d   %s%s%s",
                           spell_info[skl].mana_max,
                           spell_info[skl].mana_min, spell_info[skl].mana_change,
                           CCCYN(ch, C_NRM),
                           tmp_printbits(spell_info[skl].routines, spell_bits),
                           CCNRM(ch, C_NRM));
            }

            sb_strcat(&sb, "\r\n", NULL);
            found = true;
        }
    }
    page_string(ch->desc, sb.str);
}

ACMD(do_echo)
{
    char *mort_see;
    char *imm_see;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Yes.. but what?\r\n");
        return;
    }

    if (subcmd == SCMD_EMOTE) {
        mort_see = tmp_sprintf("$n%s%s", (*argument == '\'') ? "" : " ",
                               act_escape(argument));

        act(mort_see, false, ch, NULL, NULL, TO_CHAR);
        act(mort_see, false, ch, NULL, NULL, TO_ROOM);
    } else {
        mort_see = tmp_strdup(argument);
        imm_see = tmp_sprintf("[$n] %s", argument);

        for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
            struct creature *tch = it->data;
            if (GET_LEVEL(tch) > GET_LEVEL(ch)) {
                act(imm_see, false, ch, NULL, tch, TO_VICT);
            } else {
                act(mort_see, false, ch, NULL, tch, TO_VICT);
            }
        }
    }
}

ACMD(do_send)
{
    struct creature *vict;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Send what to who?\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, arg))) {
        send_to_char(ch, "%s", NOPERSON);
        return;
    }
    send_to_char(vict, "%s\r\n", argument);
    send_to_char(ch, "You send '%s' to %s.\r\n", argument, GET_NAME(vict));
    slog("(GC) %s send %s %s", GET_NAME(ch), GET_NAME(vict), argument);
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
static struct room_data *
find_target_room(struct creature *ch, char *rawroomstr)
{
    int tmp;
    struct room_data *location;
    struct creature *target_mob;
    struct obj_data *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr) {
        send_to_char(ch, "You must supply a room number or name.\r\n");
        return NULL;
    }
    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
        tmp = atoi(roomstr);
        if (!(location = real_room(tmp))) {
            send_to_char(ch, "No room exists with that number.\r\n");
            return NULL;
        }
    } else if (is_abbrev(roomstr, "previous")) {
        int bottom_room = ch->in_room->zone->number * 100;
        location = NULL;
        for (tmp = ch->in_room->number - 1; tmp >= bottom_room && !location;
             tmp--) {
            location = real_room(tmp);
        }
        if (!location) {
            send_to_char(ch, "No previous room exists in this zone!\r\n");
            return NULL;
        }
    } else if (is_abbrev(roomstr, "next")) {
        int top_room = ch->in_room->zone->top;

        location = NULL;
        for (tmp = ch->in_room->number + 1; tmp <= top_room && !location;
             tmp++) {
            location = real_room(tmp);
        }
        if (!location) {
            send_to_char(ch, "No next room exists in this zone!\r\n");
            return NULL;
        }
    } else if ((target_mob = get_char_vis(ch, roomstr))) {
        if (GET_LEVEL(ch) < LVL_SPIRIT && GET_NPC_SPEC(target_mob) == fate) {
            send_to_char(ch, "%s's magic repulses you.\r\n",
                         GET_NAME(target_mob));
            return NULL;
        }
        location = target_mob->in_room;
    } else if ((target_obj = get_obj_vis(ch, roomstr))) {
        if (target_obj->in_room != NULL) {
            location = target_obj->in_room;
        } else {
            send_to_char(ch, "That object is not available.\r\n");
            return NULL;
        }
    } else {
        send_to_char(ch, "No such creature or object around.\r\n");
        return NULL;
    }

    /* a location has been found -- if you're < GRGOD, check restrictions. */
    if (!is_authorized(ch, ENTER_ROOM, location)) {
        if (location->zone->number == 12 && GET_LEVEL(ch) < LVL_AMBASSADOR) {
            send_to_char(ch, "You can't go there.\r\n");
            return NULL;
        }
        if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
            send_to_char(ch, "You are not godly enough to use that room!\r\n");
            return NULL;
        }
        if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
            !can_enter_house(ch, location->number)) {
            send_to_char(ch, "That's private property -- no trespassing!\r\n");
            return NULL;
        }
    }
    return location;
}

ACMD(do_at)
{
    char command[MAX_INPUT_LENGTH];
    struct room_data *location = NULL, *original_loc = NULL;

    half_chop(argument, buf, command);
    if (!*buf) {
        send_to_char(ch, "You must supply a room number or a name.\r\n");
        return;
    }

    if (!*command) {
        send_to_char(ch, "What do you want to do there?\r\n");
        return;
    }

    if (!(location = find_target_room(ch, buf))) {
        return;
    }

    /* a location has been found. */
    original_loc = ch->in_room;
    char_from_room(ch, false);
    char_to_room(ch, location, false);
    command_interpreter(ch, command);

    /* check if the char is still there */
    if (ch->in_room == location) {
        char_from_room(ch, false);
        char_to_room(ch, original_loc, false);
    }
}

ACMD(do_distance)
{
    struct room_data *location = NULL, *tmp = ch->in_room;
    int steps = -1;

    if ((location = find_target_room(ch, argument)) == NULL) {
        return;
    }

    steps = find_distance(tmp, location);

    if (steps < 0) {
        send_to_char(ch, "There is no valid path to room %d.\r\n",
                     location->number);
    } else {
        send_to_char(ch, "Room %d is %d steps away.\r\n", location->number,
                     steps);
    }

}

void
perform_goto(struct creature *ch, struct room_data *room, bool allow_follow)
{
    struct room_data *was_in = NULL;
    const char *msg;

    if (!can_enter_house(ch, room->number) ||
        !clan_house_can_enter(ch, room) ||
        (GET_LEVEL(ch) < LVL_SPIRIT && ROOM_FLAGGED(room, ROOM_DEATH))) {
        send_to_char(ch, "You cannot enter there.\r\n");
        return;
    }

    was_in = ch->in_room;

    if (POOFOUT(ch)) {
        if (strstr(POOFOUT(ch), "$$n")) {
            msg = tmp_gsub(POOFOUT(ch), "$$n", "$n");
        } else {
            msg = tmp_sprintf("$n %s", POOFOUT(ch));
        }
    } else {
        msg = "$n disappears in a puff of smoke.";
    }

    act(msg, true, ch, NULL, NULL, TO_ROOM);
    char_from_room(ch, false);
    char_to_room(ch, room, false);
    if (room_is_open_air(room)) {
        GET_POSITION(ch) = POS_FLYING;
    }

    if (POOFIN(ch)) {
        if (strstr(POOFIN(ch), "$$n")) {
            msg = tmp_gsub(POOFIN(ch), "$$n", "$n");
        } else {
            msg = tmp_sprintf("$n %s", POOFIN(ch));
        }
    } else {
        msg = "$n appears with an ear-splitting bang.";
    }

    act(msg, true, ch, NULL, NULL, TO_ROOM);
    look_at_room(ch, ch->in_room, 0);

    if (allow_follow && ch->followers) {
        struct follow_type *k, *next;

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if (was_in == k->follower->in_room &&
                GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                !PLR_FLAGGED(k->follower, PLR_WRITING)
                && can_see_creature(k->follower, ch)) {
                perform_goto(k->follower, room, true);
            }
        }
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMPL) {
        slog("(GC) %s goto deathtrap [%d] %s.", GET_NAME(ch),
             ch->in_room->number, ch->in_room->name);
    }
}

ACMD(do_goto)
{
    struct room_data *location = NULL;

    REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);

    if ((location = find_target_room(ch, argument)) == NULL) {
        return;
    }

    perform_goto(ch, location, true);
}

ACMD(do_transport)
{
    struct room_data *was_in;
    struct creature *victim;
    char *name_str;

    name_str = tmp_getword(&argument);

    if (!*name_str) {
        send_to_char(ch, "Whom do you wish to transport?\r\n");
        return;
    }

    while (*name_str) {
        victim = get_char_vis(ch, name_str);

        if (!victim) {
            send_to_char(ch, "You can't detect any '%s'\r\n", name_str);
        } else if (victim == ch) {
            send_to_char(ch, "Sure, sure.  Try to transport yourself.\r\n");
        } else if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
            send_to_char(ch,
                         "%s is far too powerful for you to transport.\r\n",
                         GET_NAME(victim));
        } else if (room_is_open_air(ch->in_room)
                   && GET_POSITION(victim) != POS_FLYING) {
            send_to_char(ch, "You are in midair and %s isn't flying.\r\n",
                         GET_NAME(victim));
        } else {
            act("$n disappears in a mushroom cloud.", false, victim, NULL, NULL,
                TO_ROOM);
            was_in = victim->in_room;

            char_from_room(victim, false);
            char_to_room(victim, ch->in_room, false);
            act("$n arrives from a puff of smoke.", false, victim, NULL, NULL,
                TO_ROOM);
            act("$n has transported you!", false, ch, NULL, victim, TO_VICT);
            look_at_room(victim, victim->in_room, 0);

            if (victim->followers) {
                struct follow_type *k, *next;

                for (k = victim->followers; k; k = next) {
                    next = k->next;
                    if (was_in == k->follower->in_room &&
                        GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                        !PLR_FLAGGED(k->follower, PLR_WRITING)
                        && can_see_creature(k->follower, victim)) {
                        perform_goto(k->follower, ch->in_room, true);
                    }
                }
            }

            slog("(GC) %s has transported %s to %s.", GET_NAME(ch),
                 GET_NAME(victim), ch->in_room->name);
        }
        name_str = tmp_getword(&argument);
    }
}

ACMD(do_teleport)
{
    struct creature *victim;
    struct room_data *target;

    two_arguments(argument, buf, buf2);

    if (!*buf) {
        send_to_char(ch, "Whom do you wish to teleport?\r\n");
    } else if (!(victim = get_char_vis(ch, buf))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if (victim == ch) {
        send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
    } else if (GET_LEVEL(victim) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Maybe you shouldn't do that.\r\n");
    } else if (!*buf2) {
        send_to_char(ch, "Where do you wish to send this person?\r\n");
    } else if ((target = find_target_room(ch, buf2)) != NULL) {
        send_to_char(ch, "%s", OK);
        act("$n disappears in a puff of smoke.", false, victim, NULL, NULL, TO_ROOM);
        char_from_room(victim, false);
        char_to_room(victim, target, false);
        act("$n arrives from a puff of smoke.", false, victim, NULL, NULL, TO_ROOM);
        act("$n has teleported you!", false, ch, NULL, victim, TO_VICT);
        look_at_room(victim, victim->in_room, 0);

        slog("(GC) %s has teleported %s to [%d] %s.", GET_NAME(ch),
             GET_NAME(victim), victim->in_room->number, victim->in_room->name);
    }
}

gint
mob_vnum_compare(struct creature *a, struct creature *b, __attribute__ ((unused)) gpointer ignore)
{
    if (GET_NPC_VNUM(a) < GET_NPC_VNUM(b)) {
        return -1;
    }
    if (GET_NPC_VNUM(a) > GET_NPC_VNUM(b)) {
        return 1;
    }
    return 0;
}

struct mob_vnum_output_ctx {
    struct str_builder *sb;
    struct creature *ch;
    int *counter;
};

void
mob_vnum_output(struct creature *mob, struct mob_vnum_output_ctx *ctx)
{
    sb_sprintf(ctx->sb, "%3d. %s[%s%5d%s]%s %s%s\r\n", ++*(ctx->counter),
                CCGRN(ctx->ch, C_NRM), CCNRM(ctx->ch, C_NRM),
                mob->mob_specials.shared->vnum,
                CCGRN(ctx->ch, C_NRM), CCYEL(ctx->ch, C_NRM),
                mob->player.short_descr, CCNRM(ctx->ch, C_NRM));
}

void
vnum_mobile(char *searchname, struct creature *ch)
{
    GHashTableIter iter;
    gpointer key, val;
    GSequence *found_mobs = g_sequence_new(NULL);
    int counter = 0;

    g_hash_table_iter_init(&iter, mob_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct creature *mob = val;
        if (namelist_match(searchname, mob->player.name)) {
            g_sequence_insert_sorted(found_mobs, mob, (GCompareDataFunc)mob_vnum_compare, NULL);
        }
    }

    if (g_sequence_get_length(found_mobs)) {
        struct str_builder sb = str_builder_default;
        struct mob_vnum_output_ctx ctx = { .ch = ch, .counter = &counter, .sb = &sb };
        g_sequence_foreach(found_mobs, (GFunc)mob_vnum_output, &ctx);

        page_string(ch->desc, sb.str);
    } else {
        send_to_char(ch, "No mobs found with those search terms.\r\n");
    }

    g_sequence_free(found_mobs);
}

gint
obj_vnum_compare(struct obj_data *a, struct obj_data *b, __attribute__ ((unused)) gpointer ignore)
{
    if (GET_OBJ_VNUM(a) < GET_OBJ_VNUM(b)) {
        return -1;
    }
    if (GET_OBJ_VNUM(a) > GET_OBJ_VNUM(b)) {
        return 1;
    }
    return 0;
}

struct obj_vnum_output_ctx {
    struct str_builder *sb;
    struct creature *ch;
    int *counter;
};

void
obj_vnum_output(struct obj_data *obj, struct obj_vnum_output_ctx *ctx)
{
    sb_sprintf(ctx->sb, "%3d. %s[%s%5d%s]%s %s%s\r\n", ++*(ctx->counter),
                CCGRN(ctx->ch, C_NRM), CCNRM(ctx->ch, C_NRM),
                obj->shared->vnum,
                CCGRN(ctx->ch, C_NRM), CCYEL(ctx->ch, C_NRM),
                obj->name, CCNRM(ctx->ch, C_NRM));
}

void
vnum_object(char *searchname, struct creature *ch)
{
    GHashTableIter iter;
    gpointer key, val;
    GSequence *found_objs = g_sequence_new(NULL);
    int counter = 0;

    g_hash_table_iter_init(&iter, obj_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct obj_data *obj = val;
        if (namelist_match(searchname, obj->aliases)) {
            g_sequence_insert_sorted(found_objs, obj, (GCompareDataFunc)obj_vnum_compare, NULL);
        }
    }

    if (g_sequence_get_length(found_objs)) {
        struct str_builder sb = str_builder_default;

        struct obj_vnum_output_ctx ctx = { .ch = ch, .counter = &counter, .sb = &sb };
        g_sequence_foreach(found_objs, (GFunc)obj_vnum_output, &ctx);

        page_string(ch->desc, sb.str);
    } else {
        send_to_char(ch, "No objects found with those search terms.\r\n");
    }
}

ACMD(do_vnum)
{
    const char *mode;

    mode = tmp_getword(&argument);
    if (!*mode || !*argument ||
        (!is_abbrev(mode, "mob") && !is_abbrev(mode, "obj"))) {
        send_to_char(ch, "Usage: vnum { obj | mob } <name>\r\n");
        return;
    }
    if (is_abbrev(mode, "mob")) {
        vnum_mobile(argument, ch);
    } else if (is_abbrev(mode, "obj")) {
        vnum_object(argument, ch);
    }
}

#define CHARADD(sum,var) if (var) {sum += strlen(var) +1; }
static void
do_stat_memory(struct creature *ch)
{
    size_t sum = 0, total = 0;
    int i = 0, j = 0;
    struct alias_data *al;
    struct extra_descr_data *tmpdesc;
    struct special_search_data *tmpsearch;
    struct affected_type *af;
    struct descriptor_data *desc = NULL;
    struct follow_type *fol;
    struct creature *chars;
    struct zone_data *zone;
    struct room_data *rm;

    send_to_char(ch, "Current memory usage:\r\n");

    sum = top_of_world * sizeof(struct room_data);
    for (zone = zone_table; zone; zone = zone->next) {
        for (rm = zone->world; rm; rm = rm->next) {

            CHARADD(sum, rm->name);
            CHARADD(sum, rm->description);
            CHARADD(sum, rm->sounds);

            tmpdesc = rm->ex_description;
            while (tmpdesc) {
                sum += sizeof(struct extra_descr_data);
                CHARADD(sum, tmpdesc->keyword);
                CHARADD(sum, tmpdesc->description);
                tmpdesc = tmpdesc->next;
            }

            for (j = 0; j < NUM_OF_DIRS; j++) {
                if (rm->dir_option[j]) {
                    sum += sizeof(struct room_direction_data);
                    CHARADD(sum, rm->dir_option[j]->general_description);
                    CHARADD(sum, rm->dir_option[j]->keyword);
                }
            }

            tmpsearch = rm->search;
            while (tmpsearch) {
                sum += sizeof(struct special_search_data);
                tmpsearch = tmpsearch->next;
            }
        }
    }
    total = sum;
    send_to_char(ch, "%s  world structs: %9zu  (%d)\r\n", buf, sum, i);

    sum = g_hash_table_size(mob_prototypes) * (sizeof(struct creature));

    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, mob_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct creature *mob = val;
        CHARADD(sum, mob->player.name);
        CHARADD(sum, mob->player.short_descr);
        CHARADD(sum, mob->player.long_descr);
        CHARADD(sum, mob->player.description);
        CHARADD(sum, mob->player.title);

        if (mob->player_specials && i == 0) {
            sum += sizeof(struct player_special_data);
            CHARADD(sum, mob->player_specials->poofin);
            CHARADD(sum, mob->player_specials->poofout);
            al = mob->player_specials->aliases;
            while (al) {
                sum += sizeof(struct alias_data);
                CHARADD(sum, al->alias);
                CHARADD(sum, al->replacement);
                al = al->next;
            }
        }
        af = mob->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        fol = mob->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }

    total += sum;
    send_to_char(ch, "%s     mob protos: %9zu  (%d)\r\n", buf, sum, i);

    sum = 0;
    i = 0;
    for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
        chars = cit->data;
        if (!IS_NPC(chars)) {
            continue;
        }
        i++;
        sum += sizeof(struct creature);

        af = chars->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        desc = chars->desc;
        if (desc) {
            sum += sizeof(struct descriptor_data);
        }
        fol = chars->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }
    total += sum;
    send_to_char(ch, "%s        mobiles: %9zu  (%d)\r\n", buf, sum, i);

    sum = 0;
    i = 0;
    for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
        chars = cit->data;
        if (IS_NPC(chars)) {
            continue;
        }
        i++;
        sum += sizeof(struct creature);

        CHARADD(sum, chars->player.name);
        CHARADD(sum, chars->player.short_descr);
        CHARADD(sum, chars->player.long_descr);
        CHARADD(sum, chars->player.description);
        CHARADD(sum, chars->player.title);

        if (chars->player_specials) {
            sum += sizeof(struct player_special_data);
            CHARADD(sum, chars->player_specials->poofin);
            CHARADD(sum, chars->player_specials->poofout);
            al = chars->player_specials->aliases;
            while (al) {
                sum += sizeof(struct alias_data);
                CHARADD(sum, al->alias);
                CHARADD(sum, al->replacement);
                al = al->next;
            }
        }
        af = chars->affected;
        while (af) {
            sum += sizeof(struct affected_type);
            af = af->next;
        }
        desc = chars->desc;
        if (desc) {
            sum += sizeof(struct descriptor_data);
        }
        fol = chars->followers;
        while (fol) {
            sum += sizeof(struct follow_type);
            fol = fol->next;
        }
    }
    total += sum;

    send_to_char(ch, "%s        players: %9zu  (%d)\r\n", buf, sum, i);
    send_to_char(ch, "%s               ___________\r\n", buf);
    send_to_char(ch, "%s         total   %9zu\r\n", buf, total);
}

#undef CHARADD

void
do_stat_zone(struct creature *ch, struct zone_data *zone)
{
    struct obj_data *obj;
    struct descriptor_data *plr;
    struct room_data *rm = NULL;
    struct special_search_data *srch = NULL;
    int numm = 0, numo = 0, nump = 0, numr = 0, nums = 0, av_lev = 0,
        numm_proto = 0, numo_proto = 0, av_lev_proto = 0, numur = 0;

    send_to_char(ch, "Zone #%s%d: %s%s%s\r\n",
                 CCYEL(ch, C_NRM), zone->number,
                 CCCYN(ch, C_NRM), zone->name, CCNRM(ch, C_NRM));
    send_to_char(ch, "Authored by: %s\r\n",
                 (zone->author) ? zone->author : "<none>");
    send_to_char(ch, "Rooms: [%d-%d]  Respawn pt: [%d]  Reset Mode: %s\r\n",
                 zone->number * 100, zone->top, zone->respawn_pt,
                 reset_mode[zone->reset_mode]);

    send_to_char(ch, "TimeFrame: [%s]  Plane: [%s]   ",
                 time_frames[zone->time_frame], planes[zone->plane]);

    for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (IS_NPC(tch) && tch->in_room && tch->in_room->zone == zone) {
            numm++;
            av_lev += GET_LEVEL(tch);
        }
    }

    if (numm) {
        av_lev /= numm;
    }

    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, mob_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct creature *mob = val;

        if (GET_NPC_VNUM(mob) >= zone->number * 100
            && GET_NPC_VNUM(mob) <= zone->top && IS_NPC(mob)) {
            numm_proto++;
            av_lev_proto += GET_LEVEL(mob);
        }
    }

    if (numm_proto) {
        av_lev_proto /= numm_proto;
    }

    send_to_char(ch, "Owner: %s  ", (player_name_by_idnum(zone->owner_idnum) ?
                                     player_name_by_idnum(zone->owner_idnum) : "<none>"));
    send_to_char(ch, "Co-Owner: %s\r\n",
                 (player_name_by_idnum(zone->
                                       co_owner_idnum) ? player_name_by_idnum(zone->
                                                                              co_owner_idnum) : "<none>"));
    send_to_char(ch,
                 "Hours: [%3d]  Years: [%3d]  Idle:[%3d]  Lifespan: [%d]  Age: [%d]\r\n",
                 zone->hour_mod, zone->year_mod, zone->idle_time, zone->lifespan,
                 zone->age);

    send_to_char(ch,
                 "Sun: [%s (%d)] Sky: [%s (%d)] Moon: [%s (%d)] "
                 "Pres: [%3d] Chng: [%3d]\r\n",
                 sun_types[(int)zone->weather->sunlight],
                 zone->weather->sunlight, sky_types[(int)zone->weather->sky],
                 zone->weather->sky,
                 moon_sky_types[(int)zone->weather->moonlight],
                 zone->weather->moonlight,
                 zone->weather->pressure, zone->weather->change);

    sprintbit(zone->flags, zone_flags, buf2, sizeof(buf2));
    send_to_char(ch, "Flags: %s%s%s%s\r\n", CCGRN(ch, C_NRM), buf2,
                 zone_pk_flags[zone->pk_style], CCNRM(ch, C_NRM));

    if (zone->min_lvl) {
        send_to_char(ch, "Target lvl/gen: [% 2d/% 2d - % 2d/% 2d]\r\n",
                     zone->min_lvl, zone->min_gen, zone->max_lvl, zone->max_gen);
    }

    if (zone->public_desc) {
        send_to_char(ch, "Public Description:\r\n%s", zone->public_desc);
    }
    if (zone->private_desc) {
        send_to_char(ch, "Private Description:\r\n%s", zone->private_desc);
    }
    if (zone->dam_mod != 100) {
        send_to_char(ch, "Damage modifier: %d percent", zone->dam_mod);
    }

    for (obj = object_list; obj; obj = obj->next) {
        if (obj->in_room && obj->in_room->zone == zone) {
            numo++;
        }
    }

    g_hash_table_iter_init(&iter, obj_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct obj_data *obj = val;
        if (GET_OBJ_VNUM(obj) >= zone->number * 100 &&
            GET_OBJ_VNUM(obj) <= zone->top) {
            numo_proto++;
        }
    }

    for (plr = descriptor_list; plr; plr = plr->next) {
        if (plr->creature && plr->creature->in_room &&
            plr->creature->in_room->zone == zone) {
            nump++;
        }
    }

    for (rm = zone->world, numr = 0, nums = 0; rm; numr++, rm = rm->next) {
        if (!rm->description) {
            numur++;
        }
        srch = rm->search;
        while (srch) {
            nums++;
            srch = srch->next;
        }
    }

    send_to_char(ch, "\r\nZone Stats :-\r\n"
                 "  mobs in zone : %-3d, %-3d protos;   objs in zone  : %-3d, %-3d protos;\r\n"
                 "  players in zone: (%3d) %3d   rooms in zone: %-3d   undescripted rooms: %-3d\r\n"
                 "  search in zone: %d\r\n  usage count: %d\r\n"
                 "  Avg. Level [%s%d%s]real, [%s%d%s] proto\r\n\r\n",
                 numm, numm_proto, numo, numo_proto,
                 zone->num_players, nump, numr, numur, nums, zone->enter_count,
                 CCGRN(ch, C_NRM), av_lev,
                 CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), av_lev_proto, CCNRM(ch, C_NRM));
}

void
do_stat_trails(struct creature *ch)
{

    struct room_trail_data *trail = NULL;
    int i;
    time_t mytime, timediff;

    if (!ch->in_room->trail) {
        send_to_char(ch, "No trails exist within this room.\r\n");
        return;
    }
    mytime = time(NULL);
    struct str_builder sb = str_builder_default;
    for (i = 0, trail = ch->in_room->trail; trail; trail = trail->next) {
        timediff = mytime - trail->time;
        sprintbit(trail->flags, trail_flags, buf2, sizeof(buf2));
        sb_sprintf(&sb, " [%2d] -- Name: '%s', (%s), Idnum: [%5d]\r\n"
                    "         Time Passed: %ld minutes, %ld seconds.\r\n"
                    "         From dir: %s, To dir: %s, Track: [%2d]\r\n"
                    "         Flags: %s\r\n",
                    ++i, trail->name,
                    get_char_in_world_by_idnum(trail->idnum) ? "in world" : "gone",
                    trail->idnum, timediff / 60, timediff % 60,
                    (trail->from_dir >= 0) ? dirs[(int)trail->from_dir] : "NONE",
                    (trail->to_dir >= 0) ? dirs[(int)trail->to_dir] : "NONE",
                    trail->track, buf2);
    }

    page_string(ch->desc, sb.str);
}

void
build_format_prog(struct creature *ch, char *prog, struct str_builder *sb)
{
    const char *line_color = NULL;

    sb_sprintf(sb, "Prog:\r\n");

    int line_num = 1;
    for (char *line = tmp_getline(&prog);
         line; line = tmp_getline(&prog), line_num++) {

        // Line number looks like TEDII
        sb_sprintf(sb, "%s%3d%s] ", CCYEL(ch, C_NRM), line_num, CCBLU(ch, C_NRM));
        char *c = line;
        skip_spaces(&c);
        if (!line_color) {
            // We don't know the line color of this line
            if (*c == '-') {
                // comment
                line_color = CCBLU(ch, C_NRM);
            } else if (*c == '*') {
                // prog command
                c++;
                char *cmd = tmp_getword(&c);
                if (!strcasecmp(cmd, "before")
                    || !strcasecmp(cmd, "handle")
                    || !strcasecmp(cmd, "after")) {
                    line_color = CCCYN(ch, C_NRM);
                } else if (!strcasecmp(cmd, "require")
                           || !strcasecmp(cmd, "unless")
                           || !strcasecmp(cmd, "randomly")
                           || !strcasecmp(cmd, "or")) {
                    line_color = CCMAG(ch, C_NRM);
                } else {
                    line_color = CCYEL(ch, C_NRM);
                }
            } else if (*c) {
                // mob command
                line_color = CCNRM(ch, C_NRM);
            } else {
                // blank line
                line_color = "";
            }
        }
        // Append the line
        sb_sprintf(sb, "%s%s\r\n", line_color, line);

        // If not a continued line, zero out the line color
        if (line[strlen(line) - 1] != '\\') {
            line_color = NULL;
        }
    }
}

void
do_stat_room(struct creature *ch, char *roomstr)
{
    int tmp;
    struct extra_descr_data *desc;
    struct room_data *rm = ch->in_room;
    struct room_affect_data *aff = NULL;
    struct special_search_data *cur_search = NULL;
    int i, found = 0;
    struct obj_data *j = NULL;
    struct creature *k = NULL;

    if (roomstr && *roomstr) {
        if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
            tmp = atoi(roomstr);
            if (!(rm = real_room(tmp))) {
                send_to_char(ch, "No room exists with that number.\r\n");
                return;
            }
        } else {
            send_to_char(ch, "No room exists with that number.\r\n");
            return;
        }
    }

    struct str_builder sb = str_builder_default;
    sb_sprintf(&sb, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
                CCNRM(ch, C_NRM));

    sb_sprintf(&sb, "Zone: [%s%3d%s], VNum: [%s%5d%s], Type: %s, Lighting: [%d], Max: [%d]\r\n",
                CCYEL(ch, C_NRM), rm->zone->number, CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
                rm->number, CCNRM(ch, C_NRM), sector_name_by_idnum(rm->sector_type),
                rm->light, rm->max_occupancy);

    sb_sprintf(&sb, "SpecProc: %s, Flags: %s\r\n",
                (rm->func == NULL) ? "None" :
                (i = find_spec_index_ptr(rm->func)) < 0 ? "Exists" :
                spec_list[i].tag, tmp_printbits(rm->room_flags, room_bits));

    if (FLOW_SPEED(rm)) {
        sb_sprintf(&sb, "Flow (Direction: %s, Speed: %d, Type: %s (%d)).\r\n",
                    dirs[(int)FLOW_DIR(rm)], FLOW_SPEED(rm),
                    flow_types[(int)FLOW_TYPE(rm)], (int)FLOW_TYPE(rm));
    }

    sb_sprintf(&sb, "Description:\r\n%s",
                (rm->description) ? rm->description : "  None.\r\n");

    if (rm->sounds) {
        sb_sprintf(&sb, "%sSound:%s\r\n%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    rm->sounds);
    }

    if (rm->ex_description) {
        sb_sprintf(&sb, "Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = rm->ex_description; desc; desc = desc->next) {
            sb_sprintf(&sb, " %s%s", desc->keyword, (desc->next) ? ";" : "");
        }
        sb_strcat(&sb, CCNRM(ch, C_NRM), "\r\n", NULL);
    }

    if (rm->affects) {
        sb_sprintf(&sb, "Room affects:\r\n");
        for (aff = rm->affects; aff; aff = aff->next) {
            if (aff->type == RM_AFF_FLAGS) {
                sb_sprintf(&sb, "  Room flags: %s%s%s, duration[%d]\r\n",
                            CCCYN(ch, C_NRM),
                            tmp_printbits(aff->flags, room_bits),
                            CCNRM(ch, C_NRM), aff->duration);
            } else if (aff->type < NUM_DIRS) {
                sb_sprintf(&sb, "  Door flags exit [%s], %s, duration[%d]\r\n",
                            dirs[(int)aff->type],
                            tmp_printbits(aff->flags, exit_bits), aff->duration);
            } else {
                sb_sprintf(&sb, "  ERROR: Type %d.\r\n", aff->type);
            }

            sb_sprintf(&sb, "  Desc: %s\r\n",
                        aff->description ? aff->description : "None.");
        }
    }

    sb_sprintf(&sb, "Chars present:%s", CCYEL(ch, C_NRM));

    found = 0;
    for (GList *it = first_living(rm->people); it; it = next_living(it)) {
        k = it->data;
        if (can_see_creature(ch, k)) {
            sb_sprintf(&sb, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
                        (!IS_NPC(k) ? "PC" : (!IS_NPC(k) ? "NPC" : "MOB")));
        }
    }
    sb_strcat(&sb, CCNRM(ch, C_NRM), "\r\n", NULL);

    if (rm->contents) {
        sb_sprintf(&sb, "Contents:%s", CCGRN(ch, C_NRM));
        for (found = 0, j = rm->contents; j; j = j->next_content) {
            if (!can_see_object(ch, j)) {
                continue;
            }
            sb_sprintf(&sb, "%s %s", found++ ? "," : "", j->name);
        }
        sb_strcat(&sb, CCNRM(ch, C_NRM), "\r\n", NULL);
    }

    if (rm->search) {
        sb_sprintf(&sb, "SEARCHES:\r\n");
        for (cur_search = rm->search;
             cur_search; cur_search = cur_search->next) {
            format_search_data(&sb, ch, rm, cur_search);
        }
    }

    for (i = 0; i < NUM_OF_DIRS; i++) {
        if (ABS_EXIT(rm, i)) {
            sb_sprintf(&sb, "Exit %s%-5s%s:  To: [%s%s%s], Key: [%5d], Dam: [%4d], "
                        "Keywrd: %s, Type: %s\r\n",
                        CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM),
                        CCCYN(ch, C_NRM), ((ABS_EXIT(rm, i)->to_room) ? tmp_sprintf("%5d", ABS_EXIT(rm, i)->to_room->number) : "NONE"), CCNRM(ch, C_NRM),
                        ABS_EXIT(rm, i)->key,
                        ABS_EXIT(rm, i)->damage,
                        ABS_EXIT(rm, i)->keyword ? ABS_EXIT(rm, i)->keyword : "None",
                        tmp_printbits(ABS_EXIT(rm, i)->exit_info, exit_bits));
            if (ABS_EXIT(rm, i)->general_description) {
                sb_strcat(&sb, ABS_EXIT(rm, i)->general_description, NULL);
            } else {
                sb_strcat(&sb, "  No exit description.\r\n", NULL);
            }
        }
    }
    if (rm->prog_state && rm->prog_state->var_list) {
        struct prog_var *cur_var;
        sb_strcat(&sb, "Prog state variables:\r\n", NULL);
        for (cur_var = rm->prog_state->var_list; cur_var;
             cur_var = cur_var->next) {
            sb_sprintf(&sb, "     %s = '%s'\r\n", cur_var->key, cur_var->value);
        }
    }
    if (rm->prog) {
        build_format_prog(ch, rm->prog, &sb);
    }

    page_string(ch->desc, sb.str);
}

void
do_stat_object(struct creature *ch, struct obj_data *j)
{
    int i, found;
    struct extra_descr_data *desc;
    extern const char *egun_types[];
    struct room_data *rm = NULL;
    bool metric = USE_METRIC(ch);

    if (IS_OBJ_TYPE(j, ITEM_NOTE) && isname("letter", j->aliases)) {
        if (j->carried_by && GET_LEVEL(j->carried_by) > GET_LEVEL(ch)) {
            act("$n just tried to stat your mail.",
                false, ch, NULL, j->carried_by, TO_VICT);
            send_to_char(ch, "You're pretty brave, bucko.\r\n");
            return;
        }
        if (GET_LEVEL(ch) < LVL_GOD) {
            send_to_char(ch, "You can't stat mail.\r\n");
            return;
        }
    }

    struct str_builder sb = str_builder_default;
    sb_sprintf(&sb, "Name: '%s%s%s', Aliases: %s\r\n", CCGRN(ch, C_NRM),
                ((j->name) ? j->name : "<None>"), CCNRM(ch, C_NRM), j->aliases);

    sb_sprintf(&sb, "VNum: [%s%5d%s], Exist: [%3d/%3d], Type: %s, SpecProc: %s\r\n",
        CCGRN(ch, C_NRM), GET_OBJ_VNUM(j), CCNRM(ch, C_NRM), j->shared->number,
        j->shared->house_count, strlist_aref(GET_OBJ_TYPE(j), item_types),
        (j->shared->func) ? ((i =
                                  find_spec_index_ptr(j->shared->func)) <
                             0 ? "Exists" : spec_list[i].tag) : "None");
    sb_sprintf(&sb, "L-Des: %s%s%s\r\n", CCGRN(ch, C_NRM),
                ((j->line_desc) ? j->line_desc : "None"), CCNRM(ch, C_NRM));

    if (j->action_desc) {
        sb_sprintf(&sb, "Action desc: %s\r\n", j->action_desc);
    }

    if (j->ex_description) {
        sb_sprintf(&sb, "Extra descs:%s", CCCYN(ch, C_NRM));
        for (desc = j->ex_description; desc; desc = desc->next) {
            sb_strcat(&sb, " ", desc->keyword, ";", NULL);
        }
        sb_strcat(&sb, CCNRM(ch, C_NRM), "\r\n", NULL);
    }

    if (!j->line_desc) {
        sb_sprintf(&sb, "**This object currently has no description**\r\n");
    }

    if (j->creation_time) {
        switch (j->creation_method) {
        case CREATED_ZONE:
            sb_sprintf(&sb, "Created by zone #%ld on %s\r\n",
                        j->creator, tmp_ctime(j->creation_time));
            break;
        case CREATED_MOB:
            sb_sprintf(&sb, "Loaded onto mob #%ld on %s\r\n",
                        j->creator, tmp_ctime(j->creation_time));
            break;
        case CREATED_SEARCH:
            sb_sprintf(&sb, "Created by search in room #%ld on %s\r\n",
                        j->creator, tmp_ctime(j->creation_time));
            break;
        case CREATED_IMM:
            sb_sprintf(&sb, "Loaded by %s on %s\r\n",
                        player_name_by_idnum(j->creator), tmp_ctime(j->creation_time));
            break;
        case CREATED_PROG:
            sb_sprintf(&sb, "Created by prog (mob or room #%ld) on %s\r\n",
                        j->creator, tmp_ctime(j->creation_time));
            break;
        case CREATED_PLAYER:
            sb_sprintf(&sb, "Created by player %s on %s\r\n",
                        player_name_by_idnum(j->creator), tmp_ctime(j->creation_time));
            break;
        default:
            sb_sprintf(&sb, "Created on %s\r\n", tmp_ctime(j->creation_time));
            break;
        }
    }

    if (j->unique_id) {
        sb_sprintf(&sb, "Unique object id: %ld\r\n", j->unique_id);
    }

    if (j->shared->owner_id != 0) {
        if (player_idnum_exists(j->shared->owner_id)) {
            sb_sprintf(&sb, "Oedit Owned By: %s[%ld]\r\n",
                        player_name_by_idnum(j->shared->owner_id),
                        j->shared->owner_id);
        } else {
            sb_sprintf(&sb, "Oedit Owned By: NOONE[%ld]\r\n", j->shared->owner_id);
        }

    }
    sb_sprintf(&sb, "Can be worn on: ");
    sb_strcat(&sb, tmp_printbits(j->obj_flags.wear_flags, wear_bits), "\r\n",
               NULL);
    if (j->obj_flags.bitvector[0] || j->obj_flags.bitvector[1]
        || j->obj_flags.bitvector[2]) {
        sb_strcat(&sb, "Set char bits : ", NULL);
        if (j->obj_flags.bitvector[0]) {
            sb_strcat(&sb, tmp_printbits(j->obj_flags.bitvector[0],
                                     affected_bits), "  ", NULL);
        }
        if (j->obj_flags.bitvector[1]) {
            sb_strcat(&sb, tmp_printbits(j->obj_flags.bitvector[1],
                                     affected2_bits), "  ", NULL);
        }
        if (j->obj_flags.bitvector[2]) {
            sb_strcat(&sb, tmp_printbits(j->obj_flags.bitvector[2],
                                     affected3_bits), NULL);
        }
        sb_strcat(&sb, "\r\n", NULL);
    }
    sb_sprintf(&sb, "Extra flags : %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA(j), extra_bits));
    sb_sprintf(&sb, "Extra2 flags: %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA2(j), extra2_bits));
    sb_sprintf(&sb, "Extra3 flags: %s\r\n",
                tmp_printbits(GET_OBJ_EXTRA3(j), extra3_bits));

    sb_sprintf(&sb, "Weight: %s, Cost: %'d (%'d), Rent: %'d, Timer: %d\r\n",
                format_weight(GET_OBJ_WEIGHT(j), metric), GET_OBJ_COST(j),
                prototype_obj_value(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j));

    if ((rm = where_obj(j))) {
        sb_sprintf(&sb, "Absolute location: %s (%d)\r\n", rm->name, rm->number);

        sb_sprintf(&sb, "In room: %s%s%s, In obj: %s%s%s, Carry: %s%s%s, Worn: %s%s%s%s, Aux: %s%s%s\r\n",
            CCCYN(ch, C_NRM), (j->in_room) ? tmp_sprintf("%d",
                                                         j->in_room->number) : "N", CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
            (j->in_obj) ? j->in_obj->name : "N", CCNRM(ch, C_NRM), CCGRN(ch,
                                                                         C_NRM), (j->carried_by) ? GET_NAME(j->carried_by) : "N",
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
            (j->worn_by) ? GET_NAME(j->worn_by) : "N", (!j->worn_by
                                                        || j == GET_EQ(j->worn_by, j->worn_on)) ? "" : " (impl)",
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
            (j->aux_obj) ? j->aux_obj->name : "N", CCNRM(ch, C_NRM));
    }
    sb_sprintf(&sb, "Material: [%s%s%s (%d)], Maxdamage: [%'d (%'d)], Damage: [%'d]\r\n",
        CCYEL(ch, C_NRM), strlist_aref(GET_OBJ_MATERIAL(j), material_names), CCNRM(ch,
                                                                                   C_NRM), GET_OBJ_MATERIAL(j), GET_OBJ_MAX_DAM(j), set_maxdamage(j),
        GET_OBJ_DAM(j));

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
        sb_sprintf(&sb, "Color: [%d], Type: [%d], Hours: [%d], Value3: [%d]\r\n",
                    GET_OBJ_VAL(j, 0),
                    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INSTRUMENT:
        sb_sprintf(&sb, "Instrument Type: [%d] (%s)\r\n",
                    GET_OBJ_VAL(j, 0),
                    strlist_aref(GET_OBJ_VAL(j, 0), instrument_types));
        break;
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_SYRINGE:
    case ITEM_BOOK:
        sb_sprintf(&sb, "Level: %d, Spells: %s(%d), %s(%d), %s(%d)\r\n",
                    GET_OBJ_VAL(j, 0),
                    (GET_OBJ_VAL(j, 1) > 0) ? spell_to_str((int)GET_OBJ_VAL(j,
                                                                            1)) : "None", GET_OBJ_VAL(j, 1), (GET_OBJ_VAL(j,
                                                                                                                          2) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 2)) : "None",
                    GET_OBJ_VAL(j, 2), (GET_OBJ_VAL(j,
                                                    3) > 0) ? spell_to_str((int)GET_OBJ_VAL(j, 3)) : "None",
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        sb_sprintf(&sb, "Level: %d, Max Charge: %d, Current Charge: %d, Spell: %s\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            spell_to_str((int)GET_OBJ_VAL(j, 3)));
        break;
    case ITEM_WEAPON:
        if (j->shared->proto) {
            sb_sprintf(&sb, "Spell: %s (%d), Todam: %dd%d (%savg %d%s [%d-%d]), Damage Type: %s (%d)\r\n",
                ((GET_OBJ_VAL(j, 0) > 0
                  && GET_OBJ_VAL(j, 0) < TOP_NPC_SPELL) ? spell_to_str((int)GET_OBJ_VAL(j, 0)) : "NONE"), GET_OBJ_VAL(j, 0),
                GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
                CCGRN(ch, C_NRM),(GET_OBJ_VAL(j, 1) * (GET_OBJ_VAL(j, 2) + 1)) / 2, CCNRM(ch, C_NRM),

                // for displaying the max/min avg from the obj prototype
                ((GET_OBJ_VAL(j->shared->proto, 1) - (GET_OBJ_VAL(j->shared->proto, 1) / 4)) *
                 ((GET_OBJ_VAL(j->shared->proto, 2) - (GET_OBJ_VAL(j->shared->proto, 2) / 4)) + 1)) / 2,
                ((GET_OBJ_VAL(j->shared->proto, 1) + (GET_OBJ_VAL(j->shared->proto, 1) / 4)) *
                 ((GET_OBJ_VAL(j->shared->proto, 2) + (GET_OBJ_VAL(j->shared->proto, 2) / 4)) + 1)) / 2,

                (GET_OBJ_VAL(j, 3) >= 0
                 && GET_OBJ_VAL(j, 3) < 19) ? attack_hit_text[(int)GET_OBJ_VAL(j, 3)].plural : "bunk",
                GET_OBJ_VAL(j, 3));
        } else {
            sb_sprintf(&sb, "Spell: %s (%d), Todam: %dd%d, Damage Type: %s (%d)\r\n",
                ((GET_OBJ_VAL(j, 0) > 0
                  && GET_OBJ_VAL(j, 0) < TOP_NPC_SPELL) ? spell_to_str((int)GET_OBJ_VAL(j, 0)) : "NONE"), GET_OBJ_VAL(j, 0),
                GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
                (GET_OBJ_VAL(j, 3) >= 0
                 && GET_OBJ_VAL(j, 3) < 19) ? attack_hit_text[(int)GET_OBJ_VAL(j, 3)].plural : "bunk",
                GET_OBJ_VAL(j, 3));
        }
        break;
    case ITEM_CAMERA:
        sb_sprintf(&sb, "Targ room: %d\r\n", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_MISSILE:
        sb_sprintf(&sb, "Tohit: %d, Todam: %d, Type: %d\r\n",
                    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ARMOR:
        sb_sprintf(&sb, "AC-apply: [%d]\r\n", GET_OBJ_VAL(j, 0));
        break;
    case ITEM_TRAP:
        sb_sprintf(&sb, "Spell: %d, - Hitpoints: %d\r\n",
                    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
        break;
    case ITEM_CONTAINER:
        sb_sprintf(&sb, "Max-contains: %d, Locktype: %d, Key/Owner: %d, Corpse: %s, Killer: %d\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3) ? "Yes" : "No", CORPSE_KILLER(j));
        break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
        sb_sprintf(&sb, "Max-contains: %d, Contains: %d, Liquid: %s(%d), Poisoned: %s (%d)\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), strlist_aref(GET_OBJ_VAL(j,
                                                                           2), drinks), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j,
                                                                                                                       3) ? "Yes" : "No", GET_OBJ_VAL(j, 3));
        break;
    case ITEM_KEY:
        sb_sprintf(&sb, "Keytype: %d, Rentable: %s, Car Number: %d\r\n",
                    GET_OBJ_VAL(j, 0), YESNO(GET_OBJ_VAL(j, 1)), GET_OBJ_VAL(j, 2));
        break;
    case ITEM_FOOD:
        sb_sprintf(&sb, "Makes full: %d, Spell Lvl: %d, Spell : %s(%d), Poisoned: %d\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
            spell_to_str((int)GET_OBJ_VAL(j, 2)), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3));
        break;
    case ITEM_HOLY_SYMB:
        sb_sprintf(&sb, "Alignment: %s(%d), Class: %s(%d), Min Level: %d, Max Level: %d \r\n",
            strlist_aref(GET_OBJ_VAL(j, 0), alignments), GET_OBJ_VAL(j, 0),
            strlist_aref(GET_OBJ_VAL(j, 1), char_class_abbrevs), GET_OBJ_VAL(j, 1),
            GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BATTERY:
        sb_sprintf(&sb, "Max_Energy: %d, Cur_Energy: %d, Rate: %d, Cost/Unit: %d\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
            GET_OBJ_VAL(j, 3));
        break;
    case ITEM_VEHICLE:
        sb_sprintf(&sb, "Room/Key Number: %d, Door State: %d, Type: %s, v3: %d\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), tmp_printbits(GET_OBJ_VAL(j,
                                                                            2), vehicle_types), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENGINE:
        sb_sprintf(&sb, "Max_Energy: %d, Cur_Energy: %d, Run_State: %s, Consume_rate: %d\r\n",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), tmp_printbits(GET_OBJ_VAL(j,
                                                                            2), engine_flags), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_ENERGY_GUN:
        sb_sprintf(&sb, "Drain Rate: %d, Todam: %dd%d (av %d), Damage Type: %s (%d)\r\n",
                    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
                    (GET_OBJ_VAL(j, 1) * (GET_OBJ_VAL(j, 2) + 1)) / 2,
                    strlist_aref((int)GET_OBJ_VAL(j, 3), egun_types),
                    GET_OBJ_VAL(j, 3));
        break;
    case ITEM_BOMB:
        sb_sprintf(&sb, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    strlist_aref((int)GET_OBJ_VAL(j, 0), bomb_types), GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_FUSE:
        sb_sprintf(&sb, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    strlist_aref((int)GET_OBJ_VAL(j, 0), fuse_types), GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_TOBACCO:
        sb_sprintf(&sb, "Values: %s:[%s(%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    strlist_aref((int)MIN(GET_OBJ_VAL(j, 0), NUM_SMOKES - 1), smoke_types),
                    GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;

    case ITEM_GUN:
    case ITEM_BULLET:
    case ITEM_CLIP:
        sb_sprintf(&sb, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%s (%d)]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3],
                    strlist_aref(GET_OBJ_VAL(j, 3), gun_types), GET_OBJ_VAL(j, 3));
        break;
    case ITEM_INTERFACE:
        sb_sprintf(&sb, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    strlist_aref(GET_OBJ_VAL(j, 0), interface_types),
                    GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    case ITEM_MICROCHIP:
        sb_sprintf(&sb, "Values 0-3: %s:[%s (%d)] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0],
                    strlist_aref(GET_OBJ_VAL(j, 0), microchip_types),
                    GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    default:
        sb_sprintf(&sb, "Values 0-3: %s:[%d] %s:[%d] %s:[%d] %s:[%d]\r\n",
                    item_value_types[(int)GET_OBJ_TYPE(j)][0], GET_OBJ_VAL(j, 0),
                    item_value_types[(int)GET_OBJ_TYPE(j)][1], GET_OBJ_VAL(j, 1),
                    item_value_types[(int)GET_OBJ_TYPE(j)][2], GET_OBJ_VAL(j, 2),
                    item_value_types[(int)GET_OBJ_TYPE(j)][3], GET_OBJ_VAL(j, 3));
        break;
    }

    if (j->shared->proto == j) {
        char *param = GET_OBJ_PARAM(j);
        if (param && strlen(param) > 0) {
            sb_sprintf(&sb, "Spec_param: \r\n%s\r\n", GET_OBJ_PARAM(j));
        }
    }

    found = 0;
    sb_sprintf(&sb, "Affections:");
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (j->affected[i].modifier) {
            sb_sprintf(&sb, "%s %+d to %s",
                        found++ ? "," : "",
                        j->affected[i].modifier,
                        strlist_aref(j->affected[i].location, apply_types));
        }
    }
    if (!found) {
        sb_sprintf(&sb, " None");
    }

    sb_sprintf(&sb, "\r\n");

    if (j->shared->proto == j) {
        // All the rest of the stats are only meaningful if the object
        // is in the game.
        page_string(ch->desc, sb.str);
        return;
    }

    if (j->contains) {
        sb_sprintf(&sb, "Contents:\r\n");
        list_obj_to_char(j->contains, ch, SHOW_OBJ_CONTENT, true, &sb);

    }

    if (OBJ_SOILAGE(j)) {
        sb_sprintf(&sb, "Soilage: %s\r\n",
                    tmp_printbits(OBJ_SOILAGE(j), soilage_bits));
    }

    if (GET_OBJ_SIGIL_IDNUM(j)) {
        sb_sprintf(&sb, "Warding Sigil: %s (%d), level %d.\r\n",
                    player_name_by_idnum(GET_OBJ_SIGIL_IDNUM(j)),
                    GET_OBJ_SIGIL_IDNUM(j), GET_OBJ_SIGIL_LEVEL(j));
    }

    if (j->consignor) {
        sb_sprintf(&sb, "Consigned by %s (%ld) for %'" PRId64 ".\r\n",
                    player_name_by_idnum(j->consignor),
                    j->consignor,
                    j->consign_price);
    }

    build_stat_obj_tmp_affs(ch, j, &sb);

    page_string(ch->desc, sb.str);
}

void
build_stat_obj_tmp_affs(struct creature *ch, struct obj_data *obj, struct str_builder *sb)
{
    char *stat_prefix;

    if (!obj->tmp_affects) {
        return;
    }

    for (struct tmp_obj_affect *aff = obj->tmp_affects; aff; aff = aff->next) {
        stat_prefix = tmp_sprintf("AFF: (%3dhr) [%3d] %s%-20s%s",
                                  aff->duration, aff->level, CCCYN(ch, C_NRM),
                                  spell_to_str(aff->type), CCNRM(ch, C_NRM));

        for (int i = 0; i < 4; i++) {
            if (aff->val_mod[i] != 0) {
                sb_sprintf(sb, "%s %s by %d\r\n",
                            stat_prefix,
                            item_value_types[(int)GET_OBJ_TYPE(obj)][i],
                            aff->val_mod[i]);
            }
        }

        if (aff->type_mod) {
            sb_sprintf(sb, "%s type = %s\r\n", stat_prefix,
                        strlist_aref(GET_OBJ_TYPE(obj), item_type_descs));
        }

        if (aff->worn_mod) {
            sb_sprintf(sb, "%s worn + %s\r\n",
                        stat_prefix, tmp_printbits(aff->worn_mod, wear_bits));
        }

        if (aff->extra_mod) {
            const char **bit_descs[3] = {
                extra_bits, extra2_bits, extra3_bits
            };
            sb_sprintf(sb, "%s extra + %s\r\n",
                        stat_prefix,
                        tmp_printbits(aff->extra_mod,
                                      bit_descs[(int)aff->extra_index - 1]));
        }

        if (aff->weight_mod) {
            sb_sprintf(sb, "%s weight by %f\r\n", stat_prefix, aff->weight_mod);
        }

        for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
            if (aff->affect_loc[i]) {
                sb_sprintf(sb, "%s %+d to %s\r\n",
                            stat_prefix,
                            aff->affect_mod[i],
                            strlist_aref(aff->affect_loc[i], apply_types));
            }
        }

        if (aff->dam_mod) {
            sb_sprintf(sb, "%s damage by %d\r\n", stat_prefix, aff->dam_mod);
        }

        if (aff->maxdam_mod) {
            sb_sprintf(sb, "%s maxdam by %d\r\n", stat_prefix, aff->maxdam_mod);
        }
    }
}

void
do_stat_character_kills(struct creature *ch, struct creature *k)
{
    if (!IS_PC(k)) {
        send_to_char(ch, "Recent kills by a mob are not recorded.\r\n");
    } else if (!GET_RECENT_KILLS(k)) {
        send_to_char(ch, "This player has not killed anything yet.\r\n");
    } else {
        struct str_builder sb = str_builder_default;
        sb_sprintf(&sb, "Recently killed by %s:\r\n", GET_NAME(k));
        for (GList *kill_it = GET_RECENT_KILLS(k);
             kill_it;
             kill_it = kill_it->next) {
            struct kill_record *kill = kill_it->data;
            struct creature *killed = real_mobile_proto(kill->vnum);
            sb_sprintf(&sb, "%s%5d. %-30s %17d%s\r\n",
                        CCGRN(ch, C_NRM),
                        kill->vnum,
                        (killed) ? GET_NAME(killed) : "<unknown>",
                        kill->times, CCNRM(ch, C_NRM));
        }
        page_string(ch->desc, sb.str);
    }
}

void
do_stat_character_prog(struct creature *ch, struct creature *k)
{
    if (IS_PC(k)) {
        send_to_char(ch, "Players don't have progs!\r\n");
    } else if (!GET_NPC_PROG(k)) {
        send_to_char(ch, "Mobile %d does not have a prog.\r\n",
                     GET_NPC_VNUM(k));
    } else {
        struct str_builder sb = str_builder_default;
        build_format_prog(ch, GET_NPC_PROG(k), &sb);
        page_string(ch->desc, sb.str);
    }
}

void
do_stat_character_progobj(struct creature *ch, struct creature *k)
{
    void prog_display_obj(struct creature *ch, unsigned char *exec);

    if (IS_PC(k)) {
        send_to_char(ch, "Players don't have progs!\r\n");
    } else if (!GET_NPC_PROG(k)) {
        send_to_char(ch, "Mobile %d does not have a prog.\r\n",
                     GET_NPC_VNUM(k));
    } else {
        prog_display_obj(ch, GET_NPC_PROGOBJ(k));
    }
}

void
do_stat_character_description(struct creature *ch, struct creature *k)
{
    if (k->player.description) {
        page_string(ch->desc, k->player.description);
    } else {
        send_to_char(ch, "Mobile %d does not have a description.\r\n",
                     GET_NPC_VNUM(k));
    }
}

void
do_stat_character(struct creature *ch, struct creature *k, char *options)
{
    int i, num, num2, found = 0, rexp;
    const char *line_buf;
    struct follow_type *fol;
    struct affected_type *aff;
    bool metric = USE_METRIC(ch);

    if (IS_PC(k)
        && !(is_tester(ch) && ch == k)
        && !is_authorized(ch, STAT_PLAYERS, NULL)) {
        send_to_char(ch, "You can't stat this player.\r\n");
        return;
    }

    if (GET_NPC_SPEC(k) == fate && !is_authorized(ch, STAT_FATE, NULL)) {
        send_to_char(ch, "You can't stat this mob.\r\n");
        return;
    }

    for (char *opt_str = tmp_getword(&options);
         *opt_str; opt_str = tmp_getword(&options)) {
        if (is_abbrev(opt_str, "kills")) {
            do_stat_character_kills(ch, k);
            return;
        } else if (is_abbrev(opt_str, "prog")) {
            do_stat_character_prog(ch, k);
            return;
        } else if (is_abbrev(opt_str, "progobj")) {
            do_stat_character_progobj(ch, k);
            return;
        } else if (is_abbrev(opt_str, "description")) {
            do_stat_character_description(ch, k);
            return;
        }
    }

    struct str_builder sb = str_builder_default;

    if (GET_SEX(k) >= 0 && GET_SEX(k) < SEX_COUNT) {
        sb_strcat(&sb, tmp_toupper(gender_info[GET_SEX(k)].name), NULL);
    } else {
        sb_strcat(&sb, "ILLEGAL-SEX!!", NULL);
    }

    sb_sprintf(&sb,
        " %s '%s%s%s'  IDNum: [%5ld], AccountNum: [%5ld], In room %s[%s%5d%s]%s\n",
        (!IS_NPC(k) ? "PC" : (!IS_NPC(k) ? "NPC" : "MOB")), CCYEL(ch, C_NRM),
        GET_NAME(k), CCNRM(ch, C_NRM), IS_NPC(k) ? NPC_IDNUM(k) : GET_IDNUM(k),
        IS_NPC(k) ? -1 : player_account_by_idnum(GET_IDNUM(k)), CCGRN(ch,
                                                                      C_NRM), CCNRM(ch, C_NRM), k->in_room ? k->in_room->number : -1,
        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

    if (!IS_NPC(k) && GET_LEVEL(k) >= LVL_AMBASSADOR) {
        sb_sprintf(&sb, "OlcObj: [%d], OlcMob: [%d]\r\n",
                    (GET_OLC_OBJ(k) ? GET_OLC_OBJ(k)->shared->vnum : (-1)),
                    (GET_OLC_MOB(k) ?
                     GET_OLC_MOB(k)->mob_specials.shared->vnum : (-1)));
    } else {
        sb_strcat(&sb, "\r\n", NULL);
    }

    if (IS_NPC(k)) {
        sb_sprintf(&sb, "Alias: %s, VNum: %s[%s%5d%s]%s, Exist: [%3d]\r\n",
                    k->player.name, CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                    GET_NPC_VNUM(k), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    k->mob_specials.shared->number);
        sb_sprintf(&sb, "L-Des: %s%s%s\r\n", CCYEL(ch, C_NRM),
                    (k->player.long_descr ? k->player.long_descr : "<None>"),
                    CCNRM(ch, C_NRM));
    } else {
        sb_sprintf(&sb, "Title: %s\r\n",
                    (k->player.title ? k->player.title : "<None>"));
    }

/* Race, Class */
    sb_sprintf(&sb, "Race: %s, Class: %s%s/%s Gen: %d\r\n",
                race_name_by_idnum(k->player.race),
                strlist_aref(k->player.char_class, class_names),
                (IS_CYBORG(k) ?
                 tmp_sprintf("(%s)",
                             strlist_aref(GET_OLD_CLASS(k), borg_subchar_class_names)) :
                 ""),
                (IS_REMORT(k) ?
                 strlist_aref(GET_REMORT_CLASS(k), class_names) :
                 "None"), GET_REMORT_GEN(k));

    if (!IS_NPC(k)) {
        sb_sprintf(&sb, "Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s], Align: [%4d]\r\n",
                    CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
                    CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
                    CCCYN(ch, C_NRM), exp_scale[GET_LEVEL(k) + 1] - GET_EXP(k),
                    CCNRM(ch, C_NRM), GET_ALIGNMENT(k));
    } else {
        rexp = mobile_experience(k, NULL);
        sb_sprintf(&sb, "Lev: [%s%2d%s], XP: [%s%7d%s/%s%d%s] %s(%s%3d p%s)%s, Align: [%4d]\r\n",
            CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_EXP(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), rexp, CCNRM(ch,
                                                                        C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_EXP(k) > 10000000 ? (((unsigned int)GET_EXP(k)) / MAX(1,
                                                                      rexp / 100)) : (((unsigned int)(GET_EXP(k) * 100)) / MAX(1,
                                                                                                                               rexp)), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_ALIGNMENT(k));
    }

    sb_sprintf(&sb, "Height %s, Weight %s.\r\n",
                format_distance(GET_HEIGHT(k), metric),
                format_weight(GET_WEIGHT(k), metric));

    if (!IS_NPC(k)) {
        strcpy_s(buf1, sizeof(buf1), (char *)asctime(localtime(&(k->player.time.birth))));
        strcpy_s(buf2, sizeof(buf2), (char *)asctime(localtime(&(k->player.time.logon))));
        buf1[10] = buf2[10] = '\0';

        sb_sprintf(&sb, "Created: [%s], Last Logon: [%s], Played [%ldh %ldm], Age [%d]\r\n",
            buf1, buf2, k->player.time.played / 3600,
            ((k->player.time.played / 3600) % 60), GET_AGE(k));

        sb_sprintf(&sb, "Homeroom:[%d], Loadroom: [%d], Clan: %s%s%s\r\n",
                    GET_HOMEROOM(k), k->player_specials->saved.home_room,
                    CCCYN(ch, C_NRM),
                    real_clan(GET_CLAN(k)) ? real_clan(GET_CLAN(k))->name : "NONE",
                    CCNRM(ch, C_NRM));

        sb_sprintf(&sb, "Life: [%d], Thac0: [%d], Raw Reputation: [%4d]",
                    GET_LIFE_POINTS(k), (int)MIN(THACO(GET_CLASS(k), GET_LEVEL(k)),
                                                 THACO(GET_REMORT_CLASS(k), GET_LEVEL(k))), RAW_REPUTATION_OF(k));

        if (IS_IMMORT(k)) {
            sb_sprintf(&sb, ", Qpoints: [%d/%d]", GET_IMMORT_QP(k),
                        GET_QUEST_ALLOWANCE(k));
        }

        sb_sprintf(&sb, "\r\n%sMobKills:%s [%4d], %sPKills:%s [%4d], %sDeaths:%s [%4d]\r\n",
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), GET_MOBKILLS(k), CCRED(ch,
                                                                       C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(k), CCGRN(ch, C_NRM),
            CCNRM(ch, C_NRM), GET_PC_DEATHS(k));
    }
    sb_sprintf(&sb, "Str: [%s%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  "
                "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
                CCCYN(ch, C_NRM), GET_STR(k), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
    if (k->in_room || !IS_NPC(k)) { // Real Mob/Char
        sb_sprintf(&sb, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k),
            mana_gain(k), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k),
            GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    } else {                    // Virtual Mob
        sb_sprintf(&sb, "Hit p.:[%s%dd%d+%d (%d)%s]  Mana p.:[%s%d%s]  Move p.:[%s%d%s]\r\n",
            CCGRN(ch, C_NRM), GET_HIT(k), GET_MANA(k), GET_MOVE(k),
            (GET_HIT(k) * (GET_MANA(k) + 1) / 2) + GET_MOVE(k), CCNRM(ch,
                                                                      C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(k), CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_MAX_MOVE(k), CCNRM(ch, C_NRM));
    }

    sb_sprintf(&sb, "AC: [%s%d/10%s], Hitroll: [%s%2d%s], Damroll: [%s%2d%s], Speed: [%s%2d%s], DR: [%s%2d%s]\r\n",
        CCYEL(ch, C_NRM), GET_AC(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.hitroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
        k->points.damroll, CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), SPEED_OF(k),
        CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), (int)(damage_reduction(k,
                                                                   NULL) * 100), CCNRM(ch, C_NRM));

    if (!IS_NPC(k) || k->in_room) {
        sb_sprintf(&sb, "Pr:[%s%2d%s],Rd:[%s%2d%s],Pt:[%s%2d%s],Br:[%s%2d%s],Sp:[%s%2d%s],Ch:[%s%2d%s],Ps:[%s%2d%s],Ph:[%s%2d%s]\r\n",
            CCYEL(ch, C_NRM), GET_SAVE(k, 0), CCNRM(ch, C_NRM), CCYEL(ch,
                                                                      C_NRM), GET_SAVE(k, 1), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
            GET_SAVE(k, 2), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), GET_SAVE(k, 3),
            CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), GET_SAVE(k, 4), CCNRM(ch,
                                                                      C_NRM), CCYEL(ch, C_NRM), GET_SAVE(k, 5), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_SAVE(k, 6), CCNRM(ch, C_NRM), CCYEL(ch,
                                                                      C_NRM), GET_SAVE(k, 7), CCNRM(ch, C_NRM));
    }
    if (IS_NPC(k)) {
        sb_sprintf(&sb, "Gold:[%'8" PRId64 "], Cash:[%'8" PRId64 "], (Total: %'" PRId64 ")\r\n",
                    GET_GOLD(k), GET_CASH(k), GET_GOLD(k) + GET_CASH(k));
    } else {
        sb_sprintf(&sb, "Au:[%'8" PRId64 "], Bank:[%'9" PRId64 "], Cash:[%'8" PRId64 "], Enet:[%'9" PRId64 "], (Total: %'" PRId64 ")\r\n",
            GET_GOLD(k), GET_PAST_BANK(k), GET_CASH(k), GET_FUTURE_BANK(k),
            GET_GOLD(k) + GET_PAST_BANK(k) + GET_FUTURE_BANK(k) + GET_CASH(k));
    }

    if (IS_NPC(k)) {
        sb_sprintf(&sb, "Pos: %s, Dpos: %s, Attack: %s",
                    position_types[(int)GET_POSITION(k)],
                    position_types[(int)k->mob_specials.shared->default_pos],
                    attack_hit_text[k->mob_specials.shared->attack_type].singular);
        if (k->in_room) {
            sb_sprintf(&sb, ", %sFT%s: %s, %sHNT%s: %s, Timer: %d",
                        CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
                        (is_fighting(k) ? GET_NAME(random_opponent(k)) : "N"),
                        CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                        NPC_HUNTING(k) ? PERS(NPC_HUNTING(k), ch) : "N",
                        k->char_specials.timer);
        }
    } else if (k->in_room) {
        sb_sprintf(&sb, "Pos: %s, %sFT%s: %s, %sHNT%s: %s",
                    position_types[(int)GET_POSITION(k)],
                    CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
                    (is_fighting(k) ? GET_NAME(random_opponent(k)) : "N"),
                    CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                    NPC_HUNTING(k) ? PERS(NPC_HUNTING(k), ch) : "N");
    }
    if (k->desc) {
        sb_sprintf(&sb, ", Connected: %s, Idle [%d]\r\n",
                    strlist_aref((int)k->desc->input_mode, desc_modes),
                    k->char_specials.timer);
    } else {
        sb_strcat(&sb, "\r\n", NULL);
    }

    if (GET_POSITION(k) == POS_MOUNTED && MOUNTED_BY(k)) {
        sb_sprintf(&sb, "Mount: %s\r\n", GET_NAME(MOUNTED_BY(k)));
    }

    if (IS_NPC(k)) {
        sprintbit(NPC_FLAGS(k), action_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
        sprintbit(NPC2_FLAGS(k), action2_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "NPC flags(2): %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
    } else {
        if (GET_LEVEL(ch) >= LVL_CREATOR) {
            sprintbit(PLR_FLAGS(k), player_bits, buf, sizeof(buf));
        } else {
            sprintbit(PLR_FLAGS(k) & ~PLR_LOG, player_bits, buf, sizeof(buf));
        }
        sb_sprintf(&sb, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
        sprintbit(PLR2_FLAGS(k), player2_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "PLR2: %s%s%s\r\n", CCCYN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
        sprintbit(PRF_FLAGS(k), preference_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
        sprintbit(PRF2_FLAGS(k), preference2_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "PRF2: %s%s%s\r\n", CCGRN(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
        if (PLR_FLAGGED(k, PLR_FROZEN)) {
            sb_sprintf(&sb, "%sFrozen by: %s", CCCYN(ch, C_NRM),
                        player_name_by_idnum(k->player_specials->freezer_id));
            if (k->player_specials->thaw_time > 0) {
                sb_sprintf(&sb, ", will auto-thaw at %s%s\r\n",
                            tmp_ctime(k->player_specials->thaw_time),
                            CCNRM(ch, C_NRM));
            } else {
                sb_sprintf(&sb, "%s\r\n", CCNRM(ch, C_NRM));
            }
        }
    }

    if (IS_NPC(k)) {
        sb_sprintf(&sb, "Mob Spec: %s%s%s, NPC Dam: %dd%d, Morale: %d, Lair: %d, Ldr: %d\r\n",
            CCYEL(ch, C_NRM), (k->mob_specials.shared->func ? ((i =
                                                                    find_spec_index_ptr(k->mob_specials.shared->func)) <
                                                               0 ? "Exists" : spec_list[i].tag) : "None"), CCNRM(ch,
                                                                                                                 C_NRM), k->mob_specials.shared->damnodice,
            k->mob_specials.shared->damsizedice,
            k->mob_specials.shared->morale, k->mob_specials.shared->lair,
            k->mob_specials.shared->leader);

        if (NPC_SHARED(k)->move_buf) {
            sb_sprintf(&sb, "Move_buf: %s\r\n", NPC_SHARED(k)->move_buf);
        }
        if (k->mob_specials.shared->proto == k) {
            char *param = GET_NPC_PARAM(k);

            if (param && strlen(param) > 0) {
                sb_sprintf(&sb, "Spec_param: \r\n%s\r\n", GET_NPC_PARAM(k));
            }
            param = GET_LOAD_PARAM(k);
            if (param && strlen(param) > 0) {
                sb_sprintf(&sb, "Load_param: \r\n%s\r\n", GET_LOAD_PARAM(k));
            }
        }
    }

    for (i = 0, num = 0, num2 = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(k, i)) {
            num++;
        }
        if (GET_IMPLANT(k, i)) {
            num2++;
        }
    }

    found = false;
    if (k->in_room) {
        sb_sprintf(&sb, "Encum : (%.2f inv + %.2f eq) = (%.2f tot)/%f, Number: %d/%d inv, %d eq, %d imp\r\n",
            IS_CARRYING_W(k), IS_WEARING_W(k),
            (IS_CARRYING_W(k) + IS_WEARING_W(k)), CAN_CARRY_W(k),
            IS_CARRYING_N(k), (int)CAN_CARRY_N(k), num, num2);
        if (BREATH_COUNT_OF(k) || GET_FALL_COUNT(k)) {
            sb_sprintf(&sb, "Breath_count: %d, Fall_count: %d",
                        BREATH_COUNT_OF(k), GET_FALL_COUNT(k));
            found = true;
        }
    }
    if (!IS_NPC(k)) {
        sb_sprintf(&sb, "%sHunger: %d, Thirst: %d, Drunk: %d\r\n",
                    found ? ", " : "",
                    GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    } else if (found) {
        sb_strcat(&sb, "\r\n", NULL);
    }

    if (!IS_NPC(k) && GET_QUEST(k)) {
        const char *name = "None";
        struct quest *quest = quest_by_vnum(GET_QUEST(k));

        if (quest != NULL && is_playing_quest(quest, GET_IDNUM(k))) {
            name = quest->name;
        }
        sb_sprintf(&sb, "Quest [%d]: \'%s\'\r\n", GET_QUEST(k), name);
    }

    if (k->in_room && (k->master || k->followers)) {
        sb_sprintf(&sb, "Master is: %s, Followers are:",
                    ((k->master) ? GET_NAME(k->master) : "<none>"));

        line_buf = "";
        for (fol = k->followers; fol; fol = fol->next) {
            line_buf = tmp_sprintf("%s %s",
                                   (*line_buf) ? "," : "", PERS(fol->follower, ch));
            if (strlen(line_buf) >= 62) {
                sb_strcat(&sb, line_buf, "\r\n", NULL);
                line_buf = "";
            }
        }

        sb_strcat(&sb, "\r\n", NULL);
    }
    /* Showing the bitvector */
    if (AFF_FLAGS(k)) {
        sprintbit(AFF_FLAGS(k), affected_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
    }
    if (AFF2_FLAGS(k)) {
        sprintbit(AFF2_FLAGS(k), affected2_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "AFF2: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
    }
    if (AFF3_FLAGS(k)) {
        sprintbit(AFF3_FLAGS(k), affected3_bits, buf, sizeof(buf));
        sb_sprintf(&sb, "AFF3: %s%s%s\r\n", CCYEL(ch, C_NRM), buf,
                    CCNRM(ch, C_NRM));
    }
    if (GET_POSITION(k) == POS_SITTING && AFF2_FLAGGED(k, AFF2_MEDITATE)) {
        sb_sprintf(&sb, "Meditation Timer: [%d]\r\n", MEDITATE_TIMER(k));
    }

    if (IS_CYBORG(k)) {
        sb_sprintf(&sb, "Broken component: [%s (%d)], Dam Count: %d/%d.\r\n",
                    get_component_name(GET_BROKE(k), GET_OLD_CLASS(k)),
                    GET_BROKE(k), GET_TOT_DAM(k), max_component_dam(k));

        if (AFF3_FLAGGED(k, AFF3_SELF_DESTRUCT)) {
            sb_sprintf(&sb, "Self-destruct Timer: [%d]\r\n", MEDITATE_TIMER(k));
        }
    }

    if (k->prog_state && k->prog_state->var_list) {
        struct prog_var *cur_var;
        sb_strcat(&sb, "Prog state variables:\r\n", NULL);
        for (cur_var = k->prog_state->var_list; cur_var;
             cur_var = cur_var->next) {
            sb_sprintf(&sb, "     %s = '%s'\r\n", cur_var->key, cur_var->value);
        }
    }

    if (IS_NPC(k)) {
        sb_sprintf(&sb, "NPC Voice: %s%s%s, ",
                    CCYEL(ch, C_NRM), voice_name(GET_VOICE(k)), CCNRM(ch, C_NRM));
    }
    sb_sprintf(&sb, "Currently speaking: %s%s%s\r\n", CCCYN(ch, C_NRM),
                tongue_name(GET_TONGUE(k)), CCNRM(ch, C_NRM));

    if (GET_LANG_HEARD(k)) {
        sb_sprintf(&sb, "Recently heard: %s", CCCYN(ch, C_NRM));

        bool first = true;
        for (GList *lang = GET_LANG_HEARD(k); lang; lang = lang->next) {
            if (first) {
                first = false;
            } else {
                sb_strcat(&sb, CCNRM(ch, C_NRM), ", ", CCCYN(ch, C_NRM), NULL);
            }

            sb_strcat(&sb, tongue_name(GPOINTER_TO_INT(lang->data)), NULL);
        }
        sb_sprintf(&sb, "%s\r\n", CCNRM(ch, C_NRM));
    }
    sb_sprintf(&sb, "Known Languages:\r\n");
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, tongues);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        int vnum = GPOINTER_TO_INT(key);
        struct tongue *tongue = val;

        if (CHECK_TONGUE(k, GPOINTER_TO_INT(vnum))) {
            sb_sprintf(&sb, "%s%3d. %-30s %s%-17s%s%s\r\n",
                        CCCYN(ch, C_NRM),
                        GPOINTER_TO_INT(vnum),
                        tongue->name,
                        CCBLD(ch, C_SPR),
                        fluency_desc(k, GPOINTER_TO_INT(vnum)),
                        tmp_sprintf("%s[%3d]%s", CCYEL(ch, C_NRM),
                                    CHECK_TONGUE(k, GPOINTER_TO_INT(vnum)),
                                    CCNRM(ch, C_NRM)), CCNRM(ch, C_SPR));
        }
    }

    if (IS_PC(k) && GET_GRIEVANCES(k)) {
        sb_sprintf(&sb, "Grievances:\r\n");

        for (GList *it = GET_GRIEVANCES(k); it; it = it->next) {
            struct grievance *grievance = it->data;
            sb_sprintf(&sb, "%s%3d. %s got %d rep for %s at %s%s\r\n",
                        CCGRN(ch, C_NRM),
                        grievance->player_id,
                        player_name_by_idnum(grievance->player_id),
                        grievance->rep,
                        grievance_kind_descs[grievance->grievance],
                        tmp_ctime(grievance->time), CCNRM(ch, C_NRM));
        }
    }

    if (IS_PC(k)
        && k->player_specials->tags
        && g_hash_table_size(k->player_specials->tags) > 0) {
        sb_sprintf(&sb, "Tags:");

        g_hash_table_iter_init(&iter, k->player_specials->tags);

        while (g_hash_table_iter_next(&iter, &key, NULL)) {
            sb_sprintf(&sb, " %s", (char *)key);
        }
        sb_sprintf(&sb, "\r\n");
    }

    /* Routine to show what spells a char is affected by */
    if (k->affected) {
        for (aff = k->affected; aff; aff = aff->next) {
            sb_sprintf(&sb, "SPL: (%3d%s) [%2d] %s(%ld) %s%-24s%s ",
                        aff->duration + 1, aff->is_instant ? "sec" : "hr", aff->level,
                        CCYEL(ch, C_NRM), aff->owner, CCCYN(ch, C_NRM),
                        spell_to_str(aff->type), CCNRM(ch, C_NRM));
            if (aff->modifier) {
                sb_sprintf(&sb, "%+d to %s", aff->modifier,
                            apply_types[(int)aff->location]);
                if (aff->bitvector) {
                    sb_strcat(&sb, ", ", NULL);
                }
            }

            if (aff->bitvector) {
                if (aff->aff_index == 3) {
                    sprintbit(aff->bitvector, affected3_bits, buf, sizeof(buf));
                } else if (aff->aff_index == 2) {
                    sprintbit(aff->bitvector, affected2_bits, buf, sizeof(buf));
                } else {
                    sprintbit(aff->bitvector, affected_bits, buf, sizeof(buf));
                }
                sb_strcat(&sb, "sets ", buf, NULL);
            }
            sb_strcat(&sb, "\r\n", NULL);
        }
    }
    page_string(ch->desc, sb.str);
}

ACMD(do_stat)
{
    struct creature *victim = NULL;
    struct obj_data *object = NULL;
    struct zone_data *zone = NULL;
    int tmp, found;
    char *arg1 = tmp_getword(&argument);
    char *options = argument;
    char *arg2 = tmp_getword(&argument);
    if (*arg2) {
        options = argument;
    }

    if (!*arg1) {
        send_to_char(ch, "Stats on who or what?\r\n");
        return;
    } else if (is_abbrev(arg1, "room")) {
        do_stat_room(ch, arg2);
    } else if (!strncmp(arg1, "trails", 6)) {
        do_stat_trails(ch);
    } else if (is_abbrev(arg1, "zone")) {
        if (!*arg2) {
            do_stat_zone(ch, ch->in_room->zone);
        } else {
            if (is_number(arg2)) {
                tmp = atoi(arg2);
                for (found = 0, zone = zone_table; zone && found != 1;
                     zone = zone->next) {
                    if (zone->number <= tmp && zone->top > tmp * 100) {
                        do_stat_zone(ch, zone);
                        found = 1;
                    }
                }
                if (found != 1) {
                    send_to_char(ch, "No zone exists with that number.\r\n");
                }
            } else {
                send_to_char(ch, "Invalid zone number.\r\n");
            }
        }
    } else if (is_abbrev(arg1, "mob")) {
        if (!*arg2) {
            send_to_char(ch, "Stats on which mobile?\r\n");
        } else {
            if ((victim = get_char_vis(ch, arg2)) &&
                (IS_NPC(victim) || GET_LEVEL(ch) >= LVL_DEMI)) {
                do_stat_character(ch, victim, options);
            } else {
                send_to_char(ch, "No such mobile around.\r\n");
            }
        }
    } else if (is_abbrev(arg1, "player")) {
        if (!*arg2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
            if ((victim = get_player_vis(ch, arg2, 0))) {
                do_stat_character(ch, victim, options);
            } else {
                send_to_char(ch, "No such player around.\r\n");
            }
        }
    } else if (is_abbrev(arg1, "file")) {
        if (!is_authorized(ch, STAT_PLAYER_FILE, NULL)) {
            send_to_char(ch, "You cannot peer into the playerfile.\r\n");
            return;
        }
        if (!*arg2) {
            send_to_char(ch, "Stats on which player?\r\n");
        } else {
            if (!player_name_exists(arg2)) {
                send_to_char(ch, "There is no such player.\r\n");
            } else {
                victim = load_player_from_xml(player_idnum_by_name(arg2));
                if (victim) {
                    do_stat_character(ch, victim, options);
                    free_creature(victim);
                } else {
                    send_to_char(ch, "Error loading character '%s'\r\n", arg2);
                }
            }
        }
    } else if (is_abbrev(arg1, "object")) {
        if (!*arg2) {
            send_to_char(ch, "Stats on which object?\r\n");
        } else {
            if ((object = get_obj_vis(ch, arg2))) {
                do_stat_object(ch, object);
            } else {
                send_to_char(ch, "No such object around.\r\n");
            }
        }
    } else {
        if ((object = get_object_in_equip_vis(ch, arg1, ch->equipment, &tmp))) {
            do_stat_object(ch, object);
        } else if ((object = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
            do_stat_object(ch, object);
        } else if ((victim = get_char_room_vis(ch, arg1))) {
            do_stat_character(ch, victim, options);
        } else if ((object =
                        get_obj_in_list_vis(ch, arg1, ch->in_room->contents))) {
            do_stat_object(ch, object);
        } else if ((victim = get_char_vis(ch, arg1))) {
            do_stat_character(ch, victim, options);
        } else if ((object = get_obj_vis(ch, arg1))) {
            do_stat_object(ch, object);
        } else {
            send_to_char(ch, "Nothing around by that name.\r\n");
        }
    }
}

ACMD(do_shutdown)
{
    gboolean update_shutdown_timer(gpointer data);
    extern GMainLoop *main_loop;
    extern int circle_shutdown, circle_reboot, mini_mud, shutdown_idnum;
    char *arg, *arg2;
    int count = 0;

    if (subcmd != SCMD_SHUTDOWN) {
        send_to_char(ch, "If you want to shut something down, say so!\r\n");
        return;
    }
    arg = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (*arg2) {
        if (shutdown_count >= 0) {
            send_to_char(ch, "Shutdown process is already running.\r\n"
                         "Use Shutdown abort first.\r\n");
            return;
        }
        if ((count = atoi(arg2)) < 0) {
            send_to_char(ch, "Real funny shutdown time.\r\n");
            return;
        }
    }

    if (!*arg) {
        if (!mini_mud) {
            send_to_char(ch, "Please specify a shutdown mode.\r\n");
            return;
        }
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all("\r\n"
                    "As you stand amazed, you see the sun swell and fill the sky,\r\n"
                    "and you are overtaken by an intense heat as the particles\r\n"
                    "of your body are mixed with the rest of the cosmos by the\r\n"
                    "power of a blinding supernova explosion.\r\n\r\n"
                    "Shutting down.\r\n"
                    "Please visit our website at http://tempusmud.com\r\n");
        circle_shutdown = true;
        g_main_loop_quit(main_loop);
    } else if (!strcasecmp(arg, "abort")) {
        if (shutdown_count < 0) {
            send_to_char(ch, "Shutdown process is not currently running.\r\n");
        } else {
            shutdown_count = -1;
            shutdown_idnum = -1;
            shutdown_mode = SHUTDOWN_NONE;
            slog("(GC) Shutdown ABORT called by %s.", GET_NAME(ch));
            send_to_all(":: Tempus REBOOT sequence aborted ::\r\n");
        }
        return;

    } else if (!strcasecmp(arg, "reboot")) {
        if (!count) {
            touch("../.fastboot");
            slog("(GC) Reboot by %s.", GET_NAME(ch));
            send_to_all("\r\n"
                        "You stagger under the force of a sudden gale, and cringe in terror\r\n"
                        "as the sky darkens before the gathering stormclouds.  Lightning\r\n"
                        "crashes to the ground all around you, and you see the hand of the\r\n"
                        "Universal Power rush across the land, destroying all and purging\r\n"
                        "the cosmos, only to begin rebuilding once again.\r\n\r\n"
                        "Rebooting... come back in five minutes.\r\n"
                        "Please visit our website at http://tempusmud.com\r\n");
            circle_shutdown = circle_reboot = true;
            g_main_loop_quit(main_loop);
        } else {
            slog("(GC) Reboot in [%d] seconds by %s.", count, GET_NAME(ch));
            snprintf(buf, sizeof(buf), "\007\007_ Tempus REBOOT in %d seconds ::\r\n",
                     count);
            send_to_all(buf);
            shutdown_idnum = GET_IDNUM(ch);
            shutdown_count = count;
            shutdown_mode = SHUTDOWN_REBOOT;
            g_timeout_add(1000, update_shutdown_timer, NULL);
        }
    } else if (!strcasecmp(arg, "die")) {
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all
            ("As you stand amazed, you see the sun swell and fill the sky,\r\n"
            "and you are overtaken by an intense heat as the particles\r\n"
            "of your body are mixed with the rest of the cosmos by the\r\n"
            "power of a blinding supernova explosion.\r\n\r\n"
            "Shutting down for maintenance.\r\n"
            "Please visit our website at http://tempusmud.com\r\n");

        touch("../.killscript");
        circle_shutdown = true;
        g_main_loop_quit(main_loop);
    } else if (!strcasecmp(arg, "pause")) {
        slog("(GC) Shutdown by %s.", GET_NAME(ch));
        send_to_all
            ("As you stand amazed, you see the sun swell and fill the sky,\r\n"
            "and you are overtaken by an intense heat as the particles\r\n"
            "of your body are mixed with the rest of the cosmos by the\r\n"
            "power of a blinding supernova explosion.\r\n\r\n"
            "Shutting down for maintenance.\r\n"
            "Please visit our website at http://tempusmud.com\r\n");

        touch("../pause");
        circle_shutdown = true;
        g_main_loop_quit(main_loop);
    } else {
        send_to_char(ch, "Unknown shutdown option.\r\n");
    }
}

void
stop_snooping(struct creature *ch)
{
    if (!ch->desc->snooping) {
        send_to_char(ch, "You aren't snooping anyone.\r\n");
    } else {
        send_to_char(ch, "You stop snooping.\r\n");
        ch->desc->snooping->snoop_by =
            g_list_remove(ch->desc->snooping->snoop_by, ch->desc);
        ch->desc->snooping = NULL;
    }
}

ACMD(do_snoop)
{
    struct creature *victim, *tch;

    if (!ch->desc) {
        return;
    }

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        stop_snooping(ch);
    } else if (!(victim = get_char_vis(ch, arg))) {
        send_to_char(ch, "No such person around.\r\n");
    } else if (!victim->desc) {
        send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
    } else if (victim == ch) {
        stop_snooping(ch);
    } else if (PRF_FLAGGED(victim, PRF_NOSNOOP) && GET_LEVEL(ch) < LVL_ENTITY) {
        send_to_char(ch, "The gods say I don't think so!\r\n");
    } else if (victim->desc->snooping == ch->desc) {
        send_to_char(ch, "Don't be stupid.\r\n");
    } else if (ROOM_FLAGGED(victim->in_room, ROOM_GODROOM)
               && !is_authorized(ch, SNOOP_IN_GODROOMS, NULL)) {
        send_to_char(ch, "You cannot snoop into that place.\r\n");
    } else {
        if (g_list_find(victim->desc->snoop_by, ch->desc)) {
            act("You're already snooping $M.", false, ch, NULL, victim, TO_CHAR);
            return;
        }
        if (victim->desc->original) {
            tch = victim->desc->original;
        } else {
            tch = victim;
        }

        if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
            send_to_char(ch, "You can't.\r\n");
            return;
        }
        send_to_char(ch, "%s", OK);

        if (ch->desc->snooping) {
            ch->desc->snooping->snoop_by =
                g_list_remove(ch->desc->snooping->snoop_by, ch->desc);
        }

        slog("(GC) %s has begun to snoop %s.", GET_NAME(ch), GET_NAME(victim));

        ch->desc->snooping = victim->desc;
        victim->desc->snoop_by =
            g_list_prepend(victim->desc->snoop_by, ch->desc);
    }
}

ACMD(do_switch)
{
    struct creature *victim;

    char *arg = tmp_getword(&argument);

    if (ch->desc->original) {
        send_to_char(ch, "You're already switched.\r\n");
    } else if (!*arg) {
        send_to_char(ch, "Switch with who?\r\n");
    } else if (!(victim = get_char_vis(ch, arg))) {
        send_to_char(ch, "No such character.\r\n");
    } else if (ch == victim) {
        send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
    } else if (victim->desc) {
        send_to_char(ch, "You can't do that, the body is already in use!\r\n");
    } else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim)) {
        send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
    } else if (GET_LEVEL(ch) <= GET_LEVEL(victim) && GET_IDNUM(ch) != 1) {
        send_to_char(ch, "Nope.\r\n");
    } else {
        send_to_char(ch, "%s", OK);

        ch->desc->creature = victim;
        ch->desc->original = ch;

        victim->desc = ch->desc;
        ch->desc = NULL;

        slog("(%s) %s has %sswitched into %s at %d.",
             subcmd == SCMD_QSWITCH ? "QC" : "GC", GET_NAME(ch),
             subcmd == SCMD_QSWITCH ? "q" : "",
             GET_NAME(victim), victim->in_room->number);
    }
}

ACMD(do_rswitch)
{
    struct creature *orig, *victim;

    char *arg = tmp_getword(&argument);
    char *arg2 = tmp_getword(&argument);


    if (!*arg) {
        send_to_char(ch, "Switch with who?\r\n");
    } else if (!*arg2) {
        send_to_char(ch, "Switch them with who?\r\n");
    } else if (!(orig = get_char_vis(ch, arg))) {
        send_to_char(ch, "What player do you want to switch around?\r\n");
    } else if (!orig->desc) {
        send_to_char(ch, "No link there, sorry.\r\n");
    } else if (orig->desc->original) {
        send_to_char(ch, "They're already switched.\r\n");
    } else if (!(victim = get_char_vis(ch, arg2))) {
        send_to_char(ch, "No such character.\r\n");
    } else if (orig == victim || ch == victim) {
        send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
    } else if (victim->desc) {
        send_to_char(ch, "You can't do that, the body is already in use!\r\n");
    } else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim)) {
        send_to_char(ch, "You can't use a mortal's body like that!\r\n");
    } else if ((GET_LEVEL(ch) < LVL_IMPL) && GET_LEVEL(orig) < LVL_AMBASSADOR) {
        send_to_char(ch, "You cannot allow that mortal to do that.\r\n");
    } else if (GET_LEVEL(ch) < GET_LEVEL(orig)) {
        send_to_char(ch, "Maybe you should just ask them.\r\n");
    } else {
        send_to_char(ch, "%s", OK);
        send_to_char(orig, "The world seems to lurch and shift.\r\n");
        orig->desc->creature = victim;
        orig->desc->original = orig;

        victim->desc = orig->desc;
        orig->desc = NULL;

        slog("(GC) %s has rswitched %s into %s at %d.", GET_NAME(ch),
             GET_NAME(orig), GET_NAME(victim), victim->in_room->number);

    }
}

static inline struct room_data *
get_start_room(struct creature *ch)
{
    return (GET_HOME(ch) == HOME_MODRIAN ? r_mortal_start_room :
            GET_HOME(ch) == HOME_ELECTRO ? r_electro_start_room :
            GET_HOME(ch) == HOME_NEW_THALOS ? r_new_thalos_start_room :
            GET_HOME(ch) == HOME_KROMGUARD ? r_kromguard_start_room :
            GET_HOME(ch) == HOME_ELVEN_VILLAGE ? r_elven_start_room :
            GET_HOME(ch) == HOME_ISTAN ? r_istan_start_room :
            GET_HOME(ch) == HOME_MONK ? r_monk_start_room :
            GET_HOME(ch) == HOME_ARENA ? r_arena_start_room :
            GET_HOME(ch) == HOME_SKULLPORT ? r_skullport_start_room :
            GET_HOME(ch) == HOME_SOLACE_COVE ? r_solace_start_room :
            GET_HOME(ch) == HOME_MAVERNAL ? r_mavernal_start_room :
            GET_HOME(ch) == HOME_DWARVEN_CAVERNS ? r_dwarven_caverns_start_room :
            GET_HOME(ch) == HOME_HUMAN_SQUARE ? r_human_square_start_room :
            GET_HOME(ch) == HOME_SKULLPORT ? r_skullport_start_room :
            GET_HOME(ch) == HOME_DROW_ISLE ? r_drow_isle_start_room :
            GET_HOME(ch) == HOME_ASTRAL_MANSE ? r_astral_manse_start_room :
            GET_HOME(ch) == HOME_NEWBIE_SCHOOL ? r_newbie_school_start_room :
            GET_HOME(ch) == HOME_NEWBIE_TUTORIAL_COMPLETE ? r_newbie_tutorial_complete_start_room :
            r_mortal_start_room);
}

ACMD(do_return)
{
    struct creature *orig = NULL;
    bool cloud_found = false;

    if (ch->desc && (orig = ch->desc->original)) {
        // Return from being switched or gasified
        if (IS_NPC(ch) && GET_NPC_VNUM(ch) == 1518) {
            cloud_found = true;
            if (subcmd == SCMD_RETURN) {
                send_to_char(ch, "Use recorporealize.\r\n");
                return;
            }
        }
        send_to_char(ch, "You return to your original body.\r\n");

        /* JE 2/22/95 */
        /* if someone switched into your original body, disconnect them */
        if (ch->desc->original->desc) {
            close_socket(ch->desc->original->desc);
        }

        ch->desc->creature = ch->desc->original;
        ch->desc->original = NULL;

        ch->desc->creature->desc = ch->desc;
        ch->desc = NULL;

        if (cloud_found) {
            char_from_room(orig, false);
            char_to_room(orig, ch->in_room, false);
            act("$n materializes from a cloud of gas.",
                false, orig, NULL, NULL, TO_ROOM);
            if (subcmd != SCMD_NOEXTRACT) {
                creature_purge(ch, true);
            }
        }
        return;
    }

    if (IS_NPC(ch)) {
        send_to_char(ch, "Return to where?\r\n");
        return;
    }

    if (is_fighting(ch)) {
        send_to_char(ch, "No way!  You're fighting for your life!\r\n");
        return;
    }

    if (GET_LEVEL(ch) > LVL_CAN_RETURN || IS_REMORT(ch)) {
        send_to_char(ch, "You're too powerful to return home that way. Look for a different way to recall.\r\n");
        return;
    }

    // Return to newbie start room
    act("A whirling globe of multi-colored light appears and whisks you away!", false, ch, NULL, NULL, TO_CHAR);
    act("A whirling globe of multi-colored light appears and whisks $n away!", false, ch, NULL, NULL, TO_ROOM);
    char_from_room(ch, false);
    char_to_room(ch, get_start_room(ch), false);
    look_at_room(ch, ch->in_room, 0);
    act("A whirling globe of multi-colored light appears and deposits $n on the floor!", false, ch, NULL, NULL, TO_ROOM);
}

ACMD(do_mload)
{
    struct creature *mob;
    int number;

    one_argument(argument, buf);

    if (!*buf || !isdigit(*buf)) {
        send_to_char(ch, "Usage: mload <mobile vnum number>\r\n");
        return;
    }
    if ((number = atoi(buf)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (!real_mobile_proto(number)) {
        send_to_char(ch, "There is no mobile thang with that number.\r\n");
        return;
    }
    mob = read_mobile(number);
    if (mob == NULL) {
        send_to_char(ch, "The mobile couldn't be created.\r\n");
        return;
    }
    char_to_room(mob, ch->in_room, false);

    act("$n makes a quaint, magical gesture with one hand.", true, ch,
        NULL, NULL, TO_ROOM);
    act("$n has created $N!", false, ch, NULL, mob, TO_ROOM);
    act("You create $N.", false, ch, NULL, mob, TO_CHAR);

    slog("(GC) %s mloaded %s at %d.", GET_NAME(ch), GET_NAME(mob),
         ch->in_room->number);

    if (GET_NPC_PROG(mob)) {
        trigger_prog_load(mob);
    }
}

ACMD(do_oload)
{
    struct obj_data *obj = NULL;
    int number, quantity;
    char *temp, *temp2;

    temp = tmp_getword(&argument);
    temp2 = tmp_getword(&argument);

    if (!*temp || !isdigit(*temp)) {
        send_to_char(ch, "Usage: oload [quantity] <object vnum>\r\n");
        return;
    }

    if (!*temp2 || !isdigit(*temp2)) {
        number = atoi(temp);
        quantity = 1;
    } else {
        quantity = atoi(temp);
        number = atoi(temp2);
    }

    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (!real_object_proto(number)) {
        send_to_char(ch, "There is no object with that number.\r\n");
        return;
    }
    if (quantity < 0) {
        send_to_char(ch, "How do you expect to create negative objects?\r\n");
        return;
    }
    if (quantity == 0) {
        send_to_char(ch,
                     "POOF!  Congratulations!  You've created nothing!\r\n");
        return;
    }
    if (quantity > 100) {
        send_to_char(ch, "You can't possibly need THAT many!\r\n");
        return;
    }

    for (int i = 0; i < quantity; i++) {
        obj = read_object(number);
        obj->creation_method = CREATED_IMM;
        obj->creator = GET_IDNUM(ch);
        obj_to_room(obj, ch->in_room);
    }
    act("$n makes a strange magical gesture.", true, ch, NULL, NULL, TO_ROOM);
    if (quantity == 1) {
        act("$n has created $p!", false, ch, obj, NULL, TO_ROOM);
        act("You create $p.", false, ch, obj, NULL, TO_CHAR);
        slog("(GC) %s loaded %s at %d.", GET_NAME(ch),
             obj->name, ch->in_room->number);
    } else {
        act(tmp_sprintf("%s has created %s! (x%d)", GET_NAME(ch), obj->name,
                        quantity), false, ch, obj, NULL, TO_ROOM);
        act(tmp_sprintf("You create %s. (x%d)", obj->name, quantity), false,
            ch, obj, NULL, TO_CHAR);
        slog("(GC) %s loaded %s at %d. (x%d)", GET_NAME(ch), obj->name,
             ch->in_room->number, quantity);
    }
}

ACMD(do_pload)
{
    struct creature *vict = NULL;
    struct obj_data *obj = NULL;
    int number, quantity;
    char *temp, *temp2;

    temp = tmp_getword(&argument);
    temp2 = tmp_getword(&argument);

    if (!*temp || !isdigit(*temp)) {
        send_to_char(ch,
                     "Usage: pload [quantity] <object vnum> [target char]\r\n");
        return;
    }
    if (!*temp2) {
        number = atoi(temp);
        quantity = 1;
    }
    // else, :)  there are two or three arguments
    else {
        // check to see if there's a quantity argument
        if (isdigit(*temp2)) {
            quantity = atoi(temp);
            number = atoi(temp2);
            // get the vict if there is one

            temp = tmp_getword(&argument);
            if (*temp) {
                if (!(vict = get_char_vis(ch, temp))) {
                    send_to_char(ch, "%s", NOPERSON);
                    return;
                }
            }
        }
        // second arg must be vict
        else {
            number = atoi(temp);
            quantity = 1;
            // find vict.  If no visible corresponding char, return
            if (!(vict = get_char_vis(ch, temp2))) {
                send_to_char(ch, "%s", NOPERSON);
                return;
            }
        }
    }

    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (quantity == 0) {
        send_to_char(ch,
                     "POOF!  Congratulations!  You've created nothing!\r\n");
        return;
    }
    if (quantity < 0) {
        send_to_char(ch, "How do you expect to create negative objects?\r\n");
        return;
    }

    // put a cap on how many someone can load
    if (quantity > 100) {
        send_to_char(ch, "You can't possibly need THAT many!\r\n");
        return;
    }

    if (!real_object_proto(number)) {
        send_to_char(ch, "There is no object with that number.\r\n");
        return;
    }
    if (vict) {
        for (int i = 0; i < quantity; i++) {
            obj = read_object(number);
            obj->creation_method = CREATED_IMM;
            obj->creator = GET_IDNUM(ch);
            obj_to_char(obj, vict);
        }

        act("$n does something suspicious and alters reality.", true, ch, NULL, NULL,
            TO_ROOM);

        // no quantity list if there's only one object.
        if (quantity == 1) {
            act("You load $p onto $N.", true, ch, obj, vict, TO_CHAR);
            act("$N causes $p to appear in your hands.", true, vict, obj, ch,
                TO_CHAR);
        } else {
            act(tmp_sprintf("You load %s onto %s. (x%d)", obj->name,
                            GET_NAME(vict), quantity), false, ch, obj, vict, TO_CHAR);
            act(tmp_sprintf("%s causes %s to appear in your hands. (x%d)",
                            GET_NAME(ch), obj->name, quantity), false, ch, obj,
                vict, TO_VICT);
        }

    } else {
        act("$n does something suspicious and alters reality.", true, ch, NULL, NULL,
            TO_ROOM);
        for (int i = 0; i < quantity; i++) {
            obj = read_object(number);
            obj->creation_method = CREATED_IMM;
            obj->creator = GET_IDNUM(ch);
            obj_to_char(obj, ch);
        }

        if (quantity == 1) {
            act("You create $p.", false, ch, obj, NULL, TO_CHAR);
        } else {
            act(tmp_sprintf("You create %s. (x%d)", obj->name,
                            quantity), false, ch, obj, NULL, TO_CHAR);
        }
    }
    if (quantity == 1) {
        slog("(GC) %s ploaded %s on %s.", GET_NAME(ch), obj->name,
             vict ? GET_NAME(vict) : GET_NAME(ch));
    } else {
        slog("(GC) %s ploaded %s on %s. (x%d)", GET_NAME(ch),
             obj->name, vict ? GET_NAME(vict) : GET_NAME(ch), quantity);
    }
}

ACMD(do_vstat)
{
    struct creature *mob = NULL;
    struct obj_data *obj = NULL;
    bool mob_stat = false;
    char *str;
    int number, tmp;

    str = tmp_getword(&argument);

    if (!*str) {
        send_to_char(ch,
                     "Usage: vstat { { obj | mob } <number> | <alias> }\r\n");
        return;
    } else if ((mob_stat = is_abbrev(str, "mobile")) ||
               is_abbrev(str, "object")) {
        str = tmp_getword(&argument);
        if (!is_number(str)) {
            send_to_char(ch,
                         "You must specify a vnum when not using an alias.\r\n");
            return;
        }
        number = atoi(str);
    } else if ((obj = get_object_in_equip_vis(ch, str, ch->equipment, &tmp))) {
        number = GET_OBJ_VNUM(obj);
    } else if ((obj = get_obj_in_list_vis(ch, str, ch->carrying))) {
        number = GET_OBJ_VNUM(obj);
    } else if ((mob = get_char_room_vis(ch, str))) {
        number = GET_NPC_VNUM(mob);
        mob_stat = true;
    } else if ((obj = get_obj_in_list_vis(ch, str, ch->in_room->contents))) {
        number = GET_OBJ_VNUM(obj);
    } else if ((mob = get_char_vis(ch, str))) {
        number = GET_NPC_VNUM(mob);
        mob_stat = true;
    } else if ((obj = get_obj_vis(ch, str))) {
        number = GET_OBJ_VNUM(obj);
    } else {
        send_to_char(ch, "Nothing around by that name.\r\n");
        return;
    }

    if (number < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }

    if (mob_stat) {
        mob = real_mobile_proto(number);
        if (mob) {
            do_stat_character(ch, mob, argument);
        } else {
            send_to_char(ch, "There is no mobile with that vnum.\r\n");
            return;
        }
    } else {
        obj = real_object_proto(number);
        if (obj) {
            do_stat_object(ch, obj);
        } else {
            send_to_char(ch, "There is no object with that vnum.\r\n");
            return;
        }
    }
}

ACMD(do_rstat)
{
    struct creature *mob;
    int number;

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
        send_to_char(ch, "Usage: rstat { obj | mob } <id number>\r\n");
        return;
    }
    if ((number = atoi(buf2)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (is_abbrev(buf, "mob")) {
        if (!(mob = get_char_in_world_by_idnum(-number))) {
            send_to_char(ch, "There is no monster with that id number.\r\n");
        } else {
            do_stat_character(ch, mob, "");
        }
    } else if (is_abbrev(buf, "obj")) {
        send_to_char(ch, "Not enabled.\r\n");
    } else {
        send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
    }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
    struct creature *vict;
    struct obj_data *obj, *next_o;
    struct room_trail_data *trail;
    int i = 0;

    one_argument(argument, buf);

    if (*buf) {                 /* argument supplied. destroy single object
                                 * or char */

        if (!strncmp(buf, "trails", 6)) {
            while ((trail = ch->in_room->trail)) {
                i++;
                if (trail->name) {
                    free(trail->name);
                }
                if (trail->aliases) {
                    free(trail->aliases);
                }
                ch->in_room->trail = trail->next;
                free(trail);
            }
            send_to_char(ch, "%d trails purged from room.\r\n", i);
            return;
        }

        if ((vict = get_char_room_vis(ch, buf))) {
            if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
                send_to_char(ch, "Fuuuuuuuuu!\r\n");
                return;
            }
            act("$n disintegrates $N.", false, ch, NULL, vict, TO_NOTVICT);

            if (!IS_NPC(vict)) {
                mudlog(LVL_POWER, BRF, true,
                       "(GC) %s has purged %s at %d.",
                       GET_NAME(ch), GET_NAME(vict), vict->in_room->number);
            }
            creature_purge(vict, false);
        } else if ((obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents))) {
            act("$n destroys $p.", false, ch, obj, NULL, TO_ROOM);
            slog("(GC) %s purged %s at %d.", GET_NAME(ch),
                 obj->name, ch->in_room->number);
            extract_obj(obj);

        } else {
            send_to_char(ch, "Nothing here by that name.\r\n");
            return;
        }

        send_to_char(ch, "%s", OK);
    } else {                    /* no argument. clean out the room */
        act("$n gestures... You are surrounded by scorching flames!",
            false, ch, NULL, NULL, TO_ROOM);
        send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

        // as creature_purge() has a call to g_list_remove(), we can't
        // iterate through in a simple for loop because removing an
        // item from the GList messes up the iteration
        GList *next_it;
        for (GList *it = first_living(ch->in_room->people); it; it = next_it) {
            next_it = next_living(it);
            struct creature *tch = it->data;

            if (IS_NPC(tch)) {
                creature_purge(tch, true);
            }
        }

        for (obj = ch->in_room->contents; obj; obj = next_o) {
            next_o = obj->next_content;
            extract_obj(obj);
        }

        while (ch->in_room->affects) {
            affect_from_room(ch->in_room, ch->in_room->affects);
        }
        slog("(GC) %s has purged room %s.", GET_NAME(ch), ch->in_room->name);

    }
}

ACMD(do_advance)
{
    struct creature *victim;
    int newlevel;
    void do_start(struct creature *ch, int mode);

    char *name = tmp_getword(&argument);
    char *level = tmp_getword(&argument);

    if (*name) {
        if (!(victim = get_char_vis(ch, name))) {
            send_to_char(ch, "That player is not here.\r\n");
            return;
        }
    } else {
        send_to_char(ch, "Advance who?\r\n");
        return;
    }
    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
        send_to_char(ch, "Maybe that's not such a great idea.\r\n");
        return;
    }
    if (IS_NPC(victim)) {
        send_to_char(ch, "NO!  Not on NPC's.\r\n");
        return;
    }
    if (!*level || (newlevel = atoi(level)) <= 0) {
        send_to_char(ch, "That's not a level!\r\n");
        return;
    }
    if (newlevel > LVL_GRIMP) {
        send_to_char(ch, "%d is the highest possible level.\r\n", LVL_GRIMP);
        return;
    }
    if (newlevel > GET_LEVEL(ch)) {
        send_to_char(ch, "Yeah, right.\r\n");
        return;
    }
    if (newlevel < GET_LEVEL(victim)) {
        do_start(victim, false);
        GET_LEVEL(victim) = newlevel;
    } else {
        act("$n makes some strange gestures.\r\n"
            "A strange feeling comes upon you,\r\n"
            "Like a giant hand, light comes down\r\n"
            "from above, grabbing your body, that\r\n"
            "begins to pulse with colored lights\r\n"
            "from inside.\r\n\r\n"
            "Your head seems to be filled with demons\r\n"
            "from another plane as your body dissolves\r\n"
            "to the elements of time and space itself.\r\n"
            "Suddenly a silent explosion of light\r\n"
            "snaps you back to reality.\r\n\r\n"
            "You feel slightly different.", false, ch, NULL, victim, TO_VICT);
    }

    send_to_char(ch, "%s", OK);

    mudlog(MAX(GET_INVIS_LVL(ch), GET_INVIS_LVL(victim)), NRM, true,
           "(GC) %s has advanced %s to level %d (from %d)",
           GET_NAME(ch), GET_NAME(victim), newlevel, GET_LEVEL(victim));
    gain_exp_regardless(victim, exp_scale[newlevel] - GET_EXP(victim));
    crashsave(victim);
}

ACMD(do_restore)
{
    struct creature *vict;

    one_argument(argument, buf);
    if (!*buf) {
        send_to_char(ch, "Whom do you wish to restore?\r\n");
    } else if (!(vict = get_char_vis(ch, buf))) {
        send_to_char(ch, "%s", NOPERSON);
    } else {
        restore_creature(vict);
        send_to_char(ch, "%s", OK);
        act("You have been fully healed by $N!", false, vict, NULL, ch, TO_CHAR);
        mudlog(GET_LEVEL(ch), CMP, true,
               "%s has been restored by %s.", GET_NAME(vict), GET_NAME(ch));
    }
}

void
perform_vis(struct creature *ch)
{

    int level = GET_INVIS_LVL(ch);

    if (!GET_INVIS_LVL(ch) && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
        send_to_char(ch, "You are already fully visible.\r\n");
        return;
    }

    GET_INVIS_LVL(ch) = 0;
    for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch == ch || !can_see_creature(tch, ch)) {
            continue;
        }
        if (GET_LEVEL(tch) < level) {
            act("you suddenly realize that $n is standing beside you.",
                false, ch, NULL, tch, TO_VICT);
        }
    }

    send_to_char(ch, "You are now fully visible.\r\n");
}

void
perform_invis(struct creature *ch, int level)
{
    int old_level;

    if (IS_NPC(ch)) {
        return;
    }

    // We set the invis level to 0 here because of a logic problem with
    // can_see_creature().  If we keep the old level, people won't be able to
    // see people appear, and if we set the new level here, people won't be
    // able to see them disappear.  Setting the invis level to 0 ensures
    // that we can still take invisibility/transparency into account.
    old_level = GET_INVIS_LVL(ch);
    GET_INVIS_LVL(ch) = 0;

    for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (tch == ch || !can_see_creature(tch, ch)) {
            continue;
        }

        if (GET_LEVEL(tch) < old_level && GET_LEVEL(tch) >= level) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
                act("you suddenly realize that $n is standing beside you.",
                    false, ch, NULL, tch, TO_VICT);
            } else if (GET_REMORT_GEN(tch) <= GET_REMORT_GEN(ch)) {
                act("$n suddenly appears from the thin air beside you.",
                    false, ch, NULL, tch, TO_VICT);
            }
        }

        if (GET_LEVEL(tch) >= old_level && GET_LEVEL(tch) < level) {
            if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
                act("You blink and suddenly realize that $n is gone.",
                    false, ch, NULL, tch, TO_VICT);
            } else if (GET_REMORT_GEN(tch) <= GET_REMORT_GEN(ch)) {
                act("$n suddenly vanishes from your reality.",
                    false, ch, NULL, tch, TO_VICT);
            }
        }
    }

    GET_INVIS_LVL(ch) = level;
    send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
    char *arg;
    int level;

    if (IS_NPC(ch)) {
        send_to_char(ch, "You can't do that!\r\n");
        return;
    }

    if (GET_LEVEL(ch) < LVL_AMBASSADOR && !IS_REMORT(ch)) {
        send_unknown_cmd(ch);
        return;
    }

    arg = tmp_getword(&argument);
    if (!*arg) {
        if (GET_INVIS_LVL(ch) > 0) {
            perform_vis(ch);
        } else {
            perform_invis(ch, MIN(GET_LEVEL(ch), 70));
        }
    } else {
        level = MIN(atoi(arg), 70);
        if (level > GET_LEVEL(ch)) {
            send_to_char(ch,
                         "You can't go invisible above your own level.\r\n");
            return;
        }

        if (level < 1) {
            perform_vis(ch);
        } else {
            perform_invis(ch, level);
        }
    }
}

ACMD(do_gecho)
{
    struct descriptor_data *pt;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "That must be a mistake...\r\n");
    } else {
        for (pt = descriptor_list; pt; pt = pt->next) {
            if (pt->input_mode == CXN_PLAYING &&
                pt->creature && pt->creature != ch &&
                !PRF2_FLAGGED(pt->creature, PRF2_NOGECHO) &&
                !PLR_FLAGGED(pt->creature, PLR_WRITING)) {
                if (GET_LEVEL(pt->creature) >= 50 &&
                    GET_LEVEL(pt->creature) > GET_LEVEL(ch)) {
                    send_to_char(pt->creature, "[%s-g] %s\r\n", GET_NAME(ch),
                                 argument);
                } else {
                    send_to_char(pt->creature, "%s\r\n", argument);
                }
            }
        }
        send_to_char(ch, "%s\r\n", argument);
    }
}

ACMD(do_zecho)
{
    struct descriptor_data *pt;
    struct zone_data *here;
    // charmed mobs and players < LVL_GOD cant use this
    if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM)) {
        send_to_char(ch, "You probably shouldn't be using this.\r\n");
    }
    if (ch->in_room && ch->in_room->zone) {
        here = ch->in_room->zone;
    } else {
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "That must be a mistake...\r\n");
    } else {
        for (pt = descriptor_list; pt; pt = pt->next) {
            if (pt->input_mode == CXN_PLAYING &&
                pt->creature && pt->creature != ch &&
                pt->creature->in_room && pt->creature->in_room->zone == here
                && !PRF2_FLAGGED(pt->creature, PRF2_NOGECHO)
                && !PLR_FLAGGED(pt->creature, PLR_WRITING)) {
                if (GET_LEVEL(pt->creature) > GET_LEVEL(ch)) {
                    send_to_char(pt->creature, "[%s-zone] %s\r\n",
                                 GET_NAME(ch), argument);
                } else {
                    send_to_char(pt->creature, "%s\r\n", argument);
                }
            }
        }
        send_to_char(ch, "%s", argument);
    }
}

ACMD(do_oecho)
{

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "That must be a mistake...\r\n");
    } else {
        send_to_char(ch, "%s\r\n", argument);
        send_to_outdoor(argument, 1);
    }
}

ACMD(do_poofset)
{
    char **msg;

    switch (subcmd) {
    case SCMD_POOFIN:
        msg = &(POOFIN(ch));
        break;
    case SCMD_POOFOUT:
        msg = &(POOFOUT(ch));
        break;
    default:
        return;
    }

    skip_spaces(&argument);

    if (*msg) {
        free(*msg);
    }

    if (!*argument) {
        *msg = NULL;
    } else {
        *msg = strdup(argument);
    }
    send_to_char(ch, "%s", OK);
}

ACMD(do_dc)
{
    struct descriptor_data *d;
    int num_to_dc;

    char *arg = tmp_getword(&argument);
    if (!(num_to_dc = atoi(arg))) {
        send_to_char(ch,
                     "Usage: DC <connection number> (type USERS for a list)\r\n");
        return;
    }
    d = descriptor_list;
    while (d && d->desc_num != num_to_dc) {
        d = d->next;
    }

    if (!d) {
        send_to_char(ch, "No such connection.\r\n");
        return;
    }
    if (d->creature && GET_LEVEL(d->creature) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
        return;
    }
    close_socket(d);
    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    slog("(GC) Connection closed by %s.", GET_NAME(ch));
}

ACMD(do_wizcut)
{
    int level;
    struct descriptor_data *d;

    char *arg = tmp_getword(&argument);

    if (*arg) {
        level = atoi(arg);

        if (level <= 0 || level >= LVL_IMMORT) {
            send_to_char(ch, "You can only use wizcut on mortals.\r\n");
            return;
        }

        for (d = descriptor_list; d; d = d->next) {
            if (d->creature && GET_LEVEL(d->creature) <= level) {
                close_socket(d);
            }
        }

        send_to_char(ch,
                     "All players level %d and below have been disconnected.\r\n",
                     level);
        slog("(GC) %s wizcut everyone level %d and below.\r\n",
             GET_NAME(ch), level);
    }
}

ACMD(do_wizlock)
{
    int value;
    const char *when;

    char *arg = tmp_getword(&argument);
    if (*arg) {
        value = atoi(arg);
        if (value < 0 || value > GET_LEVEL(ch)) {
            send_to_char(ch, "Invalid wizlock value.\r\n");
            return;
        }
        restrict_logins = value;
        when = "now";
    } else {
        when = "currently";
    }

    switch (restrict_logins) {
    case 0:
        send_to_char(ch, "The game is %s completely open.\r\n", when);
        break;
    case 1:
        send_to_char(ch, "The game is %s closed to new players.\r\n", when);
        break;
    default:
        send_to_char(ch, "Only level %d and above may enter the game %s.\r\n",
                     restrict_logins, when);
        break;
    }
    mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)), CMP, false,
           "(GC) %s has set wizlock to %d.", GET_NAME(ch), restrict_logins);
}

ACMD(do_date)
{
    char *tmstr;
    time_t mytime;
    int d, h, m;
    extern time_t boot_time;

    if (subcmd == SCMD_DATE) {
        mytime = time(NULL);
    } else {
        mytime = boot_time;
    }

    tmstr = (char *)asctime(localtime(&mytime));
    tmstr[strlen(tmstr) - 1] = '\0';

    if (subcmd == SCMD_DATE) {
        send_to_char(ch, "Current machine time: %s\r\n", tmstr);
    } else {
        mytime = time(NULL) - boot_time;
        d = mytime / 86400;
        h = (mytime / 3600) % 24;
        m = (mytime / 60) % 60;

        send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
                     ((d == 1) ? "" : "s"), h, m);
    }

}

ACMD(do_last)
{
    int pid;
    struct creature *vict;

    char *arg = tmp_getword(&argument);
    if (!*arg) {
        send_to_char(ch, "For whom do you wish to search?\r\n");
        return;
    }
    pid = player_idnum_by_name(arg);
    if (pid <= 0) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
    vict = load_player_from_xml(pid);
    if (!vict) {
        send_to_char(ch, "There was an error.\r\n");
        slog("Unable to load character for 'LAST'.");
        return;
    }
    if ((GET_LEVEL(vict) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_GRIMP)) {
        send_to_char(ch, "You are not sufficiently godly for that!\r\n");
        free_creature(vict);
        return;
    }

    send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-20s",
                 GET_IDNUM(vict), GET_LEVEL(vict),
                 strlist_aref(GET_CLASS(vict), char_class_abbrevs), GET_NAME(vict),
                 ctime(&(vict->player.time.logon)));
    if (GET_LEVEL(ch) >= GET_LEVEL(vict) && has_mail(GET_IDNUM(vict))) {
        send_to_char(ch, "Player has unread mail.\r\n");
    }
    free_creature(vict);
}

ACMD(do_force)
{
    struct descriptor_data *i, *next_desc;
    struct creature *vict;

    char *arg = tmp_getword(&argument);
    char *msg = tmp_sprintf("$n has forced you to '%s'.", argument);

    if (!*arg || !*argument) {
        send_to_char(ch, "Whom do you wish to force do what?\r\n");
        return;
    }

    if (GET_LEVEL(ch) >= LVL_GRGOD) {
        // Check for high-level special arguments
        if (!strcasecmp("all", arg) && is_authorized(ch, FORCE_ALL, NULL)) {
            send_to_char(ch, "%s", OK);
            mudlog(GET_LEVEL(ch), NRM, true,
                   "(GC) %s forced all to %s", GET_NAME(ch), argument);
            for (GList *it = first_living(creatures); it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (ch != tch && GET_LEVEL(ch) > GET_LEVEL(tch)) {
                    act(msg, true, ch, NULL, tch, TO_VICT);
                    command_interpreter(tch, argument);
                }
            }
            return;
        }

        if (!strcasecmp("players", arg)) {
            send_to_char(ch, "%s", OK);
            mudlog(GET_LEVEL(ch), NRM, true,
                   "(GC) %s forced players to %s", GET_NAME(ch), argument);

            for (i = descriptor_list; i; i = next_desc) {
                next_desc = i->next;

                if (STATE(i) || !(vict = i->creature) ||
                    GET_LEVEL(vict) >= GET_LEVEL(ch)) {
                    continue;
                }
                act(msg, true, ch, NULL, vict, TO_VICT);
                command_interpreter(vict, argument);
            }
            return;
        }

        if (!strcasecmp("room", arg)) {
            send_to_char(ch, "%s", OK);
            mudlog(GET_LEVEL(ch), NRM, true, "(GC) %s forced room %d to %s",
                   GET_NAME(ch), ch->in_room->number, argument);
            for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (GET_LEVEL(tch) >= GET_LEVEL(ch) ||
                    (!IS_NPC(tch) && GET_LEVEL(ch) < LVL_GRGOD)) {
                    continue;
                }
                act(msg, true, ch, NULL, tch, TO_VICT);
                command_interpreter(tch, argument);
            }
            return;
        }
    }
    // Normal individual creature forcing
    if (!(vict = get_char_vis(ch, arg))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if (GET_LEVEL(ch) <= GET_LEVEL(vict)) {
        send_to_char(ch, "No, no, no!\r\n");
    } else if (!IS_NPC(vict) && !is_authorized(ch, FORCE_PLAYERS, NULL)) {
        send_to_char(ch, "You cannot force players to do things.\r\n");
    } else {
        send_to_char(ch, "%s", OK);
        act(msg, true, ch, NULL, vict, TO_VICT);
        mudlog(GET_LEVEL(ch), NRM, true, "(GC) %s forced %s to %s",
               GET_NAME(ch), GET_NAME(vict), argument);
        command_interpreter(vict, argument);
    }
}

ACMD(do_wiznet)
{
    struct descriptor_data *d;
    char emote = false;
    int level = LVL_AMBASSADOR;
    const char *subcmd_str = NULL;
    const char *subcmd_desc = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (subcmd == SCMD_IMMCHAT) {
            send_to_char(ch,
                         "Usage: imm <text> | #<level> <text> | *<emotetext> |\r\n ");
        } else {
            send_to_char(ch,
                         "Usage: wiz <text> | #<level> <text> | *<emotetext> |\r\n");
        }
        return;
    }

    if (subcmd == SCMD_WIZNET) {
        level = LVL_IMMORT;
    }

    switch (*argument) {
    case '*':
        emote = true;
        argument++;
    /* fallthrough */
    case '#':
        one_argument(argument + 1, buf1);
        if (is_number(buf1)) {
            half_chop(argument + 1, buf1, argument);
            level = MAX(atoi(buf1), LVL_AMBASSADOR);
            if (level > GET_LEVEL(ch)) {
                send_to_char(ch, "You can't %s above your own level.\r\n",
                             subcmd == SCMD_IMMCHAT ? "immchat" : "wiznet");
                return;
            }
        }
        break;
    case '\\':
        argument++;
        break;
    default:
        break;
    }
    if ((subcmd == SCMD_WIZNET && PRF_FLAGGED(ch, PRF_NOWIZ)) ||
        (subcmd == SCMD_IMMCHAT && PRF2_FLAGGED(ch, PRF2_NOIMMCHAT))) {
        send_to_char(ch, "You are offline!\r\n");
        return;
    }
    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Don't bother the gods like that!\r\n");
        return;
    }

    switch (subcmd) {
    case SCMD_IMMCHAT:
        subcmd_str = "imm";
        subcmd_desc = " imms";
        break;
    case SCMD_WIZNET:
        subcmd_str = "wiz";
        subcmd_desc = "";
        break;
    }

    if ((subcmd == SCMD_IMMCHAT && level > LVL_AMBASSADOR) ||
        (subcmd == SCMD_WIZNET && level > LVL_IMMORT)) {
        snprintf(buf1, sizeof(buf1), "%s%s: <%d> %s\r\n", GET_NAME(ch),
                 subcmd_desc, level, argument);
        snprintf(buf2, sizeof(buf2), "Someone%s: <%d> %s\r\n", subcmd_desc, level, argument);
    } else {
        if (emote) {
            snprintf(buf1, sizeof(buf1), "<%s> %s%s%s\r\n",
                     subcmd_str, GET_NAME(ch),
                     (*argument != '\'') ? " " : "", argument);
            snprintf(buf2, sizeof(buf2), "<%s> Someone%s%s\r\n", subcmd_str,
                     (*argument != '\'') ? " " : "", argument);
        } else {
            snprintf(buf1, sizeof(buf1), "%s%s: %s\r\n", GET_NAME(ch), subcmd_desc, argument);
            snprintf(buf2, sizeof(buf2), "Someone%s: %s\r\n", subcmd_desc, argument);
        }
    }

    for (d = descriptor_list; d; d = d->next) {
        if ((STATE(d) == CXN_PLAYING) && (GET_LEVEL(d->creature) >= level) &&
            (subcmd != SCMD_WIZNET
             || is_named_role_member(d->creature, "WizardBasic"))
            && (subcmd != SCMD_WIZNET || !PRF_FLAGGED(d->creature, PRF_NOWIZ))
            && (subcmd != SCMD_IMMCHAT
                || !PRF2_FLAGGED(d->creature, PRF2_NOIMMCHAT))
            && (!PLR_FLAGGED(d->creature, PLR_WRITING))) {

            if (subcmd == SCMD_IMMCHAT) {
                send_to_char(d->creature, "%s", CCYEL(d->creature, C_SPR));
            } else {
                send_to_char(d->creature, "%s", CCCYN(d->creature, C_SPR));
            }
            if (can_see_creature(d->creature, ch)) {
                send_to_char(d->creature, "%s", buf1);
            } else {
                send_to_char(d->creature, "%s", buf2);
            }

            send_to_char(d->creature, "%s", CCNRM(d->creature, C_SPR));
        }
    }
}

ACMD(do_zreset)
{
    void reset_zone(struct zone_data *zone);
    struct zone_data *zone;

    char *arg = tmp_getword(&argument);
    if (!*arg) {
        send_to_char(ch, "You must specify a zone.\r\n");
        return;
    }
    if (*arg == '*') {
        for (zone = zone_table; zone; zone = zone->next) {
            reset_zone(zone);
        }

        send_to_char(ch, "Reset world.\r\n");
        mudlog(MAX(LVL_GRGOD, GET_INVIS_LVL(ch)), NRM, true,
               "(GC) %s reset entire world.", GET_NAME(ch));
        return;
    } else if (*arg == '.') {
        zone = ch->in_room->zone;
    } else {
        zone = real_zone(atoi(arg));
    }

    if (!zone) {
        send_to_char(ch, "Invalid zone number.\r\n");
    }

    reset_zone(zone);
    send_to_char(ch, "Reset zone %d : %s.\r\n", zone->number, zone->name);
    act("$n waves $s hand.", false, ch, NULL, NULL, TO_ROOM);
    send_to_zone("You feel a strangely refreshing breeze.\r\n", zone, 0);
    slog("(GC) %s %sreset zone %d (%s)", GET_NAME(ch),
         subcmd == SCMD_OLC ? "olc-" : "", zone->number, zone->name);
}

void
perform_unaffect(struct creature *ch, struct creature *vict)
{
    if (vict->affected) {
        send_to_char(vict, "There is a brief flash of light!\r\n"
                     "You feel slightly different.\r\n");
        send_to_char(ch, "All spells removed.\r\n");
        while (vict->affected) {
            affect_remove(vict, vict->affected);
        }
    } else if (ch == vict) {
        send_to_char(ch, "You don't have any affections!\r\n");
    } else {
        send_to_char(ch, "Your victim does not have any affections!\r\n");
    }
}

void
perform_reroll(struct creature *ch, struct creature *vict)
{
    void roll_real_abils(struct creature *ch);

    roll_real_abils(vict);
    send_to_char(vict, "Your stats have been rerolled.\r\n");
    slog("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
    send_to_char(ch,
                 "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                 GET_STR(vict), GET_INT(vict), GET_WIS(vict),
                 GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
    struct creature *vict;
    long result;
    char *msg;
    char *arg = tmp_getword(&argument);
    bool loaded = false;

    // Make sure they entered something useful
    if (!*arg) {
        send_to_char(ch, "Yes, but for whom?!?\r\n");
        return;
    }
    // Make sure they specified a valid player name
    if (!player_name_exists(arg)) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }
    // Get the player or load it from file
    vict = get_char_vis(ch, arg);
    if (!vict) {
        vict = load_player_from_xml(player_idnum_by_name(arg));
        if (!vict) {
            send_to_char(ch, "That player could not be loaded.\r\n");
            return;
        }
        loaded = true;
    }

    if (IS_NPC(vict)) {
        send_to_char(ch, "You can't do that to a mob!\r\n");
    } else if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
        send_to_char(ch, "Hmmm...you'd better not.\r\n");
    } else {
        switch (subcmd) {
        case SCMD_REROLL:
            perform_reroll(ch, vict);
            break;
        case SCMD_NOTITLE: {
            result = PLR_TOG_CHK(vict, PLR_NOTITLE);
            msg = tmp_sprintf("Notitle %s for %s by %s.",
                              ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            send_to_char(ch, "%s\r\n", msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s",
                   msg);
            break;
        }
        case SCMD_NOPOST: {
            result = PLR_TOG_CHK(vict, PLR_NOPOST);
            msg = tmp_sprintf("Nopost %s for %s by %s.",
                              ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            send_to_char(ch, "%s\r\n", msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s",
                   msg);
            break;
        }
        case SCMD_SQUELCH: {
            result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
            msg = tmp_sprintf("Squelch %s for %s by %s.",
                              ONOFF(result), GET_NAME(vict), GET_NAME(ch));
            send_to_char(ch, "%s\r\n", msg);
            mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true, "(GC) %s",
                   msg);
            break;
        }
        case SCMD_FREEZE:
            do_freeze_char(argument, vict, ch);
            break;

        case SCMD_THAW:
            if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
                send_to_char(ch,
                             "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
                break;
            }
            if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
                snprintf(buf, sizeof(buf),
                         "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
                         GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
                send_to_char(ch, "%s", buf);
                break;
            }
            mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true,
                   "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
            REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
            send_to_char(vict,
                         "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
            send_to_char(ch, "Thawed.\r\n");
            if (vict->in_room) {
                act("A sudden fireball conjured from nowhere thaws $n!", false,
                    vict, NULL, NULL, TO_ROOM);
            }
            break;
        case SCMD_UNAFFECT:
            perform_unaffect(ch, vict);
            break;
        default:
            errlog("Unknown subcmd passed to do_wizutil (act.wizard.c)");
            break;
        }
        crashsave(vict);
    }

    if (loaded) {
        free_creature(vict);
    }
}

#define PRAC_TYPE        3      /* should it say 'spell' or 'skill'? */


bool
should_display_skill(struct creature *vict, int i)
{
    if (CHECK_SKILL(vict, i) != 0) {
        return true;
    }
    if (GET_CLASS(vict) >= NUM_CLASSES) {
        return false;
    }
    if (GET_LEVEL(vict) >= spell_info[i].min_level[(int)GET_CLASS(vict)]) {
        return true;
    }
    if (GET_REMORT_CLASS(vict) == 0) {
        return false;
    }
    return (GET_LEVEL(vict) >= spell_info[i].min_level[(int)GET_REMORT_CLASS(vict)]);
}

void
list_skills_to_char(struct creature *ch, struct creature *vict)
{
    char buf3[MAX_STRING_LENGTH];
    int i, sortpos;

    if (prac_params[PRAC_TYPE][(int)GET_CLASS(vict)] != 1 ||
        (GET_REMORT_CLASS(vict) >= 0 &&
         prac_params[PRAC_TYPE][(int)GET_REMORT_CLASS(vict)] != 1)) {
        snprintf_cat(buf, sizeof(buf), "%s%s%s knows of the following %ss:%s\r\n",
                     CCYEL(ch, C_CMP), PERS(vict, ch), CCBLD(ch, C_SPR),
                     SPLSKL(vict), CCNRM(ch, C_SPR));

        strcpy_s(buf2, sizeof(buf2), buf);

        for (sortpos = 1; sortpos < MAX_SPELLS; sortpos++) {
            i = skill_sort_info[sortpos];
            if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
                strcat_s(buf2, sizeof(buf2), "**OVERFLOW**\r\n");
                break;
            }
            snprintf(buf3, sizeof(buf3), "%s[%3d]", CCYEL(ch, C_NRM), GET_SKILL(vict, i));
            if (should_display_skill(vict, i)) {
                snprintf(buf, sizeof(buf), "%s%3d.%-20s %s%-17s%s %s(%3d mana)%s\r\n",
                         CCGRN(ch, C_NRM), i, spell_to_str(i), CCBLD(ch, C_SPR),
                         how_good(GET_SKILL(vict, i)),
                         GET_LEVEL(ch) > LVL_ETERNAL ? buf3 : "", CCRED(ch, C_SPR),
                         mag_manacost(vict, i), CCNRM(ch, C_SPR));
                strcat_s(buf2, sizeof(buf2), buf);
            }
        }
        snprintf(buf3, sizeof(buf3), "\r\n%s%s knows of the following skills:%s\r\n",
                 CCYEL(ch, C_CMP), PERS(vict, ch), CCNRM(ch, C_SPR));
        strcat_s(buf2, sizeof(buf2), buf3);
    } else {
        snprintf_cat(buf, sizeof(buf), "%s%s knows of the following skills:%s\r\n",
                 CCYEL(ch, C_CMP), PERS(vict, ch), CCNRM(ch, C_SPR));
        strcpy_s(buf2, sizeof(buf2), buf);
    }

    for (sortpos = 1; sortpos < MAX_SKILLS - MAX_SPELLS; sortpos++) {
        i = skill_sort_info[sortpos];
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
            strcat_s(buf2, sizeof(buf2), "**OVERFLOW**\r\n");
            break;
        }
        snprintf(buf3, sizeof(buf3), "%s[%3d]%s",
                 CCYEL(ch, C_NRM), GET_SKILL(vict, i), CCNRM(ch, NRM));
        if (should_display_skill(vict, i)) {
            snprintf(buf, sizeof(buf), "%s%3d.%-20s %s%-17s%s%s\r\n",
                     CCGRN(ch, C_NRM), i, spell_to_str(i), CCBLD(ch, C_SPR),
                     how_good(GET_SKILL(vict, i)),
                     GET_LEVEL(ch) > LVL_ETERNAL ? buf3 : "",
                     CCNRM(ch, C_SPR));
            strcat_s(buf2, sizeof(buf2), buf);
        }
    }
    page_string(ch->desc, buf2);
}

void
do_show_stats(struct creature *ch)
{
    int i = 0, j = 0, k = 0, con = 0, tr_count = 0, srch_count = 0;
    short int num_active_zones = 0;
    struct obj_data *obj;
    struct room_data *room;
    struct room_trail_data *trail;
    struct special_search_data *srch;
    struct zone_data *zone;
    extern int buf_switches, buf_largecount, buf_overflows;

    for (GList *it = first_living(creatures); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (IS_NPC(tch)) {
            j++;
        } else if (can_see_creature(ch, tch)) {
            i++;
            if (tch->desc) {
                con++;
            }
        }
    }
    for (obj = object_list; obj; obj = obj->next) {
        k++;
    }

    for (tr_count = 0, zone = zone_table; zone; zone = zone->next) {
        if (zone->idle_time < ZONE_IDLE_TIME) {
            num_active_zones++;
        }

        for (room = zone->world; room; room = room->next) {
            trail = room->trail;
            while (trail) {
                ++tr_count;
                trail = trail->next;
            }
            srch = room->search;
            while (srch) {
                ++srch_count;
                srch = srch->next;
            }
        }
    }

    send_to_char(ch, "Current statistics of Tempus:\r\n");
    send_to_char(ch, "  %5d players in game  %5d connected\r\n", i, con);
    send_to_char(ch, "  %5zd accounts cached  %5zd characters\r\n",
                 account_cache_size(), player_count());
    send_to_char(ch, "  %5d mobiles          %5d prototypes (%d id'd)\r\n",
                 j, g_hash_table_size(mob_prototypes), current_mob_idnum);
    send_to_char(ch, "  %5d objects          %5d prototypes\r\n",
                 k, g_hash_table_size(obj_prototypes));
    send_to_char(ch, "  %5d rooms            %5d zones (%d active)\r\n",
                 top_of_world + 1, top_of_zone_table, num_active_zones);
    send_to_char(ch, "  %5d searches\r\n", srch_count);
    send_to_char(ch, "  %5d large bufs\r\n", buf_largecount);
    send_to_char(ch, "  %5d buf switches     %5d overflows\r\n",
                 buf_switches, buf_overflows);
    send_to_char(ch, "  %5zd tmpstr space\r\n",
                 tmp_max_used);
#ifdef MEMTRACK
    send_to_char(ch, "  %5ld trail count      %ldMB total memory\r\n",
                 tr_count, dbg_memory_used() / (1024 * 1024));
#else
    send_to_char(ch, "  %5d trail count\r\n", tr_count);
#endif
    send_to_char(ch, "  %5zu running progs (%zu total, %zu free)\r\n",
                 prog_count(false), prog_count(true), free_prog_count());
    send_to_char(ch, "  Lunar day: %2d, phase: %s (%d)\r\n",
                 lunar_day, lunar_phases[get_lunar_phase(lunar_day)],
                 get_lunar_phase(lunar_day));

    if (GET_LEVEL(ch) >= LVL_LOGALL && log_cmds) {
        send_to_char(ch, "  Logging all commands _ file is %s.\r\n",
                     1 ? "OPEN" : "NULL");
    }
    if (shutdown_count >= 0) {
        send_to_char(ch,
                     "  Shutdown sequence ACTIVE.  Shutdown %s in [%d] seconds.\r\n",
                     shutdown_mode == SHUTDOWN_DIE ? "DIE" :
                     shutdown_mode == SHUTDOWN_PAUSE ? "PAUSE" : "REBOOT",
                     shutdown_count);
    }
}

void
show_wizcommands(struct creature *ch)
{
    if (IS_PC(ch)) {
        send_available_commands(ch, GET_IDNUM(ch));
    } else {
        send_to_char(ch, "You are a mobile. Deal with it.\r\n");
    }
}

void
show_account(struct creature *ch, char *value)
{
    char created_buf[30];
    char last_buf[30];
    int idnum = 0;
    struct account *account = NULL;
    time_t last, creation;

    if (!*value) {
        send_to_char(ch, "A name or id would help.\r\n");
        return;
    }

    if (*value == '.') {
        value++;
        if (is_number(value)) {
            idnum = atoi(value);
            if (!player_idnum_exists(idnum)) {
                send_to_char(ch, "There is no such player: %s\r\n", value);
                return;
            }
            idnum = player_account_by_idnum(atoi(value));
        } else {
            if (!player_name_exists(value)) {
                send_to_char(ch, "There is no such player: %s\r\n", value);
                return;
            }
            idnum = player_account_by_name(value);
        }

        account = account_by_idnum(idnum);
    } else {
        if (is_number(value)) {
            idnum = atoi(value);
            account = account_by_idnum(idnum);
        } else {
            account = account_by_name(value);
        }
    }

    if (account == NULL) {
        send_to_char(ch, "There is no such account: '%s'\r\n", value);
        return;
    }

    d_printf(ch->desc, "&y  Account: &n%s [%d]", account->name,
             account->id);
    if (account->email && *account->email) {
        d_printf(ch->desc, " &c<%s>&n", account->email);
    }
    struct house *h = find_house_by_owner(account->id);
    if (h) {
        d_printf(ch->desc, " &y House: &n%d", h->id);
    }
    if (account->banned) {
        d_printf(ch->desc, " &y(BANNED)&n");
    } else if (account->quest_banned) {
        d_printf(ch->desc, " &y(QBANNED)&n");
    }
    d_printf(ch->desc, "\r\n");
    d_printf(ch->desc, "    &yTrust: &n%d\r\n", account->trust);

    last = account->login_time;
    creation = account->creation_time;

    strftime(created_buf, 29, "%a %b %d, %Y %H:%M:%S", localtime(&creation));
    strftime(last_buf, 29, "%a %b %d, %Y %H:%M:%S", localtime(&last));
    d_printf(ch->desc, "&y  Started: &n%s   &yLast login: &n%s\r\n",
             created_buf, last_buf);
    if (is_named_role_member(ch, "AdminFull")) {
        d_printf(ch->desc,
                 "&y  Created: &n%-15s   &yLast: &n%-15s       &yReputation: &n%d\r\n",
                 account->creation_addr, account->login_addr, account->reputation);
    }
    d_printf(ch->desc,
             "&y  Past bank: &n%'-12" PRId64 "    &yFuture Bank: &n%'-12" PRId64,
             account->bank_past, account->bank_future);
    if (is_named_role_member(ch, "Questor")) {
        d_printf(ch->desc, "   &yQuest Points: &n%d\r\n",
                 account->quest_points);
    } else {
        d_printf(ch->desc, "\r\n");
    }
    d_printf(ch->desc,
             "&b ----------------------------------------------------------------------------&n\r\n");

    show_account_chars(ch->desc, account, true, false);
}

void
show_player(struct creature *ch, const char *value)
{
    struct creature *vict;
    char birth[80];
    char last_login[80];
    char remort_desc[80];
    long idnum = 0;

    if (!*value) {
        send_to_char(ch, "A name would help.\r\n");
        return;
    }

    /* added functionality for show player by idnum */
    if (is_number(value) && player_name_by_idnum(atoi(value))) {
        value = player_name_by_idnum(atoi(value));
    }
    if (!player_name_exists(value)) {
        send_to_char(ch, "There is no such player.\r\n");
        return;
    }

    idnum = player_idnum_by_name(value);
    vict = load_player_from_xml(idnum);
    vict->account = account_by_idnum(player_account_by_idnum(idnum));

    if (GET_REMORT_GEN(vict) <= 0) {
        remort_desc[0] = '\0';
    } else {
        snprintf(remort_desc, sizeof(remort_desc), "/%s",
                 char_class_abbrevs[(int)GET_REMORT_CLASS(vict)]);
    }
    snprintf(buf, sizeof(buf), "Player: [%ld] %-12s Act[%ld] (%s) [%2d %s %s%s]  Gen: %d\r\n",
             GET_IDNUM(vict), GET_NAME(vict),
             player_account_by_idnum(GET_IDNUM(vict)),
             genders[(int)GET_SEX(vict)], GET_LEVEL(vict),
             race_name_by_idnum(GET_RACE(vict)), char_class_abbrevs[GET_CLASS(vict)],
             remort_desc, GET_REMORT_GEN(vict));
    snprintf_cat(buf, sizeof(buf),
                 "Au: %'-8" PRId64 "  Cr: %'-8" PRId64 "  Past: %'-8" PRId64 "  Fut: %'-8" PRId64 "\r\n",
                 GET_GOLD(vict), GET_CASH(vict),
                 GET_PAST_BANK(vict), GET_FUTURE_BANK(vict));
    // Trim and fit the date to show year but not seconds.
    strftime(birth, sizeof(birth), "%Y-%m-%d %H:%M",
             localtime(&vict->player.time.birth));
    if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
        strcpy_s(last_login, sizeof(last_login), "Unknown");
    } else {
        // Trim and fit the date to show year but not seconds.
        strftime(last_login, sizeof(last_login), "%Y-%m-%d %H:%M",
                 localtime(&vict->player.time.logon));
    }
    snprintf_cat(buf, sizeof(buf),
                 "Started: %-17s Last: %-17s Played: %dh %dm\r\n",
                 birth, last_login, (int)(vict->player.time.played / 3600),
                 (int)(vict->player.time.played / 60 % 60));

    if (IS_SET(vict->char_specials.saved.act, PLR_FROZEN)) {
        snprintf_cat(buf, sizeof(buf), "%s%s is FROZEN!%s\r\n", CCCYN(ch, C_NRM),
                     GET_NAME(vict), CCNRM(ch, C_NRM));
    }

    if (IS_SET(vict->player_specials->saved.plr2_bits, PLR2_BURIED)) {
        snprintf_cat(buf, sizeof(buf), "%s%s is BURIED!%s\r\n", CCGRN(ch, C_NRM),
                     GET_NAME(vict), CCNRM(ch, C_NRM));
    }

    send_to_char(ch, "%s", buf);

    free_creature(vict);
}

void
show_zoneusage(struct creature *ch, char *value)
{
    int i;
    struct zone_data *zone;

    if (!*value) {
        i = -1;
    } else if (is_abbrev(value, "active")) {
        i = -2;
    } else {
        i = atoi(value);
    }
    snprintf(buf, sizeof(buf), "ZONE USAGE RECORD:\r\n");
    for (zone = zone_table; zone; zone = zone->next) {
        if ((i >= 0 && i != zone->number) || (i == -2 && !zone->num_players)) {
            continue;
        }
        snprintf(buf, sizeof(buf),
                 "%s[%s%3d%s] %s%-30s%s %s[%s%6d%s]%s acc, [%3d] idle  Owner: %s%s%s\r\n",
                 buf, CCYEL(ch, C_NRM), zone->number, CCNRM(ch, C_NRM), CCCYN(ch,
                                                                              C_NRM), zone->name, CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
                 CCNRM(ch, C_NRM), zone->enter_count, CCGRN(ch, C_NRM), CCNRM(ch,
                                                                              C_NRM), zone->idle_time, CCYEL(ch, C_NRM),
                 player_name_by_idnum(zone->owner_idnum), CCNRM(ch, C_NRM));
    }

    page_string(ch->desc, buf);
}

gint
top_zone_comparator(const struct zone_data *a, const struct zone_data *b)
{
    if (a->enter_count < b->enter_count) {
        return -1;
    }
    if (a->enter_count > b->enter_count) {
        return 1;
    }
    return 0;
}

void
show_topzones(struct creature *ch, char *value)
{
    int i, num_zones = 0, num = 1;
    struct zone_data *zone;
    GList *zone_list = NULL;

    char *temp = NULL;
    char *lower = tmp_tolower(value);
    int lessThan = INT_MAX, greaterThan = -1;
    bool reverse = false;

    if (ch->desc == NULL) {
        return;
    }

    temp = strstr(lower, "limit ");
    if (temp && strlen(temp) > 6) {
        num_zones = atoi(temp + 6);
    }
    temp = NULL;
    if (!num_zones) {
        num_zones = ch->desc->account->term_height - 3;
    }

    temp = strstr(lower, ">");
    if (temp && strlen(temp) > 1) {
        greaterThan = atoi(temp + 1);
    }
    temp = NULL;
    temp = strstr(lower, "<");
    if (temp && strlen(temp) > 1) {
        lessThan = atoi(temp + 1);
    }
    temp = NULL;

    temp = strstr(lower, "reverse");
    if (temp) {
        reverse = true;
    }
    temp = NULL;

    for (i = 0, zone = zone_table; i < top_of_zone_table;
         zone = zone->next, i++) {
        if (zone->enter_count > greaterThan && zone->enter_count < lessThan
            && zone->number < 700) {
            zone_list = g_list_prepend(zone_list, zone);
        }
    }

    zone_list = g_list_sort(zone_list, (GCompareFunc) top_zone_comparator);
    if (reverse) {
        zone_list = g_list_reverse(zone_list);
    }

    num_zones = MIN((int)g_list_length(zone_list), num_zones);

    struct str_builder sb = str_builder_default;

    sb_sprintf(&sb, "Usage Options: [limit #] [>#] [<#] [reverse]\r\nTOP %d zones:\r\n",
                num_zones);

    for (GList *i = zone_list; i; num++, i = i->next) {
        struct zone_data *zone = i->data;

        sb_sprintf(&sb, "%2d.[%s%3d%s] %s%-30s %s[%s%6d%s]%s accesses.  Owner: %s%s%s\r\n",
                    num, CCYEL(ch, C_NRM),
                    zone->number, CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
                    zone->name, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                    zone->enter_count, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
                    player_name_by_idnum(zone->owner_idnum), CCNRM(ch, C_NRM));
    }

    g_list_free(zone_list);

    page_string(ch->desc, sb.str);

}

void
show_mobkills(struct creature *ch, char *value, char *arg
              __attribute__ ((unused)))
{
    float ratio;
    float thresh;
    int i = 0;

    if (!*value) {
        send_to_char(ch, "Usage: show mobkills <ratio>\r\n"
                     "<ratio> should be between 0.00 and 1.00\r\n");
        return;
    }

    thresh = atof(value);

    struct str_builder sb = str_builder_default;

    sb_sprintf(&sb, "Mobiles with mobkills ratio >= %f:\r\n", thresh);
    sb_strcat(&sb, " ---- -Vnum-- -Name------------------------- -Kills- -Loaded- -Ratio-\r\n",
        NULL);
    GHashTableIter iter;
    gpointer key, val;

    g_hash_table_iter_init(&iter, mob_prototypes);

    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct creature *mob = val;

        if (!mob->mob_specials.shared->loaded) {
            return;
        }
        ratio = (float)((float)mob->mob_specials.shared->kills /
                        (float)mob->mob_specials.shared->loaded);
        if (ratio >= thresh) {
            sb_sprintf(&sb, " %3d. [%5d]  %-29s %6d %8d    %.2f\r\n",
                        ++i, GET_NPC_VNUM(mob), GET_NAME(mob),
                        mob->mob_specials.shared->kills,
                        mob->mob_specials.shared->loaded, ratio);
        }
    }

    page_string(ch->desc, sb.str);
}

const char *show_room_flags[] = {
    "rflags",
    "searches",
    "title",
    "rexdescs",
    "noindent",
    "sounds",
    "occupancy",
    "noexits",
    "mobcount",
    "mobload",
    "deathcount",
    "sector",
    "desc_length",
    "orphan_words",
    "period_space",
    "exits",
    "\n"
};

const char *show_room_modes[] = {
    "zone",
    "time",
    "plane",
    "world",
    "\n"
};

void
build_show_room(struct creature *ch, struct room_data *room, int mode,
                const char *extra, struct str_builder *sb)
{
    if (!extra) {
        extra = "";
    }

    if (mode != 0) {
        sb_sprintf(sb, "%s%s%3d %s%s%-30s %s%5d %s%-30s %s%s\r\n",
                    CCBLD(ch, C_CMP), CCYEL(ch, C_NRM), room->zone->number,
                    CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), room->zone->name,
                    CCYEL(ch, C_NRM), room->number,
                    CCCYN(ch, C_NRM), room->name, CCNRM(ch, C_NRM), extra);
    } else {
        sb_sprintf(sb, "%s#%d %s%-40s %s%s\r\n",
                    CCYEL(ch, C_NRM), room->number,
                    CCCYN(ch, C_NRM), room->name, CCNRM(ch, C_NRM), extra);
    }
}

// returns 0 if none found, 1 if found, -1 if error
int
show_rooms_in_zone(struct str_builder *sb, struct creature *ch, struct zone_data *zone, int pos,
                   int mode, char *args)
{
    struct special_search_data *srch = NULL;
    struct room_data *room;
    bool match, gt = false, lt = false;
    int found = 0, num, flags = 0, tmp_flag = 0;
    char *arg;
    GList *str_list = NULL, *str_it = NULL, *mob_names = NULL;

    arg = tmp_getword(&args);

    switch (pos) {
    case 0:                    // rflags
        if (!*arg) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> rflags [flag flag ...]\n");
            return -1;
        }

        while (*arg) {
            tmp_flag = search_block(arg, roomflag_names, 0);
            if (tmp_flag == -1) {
                send_to_char(ch, "Invalid flag %s, skipping...\n", arg);
            } else {
                flags |= (1U << tmp_flag);
            }
            arg = tmp_getword(&args);
        }

        for (room = zone->world; room; room = room->next) {
            if ((room->room_flags & flags) == flags) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;

    case 1:                    // searches
        while (*arg) {
            tmp_flag = search_block(arg, search_bits, 0);
            if (tmp_flag == -1) {
                send_to_char(ch, "Invalid flag %s, skipping...\n", arg);
            } else {
                flags |= (1U << tmp_flag);
            }
            arg = tmp_getword(&args);
        }

        for (room = zone->world; room; room = room->next) {
            for (srch = room->search; srch != NULL; srch = srch->next) {
                if ((srch->flags & flags) == flags) {
                    build_show_room(ch, room, mode,
                                     tmp_sprintf("[%-6s]",
                                                 search_cmd_short[(int)srch->command]), sb);
                    found = 1;
                }
            }
        }
        break;

    case 2:                    // title
        if (!*arg) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> title [keyword keyword ...]\n");
            return -1;
        }

        while (*arg) {
            str_list = g_list_prepend(str_list, arg);
            arg = tmp_getword(&args);
        }

        for (room = zone->world; room; room = room->next) {
            match = true;
            for (str_it = str_list; str_it; str_it = str_it->next) {
                if (room->name && !isname((char *)str_it->data, room->name)) {
                    match = false;
                }
            }

            if (match) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        g_list_free(str_list);
        break;
    case 3:                    // rexdescs
        for (room = zone->world; room; room = room->next) {
            if (room->ex_description != NULL) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 4:                    // noindent
        for (room = zone->world; room; room = room->next) {
            if (!room->description) {
                continue;
            }

            if (strncmp(room->description, "   ", 3) != 0) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 5:                    // sounds
        for (room = zone->world; room; room = room->next) {
            if (room->sounds != NULL) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 6:                    // occupancy
        if (!*arg) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }

        if (*arg == '<') {
            lt = true;
        } else if (*arg == '>') {
            gt = true;
        } else {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }

        arg = tmp_getword(&args);
        if (!*arg || !is_number(arg)) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> occupancy < < | >  # > \n");
            return -1;
        }
        num = atoi(arg);

        for (room = zone->world; room; room = room->next) {
            if ((gt && (room->max_occupancy > num))
                || (lt && (room->max_occupancy < num))) {
                build_show_room(ch, room, mode,
                                tmp_sprintf("[%2d]", room->max_occupancy), sb);
                found = 1;
            }
        }
        break;
    case 7:                    // noexits
        for (room = zone->world; room; room = room->next) {
            if (!count_room_exits(room)) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 8:                    // mobcount
        if (mode == 3) {
            // No listing all mobs in world
            send_to_char(ch, "No way!  Are you losing it?\n");
            return -1;
        }

        if (!*arg || !is_number(arg)) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> mobcount [mobcount]\n");
            return -1;
        }

        num = atoi(arg);
        for (room = zone->world; room; room = room->next) {
            for (GList *cit = first_living(room->people); cit; cit = next_living(cit)) {
                struct creature *tch = cit->data;
                if (!IS_NPC(tch)) {
                    continue;
                }
                mob_names = g_list_prepend(mob_names, tch->player.short_descr);
            }

            if (g_list_length(mob_names) >= (unsigned int)num) {
                build_show_room(ch, room, mode,
                                tmp_sprintf("[%2d]", g_list_length(mob_names)), sb);
                found = 1;
                for (str_it = mob_names; str_it; str_it = str_it->next) {
                    sb_sprintf(sb, "\t%s%s%s\r\n", CCYEL(ch, C_NRM),
                                (char *)str_it->data, CCNRM(ch, C_NRM));
                }
            }
            g_list_free(mob_names);
            mob_names = NULL;
        }
        break;
    case 9:                    // mobload
        send_to_char(ch, "disabled.\n");
        break;
    case 10:                   // deathcount
        send_to_char(ch, "disabled.\n");
        break;
    case 11:                   // sector
        if (!*arg) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> sector <sector_type>\n");
            return -1;
        }

        struct sector *sector = sector_by_name(arg, 0);
        if (!sector) {
            send_to_char(ch, "No such sector type.  Type olc h rsect.\r\n");
            return -1;
        }

        for (room = zone->world; room; room = room->next) {
            if (room->sector_type == sector->idnum) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 12:                   // desc_length
        if (!*arg) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }

        if (*arg == '<') {
            lt = true;
        } else if (*arg == '>') {
            gt = true;
        } else {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }

        arg = tmp_getword(&args);
        if (!*arg || !is_number(arg)) {
            send_to_char(ch,
                         "Usage: show rooms <world | zone> desc_length < < | >  # > \n");
            return -1;
        }
        num = atoi(arg);

        for (room = zone->world; room; room = room->next) {
            if (room->description
                && ((gt && ((unsigned)num <= strlen(room->description)))
                    || (lt && ((unsigned)num >= strlen(room->description))))) {
                build_show_room(ch, room, mode,
                                tmp_sprintf("[%zd]", strlen(room->description)), sb);
                found = 1;
            }
        }
        break;
    case 13:                   // orphan_words
        for (room = zone->world; room; room = room->next) {
            if (!room->description) {
                continue;
            }

            char *desc = room->description;
            // find the last line of the desc
            arg = desc + strlen(desc) - 3;
            // skip trailing space
            while (arg > desc && *arg == ' ') {
                arg--;
            }
            // search for spaces while decrementing to the beginning of the line
            while (arg > desc && *arg != '\n' && *arg != ' ') {
                arg--;
            }
            // we don't worry about one-line descs
            if (arg == desc) {
                continue;
            }
            // check for a space - if one doesn't exist, we have an orphaned word.
            if (*arg != ' ') {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 14:                   // period_space
        for (room = zone->world; room; room = room->next) {
            if (!room->description) {
                continue;
            }
            arg = room->description;
            match = false;
            while (*arg && !match) {
                // Find sentence terminating punctuation
                arg = strpbrk(arg, ".?!");
                if (!arg) {
                    break;
                }
                // Skip past multiple punctuation
                arg += strspn(arg, ".?!");
                // Count spaces
                int count = strspn(arg, " ");
                // Ensure that either there were two spaces after the punctuation, or
                // any spaces were trailing space
                arg += count;
                if (*arg && !isspace(*arg) && count != 2) {
                    match = true;
                }
            }
            if (match) {
                build_show_room(ch, room, mode, NULL, sb);
                found = 1;
            }
        }
        break;
    case 15:                   // exits
        for (room = zone->world; room; room = room->next) {
            for (int dir = 0; dir < NUM_DIRS; dir++) {
                struct room_direction_data *exit = ABS_EXIT(room, dir);
                if (exit) {
                    if (IS_SET(exit->exit_info,
                               EX_CLOSED | EX_LOCKED | EX_HARD_PICK | EX_PICKPROOF
                               | EX_HEAVY_DOOR | EX_TECH | EX_REINFORCED |
                               EX_SPECIAL)
                        && !IS_SET(exit->exit_info, EX_ISDOOR)) {
                        build_show_room(ch, room, mode,
                                         tmp_sprintf("doorflags on non-door for %s exit",
                                                     dirs[dir]), sb);
                        found = 1;
                    }
                    if (IS_SET(exit->exit_info, EX_CLOSED | EX_LOCKED)
                        && !exit->keyword) {
                        build_show_room(ch, room, mode,
                                         tmp_sprintf("no keyword for %s exit", dirs[dir]), sb);
                        found = 1;
                    }
                    if (exit->key && exit->key != -1
                        && !real_object_proto(exit->key)) {
                        build_show_room(ch, room, mode,
                                         tmp_sprintf("non-existent key for %s exit",
                                                     dirs[dir]), sb);
                        found = 1;
                    }
                    if (!exit->general_description) {
                        build_show_room(ch, room, mode,
                                         tmp_sprintf("no description for %s exit",
                                                     dirs[dir]), sb);
                        found = 1;
                    }
                }
            }
        }
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        send_to_char(ch, "Uh, oh... Bad juju...\r\n");
        break;
    }
    return found;
}

void
show_rooms(struct creature *ch, char *value, char *args)
{
    const char *usage =
        "Usage: show rooms <zone | time | plane | world> <flag> [options]";
    int show_mode;
    int idx, pos;
    int found = 0;
    char *arg;

    struct str_builder sb = str_builder_default;

    show_mode = search_block(value, show_room_modes, 0);
    if (show_mode < 0) {
        send_to_char(ch, "%s\n", usage);
        return;
    }

    arg = tmp_getword(&args);
    if (arg && ((pos = search_block(arg, show_room_flags, 0)) >= 0)) {
        switch (show_mode) {
        case 0:
            found =
                show_rooms_in_zone(&sb, ch, ch->in_room->zone, pos, show_mode,
                                   args);
            if (found < 0) {
                return;
            }
            break;
        case 1:
            for (struct zone_data *zone = zone_table; zone; zone = zone->next) {
                if (ch->in_room->zone->time_frame == zone->time_frame) {
                    found |=
                        show_rooms_in_zone(&sb, ch, zone, pos, show_mode, args);
                    if (found < 0) {
                        return;
                    }
                }
            }
            break;
        case 2:
            for (struct zone_data *zone = zone_table; zone; zone = zone->next) {
                if (ch->in_room->zone->plane == zone->plane) {
                    found |=
                        show_rooms_in_zone(&sb, ch, zone, pos, show_mode, args);
                    if (found < 0) {
                        return;
                    }
                }
            }
            break;
        case 3:
            for (struct zone_data *zone = zone_table; zone; zone = zone->next) {
                found |= show_rooms_in_zone(&sb, ch, zone, pos, show_mode, args);
                if (found < 0) {
                    return;
                }
            }
            break;
        default:
            slog("Can't happen at %s:%d", __FILE__, __LINE__);
        }

        if (found) {
            page_string(ch->desc, sb.str);
        } else {
            send_to_char(ch, "No matching rooms.\r\n");
        }
    } else {
        struct str_builder sb = str_builder_default;

        sb_strcat(&sb, "Valid flags are:\n", NULL);

        for (idx = 0; *show_room_flags[idx] != '\n'; idx++) {
            sb_strcat(&sb, show_room_flags[idx], "\r\n", NULL);
        }
        send_to_char(ch, "%s", sb.str);
    }
}

#define MLEVELS_USAGE "Usage: show mlevels <real | proto> [remort] [expand]\r\n"

void
show_mlevels(struct creature *ch, char *value, char *arg)
{

    bool remort = false;
    bool detailed = false;
    int count[50] = { 0 };
    int i, j;
    int max = 0;
    int to = 0;
    struct creature *mob = NULL;
    char *arg1;

    if (!*value) {
        send_to_char(ch, MLEVELS_USAGE);
        return;
    }

    for (arg1 = tmp_getword(&arg); *arg1; arg1 = tmp_getword(&arg)) {
        if (!strcmp(arg1, "remort")) {
            remort = true;
        } else if (!strcmp(arg1, "detailed")) {
            detailed = true;
        } else {
            send_to_char(ch, "Unknown option: '%s'\r\n%s", arg1,
                         MLEVELS_USAGE);
        }
    }

    struct str_builder sb = str_builder_default;

    sb_sprintf(&sb, "Mlevels for %s", remort ? "remort " : "mortal ");
    // scan the existing mobs
    if (!strcmp(value, "real")) {
        sb_strcat(&sb, "real mobiles:\r\n", NULL);
        for (GList *mit = creatures; mit; mit = mit->next) {
            mob = mit->data;
            if (IS_NPC(mob)
                && GET_LEVEL(mob) < 50 && ((remort && IS_REMORT(mob))
                                           || (!remort && !IS_REMORT(mob)))) {
                if (detailed) {
                    count[(int)GET_LEVEL(mob)]++;
                } else {
                    count[(int)(GET_LEVEL(mob) / 5)]++;
                }
            }
        }
    } else if (!strcmp(value, "proto")) {
        sb_strcat(&sb, "mobile protos:\r\n", NULL);
        GHashTableIter iter;
        gpointer key, val;

        g_hash_table_iter_init(&iter, mob_prototypes);

        while (g_hash_table_iter_next(&iter, &key, &val)) {
            struct creature *mob = val;
            if (GET_LEVEL(mob) < 50 && ((remort && IS_REMORT(mob))
                                        || (!remort && !IS_REMORT(mob)))) {
                if (detailed) {
                    count[(int)GET_LEVEL(mob)]++;
                } else {
                    count[(int)(GET_LEVEL(mob) / 5)]++;
                }
            }
        }
    } else {
        send_to_char(ch,
                     "First argument must be either 'real' or 'proto'\r\n");
        send_to_char(ch, MLEVELS_USAGE);
        return;
    }

    if (detailed) {
        to = 50;
    } else {
        to = 10;
    }

    // get max to scale the display
    for (i = 0; i < to; i++) {
        if (count[i] > max) {
            max = count[i];
        }
    }

    if (!max) {
        send_to_char(ch, "There are no mobiles?\r\n");
        return;
    }
    // print the long, detailed list
    if (detailed) {

        for (i = 0; i < 50; i++) {
            to = (count[i] * 60) / max;

            *buf2 = '\0';
            for (j = 0; j < to; j++) {
                strcat_s(buf2, sizeof(buf2), "+");
            }

            sb_sprintf(&sb, "%2d] %-60s %5d\r\n", i, buf2, count[i]);
        }
    }
    // print the smaller list
    else {
        for (i = 0; i < 10; i++) {

            to = (count[i] * 60) / max;

            *buf2 = '\0';
            for (j = 0; j < to; j++) {
                strcat_s(buf2, sizeof(buf2), "+");
            }

            sb_sprintf(&sb, "%2d-%2d] %-60s %5d\r\n", i * 5,
                        (i + 1) * 5 - 1, buf2, count[i]);
        }
    }

    page_string(ch->desc, sb.str);
}

void
build_print_zone(struct creature *ch, struct zone_data *zone, struct str_builder *sb)
{
    sb_sprintf(sb, "%s%s%3d%s %s%-50.50s%s Top: %s%5d%s Owner:%s%5d%s%s%s\r\n",
                CCBLD(ch, C_CMP), CCYEL(ch, C_NRM), zone->number,
                CCNRM(ch, C_CMP), CCCYN(ch, C_NRM), zone->name,
                CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), zone->top,
                CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), zone->owner_idnum,
                CCRED(ch, C_NRM),
                (olc_lock || IS_SET(zone->flags, ZONE_LOCKED)) ? "L" : "",
                CCNRM(ch, C_NRM));
}

void
show_zones(struct creature *ch, char *arg, char *value)
{
    struct zone_data *zone;

    struct str_builder sb = str_builder_default;

    skip_spaces(&arg);
    if (!*value) {
        build_print_zone(ch, ch->in_room->zone, &sb);
    } else if (is_number(value)) {  // show a range ( from a to b )
        int a = atoi(value);
        int b = a;

        value = tmp_getword(&arg);
        if (*value) {
            b = atoi(value);
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (zone->number >= a && zone->number <= b) {
                build_print_zone(ch, zone, &sb);
            }
        }

        if (sb.len == 0) {
            send_to_char(ch, "That is not a valid zone.\r\n");
            return;
        }
    } else if (strcasecmp("owner", value) == 0 && *arg) {
        int owner_id;

        value = tmp_getword(&arg);
        if (is_number(value)) {
            owner_id = atoi(value);
        } else {
            owner_id = player_idnum_by_name(value);
            if (!owner_id) {
                send_to_char(ch, "That player does not exist.\r\n");
                return;
            }
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (owner_id == zone->owner_idnum
                || owner_id == zone->co_owner_idnum) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp("co-owner", value) == 0 && *arg) {    // Show by name
        int owner_id;

        value = tmp_getword(&arg);
        if (is_number(value)) {
            owner_id = atoi(value);
        } else {
            owner_id = player_idnum_by_name(value);
            if (!owner_id) {
                send_to_char(ch, "That player does not exist.\r\n");
                return;
            }
        }

        for (zone = zone_table; zone; zone = zone->next) {
            if (owner_id == zone->co_owner_idnum) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "all") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            build_print_zone(ch, zone, &sb);
        }
    } else if (strcasecmp(value, "fullcontrol") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (ZONE_FLAGGED(zone, ZONE_FULLCONTROL)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "lawless") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (ZONE_FLAGGED(zone, ZONE_NOLAW)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "past") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (zone->time_frame == TIME_PAST) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "future") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (zone->time_frame == TIME_FUTURE) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "timeless") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (zone->time_frame == TIME_TIMELESS) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "norecalc") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (ZONE_FLAGGED(zone, ZONE_NORECALC)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "noauthor") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (!zone->author || !*zone->author) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "inplay") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (ZONE_FLAGGED(zone, ZONE_INPLAY)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else if (strcasecmp(value, "!inplay") == 0) {
        for (zone = zone_table; zone; zone = zone->next) {
            if (!ZONE_FLAGGED(zone, ZONE_INPLAY)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    } else {                    // Show by name
        for (zone = zone_table; zone; zone = zone->next) {
            if (strcasestr(zone->name, value)) {
                build_print_zone(ch, zone, &sb);
            }
        }
    }

    if (sb.len == 0) {
        send_to_char(ch, "No zones match that criteria.\r\n");
        return;
    }

    page_string(ch->desc, sb.str);
}

#define SFC_OBJ 1
#define SFC_MOB 2
#define SFC_ROOM 3

struct show_struct fields[] = {
    {"nothing", 0, ""},         // 0
    {"zones", LVL_AMBASSADOR, ""},  // 1
    {"player", LVL_IMMORT, "AdminBasic"},
    {"rexits", LVL_IMMORT, "OLC"},
    {"stats", LVL_AMBASSADOR, ""},
    {"boards", LVL_IMMORT, "OLC"},  // 5
    {"death", LVL_IMMORT, "WizardBasic"},
    {"account", LVL_IMMORT, "AdminBasic"},
    {"rooms", LVL_IMMORT, "OLC"},
    {"bugs", LVL_IMMORT, "WizardBasic"},
    {"typos", LVL_IMMORT, ""},  // 10
    {"ideas", LVL_IMMORT, ""},
    {"skills", LVL_IMMORT, "AdminBasic"},
    {"zonecommands", LVL_IMMORT, ""},
    {"nohelps", LVL_IMMORT, "Help"},
    {"aliases", LVL_IMMORT, "AdminBasic"},  // 15
    {"memory use", LVL_IMMORT, "Coder"},
    {"social", LVL_DEMI, ""},
    {"zoneusage", LVL_IMMORT, ""},
    {"topzones", LVL_AMBASSADOR, ""},
    {"nomaterial", LVL_IMMORT, ""}, // 20
    {"objects", LVL_IMMORT, ""},
    {"broken", LVL_IMMORT, ""},
    {"clans", LVL_AMBASSADOR, ""},
    {"specials", LVL_IMMORT, ""},
    {"paths", LVL_IMMORT, ""},  // 25
    {"pathobjs", LVL_IMMORT, ""},
    {"str_app", LVL_IMMORT, ""},
    {"timezones", LVL_IMMORT, ""},
    {"free_create", LVL_IMMORT, ""},
    {"exp_percent", LVL_IMMORT, "OLCApproval"}, // 30
    {"demand_items", LVL_IMMORT, "OLCApproval"},
    {"fighting", LVL_IMMORT, "WizardBasic"},
    {"quad", LVL_IMMORT, ""},
    {"mobiles", LVL_IMMORT, ""},
    {"hunting", LVL_IMMORT, "WizardBasic"}, // 35
    {"last_cmd", LVL_LUCIFER, ""},
    {"duperooms", LVL_IMMORT, "OLCWorldWrite"},
    {"specialization", LVL_IMMORT, ""},
    {"zexits", LVL_IMMORT, ""},
    {"mlevels", LVL_IMMORT, ""},    // 40
    {"mobkills", LVL_IMMORT, "WizardBasic"},
    {"wizcommands", LVL_IMMORT, ""},
    {"timewarps", LVL_IMMORT, ""},  // 43
    {"voices", LVL_IMMORT, ""},
    {"\n", 0, ""}
};

ACMD(do_show)
{
    int i = 0, j = 0, k = 0, l = 0, con = 0, percent, num_rms = 0, tot_rms = 0;
    int found = 0;
    int sfc_mode = 0;
    struct creature *vict;
    struct obj_data *obj;
    struct alias_data *a;
    struct zone_data *zone = NULL;
    struct room_data *room = NULL, *tmp_room = NULL;
    struct creature *mob = NULL;
    GList *protos, *mit, *cit, *oi;

    void show_shops(struct creature *ch, char *value);

    skip_spaces(&argument);
    if (!*argument) {
        GList *cmdlist = NULL;
        send_to_char(ch, "Show options:\r\n");
        for (j = 0, i = 1; fields[i].level; i++) {
            if (is_authorized(ch, SHOW, &fields[i])) {
                cmdlist = g_list_prepend(cmdlist, tmp_strdup(fields[i].cmd));
            }
        }
        cmdlist = g_list_sort(cmdlist, (GCompareFunc) strcasecmp);
        for (GList *it = cmdlist; it; it = it->next) {
            send_to_char(ch, "%-15s%s", (char *)it->data,
                         (!(++j % 5) ? "\r\n" : ""));
        }

        g_list_free(cmdlist);
        send_to_char(ch, "\r\n");
        return;
    }

    char *field = tmp_getword(&argument);
    char *value = tmp_getword(&argument);

    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
        if (!strncmp(field, fields[l].cmd, strlen(field))) {
            break;
        }
    }

    if (!is_authorized(ch, SHOW, &fields[l])) {
        send_to_char(ch, "You do not have that power.\r\n");
        return;
    }

    struct str_builder sb = str_builder_default;
    buf[0] = '\0';

    switch (l) {
    case 1:                    // zone
        show_zones(ch, argument, value);
        break;
    case 2:                    // player
        show_player(ch, value);
        break;
    case 3:                    // rexits
        if (*value) {
            k = atoi(value);
        } else {
            k = ch->in_room->number;
        }

        if (!real_room(k)) {
            send_to_char(ch, "Room %d does not exist.\r\n", k);
            return;
        }

        // check for exits TO room k, a slow operation
        sb_sprintf(&sb, "Rooms with exits TO room %d:\r\n", k);

        for (zone = zone_table, j = 0; zone; zone = zone->next) {
            for (room = zone->world; room; room = room->next) {
                if (room->number == k) {
                    continue;
                }
                for (i = 0; i < NUM_DIRS; i++) {
                    if (room->dir_option[i]
                        && room->dir_option[i]->to_room
                        && room->dir_option[i]->to_room->number == k) {
                        sb_sprintf(&sb, "%3d. [%5d] %-40s %5s -> %7d\r\n",
                                    ++j, room->number, room->name, dirs[i],
                                    room->dir_option[i]->to_room->number);
                    }
                }
            }
        }

        page_string(ch->desc, sb.str);
        break;
    case 4:
        do_show_stats(ch);
        break;
    case 5:
        gen_board_show(ch);
        break;
    case 6:
        strcpy_s(buf, sizeof(buf), "Death Traps\r\n-----------\r\n");
        for (zone = zone_table; zone; zone = zone->next) {
            for (j = 0, room = zone->world; room; room = room->next) {
                if (IS_SET(ROOM_FLAGS(room), ROOM_DEATH)) {
                    snprintf_cat(buf, sizeof(buf), "%2d: %s[%5d]%s %s%s", ++j,
                                 CCRED(ch, C_NRM), room->number, CCCYN(ch, C_NRM),
                                 room->name, CCNRM(ch, C_NRM));
                    if (room->contents) {
                        strcat_s(buf, sizeof(buf), "  (has objects)\r\n");
                    } else {
                        strcat_s(buf, sizeof(buf), "\r\n");
                    }
                }
            }
        }
        page_string(ch->desc, buf);
        break;
    case 7:                    // account
        show_account(ch, value);
        break;
    case 8:                    // rooms
        show_rooms(ch, value, argument);
        break;
    case 9:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0) {
                show_file(ch, BUG_FILE, j);
            }
        } else {
            show_file(ch, BUG_FILE, 0);
        }
        break;
    case 10:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0) {
                show_file(ch, TYPO_FILE, j);
            }
        } else {
            show_file(ch, TYPO_FILE, 0);
        }
        break;
    case 11:
        if (*value && isdigit(*value)) {
            j = atoi(value);
            if (j > 0) {
                show_file(ch, IDEA_FILE, j);
            }
        } else {
            show_file(ch, IDEA_FILE, 0);
        }
        break;
    case 12:
        if (!*value) {
            send_to_char(ch, "View whose skills?\r\n");
            return;
        }

        vict = get_player_vis(ch, value, 0);
        if (vict == NULL) {
            if (!is_named_role_member(ch, "AdminBasic")) {
                send_to_char(ch,
                             "Getting that data from file requires basic administrative rights.\r\n");
                return;
            }
            if (!player_name_exists(value)) {
                send_to_char(ch, "There is no such player.\r\n");
                return;
            }
            if (!(vict = load_player_from_xml(player_idnum_by_name(value)))) {
                send_to_char(ch, "Error loading player.\r\n");
                return;
            }
            if (GET_LEVEL(vict) > GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
                send_to_char(ch, "Sorry, you can't do that.\r\n");
                return;
            }
            list_skills_to_char(ch, vict);
            free_creature(vict);
            return;
        }
        if (IS_NPC(vict)) {
            send_to_char(ch,
                         "Mobs don't have skills.  All mobs are assumed to\r\n"
                         "have a (50 + level)%% skill proficiency.\r\n");
            return;
        }

        list_skills_to_char(ch, vict);
        break;
    case 13:                   // zone commands
        strcpy_s(buf, sizeof(buf), value);
        strcat_s(buf, sizeof(buf), argument);
        do_zone_cmdlist(ch, ch->in_room->zone, buf);
        break;
    case 14:
        show_file(ch, "log/help.log", 0);
        break;
    case 15:                   // aliases
        if (!(vict = get_char_vis(ch, value))) {
            send_to_char(ch, "You cannot see any such person.\r\n");
        } else if (IS_NPC(vict)) {
            send_to_char(ch,
                         "Now... what would a mob need with aliases??\r\n");
        } else if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
            act("You cannot determine what $S aliases might be.", false, ch, NULL,
                vict, TO_CHAR);
            act("$n has attempted to examine your aliases.", false, ch, NULL,
                vict, TO_VICT);
        } else {                /* no argument specified -- list currently defined aliases */
            snprintf(buf, sizeof(buf), "Currently defined aliases for %s:\r\n",
                     GET_NAME(vict));
            if ((a = GET_ALIASES(vict)) == NULL) {
                strcat_s(buf, sizeof(buf), " None.\r\n");
            } else {
                while (a != NULL) {
                    snprintf_cat(buf, sizeof(buf), "%s%-15s%s %s\r\n", CCCYN(ch, C_NRM),
                                 a->alias, CCNRM(ch, C_NRM), a->replacement);
                    a = a->next;
                }
            }
            page_string(ch->desc, buf);
        }
        break;
    case 16:
        do_stat_memory(ch);
        break;
    case 17:                   /* social */
        show_social_messages(ch, value);
        break;
    case 18:                   // zoneusage
        show_zoneusage(ch, value);
        break;
    case 19:                   // topzones
        show_topzones(ch, tmp_strcat(value, argument, NULL));
        break;
    case 20:                   /* nomaterial */
        strcpy_s(buf, sizeof(buf), "Objects without material types:\r\n");
        protos = g_hash_table_get_values(obj_prototypes);

        for (i = 1, oi = protos; oi; oi = oi->next) {
            obj = oi->data;
            if (GET_OBJ_MATERIAL(obj) == MAT_NONE &&
                !IS_OBJ_TYPE(obj, ITEM_SCRIPT)) {
                if (strlen(buf) > (MAX_STRING_LENGTH - 130)) {
                    strcat_s(buf, sizeof(buf), "**OVERFLOW**\r\n");
                    break;
                }
                snprintf_cat(buf, sizeof(buf), "%3d. [%5d] %s%-36s%s  (%s)\r\n", i,
                             GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                             obj->name, CCNRM(ch, C_NRM), obj->aliases);
                i++;
            }
        }
        g_list_free(protos);
        page_string(ch->desc, buf);
        break;

    case 21: /** objects **/
        do_show_objects(ch, value, argument);
        break;

    case 22: /** broken **/

        strcpy_s(buf, sizeof(buf), "Broken objects in the game:\r\n");

        for (obj = object_list, i = 1; obj; obj = obj->next) {
            if ((GET_OBJ_DAM(obj) < (GET_OBJ_MAX_DAM(obj) / 2)) ||
                IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {

                if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_DAM(obj) == -1) {
                    j = 100;
                } else if (GET_OBJ_DAM(obj) == 0 || GET_OBJ_DAM(obj) == 0) {
                    j = 0;
                } else {
                    j = (GET_OBJ_DAM(obj) * 100 / GET_OBJ_MAX_DAM(obj));
                }

                snprintf(buf2, sizeof(buf2), "%3d. [%5d] %s%-34s%s [%3d percent] %s\r\n",
                         i, GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                         obj->name,
                         CCNRM(ch, C_NRM),
                         j, IS_OBJ_STAT2(obj, ITEM2_BROKEN) ? "<broken>" : "");
                if ((strlen(buf) + strlen(buf2) + 128) > MAX_STRING_LENGTH) {
                    strcat_s(buf, sizeof(buf), "**OVERFLOW**\r\n");
                    break;
                }
                strcat_s(buf, sizeof(buf), buf2);
                i++;
            }
        }

        page_string(ch->desc, buf);
        break;

    case 23:    /** clans **/
        if (GET_LEVEL(ch) < LVL_ELEMENT) {
            do_show_clan(ch, NULL);
        } else {
            do_show_clan(ch, clan_by_name(value));
        }
        break;
    case 24:   /** specials **/
        do_show_specials(ch, value);
        break;
    case 25:                   /*paths */
        show_path(ch, value);
        break;
    case 26:                   /* pathobjs */
        show_pathobjs(ch);
        break;
    case 27:                   /* str_app */
        strcpy_s(buf, sizeof(buf), "STR      to_hit    to_dam    max_encum    max_weap\r\n");
        for (i = 0; i <= 50; i++) {
            snprintf(buf, sizeof(buf),
                     "%s%-5d     %2d         %2d         %4f          %2f\r\n",
                     buf,
                     i,
                     strength_hit_bonus(i),
                     strength_damage_bonus(i),
                     strength_carry_weight(i),
                     strength_wield_weight(i));
        }
        page_string(ch->desc, buf);
        break;

    case 28:                   /* timezones */
        if (!*value) {
            send_to_char(ch, "What time frame?\r\n");
            return;
        }
        if ((i = search_block(value, time_frames, 0)) < 0) {
            send_to_char(ch, "That is not a valid time frame.\r\n");
            return;
        }
        snprintf(buf, sizeof(buf), "TIME ZONES for: %s PRIME.\r\n", time_frames[i]);

        /* j is hour mod index, i is time_frame value */
        for (j = -3, found = 0; j < 3; found = 0, j++) {
            for (zone = zone_table; zone; zone = zone->next) {
                if (zone->time_frame == i && zone->hour_mod == j) {
                    if (!found) {
                        snprintf_cat(buf, sizeof(buf), "\r\nTime zone (%d):\r\n", j);
                    }
                    ++found;
                    snprintf_cat(buf, sizeof(buf), "%3d. %25s   weather: [%d]\r\n",
                                 zone->number, zone->name, zone->weather->sky);
                }
            }
            if (strlen(buf) > (size_t) (MAX_STRING_LENGTH * (11 + j)) / 16) {
                strcat_s(buf, sizeof(buf), "STOP\r\n");
                break;
            }
        }
        page_string(ch->desc, buf);
        break;
    case 29:
        if (!*value) {
            send_to_char(ch, "Show free_create mobs, objs, or rooms?\r\n");
            return;
        }
        if (is_abbrev(value, "objs") || is_abbrev(value, "objects")) {
            sfc_mode = SFC_OBJ;
        } else if (is_abbrev(value, "mobs") || is_abbrev(value, "mobiless")) {
            sfc_mode = SFC_MOB;
        } else if (is_abbrev(value, "rooms")) {
            sfc_mode = SFC_ROOM;
        } else {
            send_to_char(ch,
                         "You must show free_create obj, mob, or room.\r\n");
            return;
        }

        strcpy_s(buf, sizeof(buf), "Free_create for this zone:\r\n");

        for (i = ch->in_room->zone->number * 100; i < ch->in_room->zone->top;
             i++) {

            if (sfc_mode == SFC_OBJ && !real_object_proto(i)) {
                snprintf_cat(buf, sizeof(buf), "[%5d]\r\n", i);
            } else if (sfc_mode == SFC_MOB && !real_mobile_proto(i)) {
                snprintf_cat(buf, sizeof(buf), "[%5d]\r\n", i);
            } else if (sfc_mode == SFC_ROOM && !real_room(i)) {
                snprintf_cat(buf, sizeof(buf), "[%5d]\r\n", i);
            }
        }
        page_string(ch->desc, buf);
        return;

    case 30:                   /* exp_percent */
        if (!*argument || !*value) {
            send_to_char(ch,
                         "Usage: show exp <min percent> <max percent>\r\n");
            return;
        }
        if ((k = atoi(value)) < 0 || (l = atoi(argument)) < 0) {
            send_to_char(ch, "Try using positive numbers, asshole.\r\n");
            return;
        }
        if (k >= l) {
            send_to_char(ch, "Second number must be larger than first.\r\n");
            return;
        }
        if (l - k > 200) {
            send_to_char(ch,
                         "Increment too large.  Stay smaller than 200.\r\n");
            return;
        }
        strcpy_s(buf, sizeof(buf), "Mobs with exp in given range:\r\n");
        protos = g_hash_table_get_values(mob_prototypes);
        for (mit = protos; mit; mit = mit->next) {
            mob = mit->data;
            percent = GET_EXP(mob) * 100;
            if ((j = mobile_experience(mob, NULL))) {
                percent /= j;
            } else {
                percent = 0;
            }

            if (percent >= k && percent <= j) {
                if (strlen(buf) + 256 > MAX_STRING_LENGTH) {
                    strcat_s(buf, sizeof(buf), "**OVERFOW**\r\n");
                    break;
                }
                i++;
                snprintf(buf, sizeof(buf),
                         "%s%4d. %s[%s%5d%s]%s %35s%s exp:[%9d] pct:%s[%s%3d%s]%s\r\n",
                         buf, i,
                         CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                         GET_NPC_VNUM(mob),
                         CCGRN(ch, C_NRM), CCYEL(ch, C_NRM),
                         GET_NAME(mob), CCNRM(ch, C_NRM),
                         GET_EXP(mob),
                         CCRED(ch, C_NRM), CCCYN(ch, C_NRM),
                         percent, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
            }
        }
        g_list_free(protos);
        page_string(ch->desc, buf);
        break;

    case 31:                   /* demand_items */
        if (!*value) {
            send_to_char(ch,
                         "What demand level (in total units existing)?\r\n");
            return;
        }

        if ((k = atoi(value)) <= 0) {
            send_to_char(ch, "-- You are so far out. -- level > 0 !!\r\n");
            return;
        }

        strcpy_s(buf, sizeof(buf), "");
        protos = g_hash_table_get_values(obj_prototypes);
        for (oi = protos; oi; oi = oi->next) {
            obj = oi->data;
            if (obj->shared->number >= k) {
                if (strlen(buf) + 256 > MAX_STRING_LENGTH) {
                    strcat_s(buf, sizeof(buf), "**OVERFOW**\r\n");
                    break;
                }
                snprintf(buf, sizeof(buf),
                         "%s%4d. %s[%s%5d%s] %40s%s Tot:[%4d], House:[%4d]\r\n",
                         buf, ++i, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                         GET_OBJ_VNUM(obj), CCGRN(ch, C_NRM),
                         obj->name, CCNRM(ch, C_NRM),
                         obj->shared->number, obj->shared->house_count);
            }
        }
        if (i) {
            page_string(ch->desc, buf);
        } else {
            send_to_char(ch, "No items.\r\n");
        }
        break;

    case 32: {                  /* fighting */
        /*
           strcpy_s(buf, sizeof(buf), "Fighting characters:\r\n");

           CombatDataList_iterator it;
           cit = combatList.begin();
           for (; cit != combatList.end(); ++cit) {
           vict = *cit;

           if (!can_see_creature(ch, vict))
           continue;

           if (strlen(buf) > MAX_STRING_LENGTH - 128)
           break;

           snprintf_cat(buf, sizeof(buf), "%3d: %s[%s%5d%s] %s%s%s\r\n", ++i,
           CCRED(ch, C_NRM),
           CCGRN(ch, C_NRM),
           vict->in_room->number,
           CCRED(ch, C_NRM),
           IS_NPC(vict) ? CCCYN(ch, C_NRM) : CCYEL(ch, C_NRM),
           GET_NAME(vict),
           CCNRM(ch, C_NRM));

           it = vict->getCombatList()->begin();
           for (; it != vict->getCombatList()->end(); ++it) {
           snprintf_cat(buf, sizeof(buf), "             - %s%s%s\r\n",
           CCWHT(ch, C_NRM),
           GET_NAME(it->getOpponent()),
           CCNRM(ch, C_NRM));
           }

           }

           page_string(ch->desc, buf);
         */
        break;
    }
    case 33:                   /* quad */

        strcpy_s(buf, sizeof(buf), "Characters with Quad Damage:\r\n");

        for (cit = first_living(creatures); cit; cit = next_living(cit)) {
            vict = cit->data;

            if (!can_see_creature(ch, vict)
                || !affected_by_spell(vict, SPELL_QUAD_DAMAGE)) {
                continue;
            }

            if (strlen(buf) > MAX_STRING_LENGTH - 128) {
                break;
            }

            snprintf_cat(buf, sizeof(buf), " %3d. %28s --- %s [%d]\r\n", ++i,
                         GET_NAME(vict), vict->in_room->name, vict->in_room->number);

        }

        page_string(ch->desc, buf);
        break;

    case 34:
        do_show_mobiles(ch, value, argument);
        break;

    case 35:                   /* hunting */

        strcpy_s(buf, sizeof(buf), "Characters hunting:\r\n");
        for (cit = first_living(creatures); cit; cit = next_living(cit)) {
            vict = cit->data;
            if (!NPC_HUNTING(vict) || !NPC_HUNTING(vict)->in_room
                || !can_see_creature(ch, vict)) {
                continue;
            }

            if (strlen(buf) > MAX_STRING_LENGTH - 128) {
                break;
            }

            snprintf_cat(buf, sizeof(buf), " %3d. %23s [%5d] ---> %20s [%5d]\r\n", ++i,
                         GET_NAME(vict), vict->in_room->number,
                         GET_NAME(NPC_HUNTING(vict)),
                         NPC_HUNTING(vict)->in_room->number);
        }

        page_string(ch->desc, buf);
        break;

    case 36:                   /* last_cmd */

        strcpy_s(buf, sizeof(buf), "Last cmds:\r\n");
        for (i = 0; i < NUM_SAVE_CMDS; i++) {
            snprintf_cat(buf, sizeof(buf), " %2d. (%4d) %25s - '%s'\r\n",
                         i, last_cmd[i].idnum, player_name_by_idnum(last_cmd[i].idnum),
                         last_cmd[i].string);
        }

        page_string(ch->desc, buf);
        break;

    case 37:                   // duperooms

        strcpy_s(buf, sizeof(buf),
                 " Zone  Name                          Rooms       Bytes\r\n");
        for (zone = zone_table, k = con = tot_rms = 0; zone; zone = zone->next) {
            i = j = 0;
            for (room = (zone->world ? zone->world->next : NULL), num_rms = 1;
                 room; room = room->next, num_rms++) {
                if (room->description) {
                    l = strlen(room->description);
                    for (tmp_room = zone->world; tmp_room;
                         tmp_room = tmp_room->next) {
                        if (tmp_room == room) {
                            break;
                        }
                        if (!tmp_room->description) {
                            continue;
                        }
                        if (!strncasecmp(tmp_room->description,
                                         room->description, l)) {
                            i++;    // number of duplicate rooms in zone
                            j += l; // total length of duplicates in zone;
                            break;
                        }
                    }
                }
            }
            if (zone->world) {
                tot_rms += num_rms;
            }
            k += i;             // number of duplicate rooms total;
            con += j;           // total length of duplicates in world;
            if (i) {
                snprintf_cat(buf, sizeof(buf), " [%3d] %s%-30s%s  %3d/%3d  %6d\r\n",
                             zone->number, CCCYN(ch, C_NRM), zone->name,
                             CCNRM(ch, C_NRM), i, num_rms, j);
            }
        }
        snprintf(buf, sizeof(buf),
                 "%s------------------------------------------------------\r\n"
                 " Totals:                              %4d/%3d  %6d\r\n", buf, k,
                 tot_rms, con);
        page_string(ch->desc, buf);
        break;

    case 38:                   // specialization
        if (!(vict = get_char_vis(ch, value))) {
            send_to_char(ch, "You cannot see any such person.\r\n");
            return;
        }
        snprintf(buf, sizeof(buf), "Weapon specialization for %s:\r\n", GET_NAME(vict));
        for (obj = NULL, i = 0; i < MAX_WEAPON_SPEC; i++, obj = NULL) {
            if (GET_WEAP_SPEC(vict, i).vnum > 0) {
                obj = real_object_proto(GET_WEAP_SPEC(vict, i).vnum);
            }
            snprintf_cat(buf, sizeof(buf), " [%5d] %-30s [%2d]\r\n",
                         GET_WEAP_SPEC(vict, i).vnum,
                         obj ? obj->name : "--- ---", GET_WEAP_SPEC(vict, i).level);
        }
        page_string(ch->desc, buf);
        break;

#define ZEXITS_USAGE "Usage: zexit <f|t> <zone>\r\n"

    case 39:                   // zexits

        if (*value != 'f' && *value != 't') {
            send_to_char(ch, "First argument must be 'to' or 'from'.\r\n");
            send_to_char(ch, ZEXITS_USAGE);
            return;
        }
        if (!*argument) {
            send_to_char(ch, "Second argument must be a zone number.\r\n");
            send_to_char(ch, ZEXITS_USAGE);
            return;
        }

        k = atoi(argument);

        if (!(zone = real_zone(k))) {
            send_to_char(ch, "Zone %d does not exist.\r\n", k);
            return;
        }
        if (*value == 'f') {
            // check for exits FROM zone k, easy to do
            sb_sprintf(&sb, "Rooms with exits FROM zone %d:\r\n", k);

            for (room = zone->world, j = 0; room; room = room->next) {
                for (i = 0; i < NUM_DIRS; i++) {
                    if (room->dir_option[i] && room->dir_option[i]->to_room &&
                        room->dir_option[i]->to_room->zone != zone) {
                        sb_sprintf(&sb, "%3d. [%5d] %-40s %5s -> %7d\r\n",
                                    ++j, room->number, room->name, dirs[i],
                                    room->dir_option[i]->to_room->number);
                    }
                }
            }
        } else if (*value == 't') {
            // check for exits TO zone k, a slow operation
            sb_sprintf(&sb, "Rooms with exits TO zone %d:\r\n", k);

            for (zone = zone_table, j = 0; zone; zone = zone->next) {
                if (zone->number == k) {
                    continue;
                }
                for (room = zone->world; room; room = room->next) {
                    for (i = 0; i < NUM_DIRS; i++) {
                        if (room->dir_option[i] && room->dir_option[i]->to_room
                            && room->dir_option[i]->to_room->zone->number ==
                            k) {
                            sb_sprintf(&sb, "%3d. [%5d] %-40s %5s -> %7d\r\n",
                                        ++j, room->number, room->name, dirs[i],
                                        room->dir_option[i]->to_room->number);
                        }
                    }
                }
            }
        }
        page_string(ch->desc, sb.str);
        break;

    case 40:                   // mlevels
        show_mlevels(ch, value, argument);
        break;

    case 41:                   // mobkills
        show_mobkills(ch, value, argument);
        break;

    case 42:                   // wizcommands
        show_wizcommands(ch);
        break;

    case 43:                   // timewarps
        show_timewarps(ch);
        break;
    case 44:                   // voices
        show_voices(ch);
        break;
    default:
        send_to_char(ch, "Sorry, I don't understand that.\r\n");
        break;
    }
}

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC        0
#define BINARY        1
#define NUMBER        2

#define SET_OR_REMOVE(flagset, flags) { \
        if (on) {SET_BIT(flagset, flags); } \
        else if (off) {REMOVE_BIT(flagset, flags); }}

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

ACMD(do_set)
{
    static struct set_struct fields[] = {
        {"brief", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"invstart", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"title", LVL_IMMORT, PC, MISC, "AdminBasic"},
        {"maxhit", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"maxmana", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"maxmove", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"hit", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"mana", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"move", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"align", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"str", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"stradd", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"int", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"wis", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"dex", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"con", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"cha", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"sex", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"ac", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"gold", LVL_IMMORT, BOTH, NUMBER, "AdminFull"},
        {"exp", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"hitroll", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"damroll", LVL_IMMORT, BOTH, NUMBER, "Coder"},
        {"invis", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"nohassle", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"frozen", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"nowho", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"drunk", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"hunger", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"thirst", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"level", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"room", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"roomflag", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"siteok", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"name", LVL_IMMORT, BOTH, MISC, "AdminFull"},
        {"class", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"remort_class", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"homeroom", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"nodelete", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"loadroom", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"race", LVL_IMMORT, BOTH, MISC, "WizardFull"},
        {"height", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"weight", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"nosnoop", LVL_ENTITY, PC, BINARY, "WizardAdmin"},
        {"clan", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"leader", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"life", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"debug", LVL_IMMORT, PC, BINARY, "WizardBasic"},
        {"hunting", LVL_IMMORT, NPC, MISC, "Coder"},
        {"fighting", LVL_IMMORT, BOTH, MISC, "Coder"},
        {"mobkills", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"pkills", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"akills", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"newbiehelper", LVL_ETERNAL, PC, BINARY, "Coder"},
        {"holylight", LVL_IMMORT, PC, BINARY, "Coder"},
        {"notitle", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"remortinvis", LVL_IMMORT, PC, NUMBER, "AdminBasic"},
        {"halted", LVL_IMMORT, PC, BINARY, "WizardFull"},
        {"syslog", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"broken", LVL_IMMORT, PC, NUMBER, "WizardFull"},
        {"totaldamage", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"olcgod", LVL_IMMORT, PC, BINARY, "OLCAdmin"},
        {"tester", LVL_IMMORT, PC, BINARY, "OlcWorldWrite"},
        {"mortalized", LVL_IMPL, PC, BINARY, "Coder"},
        {"noaffects", LVL_IMMORT, PC, BINARY, "Coder"},
        {"age_adjust", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"cash", LVL_IMMORT, BOTH, NUMBER, "AdminFull"},
        {"generation", LVL_IMMORT, BOTH, NUMBER, "WizardFull"},
        {"path", LVL_LUCIFER, NPC, NUMBER, "Coder"},
        {"nopost", LVL_IMMORT, PC, BINARY, "AdminBasic"},
        {"logging", LVL_IMMORT, PC, BINARY, "WizardAdmin"},
        {"nopk", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"soilage", LVL_IMMORT, PC, MISC, "WizardBasic"},
        {"specialization", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"qpallow", LVL_IMMORT, PC, NUMBER, "WizardFull"}, // once included QuestorAdmin
        {"soulless", LVL_IMMORT, BOTH, BINARY, "WizardFull"},
        {"buried", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"speed", LVL_IMMORT, PC, NUMBER, "Coder"},
        {"badge", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"skill", LVL_IMMORT, PC, MISC, "WizardFull"},
        {"reputation", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"language", LVL_IMMORT, BOTH, MISC, "AdminFull"},
        {"hardcore", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"rentcode", LVL_DEMI, PC, MISC, "AdminFull"},
        {"\n", 0, BOTH, MISC, ""}
    };

    int i, l, tp;
    struct room_data *room;
    struct creature *vict = NULL, *vict2 = NULL;
    char *field, *name;
    char *arg1, *arg2;
    bool on = false, off = false;
    int value = 0;
    bool is_file = false, is_mob = false, is_player = false;
    int parse_char_class(char *arg);
    int parse_race(char *arg);

    name = tmp_getword(&argument);
    if (!strcmp(name, "file")) {
        is_file = true;
        name = tmp_getword(&argument);
    } else if (!strcasecmp(name, "player")) {
        is_player = true;
        name = tmp_getword(&argument);
    } else if (!strcasecmp(name, "mob")) {
        is_mob = true;
        name = tmp_getword(&argument);
    }
    field = tmp_getword(&argument);

    if (!*name || !*field) {
        GList *cmdlist = NULL;
        int j;
        struct str_builder sb = str_builder_default;
        sb_sprintf(&sb, "Usage: set <victim> <field> <value>\r\n");
        for (j = 0, i = 1; fields[i].level; i++) {
            if (is_authorized(ch, SET, &fields[i])) {
                cmdlist = g_list_prepend(cmdlist, tmp_strdup(fields[i].cmd));
            }
        }
        cmdlist = g_list_sort(cmdlist, (GCompareFunc) strcasecmp);
        for (GList *it = cmdlist; it; it = it->next) {
            sb_sprintf(&sb, "%-15s%s", (char *)it->data,
                        (!(++j % 5) ? "\r\n" : ""));
        }
        if ((j - 1) % 5) {
            sb_sprintf(&sb, "\r\n");
        }
        page_string(ch->desc, sb.str);

        g_list_free(cmdlist);
        return;
    }
    if (is_file) {
        vict = NULL;
        if (!player_name_exists(name)) {
            send_to_char(ch, "There is no such player.\r\n");
        } else if ((vict = get_player_vis(ch, name, 0)) != NULL) {
            is_file = false;
        } else if (!(vict = load_player_from_xml(player_idnum_by_name(name)))) {
            send_to_char(ch, "Error loading player file.\r\n");
        } else if (GET_LEVEL(vict) >= GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
            send_to_char(ch, "Sorry, you can't do that.\r\n");
        }

        if (!vict) {
            return;
        }
    } else {
        if (is_player) {
            if (!(vict = get_player_vis(ch, name, 0))) {
                send_to_char(ch, "There is no such player.\r\n");
                return;
            }
        } else if (is_mob) {
            if (!(vict = get_mobile_vis(ch, name, 0))) {
                send_to_char(ch, "There is no such mobile.\r\n");
                return;
            }
        } else {
            if (!(vict = get_char_vis(ch, name))) {
                send_to_char(ch, "There is no such creature.\r\n");
                return;
            }
        }
    }

    if (GET_LEVEL(ch) < LVL_LUCIFER) {
        if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
            send_to_char(ch, "Maybe that's not such a great idea...\r\n");
            return;
        }
    }

    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
        if (!strncmp(field, fields[l].cmd, strlen(field))) {
            break;
        }
    }

    if (GET_LEVEL(ch) < fields[l].level && subcmd != SCMD_TESTER_SET) {
        send_to_char(ch, "You are not able to perform this godly deed!\r\n");
        return;
    }

    if (!is_authorized(ch, SET, &fields[l]) && subcmd != SCMD_TESTER_SET) {
        send_to_char(ch, "You do not have that power.\r\n");
        return;
    }

    if (IS_NPC(vict) && !(fields[l].pcnpc & NPC)) {
        send_to_char(ch, "You can't do that to a beast!\r\n");
        return;
    } else if (!IS_NPC(vict) && !(fields[l].pcnpc & PC)) {
        send_to_char(ch, "That can only be done to a beast!\r\n");
        return;
    }

    if (fields[l].type == BINARY) {
        if (!strcmp(argument, "on") || !strcmp(argument, "yes")) {
            on = true;
        } else if (!strcmp(argument, "off") || !strcmp(argument, "no")) {
            off = true;
        }
        if (!(on || off)) {
            send_to_char(ch, "Value must be on or off.\r\n");
            return;
        }
    } else if (fields[l].type == NUMBER) {
        value = atoi(argument);
    }

    strcpy_s(buf, sizeof(buf), "Okay.");       /* can't use OK macro here 'cause of \r\n */

    switch (l) {
    case 0:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
        break;
    case 1:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
        break;
    case 2:
        set_title(vict, argument);
        snprintf(buf, sizeof(buf), "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
        slog("%s has changed %s's title to : '%s'.", GET_NAME(ch),
             GET_NAME(vict), GET_TITLE(vict));
        break;
    case 3:
        vict->points.max_hit = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 4:
        vict->points.max_mana = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 5:
        vict->points.max_move = RANGE(1, 30000);
        affect_total(vict);
        break;
    case 6:
        vict->points.hit = RANGE(-9, vict->points.max_hit);
        affect_total(vict);
        break;
    case 7:
        vict->points.mana = RANGE(0, vict->points.max_mana);
        affect_total(vict);
        break;
    case 8:
        vict->points.move = RANGE(0, vict->points.max_move);
        affect_total(vict);
        break;
    case 9:
        GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
        affect_total(vict);
        break;
    case 10:
        RANGE(0, 50);
        if (value <= 18) {
            vict->real_abils.str = value;
        } else if (value > 18) {
            vict->real_abils.str = value + 10;
        }
        affect_total(vict);
        break;
    case 11:
        send_to_char(ch,
                     "Stradd doesn't exist anymore... asshole.\r\n");
        return;
    case 12:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD) {
            RANGE(3, 50);
        } else {
            RANGE(3, MIN(50, GET_REMORT_GEN(vict) + 18));
        }
        vict->real_abils.intel = value;
        affect_total(vict);
        break;
    case 13:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD) {
            RANGE(3, 50);
        } else {
            RANGE(3, MIN(50, GET_REMORT_GEN(vict) + 18));
        }
        vict->real_abils.wis = value;
        affect_total(vict);
        break;
    case 14:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD) {
            RANGE(3, 50);
        } else {
            RANGE(3, MIN(50, GET_REMORT_GEN(vict) + 18));
        }
        vict->real_abils.dex = value;
        affect_total(vict);
        break;
    case 15:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD) {
            RANGE(3, 50);
        } else {
            RANGE(3, MIN(50, GET_REMORT_GEN(vict) + 18));
        }
        vict->real_abils.con = value;
        affect_total(vict);
        break;
    case 16:
        if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD) {
            RANGE(3, 50);
        } else {
            RANGE(3, MIN(50, GET_REMORT_GEN(vict) + 18));
        }
        vict->real_abils.cha = value;
        affect_total(vict);
        break;
    case 17:
        if (!strcasecmp(argument, "male")) {
            vict->player.sex = SEX_MALE;
        } else if (!strcasecmp(argument, "female")) {
            vict->player.sex = SEX_FEMALE;
        } else if (!strcasecmp(argument, "neuter")) {
            vict->player.sex = SEX_NEUTER;
        } else if (!strcasecmp(argument, "spivak")) {
            vict->player.sex = SEX_SPIVAK;
        } else if (!strcasecmp(argument, "plural")) {
            vict->player.sex = SEX_PLURAL;
        } else if (!strcasecmp(argument, "kitten")) {
            vict->player.sex = SEX_KITTEN;
        } else {
            send_to_char(ch, "Must be male, female, neuter, spivak, plural, kitten.\r\n");
            return;
        }
        break;
    case 18:
        vict->points.armor = RANGE(-200, 100);
        affect_total(vict);
        break;
    case 19:
        GET_GOLD(vict) = RANGE(0, 1000000000);
        break;
    case 20:
        vict->points.exp = RANGE(0, 1500000000);
        break;
    case 21:
        vict->points.hitroll = RANGE(-200, 200);
        affect_total(vict);
        break;
    case 22:
        vict->points.damroll = RANGE(-200, 200);
        affect_total(vict);
        break;
    case 23:
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            send_to_char(ch, "You aren't godly enough for that!\r\n");
            return;
        }
        GET_INVIS_LVL(vict) = RANGE(0, GET_LEVEL(vict));
        if (GET_INVIS_LVL(vict) > LVL_LUCIFER) {
            GET_INVIS_LVL(vict) = LVL_LUCIFER;
        }
        break;
    case 24:
        if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
            send_to_char(ch, "You aren't godly enough for that!\r\n");
            return;
        }
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
        break;
    case 25:
        if (ch == vict) {
            send_to_char(ch, "Better not -- could be a long winter!\r\n");
            return;
        }
        slog("(GC) %s set %s %sfrozen.",
             GET_NAME(ch),
             GET_NAME(vict), PLR_FLAGGED(vict, PLR_FROZEN) ? "" : "UN-");
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
        break;
    case 26:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NOWHO);
        break;
    case 27:
    case 28:
    case 29:
        if (!strcasecmp(argument, "off")) {
            GET_COND(vict, (l - 27)) = (char)-1;
            snprintf(buf, sizeof(buf), "%s's %s now off.", GET_NAME(vict), fields[l].cmd);
        } else if (is_number(argument)) {
            value = atoi(argument);
            RANGE(0, 24);
            GET_COND(vict, (l - 27)) = (char)value;
            snprintf(buf, sizeof(buf), "%s's %s set to %d.", GET_NAME(vict), fields[l].cmd,
                     value);
        } else {
            send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
            return;
        }
        break;
    case 30:
        if ((value > GET_LEVEL(ch) && GET_IDNUM(ch) != 1) || value > LVL_GRIMP) {
            send_to_char(ch, "You can't do that.\r\n");
            return;
        }

        RANGE(0, LVL_GRIMP);

        if (value >= 50 && !is_named_role_member(ch, "AdminFull")) {
            send_to_char(ch, "That should be done with advance.\r\n");
            return;
        }

        vict->player.level = (int8_t) value;
        break;
    case 31:
        if ((room = real_room(value)) == NULL) {
            send_to_char(ch, "No room exists with that number.\r\n");
            return;
        }
        char_from_room(vict, false);
        char_to_room(vict, room, false);
        break;
    case 32:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
        break;
    case 33:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
        slog("(GC) %s %ssiteok'd %s.", GET_NAME(ch), PLR_FLAGGED(vict,
                                                                 PLR_SITEOK) ? "" : "UN-", GET_NAME(vict));

        break;
    case 34:
        if (IS_PC(vict)) {
            if (!(is_named_role_member(ch, "AdminFull")
                  && is_valid_name(argument))) {
                send_to_char(ch, "That character name is invalid.\r\n");
                return;
            }

            if (player_name_exists(argument)) {
                send_to_char(ch,
                             "There is already a player by that name.\r\n");
                return;
            }
        }

        slog("%s%s set %s %s's name to '%s'",
             (IS_NPC(vict) ? "" : "(GC) "),
             GET_NAME(ch),
             (IS_NPC(vict) ? "NPC" : "PC"), GET_NAME(vict), argument);

        free(GET_NAME(vict));
        if (IS_NPC(vict)) {
            vict->player.short_descr = strdup(argument);
        } else {
            vict->player.name = strdup(argument);
        }
        // Set name
        if (IS_PC(vict)) {
            sql_exec("update players set name='%s' where idnum=%ld",
                     tmp_sqlescape(argument), GET_IDNUM(vict));
            crashsave(vict);
        }
        break;
    case 35:
        if ((i = parse_char_class(argument)) == CLASS_UNDEFINED) {
            send_to_char(ch, "'%s' is not a char class.\r\n", argument);
            return;
        }
        GET_CLASS(vict) = i;
        break;
    case 36:
        if ((i = parse_char_class(argument)) == CLASS_UNDEFINED) {
            send_to_char(ch, "That is not a char_class.\r\n");
        }
        GET_REMORT_CLASS(vict) = i;
        break;
    case 37:
        if (real_room(value) != NULL) {
            GET_HOMEROOM(vict) = value;
            snprintf(buf, sizeof(buf), "%s will enter at %d.", GET_NAME(vict),
                     GET_HOMEROOM(vict));
        } else {
            snprintf(buf, sizeof(buf), "That room does not exist!");
        }
        break;
    case 38:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
        break;
    case 39:
        if (real_room(value) != NULL) {
            GET_LOADROOM(vict) = value;
            snprintf(buf, sizeof(buf), "%s will enter the game at %d.", GET_NAME(vict),
                     GET_LOADROOM(vict));
        } else {
            snprintf(buf, sizeof(buf), "That room does not exist!");
        }
        break;
    case 40:
        if ((i = parse_race(argument)) == RACE_UNDEFINED) {
            send_to_char(ch, "That is not a race.\r\n");
            return;
        }
        GET_RACE(vict) = i;
        break;
    case 41:
        if ((value > 400) && (GET_LEVEL(ch) < LVL_CREATOR)) {
            send_to_char(ch, "That is too tall.\r\n");
            return;
        } else {
            GET_HEIGHT(vict) = value;
        }
        break;
    case 42:
        if ((value > 800) && (GET_LEVEL(ch) < LVL_CREATOR)) {
            send_to_char(ch, "That is too heavy.\r\n");
            return;
        } else {
            GET_WEIGHT(vict) = value;
        }
        break;
    case 43:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOSNOOP);
        if (!is_file && vict->desc->snoop_by) {
            for (GList *x = vict->desc->snoop_by; x; x = x->next) {
                struct descriptor_data *d = x->data;
                send_to_char(d->creature,
                             "Your snooping session has ended.\r\n");
                stop_snooping(d->creature);
            }
        }
        break;
    case 44: {
        struct clan_data *clan = real_clan(GET_CLAN(ch));

        if (is_number(argument) && !atoi(argument)) {
            if (clan && (clan->owner == GET_IDNUM(ch))) {
                clan->owner = 0;
            }
            GET_CLAN(vict) = 0;
            send_to_char(ch, "Clan set to none.\r\n");
        } else if (!clan_by_name(argument)) {
            send_to_char(ch, "There is no such clan.\r\n");
            return;
        } else {
            if (clan && (clan->owner == GET_IDNUM(ch))) {
                clan->owner = 0;
            }
            GET_CLAN(vict) = clan_by_name(argument)->number;
        }
        break;
    }
    case 45:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_CLAN_LEADER);
        break;
    case 46:
        GET_LIFE_POINTS(vict) = RANGE(0, 100);
        break;
    case 47:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_DEBUG);
        break;
    case 48:
        if (!(vict2 = get_char_vis(ch, argument))) {
            send_to_char(ch, "No such target character around.\r\n");
        } else {
            start_hunting(vict, vict2);
            send_to_char(ch, "%s now hunts %s.\r\n", GET_NAME(vict),
                         GET_NAME(vict2));
        }
        return;
    case 49:
        if (!(vict2 = get_char_vis(ch, argument))) {
            send_to_char(ch, "No such target character around.\r\n");
        } else {
            add_combat(vict, vict2, false);
            send_to_char(ch, "%s is now fighting %s.\r\n", GET_NAME(vict),
                         GET_NAME(vict2));
        }
        return;
    case 50:
        GET_MOBKILLS(vict) = value;
        break;
    case 51:
        GET_PKILLS(vict) = value;
        break;
    case 52:
        GET_ARENAKILLS(vict) = value;
        break;
    case 53:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NEWBIE_HELPER);
        break;
    case 54:
        SET_OR_REMOVE(PRF_FLAGS(vict), PRF_HOLYLIGHT);
        break;
    case 55:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOTITLE);
        break;
    case 56:
        GET_INVIS_LVL(vict) = RANGE(0, GET_LEVEL(vict));
        break;
    case 57:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_HALT);
        break;
    case 58:
        if (((tp = search_block(argument, logtypes, false)) == -1)) {
            send_to_char(ch,
                         "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
            return;
        }
        REMOVE_BIT(PRF_FLAGS(vict), PRF_LOG1 | PRF_LOG2);
        SET_BIT(PRF_FLAGS(vict),
                (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

        send_to_char(ch, "%s's syslog is now %s.\r\n", GET_NAME(vict),
                     logtypes[tp]);
        break;
    case 59:
        GET_BROKE(vict) = RANGE(0, NUM_COMPS);
        break;
    case 60:
        GET_TOT_DAM(vict) = RANGE(0, 1000000000);
        break;
    case 61:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_OLCGOD);
        break;
    case 62:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_TESTER);
        break;
    case 63:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_MORTALIZED);
        break;
    case 64:
        SET_OR_REMOVE(PRF2_FLAGS(vict), PRF2_NOAFFECTS);
        break;
    case 65:
        vict->player.age_adjust = (int8_t) RANGE(-125, 125);
        break;
    case 66:
        GET_CASH(vict) = RANGE(0, 1000000000);
        break;
    case 67:
        GET_REMORT_GEN(vict) = RANGE(0, 10);
        break;
    case 68:
        if (add_path_to_mob(vict, value)) {
            snprintf(buf, sizeof(buf), "%s now follows the path titled: %s.",
                     GET_NAME(vict), argument);
        } else {
            snprintf(buf, sizeof(buf), "Could not assign that path to mobile.");
        }
        break;
    case 69:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOPOST);
        break;
    case 70:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_LOG);
        slog("(GC) Logging %sactivated for %s by %s.",
             PLR_FLAGGED(vict, PLR_LOG) ? "" : "de-",
             GET_NAME(ch), GET_NAME(vict));
        break;
    case 71:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOPK);
        break;
    case 72:
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Usage: set <vict> soilage <pos> <value>\r\n");
            return;
        }
        if ((i = search_block(arg1, wear_eqpos, false)) < 0) {
            send_to_char(ch, "No such eqpos.\r\n");
            return;
        }
        CHAR_SOILAGE(vict, i) = atoi(arg2);
        break;
    case 73:                   // specialization
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);
        if (!*arg1 || !*arg2) {
            send_to_char(ch, "Usage: set <vict> spec <vnum> <level>\r\n");
            return;
        }
        value = atoi(arg1);
        tp = atoi(arg2);
        tp = MIN(MAX(0, tp), 10);
        for (i = 0; i < MAX_WEAPON_SPEC; i++) {
            if (GET_WEAP_SPEC(vict, i).vnum == value) {
                GET_WEAP_SPEC(vict, i).level = tp;
                if (tp == 0) {
                    GET_WEAP_SPEC(vict, i).vnum = 0;
                }
                send_to_char(ch, "[%d] spec level set to %d.\r\n", value, tp);
                return;
            }
        }

        send_to_char(ch, "No such spec on this person.\r\n");
        return;
    case 74:
        // qpoints
        GET_QUEST_ALLOWANCE(vict) = RANGE(0, 100);
        break;
    case 75:
        if (IS_NPC(vict)) {
            SET_OR_REMOVE(NPC_FLAGS(vict), NPC_SOULLESS);
        } else {
            SET_OR_REMOVE(PLR2_FLAGS(vict), PLR2_SOULLESS);
        }
        break;
    case 76:
        if (IS_PC(vict)) {
            SET_OR_REMOVE(PLR2_FLAGS(vict), PLR2_BURIED);
        } else {
            send_to_char(ch, "You can't really bury mobs.");
        }
        break;
    case 77:                   // Set Speed
        SPEED_OF(vict) = RANGE(0, 100);
        break;
    case 78:
        if (!argument || !*argument) {
            send_to_char(ch, "You have to specify the badge.\r\n");
            return;
        }
        if (IS_NPC(vict)) {
            send_to_char(ch,
                         "As good an idea as it might seem, it just won't work.\r\n");
            return;
        }
        if (strlen(argument) > MAX_BADGE_LENGTH) {
            send_to_char(ch,
                         "The badge must not be more than seven characters.\r\n");
            return;
        }
        strcpy_s(BADGE(vict), sizeof(BADGE(vict)), argument);
        // Convert to uppercase
        for (arg1 = BADGE(vict); *arg1; arg1++) {
            *arg1 = toupper(*arg1);
        }
        snprintf(buf, sizeof(buf), "You set %s's badge to %s", GET_NAME(vict), BADGE(vict));

        break;

    case 79:
        name = tmp_getquoted(&argument);
        arg1 = tmp_getword(&argument);
        perform_skillset(ch, vict, name, atoi(arg1));
        break;

    case 80:
        creature_set_reputation(vict, RANGE(0, 1000));
        break;
    case 81:
        arg1 = tmp_getword(&argument);
        arg2 = tmp_getword(&argument);

        if (*arg1 && *arg2) {
            i = find_tongue_idx_by_name(arg1);
            value = atoi(arg2);
            if (i == TONGUE_NONE) {
                send_to_char(ch, "%s is not a valid language.\r\n", arg1);
                return;
            } else if (!is_number(arg2) || value < 0 || value > 100) {
                send_to_char(ch,
                             "Language fluency must be a number from 0 to 100.\r\n");
                return;
            } else {
                SET_TONGUE(vict, i, value);
                snprintf(buf, sizeof(buf), "You set %s's fluency in %s to %d",
                         GET_NAME(vict), tongue_name(i), value);
            }
        } else {
            send_to_char(ch,
                         "Usage: set <vict> language <language> <fluency>\r\n");
            return;
        }
        break;
    case 82:
        SET_OR_REMOVE(PLR_FLAGS(vict), PLR_HARDCORE); break;
    case 83:
        if (is_file) {
            const char *rent_codes[] = {
                "undefined",
                "crash",
                "rented",
                "cryoed",
                "forcerented",
                "quit",
                "newchar",
                "creating",
                "remorting",
                "\n"
            };
            i = search_block(argument, rent_codes, false);
            if (i >= 0) {
                vict->player_specials->rentcode = i;
                snprintf(buf, sizeof(buf), "%s's rent code set to %s.",
                         GET_NAME(vict), rent_codes[i]);
            } else {
                send_to_char(ch, "Valid rent codes are:\r\n");
                for (i = 0; rent_codes[i][0] != '\n'; i++) {
                    send_to_char(ch, "  %s\r\n", rent_codes[i]);
                }
                return;
            }
        } else {
            send_to_char(ch, "Rentcode only makes sense when editing a player file\r\n");
            return;
        }
        break;
    default:
        snprintf(buf, sizeof(buf), "Can't set that!");
        break;
    }

    if (fields[l].type == BINARY) {
        send_to_char(ch, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
                     GET_NAME(vict));
    } else if (fields[l].type == NUMBER) {
        send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict),
                     fields[l].cmd, value);
    } else {
        send_to_char(ch, "%s\r\n", buf);
    }

    if (IS_PC(vict)) {
        save_player_to_xml(vict);
    }

    if (is_file) {
        free_creature(vict);
        send_to_char(ch, "Saved in file.\r\n");
    }
}

ACMD(do_aset)
{
    static struct set_struct fields[] = {
        {"bank_past", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"future_bank", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"reputation", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"qpoints", LVL_IMMORT, PC, NUMBER, "WizardFull"}, // once included QuestorAdmin
        {"qbanned", LVL_IMMORT, PC, BINARY, "AdminFull"}, // once included QuestorAdmin
        {"password", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"email", LVL_IMMORT, PC, MISC, "AdminFull"},
        {"banned", LVL_IMMORT, PC, BINARY, "AdminFull"},
        {"trust", LVL_IMMORT, PC, NUMBER, "AdminFull"},
        {"\n", 0, BOTH, MISC, ""}
    };
    char *name, *field;
    int i, l = 0;
    long long value = 0;
    struct account *account;
    bool on = false, off = false;

    name = tmp_getword(&argument);
    field = tmp_getword(&argument);

    if (!*name || !*field) {
        send_to_char(ch, "Usage: aset <victim> <field> <value>\r\n");
        GList *cmdlist = NULL;
        for (i = 0; fields[i].level != 0; i++) {
            if (!is_authorized(ch, ASET, &fields[i])) {
                continue;
            }
            cmdlist = g_list_prepend(cmdlist, tmp_strdup(fields[i].cmd));
        }
        cmdlist = g_list_sort(cmdlist, (GCompareFunc) strcmp);
        for (GList *it = cmdlist; it; it = it->next) {
            send_to_char(ch, "%12s%s", (char *)it->data,
                         (!(++l % 5) ? "\r\n" : ""));
        }
        if ((l - 1) % 5) {
            send_to_char(ch, "\r\n");
        }
        return;
    }

    if (*name == '.') {
        account = account_by_idnum(player_account_by_name(name + 1));
    } else if (is_number(name)) {
        account = account_by_idnum(atoi(name));
    } else {
        account = account_by_name(name);
    }

    if (!account) {
        send_to_char(ch, "There is no such account.\r\n");
        return;
    }

    for (l = 0; *(fields[l].cmd) != '\n'; l++) {
        if (!strncmp(field, fields[l].cmd, strlen(field))) {
            break;
        }
    }

    if (!is_authorized(ch, ASET, &fields[l])) {
        send_to_char(ch, "You are not able to perform this godly deed!\r\n");
        return;
    }

    if (fields[l].type == BINARY) {
        if (!strcmp(argument, "on") || !strcmp(argument, "yes")) {
            on = true;
        } else if (!strcmp(argument, "off") || !strcmp(argument, "no")) {
            off = true;
        }
        if (!(on || off)) {
            send_to_char(ch, "Value must be on or off.\r\n");
            return;
        }
    } else if (fields[l].type == NUMBER) {
        value = atoll(argument);
    }

    strcpy_s(buf, sizeof(buf), "Okay.");       /* can't use OK macro here 'cause of \r\n */

    switch (l) {
    case 0:
        account_set_past_bank(account, value);
        break;
    case 1:
        account_set_future_bank(account, value);
        break;
    case 2:
        account_set_reputation(account, value);
        break;
    case 3:
        account_set_quest_points(account, value);
        break;
    case 4:
        account_set_quest_banned(account, on);
        break;
    case 5:
        account_set_password(account, argument);
        snprintf(buf, sizeof(buf), "Password for account %s[%d] has been set.",
                 account->name, account->id);
        slog("(GC) %s set password for account %s[%d]",
             GET_NAME(ch), account->name, account->id);
        break;
    case 6:
        free(account->email);
        account->email = strdup(argument);
        snprintf(buf, sizeof(buf), "Email for account %s[%d] has been set to <%s>",
                 account->name, account->id, account->email);
        slog("(GC) %s set email for account %s[%d] to %s",
             GET_NAME(ch), account->name, account->id, account->email);
        break;
    case 7:
        account_set_banned(account, on);
        break;
    case 8:
        account_set_trust(account, value);
        break;
    default:
        snprintf(buf, sizeof(buf), "Can't set that!");
        break;
    }

    if (fields[l].type == BINARY) {
        slog("(GC) %s set %s for account %s[%d] to %s",
             GET_NAME(ch),
             fields[l].cmd, account->name, account->id, ONOFF(on));
        send_to_char(ch, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on),
                     account->name);
    } else if (fields[l].type == NUMBER) {
        slog("(GC) %s set %s for account %s[%d] to %lld",
             GET_NAME(ch), fields[l].cmd, account->name, account->id, value);
        send_to_char(ch, "%s's %s set to %lld.\r\n", account->name,
                     fields[l].cmd, value);
    } else {
        send_to_char(ch, "%s\r\n", buf);
    }
}

ACMD(do_syslog)
{
    int tp;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
              (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
        send_to_char(ch, "Your syslog is currently %s.\r\n", logtypes[tp]);
        return;
    }
    if (((tp = search_block(arg, logtypes, false)) == -1)) {
        send_to_char(ch,
                     "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
        return;
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
    SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

    send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}

ACMD(do_rlist)
{
    struct room_data *room;
    struct zone_data *zone;
    bool has_desc, stop = false;
    char *read_pt;

    int first, last, found = 0;

    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: rlist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }
    struct str_builder sb = str_builder_default;
    for (zone = zone_table; zone && !stop; zone = zone->next) {
        for (room = zone->world; room; room = room->next) {
            if (room->number > last) {
                stop = true;
                break;
            }
            if (room->number >= first) {
                has_desc = false;
                if (room->description) {
                    for (read_pt = room->description; *read_pt; read_pt++) {
                        if (!isspace(*read_pt)) {
                            has_desc = true;
                            break;
                        }
                    }
                }
                sb_sprintf(&sb, "%5d. %s[%s%5d%s]%s %s%-30s%s %s%s\r\n",
                            ++found,
                            CCGRN(ch, C_NRM),
                            CCNRM(ch, C_NRM),
                            room->number,
                            CCGRN(ch, C_NRM),
                            CCNRM(ch, C_NRM),
                            CCCYN(ch, C_NRM),
                            room->name,
                            CCNRM(ch, C_NRM),
                            has_desc ? "" : "(nodesc)", room->prog ? "(prog)" : "");
            }
        }
    }

    if (!found) {
        send_to_char(ch, "No rooms were found in those parameters.\r\n");
    } else {
        page_string(ch->desc, sb.str);
    }
}

ACMD(do_xlist)
{
    struct special_search_data *srch = NULL;
    struct room_data *room = NULL;
    struct zone_data *zone = NULL, *zn = NULL;
    int count = 0, srch_type = -1;
    bool overflow = 0, found = 0;
    char *arg;

    arg = tmp_getword(&argument);
    while (*arg) {
        if (is_abbrev(arg, "zone")) {
            arg = tmp_getword(&argument);
            if (!*arg) {
                send_to_char(ch, "No zone specified.\r\n");
                return;
            }
            if (!(zn = real_zone(atoi(arg)))) {
                send_to_char(ch, "No such zone ( %s ).\r\n", arg);
                return;
            }
        }
        if (is_abbrev(arg, "type")) {
            arg = tmp_getword(&argument);
            if (!*arg) {
                send_to_char(ch, "No type specified.\r\n");
                return;
            }
            if ((srch_type = search_block(arg, search_commands, false)) < 0) {
                send_to_char(ch, "No such search type ( %s ).\r\n", arg);
                return;
            }
        }
        if (is_abbrev(arg, "help")) {
            send_to_char(ch, "Usage: xlist [type <type>] [zone <zone>]\r\n");
            return;
        }
        arg = tmp_getword(&argument);
    }

    // If there's no zone argument, set it to the current zone.
    if (!zn) {
        zone = ch->in_room->zone;
    } else {
        zone = zn;
    }
    // can this person actually look at this zones searches?
    if (!is_authorized(ch, LIST_SEARCHES, zone)) {
        send_to_char(ch, "You aren't godly enough to do that here.\r\n");
        return;
    }

    struct str_builder sb = str_builder_default;
    sb_strcat(&sb, "Searches: \r\n", NULL);

    for (room = zone->world, found = false; room && !overflow;
         found = false, room = room->next) {

        for (srch = room->search, count = 0; srch && !overflow;
             srch = srch->next, count++) {

            if (srch_type >= 0 && srch_type != srch->command) {
                continue;
            }

            if (!found) {
                sb_sprintf(&sb, "Room [%s%5d%s]:\n", CCCYN(ch, C_NRM),
                            room->number, CCNRM(ch, C_NRM));
                found = true;
            }

            print_search_data_to_buf(ch, room, srch, buf, sizeof(buf));
            sb_strcat(&sb, buf, NULL);
        }
    }
    page_string(ch->desc, sb.str);
}

ACMD(do_mlist)
{
    int first, last, found = 0;
    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: mlist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }

    struct str_builder sb = str_builder_default;

    for (int vnum = first; vnum <= last; vnum++) {
        struct creature *mob = g_hash_table_lookup(mob_prototypes, GINT_TO_POINTER(vnum));
        if (mob) {
            sb_sprintf(&sb, "%5d. %s[%s%5d%s]%s %-40s%s  [%2d] <%3d> %s%s\r\n",
                        ++found,
                        CCGRN(ch, C_NRM),
                        CCNRM(ch, C_NRM),
                        mob->mob_specials.shared->vnum,
                        CCGRN(ch, C_NRM),
                        CCYEL(ch, C_NRM),
                        mob->player.short_descr,
                        CCNRM(ch, C_NRM),
                        mob->player.level,
                        NPC_SHARED(mob)->number,
                        NPC2_FLAGGED((mob), NPC2_UNAPPROVED) ? "(!ap)" : "",
                        GET_NPC_PROG(mob) ? "(prog)" : "");
        }
    }

    if (!found) {
        send_to_char(ch, "No mobiles were found in those parameters.\r\n");
    } else {
        page_string(ch->desc, sb.str);
    }
}

ACMD(do_olist)
{
    int first, last, found = 0;

    two_arguments(argument, buf, buf2);

    if (!*buf) {
        first = ch->in_room->zone->number * 100;
        last = ch->in_room->zone->top;
    } else if (!*buf2) {
        send_to_char(ch, "Usage: olist <begining number> <ending number>\r\n");
        return;
    } else {
        first = atoi(buf);
        last = atoi(buf2);
    }

    if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
        send_to_char(ch, "Values must be between 0 and 99999.\r\n");
        return;
    }

    if (first >= last) {
        send_to_char(ch, "Second value must be greater than first.\r\n");
        return;
    }

    struct str_builder sb = str_builder_default;


    for (int vnum = first; vnum <= last; vnum++) {
        struct obj_data *obj = g_hash_table_lookup(obj_prototypes, GINT_TO_POINTER(vnum));
        if (obj) {
            sb_sprintf(&sb, "%5d. %s[%s%5d%s]%s %-36s%s %s %s\r\n", ++found,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->shared->vnum,
                        CCGRN(ch, C_NRM), CCGRN(ch, C_NRM),
                        obj->name, CCNRM(ch, C_NRM),
                        !OBJ_APPROVED(obj) ? "(!aprvd)" : "",
                        (!(obj->line_desc) || !(*(obj->line_desc))) ? "(nodesc)" : "");
        }
    }

    if (!found) {
        send_to_char(ch, "No objects were found in those parameters.\r\n");
    } else {
        page_string(ch->desc, sb.str);
    }
}

ACMD(do_rename)
{
    struct obj_data *obj;
    struct creature *vict = NULL;
    char logbuf[MAX_STRING_LENGTH];

    char *arg = tmp_getword(&argument);

    if (!*argument) {
        send_to_char(ch, "What do you want to call it?\r\b");
        return;
    }
    if (!*arg) {
        send_to_char(ch, "Rename usage: rename <target> <string>\r\n");
        return;
    }
    if (strlen(argument) >= MAX_INPUT_LENGTH) {
        send_to_char(ch, "Desript too long.\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
        vict = get_char_room_vis(ch, arg);
        if (!vict) {
            if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
                send_to_char(ch, "No such object or mobile around.\r\n");
                return;
            }
        } else if (!IS_NPC(vict)) {
            send_to_char(ch, "You can do this only with NPC's.\r\n");
            return;
        }
    }

    if (obj) {
        snprintf(logbuf, sizeof(logbuf), "%s has renamed %s '%s'.", GET_NAME(ch),
                 obj->name, argument);
        obj->name = strdup(argument);
        snprintf(buf, sizeof(buf), "%s has been left here.", argument);
        strcpy_s(buf, sizeof(buf), CAP(buf));
        obj->line_desc = strdup(buf);
    } else if (vict) {
        snprintf(logbuf, sizeof(logbuf), "%s has renamed %s '%s'.", GET_NAME(ch),
                 GET_NAME(vict), argument);
        vict->player.short_descr = strdup(argument);
        if (GET_POSITION(vict) == POS_FLYING) {
            snprintf(buf, sizeof(buf), "%s is hovering here.", argument);
        } else {
            snprintf(buf, sizeof(buf), "%s is standing here.", argument);
        }
        strcpy_s(buf, sizeof(buf), CAP(buf));
        vict->player.long_descr = strdup(buf);
    }
    send_to_char(ch, "Okay, you do it.\r\n");
    mudlog(MAX(LVL_ETERNAL, GET_INVIS_LVL(ch)), CMP, true, "%s", logbuf);
    return;
}

ACMD(do_addname)
{
    struct obj_data *obj;
    struct creature *vict = NULL;
    char *new_name, *arg;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Addname usage: addname <target> <strings>\r\n");
        return;
    }

    arg = tmp_getword(&argument);

    if (!*argument) {
        send_to_char(ch, "What name keywords do you want to add?\r\b");
        return;
    }

    if (!(obj = get_obj_in_list_all(ch, arg, ch->carrying))) {
        vict = get_char_room_vis(ch, arg);
        if (!(vict)) {
            if (!(obj = get_obj_in_list_vis(ch, arg, ch->in_room->contents))) {
                send_to_char(ch, "No such object or mobile around.\r\n");
                return;
            }
        } else if (!IS_NPC(vict)) {
            send_to_char(ch, "You can do this only with NPC's.\r\n");
            return;
        }
    }

    for (new_name = tmp_getword(&argument);
         *new_name; new_name = tmp_getword(&argument)) {

        if (strlen(new_name) >= MAX_INPUT_LENGTH) {
            send_to_char(ch, "Name: \'%s\' too long.\r\n", new_name);
            continue;
        }
        // check to see if the name is already in the list

        if (obj) {
            if (isname_exact(new_name, obj->aliases)) {
                send_to_char(ch, "Name: \'%s\' is already an alias.\r\n",
                             new_name);
                continue;
            }
            snprintf(buf, EXDSCR_LENGTH, "%s %s", obj->aliases, new_name);
            obj->aliases = strdup(buf);
        } else if (vict) {
            if (isname_exact(new_name, vict->player.name)) {
                send_to_char(ch, "Name: \'%s\' is already an alias.\r\n",
                             new_name);
                continue;
            }

            snprintf(buf, sizeof(buf), "%s ", new_name);
            strcat_s(buf, sizeof(buf), vict->player.name);
            vict->player.name = strdup(tmp_sprintf("%s %s",
                                                   vict->player.name, new_name));
        }
    }
    send_to_char(ch, "Okay, you do it.\r\n");
    return;
}

ACMD(do_addpos)
{
    struct obj_data *obj;
    int bit = -1;
    static const char *keywords[] = {
        "take",                 /*   0   */
        "finger",
        "neck",
        "body",
        "head",
        "legs",                 /*   5   */
        "feet",
        "hands",
        "arms",
        "shield",
        "about",                /*   10  */
        "waist",
        "wrist",
        "wield",
        "hold",
        "crotch",               /*  15   */
        "eyes",
        "back",
        "belt",
        "face",
        "ear",                  /*   20   */
        "ass",
        "\n"
    };

    char *arg = tmp_getword(&argument);
    char *new_pos = tmp_getword(&argument);

    if (!*new_pos) {
        send_to_char(ch, "What new position?\r\n");
        return;
    }
    if (!*arg) {
        send_to_char(ch, "Addpos usage: addpos <target> <new pos>\r\n");
        send_to_char(ch, "Valid positions are:\r\n"
                     "take finger neck body about legs feet hands arms shield waist ass\r\n"
                     "wrist wield hold crotch eyes back belt face ear\r\n");
        return;
    }

    obj = get_obj_in_list_all(ch, arg, ch->carrying);

    if (!obj) {
        send_to_char(ch, "You carry no such thing.\r\n");
        return;
    }
    bit = search_block(new_pos, keywords, false);
    if (bit < 0) {
        send_to_char(ch, "That is not a valid position!\r\n");
        return;
    }

    bit = (1U << bit);

    TOGGLE_BIT(obj->obj_flags.wear_flags, bit);
    send_to_char(ch, "Bit %s for %s now %s.\r\n", new_pos, obj->name,
                 IS_SET(obj->obj_flags.wear_flags, bit) ? "ON" : "OFF");
    return;
}

ACMD(do_nolocate)
{
    struct obj_data *obj;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Nolocate usage: nolocate <target>\r\n");
        return;
    }

    obj = get_obj_in_list_all(ch, arg, ch->carrying);

    if (!obj) {
        send_to_char(ch, "You carry no such thing.\r\n");
        return;
    }
    TOGGLE_BIT(obj->obj_flags.extra2_flags, ITEM2_NOLOCATE);
    send_to_char(ch, "NoLocate is now %s for %s.\r\n",
                 IS_SET(obj->obj_flags.extra2_flags, ITEM2_NOLOCATE) ? "ON" : "OFF",
                 obj->name);
    return;
}

ACMD(do_mudwipe)
{
    struct obj_data *obj, *obj_tmp;
    struct creature *mob;
    struct zone_data *zone = NULL;
    struct room_data *room = NULL;
    int mode, i;
    skip_spaces(&argument);

    if (!strncasecmp(argument, "tempus clean", 12)) {
        mode = 0;
        snprintf(buf, sizeof(buf), "Mud WIPE CLEAN called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus objects", 14)) {
        mode = 1;
        snprintf(buf, sizeof(buf), "Mud WIPE objects called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus mobiles", 14)) {
        mode = 2;
        snprintf(buf, sizeof(buf), "Mud WIPE mobiles called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus fullmobs", 15)) {
        mode = 3;
        snprintf(buf, sizeof(buf), "Mud WIPE fullmobs called by %s.", GET_NAME(ch));
    } else if (!strncasecmp(argument, "tempus trails", 13)) {
        mode = 4;
        snprintf(buf, sizeof(buf), "Mud WIPE trails called by %s.", GET_NAME(ch));
    } else {
        send_to_char(ch, "Improper syntax.  Options are:\r\n"
                     "objects, mobiles, fullmobs, or clean.\r\n");
        return;
    }
    mudlog(GET_INVIS_LVL(ch), BRF, true, "%s", buf);

    if (mode == 4) {
        retire_trails();
        send_to_char(ch, "Done.  trails retired\r\n");
    }
    if (mode == 3) {
        for (GList *cit = creatures; cit; cit = cit->next) {
            mob = cit->data;
            if (!IS_NPC(mob)) {
                continue;
            }
            for (obj = mob->carrying; obj; obj = obj_tmp) {
                obj_tmp = obj->next_content;
                extract_obj(obj);
            }
            for (i = 0; i < NUM_WEARS; i++) {
                if (GET_EQ(mob, i)) {
                    extract_obj(GET_EQ(mob, i));
                }
            }
        }
        send_to_char(ch, "DONE.  Mobiles cleared of objects.\r\n");
    }
    if (mode == 3 || mode == 2 || mode == 0) {
        GList *next;
        for (GList *cit = creatures; cit; cit = next) {
            next = cit->next;
            mob = cit->data;
            if (IS_NPC(mob)) {
                creature_purge(mob, true);
            }
        }
        send_to_char(ch, "DONE.  Mud cleaned of all mobiles.\r\n");
    }
    if (mode == 0 || mode == 1) {
        for (zone = zone_table; zone; zone = zone->next) {
            for (room = zone->world; room; room = room->next) {
                if (ROOM_FLAGGED(room,
                                 ROOM_GODROOM | ROOM_CLAN_HOUSE | ROOM_HOUSE)) {
                    continue;
                }
                for (obj = room->contents; obj; obj = obj_tmp) {
                    obj_tmp = obj->next_content;
                    extract_obj(obj);
                }
            }
        }
        send_to_char(ch, "DONE.  Mud cleaned of in_room contents.\r\n");
    }
}

ACMD(do_zonepurge)
{
    struct obj_data *obj = NULL, *next_obj = NULL;
    struct zone_data *zone = NULL;
    int j, mob_count = 0, obj_count = 0;
    struct room_data *rm = NULL;

    skip_spaces(&argument);
    if (!*argument) {
        zone = ch->in_room->zone;
    } else if (*argument == '.') {
        zone = ch->in_room->zone;
    } else if (!is_number(argument)) {
        send_to_char(ch,
                     "You must specify the zone with its vnum number.\r\n");
        return;
    } else {
        j = atoi(argument);
        for (zone = zone_table; zone; zone = zone->next) {
            if (zone->number == j) {
                break;
            }
        }
    }

    if (zone) {
        for (rm = zone->world; rm; rm = rm->next) {
            if (rm->number < (zone->number * 100) || rm->number > zone->top) {
                continue;
            }
            if (ROOM_FLAGGED(rm, ROOM_GODROOM) || ROOM_FLAGGED(rm, ROOM_HOUSE)) {
                continue;
            }

            // clear all mobs
            GList *it = rm->people;
            while (it != NULL) {
                GList *next = it->next;
                struct creature *tch = it->data;

                if (IS_NPC(tch)) {
                    creature_purge(tch, true);
                    mob_count++;
                } else {
                    send_to_char(tch, "You feel a rush of heat wash over you!\r\n");
                }

                it = next;
            }

            // clear all objects
            for (obj = rm->contents; obj; obj = next_obj) {
                next_obj = obj->next_content;
                extract_obj(obj);
                obj_count++;
            }

        }

        send_to_char(ch, "Zone %d cleared of %d mobiles.\r\n", zone->number,
                     mob_count);
        send_to_char(ch, "Zone %d cleared of %d objects.\r\n", zone->number,
                     obj_count);

        snprintf(buf, sizeof(buf), "(GC) %s %spurged zone %d (%s)", GET_NAME(ch),
                 subcmd == SCMD_OLC ? "olc-" : "", zone->number, zone->name);
        if (subcmd != SCMD_OLC) {
            mudlog(GET_INVIS_LVL(ch),
                   subcmd == SCMD_OLC ? CMP : NRM, true, "%s", buf);
        } else {
            slog("%s", buf);
        }

    } else {
        send_to_char(ch, "Invalid zone number.\r\n");
    }
}

ACMD(do_searchfor)
{
    struct creature *mob = NULL;
    struct obj_data *obj = NULL;
    int8_t mob_found = false, obj_found = false;

    for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
        mob = cit->data;
        if (!mob->in_room) {
            mob_found = true;
            send_to_char(ch, "Char out of room: %s,  Master is: %s.\r\n",
                         GET_NAME(mob), mob->master ? GET_NAME(mob->master) : "N");
        }
    }
    for (obj = object_list; obj; obj = obj->next) {
        if (!obj->in_room && !obj->carried_by && !obj->worn_by && !obj->in_obj) {
            obj_found = true;

#ifdef TRACK_OBJS
            snprintf(buf, sizeof(buf), "Object: %s [%s] %s", obj->name,
                     obj->obj_flags.tracker.string,
                     ctime(&obj->obj_flags.tracker.lost_time));
#else
            send_to_char(ch, "Object: %s\r\n", obj->name);
#endif
        }
    }
    if (!mob_found) {
        send_to_char(ch, "No mobs out of room.\r\n");
    }
    if (!obj_found) {
        send_to_char(ch, "No objects out of room.\r\n");
    }
}

#define OSET_FORMAT "Usage: oset <object> <parameter> <value>.\r\n"

ACMD(do_oset)
{
    struct obj_data *obj = NULL;
    struct creature *vict = NULL;
    int where_worn = 0;
    char arg1[MAX_INPUT_LENGTH];
    int equip_mode = EQUIP_WORN;

    if (!*argument) {
        send_to_char(ch, OSET_FORMAT);
        return;
    }
    argument = one_argument(argument, arg1);

    if (!(obj = get_obj_vis(ch, arg1))) {
        send_to_char(ch, "No such object around.\r\n");
        return;
    }

    if ((vict = obj->worn_by)) {
        where_worn = obj->worn_on;
        if (obj == GET_EQ(obj->worn_by, obj->worn_on)) {
            equip_mode = EQUIP_WORN;
        } else if (obj == GET_IMPLANT(obj->worn_by, obj->worn_on)) {
            equip_mode = EQUIP_IMPLANT;
        } else if (obj == GET_TATTOO(obj->worn_by, obj->worn_on)) {
            equip_mode = EQUIP_TATTOO;
        }
        unequip_char(vict, where_worn, equip_mode);
    }
    perform_oset(ch, obj, argument, NORMAL_OSET);
    if (vict) {
        if (equip_char(vict, obj, where_worn, equip_mode)) {
            return;
        }
        if (!IS_NPC(vict)) {
            crashsave(vict);
        }
    }

}

ACMD(do_xlag)
{
    int raw = 0;
    struct creature *vict = NULL;
    char *vict_str = tmp_getword(&argument);
    char *amt_str = tmp_getword(&argument);

    if (!*vict_str || !*amt_str) {
        send_to_char(ch, "Usage: xlag <victim> <0.10 seconds>\r\n");
        return;
    }
    if (!(vict = get_char_vis(ch, vict_str))) {
        send_to_char(ch, "No-one by that name here.\r\n");
        return;
    }
    if ((raw = atoi(amt_str)) <= 0) {
        send_to_char(ch, "Real funny xlag time, buddy.\r\n");
        return;
    }
    if (ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch)) {
        send_to_char(ch, "Nice try, sucker.\r\n");
        GET_WAIT(ch) = raw;
        return;
    }

    GET_WAIT(vict) = raw;
    send_to_char(ch, "%s lagged %d cycles.\r\n", GET_NAME(vict), raw);
}

ACMD(do_peace)
{
    bool found = 0;

    for (GList *it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        found = 1;
        remove_all_combat(tch);
    }

    if (!found) {
        send_to_char(ch, "No-one here is fighting!\r\n");
    } else {
        send_to_char(ch, "You have brought peace to this room.\r\n");
        act("A feeling of peacefulness descends on the room.",
            false, ch, NULL, NULL, TO_ROOM);
    }
}

ACMD(do_severtell)
{
    struct creature *vict = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Sever reply of what character?\r\n");
        return;
    }

    if (!(vict = get_char_vis(ch, argument))) {
        send_to_char(ch, "Nobody by that name around here.\r\n");
        return;
    }

    if (GET_IDNUM(ch) != GET_LAST_TELL_FROM(vict)
        && GET_IDNUM(ch) != GET_LAST_TELL_TO(vict)) {
        send_to_char(ch,
                     "You are not on that person's reply buffer, pal.\r\n");
    } else {
        GET_LAST_TELL_FROM(vict) = -1;
        GET_LAST_TELL_TO(vict) = -1;
        send_to_char(ch, "Reply severed.\r\n");
    }
}

ACMD(do_qpreload)
{

    qp_reload(0);
    send_to_char(ch, "Reloading Quest Points.\r\n");
    mudlog(LVL_GRGOD, NRM, true, "(GC) %s has reloaded QP", GET_NAME(ch));

}

const char *CODER_UTIL_USAGE =
    "Usage: coderutil <command> <args>\r\n"
    "Commands: \r\n"
    "      tick - forces a mud-wide tick to occur.\r\n"
    "      hour - forces an hour of mud-time to occur.\r\n"
    "      recalc - recalculates all mobs and saves.\r\n"
    "    cmdusage - shows commands and usage counts.\r\n"
    "  unusedcmds - shows unused commands.\r\n"
    "      verify - run tempus integrity check.\r\n"
    "       chaos - makes every mob berserk.\r\n"
    "    loadzone - load zone files for zone.\r\n"
    "	 progstat - stat the progs for a room or mob.\r\n"
    "  progscript - translate a script to a prog.\r\n"
    "	  progall - translate all scripts to progs\r\n"
    "   dumpprogs - dumps state of all active progs\r\n";

bool
load_single_zone(int zone_num)
{
    void discrete_load(FILE *fl, int mode);
    void load_zones(FILE *fl, char *zonename);
    void reset_zone(struct zone_data *zone);
    void assign_mobiles(void);
    void assign_objects(void);
    void assign_rooms(void);
    void assign_artisans(void);
    void compile_all_progs(void);
    void renum_world(void);
    void renum_zone_table(void);

    FILE *db_file;
    struct zone_data *zone;

    db_file = fopen(tmp_sprintf("world/zon/%d.zon", zone_num), "r");
    if (db_file) {
        load_zones(db_file, tmp_sprintf("zon/%d.zon", zone_num));
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/wld/%d.wld", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_WLD);
        renum_world();
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/mob/%d.mob", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_MOB);
        fclose(db_file);
    }
    db_file = fopen(tmp_sprintf("world/obj/%d.obj", zone_num), "r");
    if (db_file) {
        discrete_load(db_file, DB_BOOT_OBJ);
        fclose(db_file);
    }
    zone = real_zone(zone_num);
    if (zone) {
        reset_zone(zone);
        renum_zone_table();
        compile_all_progs();
        assign_mobiles();
        assign_objects();
        assign_rooms();
        assign_artisans();
        return true;
    } else {
        return false;
    }
}

ACMD(do_coderutil)
{
    int idx, cmd_num;
    char *token;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "%s", CODER_UTIL_USAGE);
        return;
    }
    token = tmp_getword(&argument);

    if (strcmp(token, "tick") == 0) {
        point_update();
        send_to_char(ch, "*tick*\r\n");
    } else if (strcmp(token, "hour") == 0) {
        ACMD(do_time);
        weather_and_time();
        do_time(ch, "", 0, 0);
    } else if (strcmp(token, "sunday") == 0) {
        last_sunday_time = time(NULL);
        send_to_char(ch, "Ok, it's now Sunday (kinda).\r\n");
    } else if (strcmp(token, "recalc") == 0) {
        token = tmp_getword(&argument);
        recalc_all_mobs(ch, token);
    } else if (strcmp(token, "cmdusage") == 0) {
        struct str_builder sb = str_builder_default;
        for (idx = 1; idx < num_of_cmds; idx++) {
            cmd_num = cmd_sort_info[idx].sort_pos;
            if (!cmd_info[cmd_num].usage) {
                continue;
            }

            sb_sprintf(&sb, "%-15s %7lu   ",
                        cmd_info[cmd_num].command,
                        cmd_info[cmd_num].usage);
            if (idx % 3 == 0) {
                sb_strcat(&sb, "\r\n", NULL);
            }
        }
        sb_strcat(&sb, "\r\n", NULL);
        page_string(ch->desc, sb.str);
    } else if (strcmp(token, "unusedcmds") == 0) {
        struct str_builder sb = str_builder_default;
        for (idx = 1; idx < num_of_cmds; idx++) {
            cmd_num = cmd_sort_info[idx].sort_pos;
            if (cmd_info[cmd_num].usage) {
                continue;
            }

            sb_sprintf(&sb, "%-16s", cmd_info[cmd_num].command);
            if (!(idx % 5)) {
                sb_strcat(&sb, "\r\n", NULL);
            }
        }
        sb_strcat(&sb, "\r\n", NULL);
        page_string(ch->desc, sb.str);
    } else if (strcmp(token, "verify") == 0) {
        verify_tempus_integrity(ch);
    } else if (strcmp(token, "chaos") == 0) {
        for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
            struct creature *tch = cit->data;
            struct creature *attacked;
            int old_mob2_flags = NPC2_FLAGS(tch);

            NPC2_FLAGS(tch) |= NPC2_ATK_MOBS;
            perform_barb_berserk(tch, &attacked);
            NPC2_FLAGS(tch) = old_mob2_flags;
        }
        send_to_char(ch, "The entire world goes mad...\r\n");
        slog("%s has doomed the world to chaos.", GET_NAME(ch));
    } else if (strcmp(token, "loadzone") == 0) {
        int zone_num;
        struct zone_data *zone;

        token = tmp_getword(&argument);
        if (!is_number(token)) {
            send_to_char(ch, "You must specify a zone number.\r\n");
            return;
        }
        zone_num = atoi(token);
        zone = real_zone(zone_num);
        if (zone) {
            send_to_char(ch, "Zone %d is already loaded!\r\n", zone_num);
            return;
        }

        if (load_single_zone(zone_num)) {
            send_to_char(ch, "Zone %d loaded and reset.\r\n", zone_num);
        } else {
            send_to_char(ch, "Zone could not be found or reset.\r\n");
        }
    } else if (strcmp(token, "progstat") == 0) {
        void *owner = NULL;
        extern GList *active_progs;
        extern gint prog_tick;
        unsigned char *prog_get_obj(void *owner,
                                    enum prog_evt_type owner_type);

        token = tmp_getword(&argument);
        if (!strcmp(token, "room")) {
            owner = ch->in_room;
        } else {
            owner = get_char_room_vis(ch, token);
            if (!owner) {
                owner = get_char_vis(ch, token);
            }
            if (!owner) {
                send_to_char(ch, "You can't find any '%s'\r\n", token);
                return;
            }
        }
        struct str_builder sb = str_builder_default;
        sb_sprintf(&sb, "progid    trigger   target          wait statement\r\n");
        sb_sprintf(&sb, "--------- --------- --------------- ---- ----------------------------\r\n");
        const char *prog_event_kind_desc[] =
        { "command", "idle", "fight", "give", "enter", "leave", "load",
          "tick", "spell", "combat", "death", "dying" };

        for (GList *cur = active_progs; cur; cur = cur->next) {
            struct prog_env *prog = cur->data;
            if (prog->owner == owner) {
                unsigned char *exec =
                    prog_get_obj(prog->owner, prog->owner_type);
                int cmd = *((short *)(exec + prog->exec_pt));
                int arg_addr =
                    *((short *)(exec + prog->exec_pt + sizeof(short)));
                sb_sprintf(&sb, "0x%lx %-8s  %-15s %4d %s %s\r\n",
                            (unsigned long)prog,
                            prog_event_kind_desc[(int)prog->evt.kind],
                            (prog->target) ? GET_NAME(prog->target) : "<none>",
                            prog->next_tick - prog_tick, prog_cmds[cmd].str, (char *)exec + arg_addr);
            }
        }

        page_string(ch->desc, sb.str);
    } else if (strcmp(token, "dumpprogs") == 0) {
        extern GList *active_progs;
        extern gint prog_tick;
        unsigned char *prog_get_obj(void *owner,
                                    enum prog_evt_type owner_type);
        char *prog_get_desc(struct prog_env *env);
        FILE *ouf;

        ouf = fopen("progdump.txt", "w");
        if (ouf == NULL) {
            send_to_char(ch, "Couldn't open file.\r\n");
            return;
        }
        fprintf(ouf, "owner        progid    trigger   target          wait statement\r\n");
        fprintf(ouf, "------------ --------- --------- --------------- ---- ----------------------------\r\n");
        const char *prog_event_kind_desc[] =
        { "command", "idle", "fight", "give", "enter", "leave", "load",
          "tick", "spell", "combat", "death", "dying" };

        for (GList *cur = active_progs; cur; cur = cur->next) {
            struct prog_env *prog = cur->data;
            unsigned char *exec =
                prog_get_obj(prog->owner, prog->owner_type);
            int cmd = *((short *)(exec + prog->exec_pt));
            int arg_addr =
                *((short *)(exec + prog->exec_pt + sizeof(short)));
            fprintf(ouf, "%12s 0x%lx %-8s  %-15s %4d %s %s\r\n",
                    prog_get_desc(prog),
                    (unsigned long)prog,
                    prog_event_kind_desc[(int)prog->evt.kind],
                    (prog->target) ? GET_NAME(prog->target) : "<none>",
                    prog->next_tick - prog_tick, prog_cmds[cmd].str, (char *)exec + arg_addr);
        }
        fclose(ouf);
        send_to_char(ch, "Dumped.\r\n");
    } else {
        send_to_char(ch, "%s", CODER_UTIL_USAGE);
    }
}

const char *ACCOUNT_USAGE =
    "Usage: account <command> <args>\r\n"
    "Commands: \r\n"
    "      disable <id>\r\n"
    "      enable <id>\r\n"
    "      movechar <Char ID> <to ID>\r\n"
    "      exhume <account ID> <Character ID>\r\n"
    "      reload <account ID>\r\n";
ACMD(do_account)
{
    char *token;
    int account_id = 0;
    long vict_id = 0;
    struct account *account = NULL;
    struct creature *vict;

    token = tmp_getword(&argument);
    if (!token) {
        send_to_char(ch, "%s", ACCOUNT_USAGE);
        return;
    }

    if (strcmp(token, "disable") == 0) {
        send_to_char(ch, "Not Implemented.\r\n");
    } else if (strcmp(token, "enable") == 0) {
        send_to_char(ch, "Not Implemented.\r\n");
    } else if (strcmp(token, "movechar") == 0) {
        struct account *dst_account;

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Specify a character id.\r\n");
            return;
        }
        vict_id = atol(token);
        if (!player_idnum_exists(vict_id)) {
            send_to_char(ch, "That player does not exist.\r\n");
            return;
        }
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Specify an account id.\r\n");
            return;
        }

        account_id = player_account_by_idnum(vict_id);
        account = account_by_idnum(account_id);

        account_id = atoi(token);
        dst_account = account_by_idnum(account_id);
        if (!dst_account) {
            send_to_char(ch, "That account does not exist.\r\n");
            return;
        }
        account_move_char(account, vict_id, dst_account);

        // Update descriptor
        vict = get_char_in_world_by_idnum(vict_id);
        if (vict && vict->desc) {
            vict->desc->account = dst_account;
        }

        send_to_char(ch,
                     "%s[%ld] has been moved from account %s[%d] to %s[%d]\r\n",
                     player_name_by_idnum(vict_id), vict_id, account->name, account->id,
                     dst_account->name, dst_account->id);
    } else if (strcmp(token, "exhume") == 0) {
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Specify an account id.\r\n");
            return;
        }
        account_id = atoi(token);

        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Specify a character id.\r\n");
            return;
        }
        vict_id = atol(token);
        account = account_by_idnum(account_id);
        if (account == NULL) {
            send_to_char(ch, "No such account: %d\r\n", account_id);
            return;
        }
        account_exhume_char(account, ch, vict_id);
    } else if (strcmp(token, "reload") == 0) {
        token = tmp_getword(&argument);
        if (!token) {
            send_to_char(ch, "Specify an account id.\r\n");
            return;
        }

        account_id = atoi(token);
        account = account_by_idnum(account_id);
        if (!account) {
            send_to_char(ch, "No such account: %s\r\n", token);
            return;
        }

        if (account_reload(account)) {
            send_to_char(ch,
                         "Account successfully reloaded from db\r\n");
        } else {
            send_to_char(ch,
                         "Error: Account could not be reloaded\r\n");
        }
        return;
    } else {
        send_to_char(ch, "%s", ACCOUNT_USAGE);
    }
}

ACMD(do_tester)
{
    ACMD(do_gen_tog);
    void do_start(struct creature *ch, int mode);

    static const char *TESTER_UTIL_USAGE =
        "Options are:\r\n"
        "    advance <level>\r\n"
        "    unaffect\r\n"
        "    reroll\r\n"
        "    stat\r\n"
        "    goto\r\n"
        "    restore\r\n"
        "    class <char_class>\r\n"
        "    race <race>\r\n"
        "    remort <char_class>\r\n"
        "    maxhit <value>\r\n"
        "    maxmana <value>\r\n"
        "    maxmove <value>\r\n"
        "    nohassle\r\n"
        "    roomflags\r\n"
        "    align\r\n"
        "    generation\r\n"
        "    debug\r\n"
        "    loadroom <val>\r\n"
        "    hunger|thirst|drunk <val>|off\r\n"
        "    str|con|int|wis|dex|cha <val>\r\n";

    static const char *tester_cmds[] = {
        "advance",
        "unaffect",
        "reroll",
        "stat",
        "goto",
        "restore",              /* 5 */
        "class",
        "race",
        "remort",
        "maxhit",
        "maxmana",              /* 10 */
        "maxmove",
        "nohassle",
        "roomflags",
        "align",
        "generation",
        "debug",
        "str",
        "int",
        "wis",
        "con",
        "dex",
        "cha",
        "maxstat",
        "hunger",
        "thirst",
        "drunk",
        "loadroom",
        "\n"
    };

    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int8_t tcmd;
    int i;

    if (!is_tester(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        send_to_char(ch, "You are not a tester.\r\n");
        return;
    }

    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Test what?\r\n");
        send_to_char(ch, "%s", TESTER_UTIL_USAGE);
        return;
    }

    if ((tcmd = search_block(arg1, tester_cmds, false)) < 0) {
        send_to_char(ch, "%s", TESTER_UTIL_USAGE);
        return;
    }

    switch (tcmd) {
    case 0:                    /* advance */
        if (!*arg2) {
            send_to_char(ch, "Advance to what level?\r\n");
        } else if (!is_number(arg2)) {
            send_to_char(ch, "The argument must be a number.\r\n");
        } else if ((i = atoi(arg2)) <= 0) {
            send_to_char(ch, "That's not a level!\r\n");
        } else if (i >= LVL_AMBASSADOR) {
            send_to_char(ch, "Advance: I DON'T THINK SO!\r\n");
        } else {
            if (i < GET_LEVEL(ch)) {
                do_start(ch, true);
                GET_LEVEL(ch) = i;
            }

            send_to_char(ch,
                         "Your body vibrates for a moment.... you feel different!\r\n");
            gain_exp_regardless(ch, exp_scale[i] - GET_EXP(ch));

            for (i = 1; i < MAX_SKILLS; i++) {
                if (spell_info[i].min_level[(int)GET_CLASS(ch)] <=
                    GET_LEVEL(ch)
                    || (IS_REMORT(ch)
                        && spell_info[i].
                        min_level[(int)GET_REMORT_CLASS(ch)] <=
                        GET_LEVEL(ch))) {
                    SET_SKILL(ch, i, LEARNED(ch));
                }
            }
            GET_HIT(ch) = GET_MAX_HIT(ch);
            GET_MANA(ch) = GET_MAX_MANA(ch);
            GET_MOVE(ch) = GET_MAX_MOVE(ch);
            crashsave(ch);
        }
        break;
    case 1:                    /* unaffect */
        perform_unaffect(ch, ch);
        break;
    case 2:                    /* reroll */
        perform_reroll(ch, ch);
        break;
    case 3:                    /* stat */
        do_stat(ch, arg2, 0, 0);
        break;
    case 4:                    /* goto */
        do_goto(ch, arg2, 0, 0);
        break;
    case 5:                    /* restore */
        GET_HIT(ch) = GET_MAX_HIT(ch);
        GET_MANA(ch) = GET_MAX_MANA(ch);
        GET_MOVE(ch) = GET_MAX_MOVE(ch);
        send_to_char(ch, "You are fully healed!\r\n");
        break;
    case 6:                    /* char_class  */
    case 7:                    /* race   */
    case 8:                    /* remort */
    case 9:                    /* maxhit */
    case 10:                   /* maxmana */
    case 11:                   /* maxmove */
        snprintf(buf, sizeof(buf), "self %s %s", arg1, arg2);
        do_set(ch, buf, 0, SCMD_TESTER_SET);
        break;
    case 12:
        do_gen_tog(ch, tmp_strdup(""), CMD_TESTER, SCMD_NOHASSLE);
        break;
    case 13:
        do_gen_tog(ch, tmp_strdup(""), CMD_TESTER, SCMD_ROOMFLAGS);
        break;
    case 14:
        if (!*arg2) {
            send_to_char(ch, "Set align to what?\r\n");
        } else {
            GET_ALIGNMENT(ch) = atoi(arg2);
            send_to_char(ch, "Align set to %d.\r\n", GET_ALIGNMENT(ch));
        }
        break;
    case 15:
        if (!*arg2) {
            send_to_char(ch, "Set gen to what?\r\n");
        } else {
            GET_REMORT_GEN(ch) = MAX(0, MIN(10, atoi(arg2)));
            send_to_char(ch, "gen set to %d.\r\n", GET_REMORT_GEN(ch));
        }
        break;
    case 16:
        do_gen_tog(ch, tmp_strdup(""), CMD_TESTER, SCMD_DEBUG);
        break;
    case 17:                   // strength
    case 18:                   // intelligence
    case 19:                   // wisdom
    case 20:                   // constitution
    case 21:                   // dexterity
    case 22:                   // charisma
    case 24:                   // Hunger
    case 25:                   // Thirst
    case 26:                   // Drunk
    case 27:                   // Loadroom
        do_set(ch, tmp_sprintf("self %s %s", arg1, arg2), 0,
               SCMD_TESTER_SET);
        break;
    case 23:                   // Max Stats
        do_set(ch, tmp_strdup("self str 50"), 0, SCMD_TESTER_SET);
        do_set(ch, tmp_strdup("self int 50"), 0, SCMD_TESTER_SET);
        do_set(ch, tmp_strdup("self wis 50"), 0, SCMD_TESTER_SET);
        do_set(ch, tmp_strdup("self con 50"), 0, SCMD_TESTER_SET);
        do_set(ch, tmp_strdup("self dex 50"), 0, SCMD_TESTER_SET);
        do_set(ch, tmp_strdup("self cha 50"), 0, SCMD_TESTER_SET);
        break;
    default:
        snprintf(buf, sizeof(buf), "$p: Invalid command '%s'.", arg1);
        send_to_char(ch, "%s", TESTER_UTIL_USAGE);
        break;
    }
    return;
}

int
do_freeze_char(char *argument, struct creature *vict, struct creature *ch)
{
    static const int ONE_MINUTE = 60;
    static const int ONE_HOUR = 3600;
    static const int ONE_DAY = 86400;
    static const int ONE_YEAR = 31536000;

    if (ch == vict) {
        send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
        return 0;
    }

    char *thaw_time_str = tmp_getword(&argument);
    time_t now = time(NULL);
    time_t thaw_time;
    const char *msg = "";

    if (!*thaw_time_str) {
        thaw_time = now + ONE_DAY;
        msg = tmp_sprintf("%s frozen for 1 day", vict->player.name);
    } else if (isname_exact(thaw_time_str, "forever")) {
        thaw_time = -1;
        msg = tmp_sprintf("%s frozen forever", vict->player.name);
    } else {
        int amt = atoi(thaw_time_str);
        switch (thaw_time_str[strlen(thaw_time_str) - 1]) {
        case 's':
            thaw_time = now + amt;
            msg = tmp_sprintf("%s frozen for %d second%s",
                              vict->player.name, amt, (amt == 1) ? "" : "s");
            break;
        case 'm':
            thaw_time = now + (amt * ONE_MINUTE);
            msg = tmp_sprintf("%s frozen for %d minute%s",
                              vict->player.name, amt, (amt == 1) ? "" : "s");
            break;
        case 'h':
            thaw_time = now + (amt * ONE_HOUR);
            msg = tmp_sprintf("%s frozen for %d hour%s",
                              vict->player.name, amt, (amt == 1) ? "" : "s");
            break;
        case 'd':
            thaw_time = now + (amt * ONE_DAY);
            msg = tmp_sprintf("%s frozen for %d day%s",
                              vict->player.name, amt, (amt == 1) ? "" : "s");
            break;
        default:
            send_to_char(ch,
                         "Usage: freeze <target> [ XXXs | XXXm | XXXh | XXXd | forever]\r\n");
            return false;
        }

        if ((thaw_time - now) >= ONE_YEAR) {
            send_to_char(ch, "Come now, Let's be reasonable, shall we?\r\n");
            return false;
        }
    }

    SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
    GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
    vict->player_specials->thaw_time = thaw_time;
    vict->player_specials->freezer_id = GET_IDNUM(ch);

    send_to_char(vict, "A bitter wind suddenly rises and drains every erg "
                 "of heat from your body!\r\nYou feel frozen!\r\n");

    send_to_char(ch, "%s\r\n", msg);
    mudlog(MAX(LVL_POWER, GET_INVIS_LVL(ch)), BRF, true, "(GC) %s by %s",
           msg, GET_NAME(ch));

    if (vict->in_room) {
        act("A sudden cold wind conjured from nowhere freezes $N!", false,
            ch, NULL, vict, TO_ROOM);
    }

    return true;
}

#define USERS_USAGE \
    "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c char_claslist] [-p]\r\n"

ACMD(do_users)
{
    long find_char_class_bitvector(char arg);

    char *name_search = NULL, *host_search = NULL;
    const char *char_name, *account_name, *state;
    struct creature *tch;
    struct descriptor_data *d;
    int low = 0, high = LVL_GRIMP, i, num_can_see = 0;
    int showchar_class = 0, playing = 0, deadweight = 0;
    char *arg;
    char timebuf[27], idletime[10];

    while (*(arg = tmp_getword(&argument)) != '\0') {
        if (*arg == '-') {
            switch (*(arg + 1)) {
            case 'k':
                playing = 1;
                break;
            case 'p':
                playing = 1;
                break;
            case 'd':
                deadweight = 1;
                break;
            case 'l':
                playing = 1;
                arg = tmp_getword(&argument);
                sscanf(arg, "%d-%d", &low, &high);
                break;
            case 'n':
                playing = 1;
                name_search = tmp_getword(&argument);
                break;
            case 'h':
                playing = 1;
                host_search = tmp_getword(&argument);
                break;
            case 'c':
                playing = 1;
                arg = tmp_getword(&argument);
                for (i = 0; i < (int)strlen(arg); i++) {
                    showchar_class |= find_char_class_bitvector(arg[i]);
                }
                break;
            default:
                send_to_char(ch, USERS_USAGE);
                return;
            }                   /* end of switch */

        } else {                /* endif */
            send_to_char(ch, USERS_USAGE);
            return;
        }
    }                           /* end while (parser) */

    struct str_builder sb = str_builder_default;
    sb_strcat(&sb, " Num Account      Character     State          Idl Login@   Site\r\n",
        " --- ------------ ------------- --------------- -- -------- ---------------\r\n",
        NULL);

    for (d = descriptor_list; d; d = d->next) {
        if (IS_PLAYING(d) && playing) {
            continue;
        }
        if (STATE(d) == CXN_PLAYING && deadweight) {
            continue;
        }
        if (STATE(d) == CXN_PLAYING) {
            if (d->original) {
                tch = d->original;
            } else if (!(tch = d->creature)) {
                continue;
            }
            if (host_search && !strstr(d->host, host_search)) {
                continue;
            }
            if (name_search && strcasecmp(GET_NAME(tch), name_search)) {
                continue;
            }
            if (GET_LEVEL(ch) < LVL_LUCIFER) {
                if (!can_see_creature(ch, tch) || GET_LEVEL(tch) < low
                    || GET_LEVEL(tch) > high) {
                    continue;
                }
            }
            if (showchar_class && !(showchar_class & (1U << GET_CLASS(tch)))) {
                continue;
            }
            if (GET_LEVEL(ch) < LVL_LUCIFER &&
                GET_INVIS_LVL(ch) > GET_LEVEL(ch)) {
                continue;
            }
            if (!can_see_creature(ch, d->creature)) {
                continue;
            }
        }

        strftime(timebuf, 24, "%H:%M:%S", localtime(&d->login_time));
        if (STATE(d) == CXN_PLAYING && d->original) {
            state = "switched";
        } else {
            state = strlist_aref(STATE(d), desc_modes);
        }

        if (d->creature && STATE(d) == CXN_PLAYING &&
            (GET_LEVEL(d->creature) < GET_LEVEL(ch)
             || GET_LEVEL(ch) >= LVL_LUCIFER)) {
            snprintf(idletime, sizeof(idletime), "%2d",
                     d->creature->char_specials.timer * SECS_PER_MUD_HOUR /
                     SECS_PER_REAL_MIN);
        } else {
            snprintf(idletime, sizeof(idletime), "%2d", d->idle);
        }

        if (!d->creature) {
            char_name = "      -      ";
        } else if (d->original) {
            char_name = tmp_sprintf("%s%-13s%s",
                                    CCCYN(ch, C_NRM), d->original->player.name, CCNRM(ch, C_NRM));
        } else if (IS_IMMORT(d->creature)) {
            char_name = tmp_sprintf("%s%-13s%s",
                                    CCGRN(ch, C_NRM), d->creature->player.name, CCNRM(ch, C_NRM));
        } else {
            char_name = tmp_sprintf("%-13s", d->creature->player.name);
        }

        if (d->account) {
            account_name = tmp_substr(d->account->name, 0, 12);
        } else {
            account_name = "   -   ";
        }

        sb_sprintf(&sb, "%4d %-12s %s %-15s %-2s %-8s ",
                    d->desc_num, account_name, char_name, state, idletime, timebuf);

        if (*d->host) {
            if (!isbanned(d->host, buf, sizeof(buf))) {
                sb_strcat(&sb, CCGRN(ch, C_NRM), NULL);
            } else if (d->creature && PLR_FLAGGED(d->creature, PLR_SITEOK)) {
                sb_strcat(&sb, CCMAG(ch, C_NRM), NULL);
            } else {
                sb_strcat(&sb, CCRED(ch, C_NRM), NULL);
            }
            sb_sprintf(&sb, "%s%s\r\n", d->host, CCNRM(ch, C_NRM));
        } else {
            sb_strcat(&sb, CCRED_BLD(ch, C_SPR), "[unknown]\r\n", CCNRM(ch, C_SPR), NULL);
        }

        num_can_see++;
    }

    sb_sprintf(&sb, "\r\n%d visible sockets connected.\r\n", num_can_see);
    page_string(ch->desc, sb.str);
}

ACMD(do_edit)
{
    char *arg;

    if (GET_LEVEL(ch) < LVL_GOD) {
        send_to_char(ch, "You don't see anything you can edit.\r\n");
        return;
    }

    arg = tmp_getword(&argument);

    if (isname(arg, "bugs")) {
        act("$n begins to edit the bug file.", false, ch, NULL, NULL, TO_ROOM);
        start_editing_file(ch->desc, BUG_FILE);
    } else if (isname(arg, "typos")) {
        act("$n begins to edit the typo file.", false, ch, NULL, NULL, TO_ROOM);
        start_editing_file(ch->desc, TYPO_FILE);
    } else if (isname(arg, "ideas")) {
        act("$n begins to edit the idea file.", false, ch, NULL, NULL, TO_ROOM);
        start_editing_file(ch->desc, IDEA_FILE);
    } else {
        send_to_char(ch, "Edit what?!?\r\n");
    }

    return;
}

ACMD(do_delete)
{
    char *name;
    struct creature *vict;
    struct account *acct;
    int vict_id, acct_id;
    bool from_file;
    char *tmp_name;

    name = tmp_getword(&argument);
    if (!is_named_role_member(ch, "AdminFull")
        && !is_named_role_member(ch, "WizardFull")) {
        send_to_char(ch, "No way!  You're not authorized!\r\n");
        mudlog(GET_INVIS_LVL(ch), BRF, true, "%s denied deletion of '%s'",
               GET_NAME(ch), name);
        return;
    }

    if (!*name) {
        send_to_char(ch, "Come, come.  Don't you want to delete SOMEONE?\r\n");
        return;
    }

    if (!player_name_exists(name)) {
        send_to_char(ch, "That player does not exist.\r\n");
        return;
    }

    acct_id = player_account_by_name(name);
    acct = account_by_idnum(acct_id);
    if (!acct) {
        errlog("Victim found without account");
        send_to_char(ch, "The command mysteriously failed (XYZZY)\r\n");
        return;
    }

    vict_id = player_idnum_by_name(name);
    vict = get_char_in_world_by_idnum(vict_id);
    from_file = !vict;
    if (from_file) {
        vict = load_player_from_xml(vict_id);
        if (!vict) {
            send_to_char(ch, "Error loading char from file.\r\n");
            return;
        }
    }

    tmp_name = tmp_strdup(GET_NAME(vict));

    account_delete_char(acct, vict);
    account_reload(acct);

    send_to_char(ch, "Character '%s' has been deleted from account %s.\r\n",
                 tmp_name, acct->name);
    mudlog(LVL_IMMORT, NRM, true,
           "%s has deleted %s[%d] from account %s[%d]",
           GET_NAME(ch), tmp_name, vict_id, acct->name, acct->id);

    if (from_file) {
        free_creature(vict);
    }
}

ACMD(do_badge)
{
    skip_spaces(&argument);

    if (IS_NPC(ch)) {
        send_to_char(ch, "You've got no badge.\r\n");
    } else if (strlen(argument) > MAX_BADGE_LENGTH) {
        send_to_char(ch,
                     "Sorry, badges can't be longer than %d characters.\r\n",
                     MAX_BADGE_LENGTH);
    } else {
        strcpy_s(BADGE(ch), sizeof(BADGE(ch)), argument);
        // Convert to uppercase
        for (char *cp = BADGE(ch); *cp; cp++) {
            *cp = toupper(*cp);
        }
        send_to_char(ch, "Okay, your badge is now %s.\r\n", BADGE(ch));
    }
}

void
check_log(struct creature *ch, const char *fmt, ...)
{
#define MAX_FRAMES 10
    va_list args;
    const char *backtrace_str = "";
    void *ret_addrs[MAX_FRAMES + 1];
    int x = 0;
    char *msg;

    va_start(args, fmt);
    msg = tmp_vsprintf(fmt, args);
    va_end(args);

    if (ch) {
        send_to_char(ch, "%s\r\n", msg);
    }

    memset(ret_addrs, 0x0, sizeof(ret_addrs));
    backtrace(ret_addrs, MAX_FRAMES);

    while (x < MAX_FRAMES && ret_addrs[x]) {
        backtrace_str = tmp_sprintf("%s%p%s", backtrace_str, ret_addrs[x],
                                    (ret_addrs[x + 1]) ? " < " : "");
        x++;
    }

    mlog(ROLE_NOONE, LVL_AMBASSADOR, NRM, true, "CHECK: %s", msg);
    mlog(ROLE_NOONE, LVL_AMBASSADOR, NRM, true, "TRACE: %s", backtrace_str);
}

bool
check_ptr(struct creature *ch __attribute__ ((unused)),
          void *ptr __attribute__ ((unused)),
          size_t expected_len __attribute__ ((unused)),
          const char *str __attribute__ ((unused)),
          int vnum __attribute__ ((unused)))
{
#ifdef MEMTRACK
    struct dbg_mem_blk *mem;

    if (!ptr) {
        return true;
    }

    mem = dbg_get_block(ptr);
    if (!mem) {
        check_log(ch, "Invalid pointer found in %s %d", str, vnum);
        return false;
    }

    if (expected_len > 0 && mem->size != expected_len) {
        check_log(ch, "Expected block of size %d, got size %d in %s %d",
                  expected_len, mem->size, str, vnum);
        return false;
    }
    return true;
#else
    return true;
#endif
}

void
verify_tempus_integrity(struct creature *ch)
{
    GList *protos, *mit, *cit, *oit;
    struct creature *vict;
    struct obj_data *obj, *contained;
    struct room_data *room;
    struct zone_data *zone;
    struct extra_descr_data *cur_exdesc;
    struct memory_rec *cur_mem;
    struct tmp_obj_affect *obj_aff;
    int idx;

    // Check prototype mobs
    protos = g_hash_table_get_values(mob_prototypes);
    for (mit = protos; mit; mit = mit->next) {
        vict = mit->data;

        if (!vict->player.name) {
            check_log(ch, "Alias of creature proto #%d (%s) is NULL!",
                      NPC_IDNUM(vict), vict->player.short_descr);
        }
        check_ptr(ch, vict->player.name, 0,
                  "aliases of creature proto", NPC_IDNUM(vict));
        check_ptr(ch, vict->player.short_descr, 0,
                  "name of creature proto", NPC_IDNUM(vict));
        check_ptr(ch, vict->player.long_descr, 0,
                  "linedesc of creature proto", NPC_IDNUM(vict));
        check_ptr(ch, vict->player.description, 0,
                  "description of creature proto", NPC_IDNUM(vict));
        check_ptr(ch, vict->player.description, 0,
                  "description of creature proto", NPC_IDNUM(vict));
        check_ptr(ch, vict->mob_specials.func_data, 0,
                  "func_data of creature proto", NPC_IDNUM(vict));

        // Loop through memory
        for (cur_mem = vict->mob_specials.memory; cur_mem;
             cur_mem = cur_mem->next) {
            if (!check_ptr(ch, cur_mem, sizeof(struct memory_rec),
                           "memory of creature proto", NPC_IDNUM(vict))) {
                break;
            }
        }
        // Mobile shared structure
        if (check_ptr(ch, vict->mob_specials.func_data,
                      sizeof(struct mob_shared_data),
                      "shared struct of creature proto", NPC_IDNUM(vict))) {

            check_ptr(ch, vict->mob_specials.shared->move_buf, 0,
                      "move_buf of creature proto", NPC_IDNUM(vict));
            check_ptr(ch, vict->mob_specials.shared->func_param, 0,
                      "func_param of creature proto", NPC_IDNUM(vict));
            check_ptr(ch, vict->mob_specials.shared->func_param, 0,
                      "load_param of creature proto", NPC_IDNUM(vict));
        }

        if (vict->mob_specials.shared->proto != vict) {
            check_log(ch, "Prototype of prototype is not itself on mob %d",
                      NPC_IDNUM(vict));
        }
    }
    g_list_free(protos);

    // Check prototype objects
    protos = g_hash_table_get_values(obj_prototypes);
    for (oit = protos; oit; oit = oit->next) {
        obj = oit->data;
        check_ptr(ch, obj, sizeof(struct obj_data), "object proto", -1);
        check_ptr(ch, obj->name, 0, "name of object proto", GET_OBJ_VNUM(obj));
        check_ptr(ch, obj->aliases, 0,
                  "aliases of object proto", GET_OBJ_VNUM(obj));
        check_ptr(ch, obj->line_desc, 0,
                  "line desc of object proto", GET_OBJ_VNUM(obj));
        check_ptr(ch, obj->action_desc, 0,
                  "action desc of object proto", GET_OBJ_VNUM(obj));
        for (cur_exdesc = obj->ex_description; cur_exdesc;
             cur_exdesc = cur_exdesc->next) {
            if (!check_ptr(ch, cur_exdesc, sizeof(struct extra_descr_data),
                           "extradesc of object proto", GET_OBJ_VNUM(obj))) {
                break;
            }
            check_ptr(ch, cur_exdesc->keyword, 0,
                      "keyword of extradesc of object proto", GET_OBJ_VNUM(obj));
            check_ptr(ch, cur_exdesc->description, 0,
                      "description of extradesc of object proto", GET_OBJ_VNUM(obj));
        }
        check_ptr(ch, obj->shared, sizeof(struct obj_shared_data),
                  "shared struct of object proto", GET_OBJ_VNUM(obj));
        check_ptr(ch, obj->shared->func_param, 0,
                  "func param of object proto", GET_OBJ_VNUM(obj));
    }

    g_list_free(protos);

    // Check rooms
    for (zone = zone_table; zone; zone = zone->next) {
        for (room = zone->world; room; room = room->next) {
            check_ptr(ch, room->name, 0, "title of room", room->number);
            check_ptr(ch, room->name, 0, "description of room", room->number);
            check_ptr(ch, room->sounds, 0, "sounds of room", room->number);
            for (cur_exdesc = room->ex_description; cur_exdesc;
                 cur_exdesc = cur_exdesc->next) {
                if (!check_ptr(ch, cur_exdesc, sizeof(struct extra_descr_data),
                               "extradesc of room", room->number)) {
                    break;
                }
                check_ptr(ch, cur_exdesc->keyword, 0,
                          "keyword of extradesc of room", room->number);
                check_ptr(ch, cur_exdesc->description, 0,
                          "description of extradesc of room", room->number);
            }
            if (ABS_EXIT(room, NORTH)
                && check_ptr(ch, ABS_EXIT(room, NORTH),
                             sizeof(struct room_direction_data),
                             "north exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, NORTH)->general_description, 0,
                          "description of north exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, NORTH)->keyword, 0,
                          "keywords of north exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, NORTH)->to_room,
                          sizeof(struct room_data),
                          "destination of north exit of room", room->number);
            }
            if (ABS_EXIT(room, SOUTH)
                && check_ptr(ch, ABS_EXIT(room, SOUTH),
                             sizeof(struct room_direction_data),
                             "south exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, SOUTH)->general_description, 0,
                          "description of south exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, SOUTH)->keyword, 0,
                          "keywords of south exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, SOUTH)->to_room,
                          sizeof(struct room_data),
                          "destination of south exit of room", room->number);
            }
            if (ABS_EXIT(room, EAST)
                && check_ptr(ch, ABS_EXIT(room, EAST),
                             sizeof(struct room_direction_data),
                             "east exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, EAST)->general_description, 0,
                          "description of east exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, EAST)->keyword, 0,
                          "keywords of east exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, EAST)->to_room,
                          sizeof(struct room_data),
                          "destination of east exit of room", room->number);
            }
            if (ABS_EXIT(room, WEST)
                && check_ptr(ch, ABS_EXIT(room, WEST),
                             sizeof(struct room_direction_data),
                             "west exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, WEST)->general_description, 0,
                          "description of west exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, WEST)->keyword, 0,
                          "keywords of west exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, WEST)->to_room,
                          sizeof(struct room_data),
                          "destination of west exit of room", room->number);
            }
            if (ABS_EXIT(room, UP)
                && check_ptr(ch, ABS_EXIT(room, UP),
                             sizeof(struct room_direction_data),
                             "up exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, UP)->general_description, 0,
                          "description of up exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, UP)->keyword, 0,
                          "keywords of up exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, UP)->to_room,
                          sizeof(struct room_data), "destination of up exit of room",
                          room->number);
            }
            if (ABS_EXIT(room, DOWN)
                && check_ptr(ch, ABS_EXIT(room, DOWN),
                             sizeof(struct room_direction_data),
                             "down exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, DOWN)->general_description, 0,
                          "description of down exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, DOWN)->keyword, 0,
                          "keywords of down exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, DOWN)->to_room,
                          sizeof(struct room_data),
                          "destination of down exit of room", room->number);
            }
            if (ABS_EXIT(room, PAST)
                && check_ptr(ch, ABS_EXIT(room, PAST),
                             sizeof(struct room_direction_data),
                             "past exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, PAST)->general_description, 0,
                          "description of past exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, PAST)->keyword, 0,
                          "keywords of past exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, PAST)->to_room,
                          sizeof(struct room_data),
                          "destination of past exit of room", room->number);
            }
            if (ABS_EXIT(room, FUTURE)
                && check_ptr(ch, ABS_EXIT(room, FUTURE),
                             sizeof(struct room_direction_data),
                             "future exit of room", room->number)) {
                check_ptr(ch, ABS_EXIT(room, FUTURE)->general_description, 0,
                          "description of future exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, FUTURE)->keyword, 0,
                          "keywords of future exit of room", room->number);
                check_ptr(ch, ABS_EXIT(room, FUTURE)->to_room,
                          sizeof(struct room_data),
                          "destination of future exit of room", room->number);
            }
            for (contained = room->contents;
                 contained; contained = contained->next_content) {
                if (!check_ptr(ch, contained, sizeof(struct obj_data),
                               "object in room", room->number)) {
                    break;
                }
                if (contained->in_room != room) {
                    check_log(ch,
                              "expected object %ld carrier = room %d (%p), got %p",
                              contained->unique_id, room->number, room,
                              contained->in_room);
                }
            }

        }
    }

    // Check zones
    // Check mobiles in game
    for (cit = first_living(creatures); cit; cit = next_living(cit)) {
        vict = cit->data;

        check_ptr(ch, vict, sizeof(struct creature),
                  "mobile", GET_NPC_VNUM(vict));
        for (idx = 0; idx < NUM_WEARS; idx++) {
            if (GET_EQ(vict, idx)
                && check_ptr(ch, GET_EQ(vict, idx), sizeof(struct obj_data),
                             "object worn by mobile", GET_NPC_VNUM(vict))) {
                if (GET_EQ(vict, idx)->worn_by != vict) {
                    check_log(ch, "expected object wearer wrong!");
                }

            }
            if (GET_IMPLANT(vict, idx)
                && check_ptr(ch, GET_IMPLANT(vict, idx),
                             sizeof(struct obj_data), "object implanted in mobile",
                             GET_NPC_VNUM(vict))) {
                if (GET_IMPLANT(vict, idx)->worn_by != vict) {
                    check_log(ch, "expected object implanted wrong!");
                }

            }
            if (GET_TATTOO(vict, idx)
                && check_ptr(ch, GET_TATTOO(vict, idx),
                             sizeof(struct obj_data), "object tattooed in mobile",
                             GET_NPC_VNUM(vict))) {
                if (GET_TATTOO(vict, idx)->worn_by != vict) {
                    check_log(ch, "expected object tattooed wrong!");
                }
            }
        }

        for (contained = vict->carrying;
             contained; contained = contained->next_content) {
            if (!check_ptr(ch, contained, sizeof(struct obj_data),
                           "object carried by mobile vnum %d", GET_NPC_VNUM(vict))) {
                break;
            }
            if (contained->carried_by != vict) {
                check_log(ch,
                          "expected object %ld carrier = mob %d (%p), got %p",
                          contained->unique_id, GET_NPC_VNUM(vict), vict,
                          contained->carried_by);
            }
        }
        for (GList *it = first_living(vict->fighting); it; it = next_living(it)) {
            struct creature *tch = it->data;
            if (!tch->in_room) {
                check_log(ch,
                          "mob %p fighting extracted creature %p",
                          vict, tch);
                raise(SIGSEGV);
            }
        }
    }

    // Check objects in game
    for (obj = object_list; obj; obj = obj->next) {
        check_ptr(ch, obj, sizeof(struct obj_data), "object", obj->unique_id);
        check_ptr(ch, obj->name, 0, "name of object", obj->unique_id);
        check_ptr(ch, obj->aliases, 0, "aliases of object", obj->unique_id);
        check_ptr(ch, obj->line_desc, 0,
                  "line desc of object", obj->unique_id);
        check_ptr(ch, obj->action_desc, 0,
                  "action desc of object", obj->unique_id);
        for (cur_exdesc = obj->ex_description; cur_exdesc;
             cur_exdesc = cur_exdesc->next) {
            if (!check_ptr(ch, cur_exdesc, sizeof(struct extra_descr_data),
                           "extradesc of obj", obj->unique_id)) {
                break;
            }
            check_ptr(ch, cur_exdesc->keyword, 0,
                      "keyword of extradesc of obj", obj->unique_id);
            check_ptr(ch, cur_exdesc->description, 0,
                      "description of extradesc of room", obj->unique_id);
        }

        check_ptr(ch, obj->shared, sizeof(struct obj_shared_data),
                  "shared data of object", obj->unique_id);
        check_ptr(ch, obj->in_room, sizeof(struct room_data),
                  "room location of object", obj->unique_id);
        check_ptr(ch, obj->in_obj, sizeof(struct obj_data),
                  "object location of object", obj->unique_id);
        check_ptr(ch, obj->carried_by, sizeof(struct creature),
                  "carried_by location of object", obj->unique_id);
        check_ptr(ch, obj->worn_by, sizeof(struct creature),
                  "worn_by location of object", obj->unique_id);
        check_ptr(ch, obj->func_data, 0,
                  "special data of object", obj->unique_id);
        if (!(obj->in_room || obj->in_obj || obj->carried_by || obj->worn_by)) {
            check_log(ch, "object %s (#%d) (uid %lu) stuck in limbo",
                      obj->name, GET_OBJ_VNUM(obj), obj->unique_id);
        }

        for (contained = obj->contains;
             contained; contained = contained->next_content) {
            if (!check_ptr(ch, contained, sizeof(struct obj_data),
                           "object contained by object", obj->unique_id)) {
                break;
            }
            if (contained->in_obj != obj) {
                check_log(ch,
                          "expected object %lu's in_obj = obj %lu(%p), got %p",
                          contained->unique_id, obj->unique_id, obj,
                          contained->in_obj);
            }
        }
        for (obj_aff = obj->tmp_affects; obj_aff; obj_aff = obj_aff->next) {
            if (!check_ptr(ch, obj_aff, sizeof(struct tmp_obj_affect),
                           "tmp obj affect in object", obj->unique_id)) {
                break;
            }
        }

        // Check object for empty alias list
        char *alias;

        alias = obj->aliases;
        skip_spaces(&alias);
        if (*alias == '\0') {
            check_log(ch,
                      "object %s (#%d) (uid %lu) has no alias - setting to obj%lu",
                      obj->name, GET_OBJ_VNUM(obj), obj->unique_id, obj->unique_id);
            free(obj->aliases);
            obj->aliases = strdup(tmp_sprintf("obj%lu", obj->unique_id));
        }
    }

    // Check accounts
    // Check descriptors
    // Check players in game
    // Check dyntext items
    // Check help
    // Check security groups
}
