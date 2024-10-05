/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: utils.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <execinfo.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

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
#include "account.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "obj_data.h"
#include "strutil.h"

extern struct follow_type *order_next_k;
char ANSI[20];

__attribute__((noreturn)) void
safe_exit(int mode)
{
    touch("../pause");
    slog("Exiting with status %d from safe_exit().", mode);
    exit(mode);
}

long
skill_cost(struct creature *ch, int skill)
{
    // Mort costs per prac: Level 1: 100, Level 49: 240k
    long skill_lvl, cost;

    skill_lvl = SPELL_LEVEL(skill, GET_CLASS(ch));
    if (IS_REMORT(ch) && skill_lvl > SPELL_LEVEL(skill, GET_REMORT_CLASS(ch))) {
        skill_lvl = SPELL_LEVEL(skill, GET_REMORT_CLASS(ch));
    }

    cost = skill_lvl * skill_lvl * 100;

    // Remort costs: gen 1, lvl 35: 245k  gen 10, lvl 49: 2641100
    if (SPELL_GEN(skill, GET_CLASS(ch))) {
        cost *= SPELL_GEN(skill, GET_CLASS(ch)) + 1;
    }

    return cost;
}

/* log a death trap hit */
void
log_death_trap(struct creature *ch)
{
    mudlog(LVL_AMBASSADOR, BRF, true,
           "%s hit death trap #%d (%s)", GET_NAME(ch),
           ch->in_room->number, ch->in_room->name);
}

/* the "touch" command, essentially. */
int
touch(const char *path)
{
    FILE *fl;

    if (!(fl = fopen(path, "a"))) {
        perror(path);
        return -1;
    } else {
        fclose(fl);
        return 0;
    }
}

/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void
mlog(const char *group, int8_t level, enum log_type type, bool file,
     const char *fmt, ...)
{
    bool is_named_role_member(struct creature *ch, const char *role_name);

    extern struct descriptor_data *descriptor_list;
    struct descriptor_data *i;
    va_list args;
    char *msg;
    enum log_type tp;
    char timebuf[25];
    time_t ct;
    struct tm *ctm;

    va_start(args, fmt);
    msg = tmp_vsprintf(fmt, args);
    va_end(args);

    ct = time(NULL);
    ctm = localtime(&ct);

    if (file) {
        fprintf(stderr, "%-19.19s :: %s\n", asctime(ctm), msg);
    }

    if (group == ROLE_NOONE) {
        return;
    }
    if (level < 0) {
        return;
    }

    strftime(timebuf, 24, " - %b %d %T", ctm);

    for (i = descriptor_list; i; i = i->next) {
        if (i->input_mode == CXN_PLAYING
            && !PLR_FLAGGED(i->creature, PLR_WRITING)) {

            tp = ((PRF_FLAGGED(i->creature, PRF_LOG1) ? 1 : 0) +
                  (PRF_FLAGGED(i->creature, PRF_LOG2) ? 2 : 0));

            if (is_named_role_member(i->creature, group)
                && (GET_LEVEL(i->creature) >= level)
                && (tp >= type)) {
                send_to_char(i->creature, "%s[ %s%s ]%s\r\n",
                             CCGRN(i->creature, C_NRM),
                             msg, timebuf, CCNRM(i->creature, C_NRM));
            }
        }
    }
}

/* writes a string to the log */
void
slog(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    mlog(ROLE_NOONE, -1, CMP, true, "%s", tmp_vsprintf(fmt, args));
    va_end(args);
}

/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void
mudlog(int8_t level, enum log_type type, bool file, const char *fmt, ...)
{

    va_list args;

    va_start(args, fmt);
    mlog(ROLE_EVERYONE, level, type, file, "%s", tmp_vsprintf(fmt, args));
    va_end(args);
}

void
errlog(const char *fmt, ...)
{
#define MAX_FRAMES 10
    va_list args;
    const char *backtrace_str = "";
    void *ret_addrs[MAX_FRAMES + 1];
    int x = 0;

    memset(ret_addrs, 0x0, sizeof(ret_addrs));
    backtrace(ret_addrs, MAX_FRAMES);

    while (x < MAX_FRAMES && ret_addrs[x]) {
        backtrace_str = tmp_sprintf("%s%p%s", backtrace_str, ret_addrs[x],
                                    (ret_addrs[x + 1]) ? " " : "");
        x++;
    }

    va_start(args, fmt);
    mlog(ROLE_CODER, LVL_AMBASSADOR, NRM, true,
         "SYSERR: %s", tmp_vsprintf(fmt, args));
    va_end(args);

    mlog(ROLE_NOONE, LVL_AMBASSADOR, NRM, true, "TRACE: %s", backtrace_str);
}

void
zerrlog(struct zone_data *zone, const char *fmt, ...)
{
    struct descriptor_data *d;
    va_list args;
    bool display;
    char *msg;

    // Construct the message
    va_start(args, fmt);
    msg = tmp_vsprintf(fmt, args);
    va_end(args);
    msg = tmp_sprintf("Zone #%d: %s", zone->number, msg);

    // Log the error to file first
    slog("ZONEERR: %s", msg);

    for (d = descriptor_list; d; d = d->next) {
        // Only playing immortals can see zone errors
        if (!IS_PLAYING(d) || GET_LEVEL(d->creature) < 50) {
            continue;
        }

        // Zone owners get to see them
        display = GET_IDNUM(d->creature) == zone->owner_idnum;

        // Zone co-owners also get to see them
        if (!display) {
            display = GET_IDNUM(d->creature) == zone->co_owner_idnum;
        }

        // Immortals within the zone see them
        if (!display && d->creature->in_room) {
            display = d->creature->in_room->zone == zone;
        }

        if (display) {
            d_printf(d, "&y%s&n\r\n", msg);
        }
    }
}

/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data
real_time_passed(time_t t2, time_t t1)
{
    long secs;
    struct time_info_data now;

    secs = (long)(t2 - t1);

    now.hours = (secs / SECS_PER_REAL_HOUR) % 24;   /* 0..23 hours */
    secs -= SECS_PER_REAL_HOUR * now.hours;

    now.day = (secs / SECS_PER_REAL_DAY);   /* 0..34 days  */

    now.month = -1;
    now.year = -1;

    return now;
}

/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data
mud_time_passed(time_t t2, time_t t1)
{
    long secs;
    struct time_info_data now;

    secs = (long)(t2 - t1);

    now.hours = (secs / SECS_PER_MUD_HOUR) % 24;    /* 0..23 hours */
    secs -= SECS_PER_MUD_HOUR * now.hours;

    now.day = (secs / SECS_PER_MUD_DAY) % 35;   /* 0..34 days  */
    secs -= SECS_PER_MUD_DAY * now.day;

    now.month = (secs / SECS_PER_MUD_MONTH) % 17;   /* 0..16 months */
    secs -= SECS_PER_MUD_MONTH * now.month;

    now.year = (secs / SECS_PER_MUD_YEAR);  /* 0..XX? years */

    return now;
}

struct time_info_data
age(struct creature *ch)
{
    struct time_info_data player_age;
    struct race *race = race_by_idnum(GET_RACE(ch));
    int age_adjust = (race) ? race->age_adjust : 18;

    player_age = mud_time_passed(time(NULL), ch->player.time.birth);
    player_age.year += age_adjust;

    return player_age;
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool
circle_follow(struct creature *ch, struct creature *victim)
{
    struct creature *k;

    for (k = victim; k; k = k->master) {
        if (k == ch) {
            return true;
        }
    }

    return false;
}

bool
can_charm_more(struct creature *ch)
{
    struct follow_type *cur;
    int count = 0;

    // We can always charm one
    if (!ch->followers) {
        return true;
    }

    for (cur = ch->followers; cur; cur = cur->next) {
        if (AFF_FLAGGED(cur->follower, AFF_CHARM)) {
            count++;
        }
    }

    return (count < GET_CHA(ch) / 2 + GET_REMORT_GEN(ch));
}

void
remove_follower(struct creature *ch)
{
    struct follow_type *j, *k;

    if (ch->master->followers->follower == ch) {    /* Head of follower-list? */
        k = ch->master->followers;
        ch->master->followers = k->next;
        free(k);
    } else {                    /* locate follower who is not head of list */
        k = ch->master->followers;
        while (k->next->follower != ch) {
            k = k->next;
        }

        j = k->next;
        k->next = j->next;
        free(j);
    }

    ch->master = NULL;
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void
stop_follower(struct creature *ch)
{
    if (!ch->master) {
        raise(SIGSEGV);
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && !NPC2_FLAGGED(ch, NPC2_MOUNT)) {
        act("You realize that $N is a jerk!", false, ch, NULL, ch->master,
            TO_CHAR);
        act("$n realizes that $N is a jerk!", false, ch, NULL, ch->master,
            TO_NOTVICT);
        act("$n hates your guts!", false, ch, NULL, ch->master, TO_VICT);
        if (affected_by_spell(ch, SPELL_CHARM)) {
            affect_from_char(ch, SPELL_CHARM);
        }
    } else {
        act("You stop following $N.", false, ch, NULL, ch->master, TO_CHAR);
        act("$n stops following $N.", true, ch, NULL, ch->master, TO_NOTVICT);
        if (GET_INVIS_LVL(ch) < GET_LEVEL(ch->master)
            && !AFF_FLAGGED(ch, AFF_SNEAK)) {
            act("$n stops following you.", true, ch, NULL, ch->master, TO_VICT);
        }
    }

    remove_follower(ch);
    REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}

/* Called when a character that follows/is followed dies */
void
die_follower(struct creature *ch)
{
    struct follow_type *j, *k;

    if (order_next_k && order_next_k->follower && ch == order_next_k->follower) {
        order_next_k = NULL;
    }

    if (ch->master) {
        stop_follower(ch);
    }

    for (k = ch->followers; k; k = j) {
        j = k->next;
        stop_follower(k->follower);
    }
}

bool
player_in_room(struct room_data *room)
{
    for (GList *it = first_living(room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if (!IS_NPC(tch) && GET_LEVEL(tch) < LVL_AMBASSADOR) {
            return true;
        }
    }
    return false;
}

void
new_follower(struct creature *ch, struct creature *leader)
{
    struct follow_type *k;

    ch->master = leader;

    CREATE(k, struct follow_type, 1);

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void
add_follower(struct creature *ch, struct creature *leader)
{
    if (ch->master) {
        raise(SIGSEGV);
    }

    new_follower(ch, leader);

    act("You now follow $N.", false, ch, NULL, leader, TO_CHAR);
    if (can_see_creature(leader, ch)) {
        act("$n starts following you.", true, ch, NULL, leader, TO_VICT);
    }
    act("$n starts to follow $N.", true, ch, NULL, leader, TO_NOTVICT);
}

void
add_stalker(struct creature *ch, struct creature *leader)
{
    struct follow_type *k;

    if (ch->master) {
        raise(SIGSEGV);
    }

    ch->master = leader;

    CREATE(k, struct follow_type, 1);

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;

    act("You are now stalking $N.", false, ch, NULL, leader, TO_CHAR);
    if (can_see_creature(leader, ch)) {
        if (CHECK_SKILL(ch, SKILL_STALK) < (number(0, 80) + GET_WIS(leader))) {
            act("$n starts following you.", true, ch, NULL, leader, TO_VICT);
            act("$n starts to follow $N.", true, ch, NULL, leader, TO_NOTVICT);
        } else {
            gain_skill_prof(ch, SKILL_STALK);
        }
    }
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file or 0 if EOF.
 */
int
get_line(FILE *fl, char *buf, size_t buf_size)
{
    int lines = 0;

    while (fgets(buf, buf_size, fl)) {
        lines++;
        if (buf[0] && buf[0] != '*') {
            break;
        }
    }
    if (feof(fl)) {
        return 0;
    }

    char *c = strchr(buf, '\n');
    if (c) {
        *c = '\0';
    }

    return lines;
}

void
num2str(char *str, size_t size, int num)
{
    if (num == 0) {
        strcpy_s(str, size, "0");
        return;
    }

    const char *encoding = "abcdefghijklmnopqrstuvwxyzABCDEF";
    const char *c = encoding;
    for (int i = 0; size > 1 && i < 32; i++, c++) {
        if (num & (1U << i)) {
            *str++ = *c;
            size--;
        }

    }

    *str++ = '\0';
}

char *
GET_DISGUISED_NAME(struct creature *ch, struct creature *tch)
{
    struct affected_type *af = NULL;
    struct creature *mob = NULL;
    static char buf[1024];

    if (IS_NPC(tch)) {
        return GET_NAME(tch);
    }

    if (!(af = affected_by_spell(tch, SKILL_DISGUISE))) {
        return GET_NAME(tch);
    }

    if (!(mob = real_mobile_proto(af->modifier))) {
        return GET_NAME(tch);
    }

    if (CAN_DETECT_DISGUISE(ch, tch, af->duration)) {
        snprintf(buf, sizeof(buf), "%s (disguised as %s)", GET_NAME(tch), GET_NAME(mob));
        return (buf);
    }
    gain_skill_prof(tch, SKILL_DISGUISE);
    return GET_NAME(mob);
}

int
GET_SKILL(struct creature *ch, int i)
{
    if (i < 0 || i > MAX_SKILLS) {
        return 0;
    }
    return ch->player_specials->saved.skills[i];
}

void
SET_SKILL(struct creature *ch, int i, int val)
{
    if (i < 0 || i > MAX_SKILLS) {
        return;
    }
    ch->player_specials->saved.skills[i] = val;
}

int
CHECK_SKILL(struct creature *ch, int i)
{
    int level = 0;
    struct affected_type *af_ptr = NULL;

    if (i > MAX_SKILLS) {
        return 0;
    }

    if (!IS_NPC(ch)) {
        return ch->player_specials->saved.skills[i];
    }

    if (GET_CLASS(ch) < NUM_CLASSES) {
        if (GET_LEVEL(ch) >= spell_info[i].min_level[(int)GET_CLASS(ch)]) {
            level = 50 + GET_LEVEL(ch);
        }
    }

    if (!level &&
        GET_REMORT_CLASS(ch) < NUM_CLASSES && GET_REMORT_CLASS(ch) >= 0) {
        if (GET_LEVEL(ch) >=
            spell_info[i].min_level[(int)GET_REMORT_CLASS(ch)]) {
            level = 50 + GET_LEVEL(ch);
        }
    }
    if (!level) {
        if (IS_GIANT(ch)) {
            if (GET_LEVEL(ch) >= spell_info[i].min_level[CLASS_BARB]) {
                level = 50 + GET_LEVEL(ch);
            }
        }
    }
    if (IS_DEVIL(ch)) {
        level += GET_LEVEL(ch) / 2;
    }

    if (level > 0 && (af_ptr = affected_by_spell(ch, SPELL_AMNESIA))) {
        level = MAX(0, level - af_ptr->duration);
    }

    return level;
}

int
CHECK_TONGUE(struct creature *ch, int i)
{
    return ch->language_data.tongues[i];
}

void
WAIT_STATE(struct creature *ch, int cycle)
{
    int wait;

    if (GET_LEVEL(ch) >= LVL_TIMEGOD) {
        return;
    }

    wait = cycle;

    if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
        wait -= cycle / 4;
    }
    if (AFF2_FLAGGED(ch, AFF2_SLOW)) {
        wait += cycle / 4;
    }
    if (SPEED_OF(ch)) {
        wait -= (cycle * SPEED_OF(ch)) / 100;
    }

    GET_WAIT(ch) = MAX(GET_WAIT(ch), wait);
}

const char *
OBJN(struct obj_data *obj, struct creature *vict)
{
    if (obj == NULL) {
        return "<NULL>";
    }
    if (can_see_object(vict, obj)) {
        return fname((obj)->aliases);
    }
    return "something";
}

const char *
OBJS(struct obj_data *obj, struct creature *vict)
{
    if (obj == NULL) {
        return "<NULL>";
    }
    if (can_see_object(vict, obj)) {
        return obj->name;
    }
    return "something";
}

const char *
PERS(struct creature *ch, struct creature *sub)
{
    if (ch == NULL) {
        return "<NULL>";
    }
    if (can_see_creature(sub, ch)) {
        return GET_DISGUISED_NAME(sub, ch);
    }
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        return "a divine presence";
    }
    return "someone";
}

const char *
CURRENCY(struct creature *ch)
{
    if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
        return "credit";
    }
    return "coin";
}

bool
CAN_GO(struct creature *ch, int door)
{
    struct room_direction_data *exit = EXIT(ch, door);

    // No exit
    if (!exit || exit->to_room == NULL) {
        return false;
    }

    // Closed door
    if ((IS_SET(exit->exit_info, EX_NOPASS)
         || IS_SET(exit->exit_info, EX_CLOSED))
        && !NON_CORPOREAL_MOB(ch)
        && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        return false;
    }

    // Unapproved mob trying to move into approved zone
    if (NPC2_FLAGGED(ch, NPC2_UNAPPROVED)
        && !ZONE_FLAGGED(exit->to_room->zone, ZONE_MOBS_APPROVED)) {
        return false;
    }

    return true;
}

bool
OCAN_GO(struct obj_data *obj, int door)
{
    struct room_direction_data *exit = OEXIT(obj, door);
    return (exit != NULL &&
            !IS_SET(exit->exit_info, EX_CLOSED | EX_NOPASS) &&
            exit->to_room != NULL);
}

struct extra_descr_data *
exdesc_list_dup(struct extra_descr_data *list)
{
    struct extra_descr_data *cur_old, *cur_new;
    struct extra_descr_data *prev_new = NULL, *result = NULL;

    for (cur_old = list; cur_old; cur_old = cur_old->next) {
        CREATE(cur_new, struct extra_descr_data, 1);
        cur_new->next = NULL;
        cur_new->keyword = strdup(cur_old->keyword);
        cur_new->description = strdup(cur_old->description);
        if (prev_new) {
            prev_new->next = cur_new;
        } else {
            result = cur_new;
        }
        prev_new = cur_new;
    }

    return result;
}

void
check_bits_32(int bitv, int *newbits)
{
    for (int i = 0; i < 32; i++) {
        if (bitv & (1U << i)) {
            *newbits &= ~(1U << i);
        }
    }
}

char *
format_distance(int cm, bool metric)
{
    if (metric) {
        return tmp_sprintf("%'dcm", cm);
    } else {
        float inches = cm / 2.54;
        if (inches < 1) {
            return tmp_strdup("1/2 in");
        }
        if (inches < 12) {
            return tmp_sprintf("%d in", (int)inches);
        } else {
            int ft = inches / 12;
            return tmp_sprintf("%'d ft, %d in", ft, (int)(inches - (ft * 12)));
        }
    }
}

char *
format_weight(float lbs, bool metric)
{
    if (metric) {
        float kg = lbs / 2.2;
        if (kg < 1.0) {
            return tmp_sprintf("%'dg", (int)(kg * 1000));
        } else {
            return tmp_sprintf("%'.2fkg", kg);
        }
    } else {
        if (lbs < 1) {
            return tmp_sprintf("%'.2f oz", lbs * 16);
        } else {
            return tmp_sprintf("%'.2f lb", lbs);
        }
    }
}

int
parse_distance(char *str, bool metric)
{
    if (metric) {
        return atoi(str);
    } else {
        int feet = atoi(tmp_getword(&str));
        int inches = atoi(tmp_getword(&str));

        inches += feet * 12;

        return (inches + 1) * 2.54;
    }
}

float
parse_weight(char *str, bool metric)
{
    if (metric) {
        float kg = strtof(str, NULL);
        return kg * 2.2;
    } else {
        float lbs = strtof(tmp_getword(&str), NULL);
        float oz = strtof(tmp_getword(&str), NULL);

        return lbs + oz / 16.0;
    }
}
