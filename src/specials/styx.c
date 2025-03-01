/* ************************************************************************
*   File: styx.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: styx.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "tmpstr.h"
#include "spells.h"

/*   external vars  */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;

#define DUNGEON_SE      126
#define STYX_PRIESTESS  103

SPECIAL(underworld_goddess)
{
    struct creature *vict = NULL;
    struct creature *styx = NULL;
    struct room_data *room;

    /* See if Styx is in the room */
    for (GList *it = ch->in_room->people; it; it = it->next) {
        struct creature *tch = it->data;
        if (!strcmp(GET_NAME(tch), "Styx")) {
            styx = tch;
            break;
        }
    }
    /* First and foremost, if I am fighting, beat the shit out of them. */
    if (!cmd && (GET_POSITION(ch) == POS_FIGHTING)) {
        vict = NULL;
        for (GList *it = ch->in_room->people; it; it = it->next) {
            struct creature *tch = it->data;
            if (g_list_find(tch->fighting, ch) && !number(0, 2)) {
                break;
            }
        }

        /* if I didn't pick any of those, then just slam the guy I'm fighting */
        if (vict == NULL) {
            vict = random_opponent(ch);
        }
        if (vict == NULL) {
            return 1;
        }

        /* if I'm fighting styx, try to teleport him away.  */
        if (vict == styx) {
            cast_spell(ch, vict, NULL, NULL, SPELL_RANDOM_COORDINATES);
        }

        /* Whip up some magic!  */
        switch (number(0, 9)) {
        case 0:
            cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL);
            break;
        case 1:
            cast_spell(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE);
            break;
        case 2:
            cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT);
            break;
        case 3:
            cast_spell(ch, vict, NULL, NULL, SPELL_SLEEP);
            break;
        case 4:
        case 5:
        case 6:
            cast_spell(ch, ch, NULL, NULL, SPELL_GREATER_HEAL);
            break;
        }

        /*  Check to make sure I haven't killed him! */
        if (g_list_find(vict->fighting, ch)) {
            return 1;
        }

        /* And maybe say something nice! */
        switch (number(0, 7)) {
        case 0:
            send_to_char(vict,
                         "The goddess tells you, 'You are a fool "
                         "to fight me %s.'\r\n", GET_NAME(vict));
            break;
        case 1:
            send_to_char(vict,
                         "The goddess tells you, 'Fool!  Now watch "
                         "yourself perish!'\r\n");
            break;
        }
        return 1;
    }

    /* check to see if Styx is hurt. */
    if (!cmd && styx && ((GET_MAX_HIT(styx) - GET_HIT(styx)) > 0)) {
        switch (number(0, 1)) {
        case 0:
            send_to_char(styx,
                         "The goddess tell you, 'My Love, you are hurt!  Let me heal you.'\r\n");
            cast_spell(ch, styx, NULL, NULL, SPELL_GREATER_HEAL);
            break;
        }

        /* If Styx is fighting, send a present to his opponent. */
        if ((vict = random_opponent(styx))) {
            switch (number(0, 4)) {
            case 0:
                send_to_char(vict,
                             "The Goddess shouts, 'Chew on this worm face!'\r\n");
                cast_spell(ch, vict, NULL, NULL, SPELL_FIREBALL);
                break;
            case 1:
                send_to_char(vict,
                             "The Goddess shouts, 'You FIEND!  Take this!'\r\n");
                cast_spell(ch, vict, NULL, NULL, SPELL_FLAME_STRIKE);
                break;
            case 2:
                send_to_char(vict,
                             "The Goddess shouts, 'Have a LIGHT sewer breath!'\r\n");
                cast_spell(ch, vict, NULL, NULL, SPELL_LIGHTNING_BOLT);
                break;
            case 3:
                cast_spell(ch, vict, NULL, NULL, SPELL_SLEEP);
                break;
            }
        }
        return 1;
    }

    /* If Nothing else, maybe send Styx a kiss */
    if (!cmd && styx) {
        switch (number(0, 20)) {
        case 0:
            act("The Goddess of the Underworld kisses $n.", false, styx, NULL, NULL,
                TO_ROOM);
            send_to_char(styx, "The goddess kisses you very gently.\r\n");
            break;
        }
        return 1;
    }

    /* If styx is not with ME then he might be in the dungeon with that HARLET, */
    /*   so, she might as well worship him TOO! */
    if (!cmd && !styx && !number(0, 5)) {
        /*  First get the room number for prison cell where that harlet stays! */
        if (!(room = real_room(DUNGEON_SE))) {
            return 0;
        }

        /* Now see if Styx is there */
        for (GList *it = room->people; it; it = it->next) {
            struct creature *tch = it->data;
            if (!strcmp(GET_NAME(tch), "Styx")) {
                styx = tch;
                break;          /* breaks out of for */
            }
        }
        /*  If Styx is there, see if one of those cutsie priestess girls are there too.   */
        if (styx) {
            vict = NULL;
            for (GList *it = styx->in_room->people; it; it = it->next) {
                struct creature *tch = it->data;
                if (IS_NPC(tch) && (GET_NPC_VNUM(tch) == STYX_PRIESTESS)) {
                    vict = tch;
                    break;      /* breaks for loop! */
                }
            }

            if (vict) {
                act("The young priestess starts kissing $n all over his body!",
                    false, styx, NULL, NULL, TO_ROOM);
                send_to_char(styx,
                             "The young priestess starts kissing you in some very sensitive areas!\r\n");
                return true;
            }
        }
    }

    /* NOW, check for the pass-phrase for stroking! MUHAHA */

    if (CMD_IS("say") && !strncasecmp(argument, " styx sent me", 13)) {
        act("The Goddess of the Underworld starts stroking $n's inner thigh.",
            false, ch, NULL, NULL, TO_ROOM);
        send_to_char(ch,
                     "The Goddess of the Underworld gently strokes your inner thigh with feathery touches.\r\n");
        return true;
    }

    return false;
}
