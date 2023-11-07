/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.comm.c                       -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#define _GNU_SOURCE
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
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "clan.h"
#include "tmpstr.h"
#include "spells.h"
#include "obj_data.h"
#include "strutil.h"
#include "language.h"
#include "prog.h"
#include "editor.h"
#include "ban.h"

/* extern variables */
extern const char *class_names[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
int Nasty_Words(char *words);
extern int quest_status;
extern const char *language_names[];

int parse_player_class(char *arg);
void summon_cityguards(struct room_data *room);

void
perform_say(struct creature *ch, const char *saystr, const char *message)
{
    message = act_escape(message);
    act(tmp_sprintf("&BYou %s$a$l, &c'$[%s]'", saystr, message),
        0, ch, NULL, NULL, TO_CHAR);
    act(tmp_sprintf("&B$n %ss$a$l, &c'$[%s]'", saystr, message),
        0, ch, NULL, NULL, TO_ROOM);
}

void
perform_say_to(struct creature *ch, struct creature *target,
               const char *message)
{
    message = act_escape(message);
    act(tmp_sprintf("&BYou$a say to $T$l, &c'$[%s]'", message),
        0, ch, NULL, target, TO_CHAR);
    act(tmp_sprintf("&B$n$a says to $T$l, &c'$[%s]'", message),
        0, ch, NULL, target, TO_ROOM);
}

void
perform_say_to_obj(struct creature *ch, struct obj_data *obj,
                   const char *message)
{
    message = act_escape(message);
    act(tmp_sprintf("&BYou$a say to $p$l, &c'$[%s]'", message),
        0, ch, obj, NULL, TO_CHAR);
    act(tmp_sprintf("&B$n$a says to $p$l, &c'$[%s]'", message),
        0, ch, obj, NULL, TO_ROOM);
}

ACMD(do_sayto)
{
    struct creature *vict = NULL;
    struct obj_data *o = NULL;
    char *name;
    int ignore;

    if (PLR_FLAGGED(ch, PLR_AFK)) {
        send_to_char(ch, "You are no longer afk.\r\n");
        REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
        free(AFK_REASON(ch));
        AFK_REASON(ch) = NULL;
    }

    if (!*argument) {
        send_to_char(ch, "Say what to who?\r\n");
    }
    name = tmp_getword(&argument);
    skip_spaces(&argument);

    o = NULL;
    if (!*name) {
        send_to_char(ch, "Say what to who?\r\n");
        return;
    }
    if (!*argument) {
        send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
        return;
    }

    if (!(vict = get_char_room_vis(ch, name))) {
        if (!o) {
            o = get_object_in_equip_vis(ch, name, ch->equipment, &ignore);
        }
        if (!o) {
            o = get_obj_in_list_vis(ch, name, ch->carrying);
        }
        if (!o) {
            o = get_obj_in_list_vis(ch, name, ch->in_room->contents);
        }
    }

    if (vict) {
        perform_say_to(ch, vict, argument);
    } else if (o) {
        perform_say_to_obj(ch, o, argument);
    } else {
        send_to_char(ch, "No-one by that name here.\r\n");
    }
}

const char *
select_say_cmd(struct creature *ch, const char *message)
{
    int len = strlen(message);
    const char *end = message + len - 1;

    if (len > 3 && !strcmp("???", end - 2)) {
        return "yell";
    }
    if (len > 2 && !strcmp("??", end - 1)) {
        return "demand";
    }
    if ('?' == *end) {
        return "ask";
    }
    if (len > 3 && !strcmp("!!!", end - 2)) {
        return "scream";
    }
    if (len > 2 && !strcmp("!!", end - 1)) {
        return "yell";
    }
    if ('!' == *end) {
        return "exclaim";
    }
    if (len > 3 && !strcmp("...", end - 2)) {
        return "mutter";
    }
    if (len > 320) {
        return "drone";
    }
    if (len > 160) {
        return "ramble";
    }
    if (GET_HIT(ch) < GET_MAX_HIT(ch) / 20) {
        return "moan";
    }
    if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 20) {
        return "gasp";
    }
    if (room_is_underwater(ch->in_room)) {
        return "gurgle";
    }
    if (AFF2_FLAGGED(ch, AFF2_ABLAZE) && !CHAR_WITHSTANDS_FIRE(ch)) {
        return "scream";
    }
    if (IS_POISONED(ch)) {
        return "choke";
    }
    if (IS_SICK(ch)) {
        return "wheeze";
    }
    if (GET_COND(ch, DRUNK) > 10) {
        return "slur";
    }
    if (strcasestr(message, "y'all") || strstr(message, "ain't")) {
        return "drawl";
    }
    if (AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        return "rave";
    }
    if (AFF2_FLAGGED(ch, AFF2_INTIMIDATED)) {
        return "whimper";
    }
    if (AFF_FLAGGED(ch, AFF_CONFUSION)) {
        return "stammer";
    }
    for (int i = 0; i < num_nasty; i++) {
        if (!strcasecmp(message, nasty_list[i])) {
            return "curse";
        }
    }
    return "say";
}

ACMD(do_say)
{
    int find_action(int cmd);
    ACMD(do_action);
    const char *cmdstr = cmd_info[cmd].command;

    if (PLR_FLAGGED(ch, PLR_AFK)) {
        send_to_char(ch, "You are no longer afk.\r\n");
        REMOVE_BIT(PLR_FLAGS(ch), PLR_AFK);
        free(AFK_REASON(ch));
        AFK_REASON(ch) = NULL;
    }

    skip_spaces(&argument);

    if (cmdstr[0] == '\'') {
        cmdstr = "say";
    }

    if (*argument) {
        if (!strcmp(cmdstr, "say")) {
            cmdstr = select_say_cmd(ch, argument);
        }
        perform_say(ch, cmdstr, argument);
    } else if (find_action(cmd) == -1) {
        send_to_char(ch, "Yes, but WHAT do you want to %s?\r\n", cmdstr);
    } else {
        do_action(ch, argument, cmd, subcmd);
    }
}

static bool
can_channel_comm(struct creature *ch, struct creature *tch)
{
    // Immortals are hearable everywhere
    if (ch->player.level >= LVL_IMMORT) {
        return true;
    }

    // Anyone can hear people inside the same rooms
    if (ch->in_room == tch->in_room) {
        return true;
    }

    // People outside of soundproof rooms can't hear or speak out
    if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) ||
        ROOM_FLAGGED(tch->in_room, ROOM_SOUNDPROOF)) {
        return false;
    }

    // Players can hear each other inside the same zone
    if (ch->in_room->zone == tch->in_room->zone) {
        return true;
    }

    // but not outside if the zone is isolated or soundproof
    if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED | ZONE_SOUNDPROOF) ||
        ZONE_FLAGGED(tch->in_room->zone, ZONE_ISOLATED | ZONE_SOUNDPROOF)) {
        return false;
    }

    // remorts ignore planar and temporal boundries
    if (IS_REMORT(ch) || IS_REMORT(tch)) {
        return true;
    }

    // mortals can't speak on different planes
    if (ch->in_room->zone->plane != tch->in_room->zone->plane) {
        return false;
    }

    // mortals can speak to each other in the same times
    if (ch->in_room->zone->time_frame == tch->in_room->zone->time_frame) {
        return true;
    }

    // or to or from timeless zones
    if (ch->in_room->zone->time_frame == TIME_TIMELESS ||
        tch->in_room->zone->time_frame == TIME_TIMELESS) {
        return true;
    }

    // otherwise, they can't
    return false;
}

ACMD(do_gsay)
{
    struct creature *k;
    struct follow_type *f;

    skip_spaces(&argument);

    if (!AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch, "But you are not the member of a group!\r\n");
        return;
    }
    if (!*argument) {
        send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
    } else {
        if (ch->master) {
            k = ch->master;
        } else {
            k = ch;
        }

        argument = act_escape(argument);
        if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch) && can_channel_comm(ch, k)) {
            snprintf(buf, sizeof(buf), "%s$n tells the group,%s '%s'%s", CCGRN(k, C_NRM),
                     CCYEL(k, C_NRM), argument, CCNRM(k, C_NRM));
            act(buf, false, ch, NULL, k, TO_VICT | TO_SLEEP);
        }
        for (f = k->followers; f; f = f->next) {
            if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch) &&
                can_channel_comm(ch, f->follower)) {
                snprintf(buf, sizeof(buf), "%s$n tells the group,%s '%s'%s",
                         CCGRN(f->follower, C_NRM), CCYEL(f->follower, C_NRM),
                         argument, CCNRM(f->follower, C_NRM));
                act(buf, false, ch, NULL, f->follower, TO_VICT | TO_SLEEP);
            }
        }

        snprintf(buf, sizeof(buf), "%sYou tell the group,%s '%s'%s", CCGRN(ch, C_NRM),
                 CCYEL(ch, C_NRM), argument, CCNRM(ch, C_NRM));
        act(buf, false, ch, NULL, NULL, TO_CHAR | TO_SLEEP);
    }
}

void
perform_tell(struct creature *ch, struct creature *vict, const char *arg)
{
    char *act_str = tmp_sprintf("&r$t$a tell$%% $T,&n '%s'",
                                act_escape(arg));

    act(act_str, false, ch, NULL, vict, TO_CHAR | TO_SLEEP);
    act(act_str, false, ch, NULL, vict, TO_VICT | TO_SLEEP);

    if (PLR_FLAGGED(vict, PLR_AFK)
        && g_list_find(AFK_NOTIFIES(vict), GINT_TO_POINTER(GET_IDNUM(ch)))) {
        AFK_NOTIFIES(vict) =
            g_list_prepend(AFK_NOTIFIES(vict), GINT_TO_POINTER(GET_IDNUM(ch)));
        if (AFK_REASON(vict)) {
            act(tmp_sprintf("$N is away from the keyboard: %s",
                            AFK_REASON(vict)), true, ch, NULL, vict, TO_CHAR);
        } else {
            act("$N is away from the keyboard.", true, ch, NULL, vict, TO_CHAR);
        }
    }

    if (PRF2_FLAGGED(vict, PRF2_AUTOPAGE) && !IS_NPC(ch)) {
        send_to_char(vict, "\007\007");
    }

    if (!IS_NPC(ch)) {
        GET_LAST_TELL_FROM(vict) = GET_IDNUM(ch);
        GET_LAST_TELL_TO(ch) = GET_IDNUM(vict);
    }
}

static bool
can_send_tell(struct creature *ch, struct creature *tch)
{
    // Immortals are hearable everywhere
    if (ch->player.level >= LVL_IMMORT) {
        return true;
    }

    // Anyone can hear people inside the same rooms
    if (ch->in_room == tch->in_room) {
        return true;
    }

    // People outside of soundproof rooms can't hear or speak out
    if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) ||
        ROOM_FLAGGED(tch->in_room, ROOM_SOUNDPROOF)) {
        return false;
    }

    // Players can hear each other inside the same zone
    if (ch->in_room->zone == tch->in_room->zone) {
        return true;
    }

    // but not outside if the zone is isolated or soundproof
    if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED | ZONE_SOUNDPROOF) ||
        ZONE_FLAGGED(tch->in_room->zone, ZONE_ISOLATED | ZONE_SOUNDPROOF)) {
        return false;
    }

    // Can't speak on different planes
    if (ch->in_room->zone->plane != tch->in_room->zone->plane) {
        return false;
    }

    // Can speak to each other in the same times
    if (ch->in_room->zone->time_frame == tch->in_room->zone->time_frame) {
        return true;
    }

    // or to or from timeless zones
    if (ch->in_room->zone->time_frame == TIME_TIMELESS ||
        tch->in_room->zone->time_frame == TIME_TIMELESS) {
        return true;
    }

    // otherwise, they can't
    return false;
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
    struct creature *vict;

    char buf2[MAX_INPUT_LENGTH];
    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2) {
        send_to_char(ch, "Who do you wish to tell what??\r\n");
    } else if (!(vict = get_player_vis(ch, buf, false))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if (ch == vict) {
        send_to_char(ch, "You try to tell yourself something.\r\n");
    } else if (PRF_FLAGGED(ch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch,
                     "You can't tell other people while you have notell on.\r\n");
    } else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
               && GET_LEVEL(ch) < LVL_GRGOD && ch->in_room != vict->in_room) {
        send_to_char(ch, "The walls seem to absorb your words.\r\n");
    } else if (!IS_NPC(vict) && !vict->desc) { /* linkless */
        act("$E's linkless at the moment.", false, ch, NULL, vict,
            TO_CHAR | TO_SLEEP);
    } else if (PLR_FLAGGED(vict, PLR_WRITING)) {
        act("$E's writing a message right now; try again later.",
            false, ch, NULL, vict, TO_CHAR | TO_SLEEP);
    } else if ((PRF_FLAGGED(vict, PRF_NOTELL) ||
                PLR_FLAGGED(vict, PLR_OLC) ||
                (ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF) &&
                 ch->in_room != vict->in_room)) &&
               !(GET_LEVEL(ch) >= LVL_GRGOD && GET_LEVEL(ch) > GET_LEVEL(vict))) {
        act("$E can't hear you.", false, ch, NULL, vict, TO_CHAR | TO_SLEEP);
    } else {
        if (!can_send_tell(ch, vict)) {
            if (!(affected_by_spell(ch, SPELL_TELEPATHY) ||
                  affected_by_spell(vict, SPELL_TELEPATHY))) {
                act("Your telepathic voice cannot reach $M.",
                    false, ch, NULL, vict, TO_CHAR);
                return;
            }

            WAIT_STATE(ch, 1 RL_SEC);
        }

        perform_tell(ch, vict, buf2);
    }
}

ACMD(do_reply)
{
    skip_spaces(&argument);

    if (GET_LAST_TELL_FROM(ch) == NOBODY) {
        send_to_char(ch, "You have no-one to reply to!\r\n");
        return;
    }
    if (!*argument) {
        send_to_char(ch, "What is your reply?\r\n");
        return;
    }
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.
     */

    struct creature *tch = get_char_in_world_by_idnum(GET_LAST_TELL_FROM(ch));

    if (!tch) {
        send_to_char(ch, "They are no longer playing.\r\n");
    } else if (PRF_FLAGGED(ch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch,
                     "You can't tell other people while you have notell on.\r\n");
    } else if (!IS_NPC(tch) && tch->desc == NULL) {
        send_to_char(ch, "They are linkless at the moment.\r\n");
    } else if (PLR_FLAGGED(tch, PLR_WRITING)) {
        send_to_char(ch, "They are writing at the moment.\r\n");
    } else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
               && GET_LEVEL(ch) < LVL_GRGOD && GET_LEVEL(tch) < LVL_GRGOD
               && ch->in_room != tch->in_room) {
        send_to_char(ch, "The walls seem to absorb your words.\r\n");
    } else {
        if (!can_send_tell(ch, tch) && !can_send_tell((tch), ch)) {
            if (!(affected_by_spell(ch, SPELL_TELEPATHY) ||
                  affected_by_spell((tch), SPELL_TELEPATHY))) {
                act("Your telepathic voice cannot reach $M.",
                    false, ch, NULL, tch, TO_CHAR);
                return;
            }

            WAIT_STATE(ch, 1 RL_SEC);
        }

        perform_tell(ch, tch, argument);
    }
}

ACMD(do_retell)
{
    skip_spaces(&argument);

    if (GET_LAST_TELL_TO(ch) == NOBODY) {
        send_to_char(ch, "You have no-one to retell!\r\n");
        return;
    }
    if (!*argument) {
        send_to_char(ch, "What did you want to tell?\r\n");
        return;
    }

    struct creature *tch = get_char_in_world_by_idnum(GET_LAST_TELL_TO(ch));

    if (!tch) {
        send_to_char(ch, "They are no longer playing.\r\n");
    } else if (PRF_FLAGGED(ch, PRF_NOTELL) && GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch,
                     "You can't tell other people while you have notell on.\r\n");
    } else if (!IS_NPC(tch) && tch->desc == NULL) {
        send_to_char(ch, "They are linkless at the moment.\r\n");
    } else if (PLR_FLAGGED(tch, PLR_WRITING)) {
        send_to_char(ch, "They are writing at the moment.\r\n");
    } else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
               && GET_LEVEL(ch) < LVL_GRGOD && GET_LEVEL(tch) < LVL_GRGOD
               && ch->in_room != tch->in_room) {
        send_to_char(ch, "The walls seem to absorb your words.\r\n");
    } else {
        if (!can_send_tell(ch, tch) && !can_send_tell((tch), ch)) {
            if (!(affected_by_spell(ch, SPELL_TELEPATHY) ||
                  affected_by_spell((tch), SPELL_TELEPATHY))) {
                act("Your telepathic voice cannot reach $M.",
                    false, ch, NULL, tch, TO_CHAR);
                return;
            }

            WAIT_STATE(ch, 1 RL_SEC);
        }

        perform_tell(ch, tch, argument);
    }
}

ACMD(do_whisper)
{
    struct creature *vict;
    char *vict_str = tmp_getword(&argument);

    if (!*vict_str || !*argument) {
        send_to_char(ch, "To whom do you want to whisper.. and what??\r\n");
    } else if (!(vict = get_char_room_vis(ch, vict_str))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if (vict == ch) {
        send_to_char(ch,
                     "You can't get your mouth close enough to your ear...\r\n");
    } else {
        act(tmp_sprintf("&yYou$a whisper to $N$l,&n '$[%s]'",
                        act_escape(argument)), false, ch, NULL, vict, TO_CHAR);
        act(tmp_sprintf("&y$n$a whispers to you$l,&n '$[%s]'",
                        act_escape(argument)), false, ch, NULL, vict, TO_VICT);
        act("$n$a whispers something to $N.", false, ch, NULL, vict, TO_NOTVICT);
    }
}

const size_t MAX_NOTE_LENGTH = 16384;   /* arbitrary */

ACMD(do_write)
{
    struct obj_data *paper = NULL, *pen = NULL;
    char *papername, *penname;

    papername = buf1;
    penname = buf2;

    two_arguments(argument, papername, penname);

    if (!ch->desc) {
        return;
    }

    if (!*papername) {          /* nothing was delivered */
        send_to_char(ch,
                     "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
        return;
    }
    if (*penname) {             /* there were two arguments */
        if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
            send_to_char(ch, "You have no %s.\r\n", papername);
            return;
        }
        if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
            send_to_char(ch, "You have no %s.\r\n", penname);
            return;
        }
    } else {                    /* there was one arg.. let's see what we can find */
        if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
            send_to_char(ch, "There is no %s in your inventory.\r\n",
                         papername);
            return;
        }
        if (IS_OBJ_TYPE(paper, ITEM_PEN)) {  /* oops, a pen.. */
            pen = paper;
            paper = NULL;
        } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
            send_to_char(ch, "That thing has nothing to do with writing.\r\n");
            return;
        }
        /* One object was found.. now for the other one. */
        if (!GET_EQ(ch, WEAR_HOLD)) {
            send_to_char(ch, "You can't write with %s %s alone.\r\n",
                         AN(papername), papername);
            return;
        }
        if (!can_see_object(ch, GET_EQ(ch, WEAR_HOLD))) {
            send_to_char(ch,
                         "The stuff in your hand is invisible!  Yeech!!\r\n");
            return;
        }
        if (pen) {
            paper = GET_EQ(ch, WEAR_HOLD);
        } else {
            pen = GET_EQ(ch, WEAR_HOLD);
        }
    }

    /* ok.. now let's see what kind of stuff we've found */
    if (GET_OBJ_TYPE(pen) != ITEM_PEN) {
        act("$p is no good for writing with.", false, ch, pen, NULL, TO_CHAR);
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
        act("You can't write on $p.", false, ch, paper, NULL, TO_CHAR);
    } else if (paper->action_desc) {
        send_to_char(ch, "There's something written on it already.\r\n");
    } else {
        if (paper->action_desc == NULL) {
            CREATE(paper->action_desc, char, MAX_NOTE_LENGTH);
        }
        start_editing_text(ch->desc, &paper->action_desc, MAX_NOTE_LENGTH);
        SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
        act("$n begins to jot down a note..", true, ch, NULL, NULL, TO_ROOM);
    }
}

ACMD(do_page)
{
    struct creature *vict;
    char *target_str;

    target_str = tmp_getword(&argument);

    if (IS_NPC(ch)) {
        send_to_char(ch, "Monsters can't page.. go away.\r\n");
    } else if (!*target_str) {
        send_to_char(ch, "Whom do you wish to page?\r\n");
    } else {
        char *msg = tmp_sprintf("\007*%s* %s", GET_NAME(ch), argument);

        if ((vict = get_char_vis(ch, target_str)) != NULL) {
            send_to_char(vict, "%s%s%s%s\r\n",
                         CCYEL(vict, C_SPR),
                         CCBLD(vict, C_NRM), msg, CCNRM(vict, C_SPR));
            send_to_char(ch, "%s%s%s%s\r\n",
                         CCYEL(ch, C_SPR), CCBLD(ch, C_NRM), msg, CCNRM(ch, C_SPR));
            return;
        } else {
            send_to_char(ch, "There is no such person in the game!\r\n");
        }
    }
}

ACMD(do_chat)
{
    struct creature *vict;
    char *vict_str = tmp_getword(&argument);

    if (!*vict_str) {
        send_to_char(ch, "With whom do you wish to chat??\r\n");
    } else if (!(vict = get_char_room_vis(ch, vict_str))) {
        send_to_char(ch, "%s", NOPERSON);
    } else if (vict == ch) {
        switch (number(0,2)) {
        case 0:
            send_to_char(ch, "%s doesn't tell you anything you didn't already know.\r\n", HSSH(ch));
            break;
        case 1:
            send_to_char(ch, "Your conversational partner seems a bit predictable.\r\n");
            break;
        case 2:
            send_to_char(ch, "Talking to yourself is a sign of impending mental collapse.\r\n");
            break;
        }
    } else if (IS_PC(vict)) {
        send_to_char(ch, "Use the SAY command to talk with other players.\r\n");
    } else if (!AWAKE(vict)) {
        act("$n is unconscious and will not speak to you.", false, vict, NULL, ch, TO_VICT);
    } else if (is_fighting(vict)) {
        act("$n is too busy fighting to talk right now!", false, vict, NULL, ch, TO_VICT);
    } else if ((IS_EVIL(ch) && NPC_FLAGGED(vict, NPC_AGGR_EVIL)) ||
               (IS_GOOD(ch) && NPC_FLAGGED(vict, NPC_AGGR_GOOD)) ||
               (IS_NEUTRAL(ch) && NPC_FLAGGED(vict, NPC_AGGR_NEUTRAL))) {
        if (IS_ANIMAL(vict)) {
            act("You viciously snarl at $N!", false, vict, NULL, ch, TO_CHAR);
            act("$n viciously snarls at you!", false, vict, NULL, ch, TO_VICT);
            act("$n viciously snarls at $N!", false, vict, NULL, ch, TO_NOTVICT);
        } else {
            perform_say_to(vict, ch, "You think I'm going to talk with YOU?");
        }
    } else {
        if (!trigger_prog_chat(ch, vict)) {
            if (AFF_FLAGGED(ch, AFF_CHARM)) {
                act("$n merely stares at you in adoration.", false, vict, NULL, ch, TO_VICT);
            } else if (NPC_FLAGGED(vict, NPC_AGGRESSIVE)) {
                act("$n merely stares at you with hate and fury.", false, vict, NULL, ch, TO_VICT);
            } else {
                act("$n doesn't have much to say to you.", false, vict, NULL, ch, TO_VICT);
            }
        }
    }
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
 *********************************************************************/
#define INTERPLANAR  false
#define PLANAR       true
#define NOT_EMOTE    false
#define IS_EMOTE     true

struct channel_info_t {
    const char *name;
    int deaf_vector;
    int deaf_flag;
    bool check_plane;
    bool is_emote;
    const char *desc_color;
    const char *text_color;
    const char *msg_noton;
    const char *msg_muted;
};

struct channel_info_t channels[] = {
    {"holler", 2, PRF2_NOHOLLER, INTERPLANAR, NOT_EMOTE,
     "&Y", "&r",
     "Ha!  You are noholler buddy.",
     "You find yourself unable to holler!"},
    {"shout", 1, PRF_DEAF, PLANAR, NOT_EMOTE,
     "&y", "&c",
     "Turn off your noshout flag first!",
     "You cannot shout!!"},
    {"gossip", 1, PRF_NOGOSS, PLANAR, NOT_EMOTE,
     "&g", "&n",
     "You aren't even on the channel!",
     "You cannot gossip!!"},
    {"auction", 1, PRF_NOAUCT, INTERPLANAR, NOT_EMOTE,
     "&m", "&n",
     "You aren't even on the channel!",
     "Only licenced auctioneers can auction!!"},
    {"congrat", 1, PRF_NOGRATZ, PLANAR, NOT_EMOTE,
     "&g", "&m",
     "You aren't even on the channel!",
     "You cannot congratulate!!"},
    {"sing", 1, PRF_NOMUSIC, PLANAR, NOT_EMOTE,
     "&c", "&y",
     "You aren't even on the channel!",
     "You cannot sing!!"},
    {"spew", 1, PRF_NOSPEW, PLANAR, NOT_EMOTE,
     "&r", "&y",
     "You aren't even on the channel!",
     "You cannot spew!!"},
    {"dream", 1, PRF_NODREAM, PLANAR, NOT_EMOTE,
     "&c", "&W",
     "You aren't even on the channel!",
     "You cannot dream!!"},
    {"project", 1, PRF_NOPROJECT, INTERPLANAR, NOT_EMOTE,
     "&N", "&c",
     "You are not open to projections yourself...",
     "You cannot project.  The immortals have muted you."},
    {"newbie", -2, PRF2_NEWBIE_HELPER, PLANAR, NOT_EMOTE,
     "&y", "&n",
     "You aren't on the illustrious newbie channel.",
     "The immortals have muted you for bad behavior!"},
    {"clan-say", 1, PRF_NOCLANSAY, PLANAR, NOT_EMOTE,
     "&c", "&n",
     "You aren't listening to the words of your clan.",
     "The immortals have muted you.  You may not clan say."},
    {"guild-say", 2, PRF2_NOGUILDSAY, PLANAR, NOT_EMOTE,
     "&m", "&y",
     "You aren't listening to the rumors of your guild.",
     "You may not guild-say, for the immortals have muted you."},
    {"clan-emote", 1, PRF_NOCLANSAY, PLANAR, IS_EMOTE,
     "&c", "&c",
     "You aren't listening to the words of your clan.",
     "The immortals have muted you.  You may not clan emote."},
    {"petition", 1, PRF_NOPETITION, INTERPLANAR, NOT_EMOTE,
     "&m", "&c",
     "You aren't listening to petitions at this time.",
     "The immortals have turned a deaf ear to your petitions."},
    {"haggle", 1, PRF_NOHAGGLE, PLANAR, NOT_EMOTE,
     "&m", "&n",
     "You're not haggling with your peers.",
     "Your haggling has been muted."},
    {"plug", 1, PRF_NOPLUG, INTERPLANAR, NOT_EMOTE,
     "&Y", "&C",
     "You're not listening to the plugs.",
     "Your plugs have been muted."},
};

const char *
random_curses(void)
{
    char curse_map[] = "!@#$%^&*()";
    char curse_buf[7];
    int curse_len, map_len, idx;

    curse_len = number(4, 6);
    map_len = strlen(curse_map);
    for (idx = 0; idx < curse_len; idx++) {
        curse_buf[idx] = curse_map[number(0, map_len - 1)];
    }
    curse_buf[curse_len] = '\0';

    return tmp_strdup(curse_buf);
}

ACMD(do_gen_comm)
{
    extern int level_can_shout;
    extern int holler_move_cost;
    const struct channel_info_t *chan;
    struct descriptor_data *i;
    struct clan_data *clan;
    const char *str, *sub_channel_desc;
    int eff_is_neutral, eff_is_good, eff_is_evil, eff_class, eff_clan;
    int old_language = GET_TONGUE(ch);
    char *imm_actstr, *actstr;

    chan = &channels[subcmd];

    // pets can't shout on interplanar channels
    if (!ch->desc && ch->master && !chan->check_plane) {
        return;
    }

    // Drunk people not allowed!
    if ((GET_COND(ch, DRUNK) > 5) && (number(0, 3) >= 2)) {
        send_to_char(ch,
                     "You try to %s, but somehow it just doesn't come out right.\r\n",
                     chan->name);
        return;
    }
    if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
        send_to_char(ch, "%s", chan->msg_muted);
        return;
    }

    /* make sure the char is on the channel */
    if (chan->deaf_vector == 1 && PRF_FLAGGED(ch, chan->deaf_flag)) {
        send_to_char(ch, "%s\r\n", chan->msg_noton);
        return;
    }
    if (chan->deaf_vector == 2 && PRF2_FLAGGED(ch, chan->deaf_flag)) {
        send_to_char(ch, "%s\r\n", chan->msg_noton);
        return;
    }
    if (chan->deaf_vector == -2 && !PRF2_FLAGGED(ch, chan->deaf_flag)) {
        send_to_char(ch, "%s\r\n", chan->msg_noton);
        return;
    }
    // These are all restrictions, to which immortals and npcs are uncaring
    // Charmed creatures, however, are always subject
    if ((!IS_NPC(ch) && !IS_IMMORT(ch)) || AFF_FLAGGED(ch, AFF_CHARM)) {
        /* level_can_shout defined in config.c */
        if (GET_LEVEL(ch) < level_can_shout &&
            subcmd != SCMD_NEWBIE &&
            subcmd != SCMD_CLANSAY && GET_REMORT_GEN(ch) <= 0) {
            send_to_char(ch,
                         "You must be at least level %d before you can %s.\r\n",
                         level_can_shout, chan->name);
            send_to_char(ch, "Try using the newbie channel instead.\r\n");
            return;
        }
        // Only remort players can project
        if (subcmd == SCMD_PROJECT && !IS_REMORT(ch) &&
            GET_LEVEL(ch) < LVL_AMBASSADOR) {
            send_to_char(ch,
                         "You do not know how to project yourself that way.\r\n");
            return;
        }
        // Players can't auction anymore
        if (subcmd == SCMD_AUCTION) {
            send_to_char(ch,
                         "Only licensed auctioneers can use that channel!\r\n");
            return;
        }

        if (subcmd == SCMD_PLUG) {
            send_to_char(ch, "Only approved criers can use this channel!\r\n");
            return;
        }

        if ((subcmd == SCMD_DREAM &&
             (GET_POSITION(ch) != POS_SLEEPING)) &&
            !ZONE_IS_ASLEEP(ch->in_room->zone)) {
            send_to_char(ch,
                         "You attempt to dream, but realize you need to sleep first.\r\n");
            return;
        }

        if (subcmd == SCMD_NEWBIE && !PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)) {
            send_to_char(ch,
                         "You aren't on the illustrious newbie channel.\r\n");
            return;
        }

        if (subcmd == SCMD_GUILDSAY) {
            if (IS_NEUTRAL(ch)) {
                // Clerics and knights cannot be neutral
                if (GET_CLASS(ch) == CLASS_CLERIC) {
                    send_to_char(ch,
                                 "You have been cast out of the ranks of the blessed.\r\n");
                    return;
                }
                if (GET_CLASS(ch) == CLASS_KNIGHT) {
                    send_to_char(ch,
                                 "You have been cast out of the ranks of the honored.\r\n");
                    return;
                }

            } else {
                // Monks must be neutral
                if (GET_CLASS(ch) == CLASS_MONK) {
                    send_to_char(ch,
                                 "You have been cast out of the monks until your neutrality is regained.\r\n");
                    return;
                }
            }
        }
    }
    // skip leading spaces
    skip_spaces(&argument);

    /* make sure that there is something there to say! */
    if (!*argument) {
        send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n",
                     chan->name, chan->name);
        return;
    }

    if (subcmd == SCMD_HOLLER && GET_LEVEL(ch) < LVL_TIMEGOD) {
        if (GET_MOVE(ch) < holler_move_cost) {
            send_to_char(ch, "You're too exhausted to holler.\r\n");
            return;
        } else {
            GET_MOVE(ch) -= holler_move_cost;
        }
    }

    eff_is_neutral = IS_NEUTRAL(ch);
    eff_is_evil = IS_EVIL(ch);
    eff_is_good = IS_GOOD(ch);
    eff_class = GET_CLASS(ch);
    eff_clan = GET_CLAN(ch);

    if (subcmd == SCMD_GUILDSAY && is_authorized(ch, HEAR_ALL_CHANNELS, NULL)
        && *argument == '>') {
        char *class_str, *tmp_arg;

        tmp_arg = argument + 1;
        class_str = tmp_getword(&tmp_arg);
        if (!strcmp(class_str, "e-cleric")) {
            eff_is_neutral = false;
            eff_is_evil = true;
            eff_is_good = false;
            eff_class = CLASS_CLERIC;
        } else if (!strcmp(class_str, "g-cleric")) {
            eff_is_neutral = false;
            eff_is_evil = false;
            eff_is_good = true;
            eff_class = CLASS_CLERIC;
        } else if (!strcmp(class_str, "e-knight")) {
            eff_is_neutral = false;
            eff_is_evil = true;
            eff_is_good = false;
            eff_class = CLASS_KNIGHT;
        } else if (!strcmp(class_str, "g-knight")) {
            eff_is_neutral = false;
            eff_is_evil = false;
            eff_is_good = true;
            eff_class = CLASS_KNIGHT;
        } else {
            eff_class = parse_player_class(class_str);
        }

        if (eff_class == CLASS_MONK) {
            eff_is_neutral = true;
            eff_is_evil = false;
            eff_is_good = false;
        }
        if (eff_class == CLASS_UNDEFINED) {
            send_to_char(ch, "That guild doesn't exist in this universe.\r\n");
            return;
        }

        argument = tmp_arg;
    }

    if (subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE) {
        if (is_authorized(ch, HEAR_ALL_CHANNELS, NULL) && *argument == '>') {
            char *tmp_arg;

            tmp_arg = argument + 1;
            clan = clan_by_name(tmp_getword(&tmp_arg));
            if (!clan) {
                send_to_char(ch, "That clan does not exist.\r\n");
                return;
            }

            eff_clan = clan->number;
            argument = tmp_arg;
        }

        if (IS_NPC(ch) || !eff_clan) {
            send_to_char(ch, "You aren't a member of a clan!\r\n");
            return;
        }
    }

    if (subcmd == SCMD_GUILDSAY) {
        if (eff_class >= 0 && eff_class < TOP_CLASS) {
            str = tmp_tolower(class_names[eff_class]);
        } else {
            str = tmp_sprintf("#%d", eff_class);
        }
        if (eff_class == CLASS_CLERIC || eff_class == CLASS_KNIGHT) {
            str = tmp_sprintf("%s-%s", (eff_is_good ? "g" : "e"), str);
        }
        sub_channel_desc = tmp_strcat("[", str, "] ", NULL);
    } else if (subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE) {
        clan = real_clan(eff_clan);

        if (clan) {
            str = tmp_tolower(clan->name);
        } else {
            str = tmp_sprintf("#%d", eff_clan);
        }
        sub_channel_desc = tmp_strcat("[", str, "] ", NULL);
    } else {
        sub_channel_desc = "";
    }

    // Check to see if they're calling for help
    if (subcmd == SCMD_SHOUT && IS_NPC(ch)
        && !AFF_FLAGGED(ch, AFF_CHARM)
        && strstr(argument, "help")
        && strstr(argument, "!")) {
        summon_cityguards(ch->in_room);
    }

    // Construct all the emits ahead of time.
    if (chan->is_emote) {
        imm_actstr = tmp_sprintf("%s%s$n$a %s%s",
                                 chan->desc_color,
                                 sub_channel_desc, chan->text_color, act_escape(argument));
        actstr = tmp_sprintf("%s$n$a %s%s",
                             chan->desc_color, chan->text_color, act_escape(argument));
    } else {
        // Newbie channel is always in common
        if (subcmd == SCMD_NEWBIE) {
            GET_TONGUE(ch) = TONGUE_COMMON;
        }

        imm_actstr = tmp_sprintf("%s%s$t$a %s$%%$l, %s'$[%s]'",
                                 chan->desc_color,
                                 sub_channel_desc,
                                 chan->name, chan->text_color, act_escape(argument));
        actstr = tmp_sprintf("%s$t$a %s$%%$l, %s'$[%s]'",
                             chan->desc_color,
                             chan->name, chan->text_color, act_escape(argument));
    }

    /* now send all the strings out */
    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CXN_PLAYING || !i->creature ||
            PLR_FLAGGED(i->creature, PLR_WRITING)) {
            continue;
        }

        if (chan->deaf_vector == 1 &&
            PRF_FLAGGED(i->creature, chan->deaf_flag)) {
            continue;
        }
        if (chan->deaf_vector == 2 &&
            PRF2_FLAGGED(i->creature, chan->deaf_flag)) {
            continue;
        }
        if (chan->deaf_vector == -2 &&
            !PRF2_FLAGGED(i->creature, chan->deaf_flag)) {
            continue;
        }

        // Must be in same clan or an admin to hear clansay
        if ((subcmd == SCMD_CLANSAY || subcmd == SCMD_CLANEMOTE)
            && GET_CLAN(i->creature) != eff_clan
            && !is_authorized(i->creature, HEAR_ALL_CHANNELS, NULL)) {
            continue;
        }

        // Must be in same guild or an admin to hear guildsay
        if (subcmd == SCMD_GUILDSAY &&
            GET_CLASS(i->creature) != eff_class &&
            !is_authorized(i->creature, HEAR_ALL_CHANNELS, NULL)) {
            continue;
        }

        // Evil and good clerics and knights have different guilds
        if (subcmd == SCMD_GUILDSAY &&
            (GET_CLASS(i->creature) == CLASS_CLERIC ||
             GET_CLASS(i->creature) == CLASS_KNIGHT) &&
            !is_authorized(i->creature, HEAR_ALL_CHANNELS, NULL)) {
            if (eff_is_neutral) {
                continue;
            }
            if (eff_is_evil && !IS_EVIL(i->creature)) {
                continue;
            }
            if (eff_is_good && !IS_GOOD(i->creature)) {
                continue;
            }
        }
        // Outcast monks don't hear other monks
        if (subcmd == SCMD_GUILDSAY &&
            GET_CLASS(i->creature) == CLASS_MONK &&
            !IS_NEUTRAL(i->creature) &&
            !is_authorized(i->creature, HEAR_ALL_CHANNELS, NULL)) {
            continue;
        }

        if (IS_NPC(ch) || !IS_IMMORT(i->creature)) {
            if (subcmd == SCMD_PROJECT && !IS_REMORT(i->creature)) {
                continue;
            }

            if (subcmd == SCMD_DREAM &&
                GET_POSITION(i->creature) != POS_SLEEPING) {
                continue;
            }

            if (subcmd == SCMD_SHOUT &&
                ((ch->in_room->zone != i->creature->in_room->zone) ||
                 GET_POSITION(i->creature) < POS_RESTING)) {
                continue;
            }

            if (subcmd == SCMD_PETITION && i->creature != ch) {
                continue;
            }

            if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) ||
                 ROOM_FLAGGED(i->creature->in_room, ROOM_SOUNDPROOF)) &&
                !IS_IMMORT(ch) && i->creature->in_room != ch->in_room) {
                continue;
            }

            if (chan->check_plane && !can_channel_comm(ch, i->creature)) {
                continue;
            }
        }

        if (IS_IMMORT(i->creature)) {
            act(imm_actstr, false, ch, NULL, i->creature, TO_VICT | TO_SLEEP);
        } else {
            act(actstr, false, ch, NULL, i->creature, TO_VICT | TO_SLEEP);
        }
    }

    GET_TONGUE(ch) = old_language;

    if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)
        && !IS_IMMORT(ch)
        && subcmd != SCMD_PETITION) {
        send_to_char(ch, "The walls seem to absorb your words.\r\n");
    }
}

#undef __act_comm_cc__
