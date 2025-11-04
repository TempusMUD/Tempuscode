#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
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
#include "specs.h"
#include "language.h"
#include "prog.h"
#include "voice.h"
#include "olc.h"
#include "editor.h"
#include "strutil.h"
#include "challenge.h"

struct challenge *
do_create_challenge(struct creature *ch, int vnum)
{
    if (g_hash_table_lookup(challenges, GINT_TO_POINTER(vnum))) {
        send_to_char(ch, "ERROR: Challenge already exists.\r\n");
        return NULL;
    }

    struct zone_data *zone = zone_owner(vnum);
    if (!zone) {
        send_to_char(ch, "ERROR: A zone must be defined for the challenge first.\r\n");
        return NULL;
    }

    if (!is_authorized(ch, EDIT_ZONE, zone)) {
        send_to_char(ch, "Try creating challenges in your own zone, luser.\r\n");
        mudlog(GET_INVIS_LVL(ch), BRF, true,
               "OLC: %s failed attempt to CREATE challenge %d.", GET_NAME(ch), vnum);
        return NULL;
    }

    struct challenge *chal = make_challenge();

    chal->idnum = vnum;
    chal->label = strdup("fresh-n-blank");
    chal->name = strdup("A Fresh Blank Challenge");
    chal->desc = strdup("A Fresh Blank Challenge");
    chal->stages = 1;
    g_hash_table_insert(challenges, GINT_TO_POINTER(chal->idnum), chal);

    return chal;
}

bool
do_destroy_challenge(struct creature *ch, int vnum)
{
    if (!challenge_by_idnum(vnum)) {
        send_to_char(ch, "That challenge does not exist.\r\n");
        return false;
    }
    if (GET_OLC_CHALLENGE(ch) && vnum == GET_OLC_CHALLENGE(ch)->idnum) {
        GET_OLC_CHALLENGE(ch) = NULL;
    }
    g_hash_table_remove(challenges, GINT_TO_POINTER(vnum));
    return true;
}

void
do_challenge_edit(struct creature *ch, char *argument)
{
    struct challenge *chal = NULL;

    chal = GET_OLC_CHALLENGE(ch);

    if (!*argument) {
        if (!chal) {
            send_to_char(ch, "You are not currently editing a challenge.\r\n");
        } else {
            send_to_char(ch, "Current olc challenge: [%5d] %s\r\n",
                         chal->idnum, chal->name);
        }
        return;
    }

    if (!is_number(argument)) {
        if (is_abbrev(argument, "exit")) {
            send_to_char(ch, "Exiting challenge editor.\r\n");
            GET_OLC_CHALLENGE(ch) = NULL;
            return;
        }
        send_to_char(ch, "The argument must be a number.\r\n");
        return;
    }

    int j = atoi(argument);
    chal = challenge_by_idnum(j);

    if (!chal) {
        send_to_char(ch, "There is no such challenge.\r\n");
        return;
    }

    struct zone_data *zone = zone_owner(j);
    if (!zone) {
        send_to_char(ch, "That challenge does not belong to any zone!!\r\n");
        errlog("mobile %d not in any zone", j);
        return;
    }


    if (!is_authorized(ch, EDIT_ZONE, zone)) {
        send_to_char(ch, "You do not have permission to edit that challenge.\r\n");
        return;
    }

    if (!OLC_EDIT_OK(ch, zone, 0)) {
        send_to_char(ch, "Challenge OLC is not approved for this zone.\r\n");
        return;
    }

    for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
        if (d->creature && GET_OLC_CHALLENGE(d->creature) == chal) {
            act("$N is already editing that challenge.", false, ch, NULL,
                d->creature, TO_CHAR);
            return;
        }
    }

    GET_OLC_CHALLENGE(ch) = chal;
    send_to_char(ch, "Now editing challenge [%d] %s%s%s\r\n",
                 chal->idnum, CCGRN(ch, C_NRM), chal->name, CCNRM(ch, C_NRM));
}

void
do_challenge_stat(struct creature *ch, char *argument)
{
    struct challenge *chal = GET_OLC_CHALLENGE(ch);

    if (!chal) {
        send_to_char(ch, "You are not currently editing a challenge.\r\n");
        return;
    }

    send_to_char(ch, "%s CHALLENGE %d -- %d stage%s\r\nLabel: %s\r\nName: '%s'\r\nDescription: %s\r\n",
                 chal->secret ? "SECRET":"PUBLIC",
                 chal->idnum,
                 chal->stages,
                 (chal->stages == 1) ? "":"s",
                 chal->label,
                 chal->name,
                 chal->desc);
}

void
do_challenge_set(struct creature *ch, char *argument)
{
    struct challenge *chal = GET_OLC_CHALLENGE(ch);

    if (!chal) {
        send_to_char(ch, "You are not currently editing a challenge.\r\n");
        return;
    }

    if (!*argument) {
        page_string(ch->desc, "Valid cset commands:\r\n"
                    "label\r\n"
                    "name\r\n"
                    "description\r\n"
                    "secret\r\n"
                    "stages\r\n");
        return;
    }

    char *arg1 = tmp_getword(&argument);

    if (is_abbrev(arg1, "label")) {
        free(chal->label);
        chal->label = strdup(argument);
        send_to_char(ch, "Challenge %d label set to '%s'\r\n", chal->idnum, chal->label);
    } else if (is_abbrev(arg1, "name")) {
        free(chal->name);
        chal->name = strdup(argument);
        send_to_char(ch, "Challenge %d name set to '%s'\r\n", chal->idnum, chal->name);
    } else if (is_abbrev(arg1, "description")) {
        free(chal->desc);
        chal->desc = strdup(argument);
        send_to_char(ch, "Challenge %d description set to '%s'\r\n", chal->idnum, chal->desc);
    } else if (is_abbrev(arg1, "secret")) {
        chal->secret = (streq(argument, "yes") || streq(argument, "on"));
        send_to_char(ch, "Challenge %d secrecy set to '%s'\r\n", chal->idnum, chal->secret ? "SECRET":"PUBLIC");
    } else if (is_abbrev(arg1, "stages")) {
        if (!is_number(argument)) {
            send_to_char(ch, "Challenge stages must be a number greater than zero.\r\n");
            return;
        }
        int i = atoi(argument);
        if (i < 1) {
            send_to_char(ch, "Challenge stages must be a number greater than zero.\r\n");
            return;
        }
        chal->stages = i;
        send_to_char(ch, "Challenge %d stage count set to %d\r\n", chal->idnum, chal->stages);
    } else {
        send_to_char(ch, "Invalid cset command '%s'.\r\n", arg1);
    }
}

void
do_approve_challenge(struct creature *ch, char *argument)
{
    int j = atoi(argument);
    struct challenge *chal = challenge_by_idnum(j);

    if (!chal) {
        send_to_char(ch, "There is no such challenge.\r\n");
        return;
    }

   struct zone_data *zone = zone_owner(j);
    if (!zone) {
        send_to_char(ch, "That challenge does not belong to any zone!!\r\n");
        errlog("mobile %d not in any zone", j);
        return;
    }


    if (!is_authorized(ch, APPROVE_ZONE, zone)) {
        send_to_char(ch, "You do not have permission to approve that challenge.\r\n");
        return;
    }

    if (chal->approved) {
        send_to_char(ch, "That challenge is already approved.\r\n");
        return;
    }

    chal->approved = 1;
    save_challenges("etc/challenges.xml");
    send_to_char(ch, "Challenge %d approved for full inclusion in game.\r\n", chal->idnum);
}
