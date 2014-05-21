//
// File: quest.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

//
//  File: quest.c -- Quest system for Tempus MUD
//  by Fireball, December 1997
//
//  Copyright 1998 John Watson
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
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "screen.h"
#include "players.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"

// external funcs here
ACMD(do_switch);

// external vars here
extern struct descriptor_data *descriptor_list;

// internal vars here

const struct qcontrol_option {
    const char *keyword;
    const char *usage;
    int level;
} qc_options[] = {
    {
    "show", "[quest vnum]", LVL_AMBASSADOR}, {
    "create", "<type> <name>", LVL_AMBASSADOR}, {
    "end", "<quest vnum>", LVL_AMBASSADOR}, {
    "add", "<name> <vnum>", LVL_AMBASSADOR}, {
    "kick", "<player> <quest vnum>", LVL_AMBASSADOR},   // 5
    {
    "flags", "<quest vnum> <+/-> <flags>", LVL_AMBASSADOR}, {
    "comment", "<quest vnum> <comments>", LVL_AMBASSADOR}, {
    "describe", "<quest vnum>", LVL_AMBASSADOR}, {
    "update", "<quest vnum>", LVL_AMBASSADOR}, {
    "ban", "<player> <quest vnum>|\'all\'", LVL_AMBASSADOR},    // 10
    {
    "unban", "<player> <quest vnum>|\'all\'", LVL_AMBASSADOR}, {
    "mute", "<player> <quest vnum>", LVL_AMBASSADOR}, {
    "unmute", "<player> <quest vnum>", LVL_AMBASSADOR}, {
    "level", "<quest vnum> <access level>", LVL_AMBASSADOR}, {
    "minlev", "<quest vnum> <minlev>", LVL_AMBASSADOR}, // 15
    {
    "maxlev", "<quest vnum> <maxlev>", LVL_AMBASSADOR}, {
    "mingen", "<quest vnum> <min generation>", LVL_AMBASSADOR}, {
    "maxgen", "<quest vnum> <max generation>", LVL_AMBASSADOR}, {
    "mload", "<mobile vnum> <vnum>", LVL_IMMORT}, {
    "purge", "<quest vnum> <mobile name>", LVL_IMMORT}, // 20
    {
    "save", "", LVL_IMMORT}, {
    "help", "<topic>", LVL_AMBASSADOR}, {
    "switch", "<mobile name>", LVL_IMMORT}, {
    "title", "<quest vnum> <title>", LVL_AMBASSADOR}, {
    "oload", "<item num> <vnum>", LVL_AMBASSADOR}, {
    "trans", "<quest vnum> [room number]", LVL_AMBASSADOR}, {
    "award", "<quest vnum> <player> <pts> [comments]", LVL_AMBASSADOR}, {
    "penalize", "<quest vnum> <player> <pts> [reason]", LVL_AMBASSADOR}, {
    "restore", "<quest vnum>", LVL_AMBASSADOR}, {
    "loadroom", "<quest vnum> <room number>", LVL_AMBASSADOR}, {
    NULL, NULL, 0}              // list terminator
};

const char *qtypes[] = {
    "trivia",
    "scavenger",
    "hide-and-seek",
    "roleplay",
    "pkill",
    "award/payment",
    "misc",
    "\n"
};

const char *qtype_abbrevs[] = {
    "trivia",
    "scav",
    "h&s",
    "RP",
    "pkill",
    "A/P",
    "misc",
    "\n"
};

const char *quest_bits[] = {
    "REVIEWED",
    "NOWHO",
    "NO_OUTWHO",
    "NOJOIN",
    "NOLEAVE",
    "HIDE",
    "WHOWHERE",
    "ARENA",
    "\n"
};

const char *quest_bit_descs[] = {
    "This quest has been reviewed.",
    "The \'quest who\' command does not work in this quest.",
    "Players in this quest cannot use the \'who\' command.",
    "Players may not join this quest.",
    "Players may not leave this quest.",
    "Players cannot see this quest until this flag is removed.",
    "\'quest who\' will show the locations of other questers.",
    "Players will only die arena deaths.",
    "\n"
};

const char *qlog_types[] = {
    "off",
    "brief",
    "normal",
    "complete",
    "\n"
};

const char *qp_bits[] = {
    "DEAF",
    "MUTE",
    "\n"
};

#define QUEST_PATH "etc/quest.xml"
#define PLURAL(num) (num == 1 ? "" : "s")

GList *quests = NULL;

/**
 * next_quest_vnum
 *
 * Returns: a number one more than the current maximum quest vnum
 *
 */
int
next_quest_vnum(void)
{
    int new_id = 1;

    for (GList *qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        if (new_id <= quest->vnum)
            new_id = quest->vnum + 1;
    }
    return new_id;
}

/**
 * make_quest:
 * @owner_id The player id of the player creating the quest
 * @owner_level The level of the player creating the quest
 * @type The type of the quest
 * @name The name of the quest
 *
 * Creates a new #quest, initializes its properties and assigns it a
 * new quest vnum.
 *
 * Returns: A pointer to the created quest
 **/
struct quest *
make_quest(long owner_id, int owner_level, int type, const char *name)
{
    struct quest *quest;

    CREATE(quest, struct quest, 1);
    quest->vnum = next_quest_vnum();
    quest->type = type;
    quest->name = strdup(name);
    quest->owner_id = owner_id;
    quest->owner_level = owner_level;

    quest->flags = QUEST_HIDE;
    quest->started = time(NULL);
    quest->ended = 0;

    quest->description = (char *)malloc(sizeof(char) * 2);
    quest->description[0] = ' ';
    quest->description[1] = '\0';

    quest->updates = (char *)malloc(sizeof(char) * 2);
    quest->updates[0] = ' ';
    quest->updates[1] = '\0';

    quest->max_players = 0;
    quest->awarded = 0;
    quest->penalized = 0;
    quest->minlevel = 0;
    quest->maxlevel = 49;
    quest->mingen = 0;
    quest->maxgen = 10;
    quest->loadroom = -1;

    return quest;
}

/**
 * load_quest:
 * @n An XML node pointer with the quest data
 * @doc A pointer to the XML document which owns @n
 *
 * Loads a single quest from an XML document.
 *
 * Returns: a newly allocated quest with fields set
 **/
struct quest *
load_quest(xmlNodePtr n, xmlDocPtr doc)
{
    xmlChar *s;
    struct quest *quest;

    CREATE(quest, struct quest, 1);
    quest->vnum = xmlGetIntProp(n, "VNUM", 0);
    quest->owner_id = xmlGetLongProp(n, "OWNER", 0);
    quest->started = (time_t) xmlGetLongProp(n, "STARTED", 0);
    quest->ended = (time_t) xmlGetLongProp(n, "ENDED", 0);
    quest->max_players = xmlGetIntProp(n, "MAX_PLAYERS", 0);
    quest->maxlevel = xmlGetIntProp(n, "MAX_LEVEL", 0);
    quest->minlevel = xmlGetIntProp(n, "MIN_LEVEL", 0);
    quest->maxgen = xmlGetIntProp(n, "MAX_GEN", 0);
    quest->mingen = xmlGetIntProp(n, "MIN_GEN", 0);
    quest->awarded = xmlGetIntProp(n, "AWARDED", 0);
    quest->penalized = xmlGetIntProp(n, "PENALIZED", 0);
    quest->owner_level = xmlGetIntProp(n, "OWNER_LEVEL", 0);
    quest->flags = xmlGetIntProp(n, "FLAGS", 0);
    quest->loadroom = xmlGetIntProp(n, "LOADROOM", 0);
    char *typest = (char *)xmlGetProp(n, (xmlChar *) "TYPE");
    quest->type = search_block(typest, qtype_abbrevs, true);
    free(typest);

    quest->name = (char *)xmlGetProp(n, (xmlChar *) "NAME");

    quest->description = quest->updates = NULL;

    xmlNodePtr cur = n->xmlChildrenNode;
    while (cur != NULL) {
        if ((xmlMatches(cur->name, "Description"))) {
            s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (s != NULL) {
                quest->description = (char *)s;
            }
        } else if ((xmlMatches(cur->name, "Update"))) {
            s = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (s != NULL) {
                quest->updates = (char *)s;
            }
        } else if ((xmlMatches(cur->name, "Player"))) {
            long id = xmlGetLongProp(cur, "ID", 0);
            int flags = xmlGetIntProp(cur, "FLAGS", 0);
            struct qplayer_data *player;
            CREATE(player, struct qplayer_data, 1);
            player->idnum = id;
            player->flags = flags;
            player->deaths = xmlGetIntProp(cur, "DEATHS", 0);
            player->mobkills = xmlGetIntProp(cur, "MKILLS", 0);
            player->pkills = xmlGetIntProp(cur, "PKILLS", 0);
            quest->players = g_list_prepend(quest->players, player);
        } else if ((xmlMatches(cur->name, "Ban"))) {
            long id = xmlGetLongProp(cur, "ID", 0);
            quest->bans = g_list_prepend(quest->bans, GINT_TO_POINTER(id));
        }
        cur = cur->next;
    }

    quest->players = g_list_reverse(quest->players);
    quest->bans = g_list_reverse(quest->bans);

    return quest;
}

/**
 * save_quest:
 * @quest the quest to be saved
 * @out A file pointer to the output file
 *
 * Saves a single quest in XML format.
 **/
void
save_quest(struct quest *quest, FILE * out)
{
    const char *indent = "    ";

    fprintf(out,
        "%s<Quest VNUM=\"%d\" NAME=\"%s\" OWNER=\"%ld\" STARTED=\"%d\" "
        "ENDED=\"%d\" MAX_PLAYERS=\"%d\" MAX_LEVEL=\"%d\" MIN_LEVEL=\"%d\" "
        "MAX_GEN=\"%d\" MIN_GEN=\"%d\" AWARDED=\"%d\" PENALIZED=\"%d\" "
        "TYPE=\"%s\" OWNER_LEVEL=\"%d\" FLAGS=\"%d\" LOADROOM=\"%d\">\n",
        indent, quest->vnum, xmlEncodeTmp(quest->name), quest->owner_id,
        (int)quest->started, (int)quest->ended, quest->max_players,
        quest->maxlevel, quest->minlevel, quest->maxgen, quest->mingen,
        quest->awarded, quest->penalized,
        xmlEncodeTmp(qtype_abbrevs[quest->type]), quest->owner_level,
        quest->flags, quest->loadroom);

    if (quest->description)
        fprintf(out, "%s%s<Description>%s</Description>\n",
            indent, indent, xmlEncodeTmp(quest->description));
    if (quest->updates)
        fprintf(out, "%s%s<Update>%s</Update>\n",
            indent, indent, xmlEncodeTmp(quest->updates));

    for (GList * pit = quest->players; pit; pit = pit->next) {
        struct qplayer_data *player = pit->data;
        fprintf(out,
            "%s%s<Player ID=\"%ld\" FLAGS=\"%d\" DEATHS=\"%d\" MKILLS=\"%d\" PKILLS=\"%d\"/>\n",
            indent, indent, player->idnum, player->flags, player->deaths,
            player->mobkills, player->pkills);
    }

    for (GList * bit = quest->bans; bit; bit = bit->next) {
        fprintf(out, "%s%s<Ban ID=\"%d\" FLAGS=\"0\"/>\n",
            indent, indent, GPOINTER_TO_INT(bit->data));
    }
    fprintf(out, "%s</Quest>\n", indent);
}

/**
 * free_quest:
 * @quest the quest to be freed
 *
 * Deallocates all memory used by the given quest.
 **/
void
free_quest(struct quest *quest)
{
    free(quest->name);
    free(quest->description);
    free(quest->updates);
    g_list_foreach(quest->players, (GFunc) free, NULL);
    g_list_free(quest->players);
    g_list_free(quest->bans);
    free(quest);
}

/**
 * quest_player_by_idnum:
 * @quest the #quest to search for a quest player
 * @idnum the idnum of the player to look for
 *
 * Finds the quest player record of the player with the given id.
 *
 * Returns: A pointer to a #qplayer_data struct if found.  %NULL if
 * not found.
 **/
struct qplayer_data *
quest_player_by_idnum(struct quest *quest, int idnum)
{
    for (GList * pit = quest->players; pit; pit = pit->next) {
        struct qplayer_data *player = pit->data;
        if (player->idnum == idnum)
            return player;
    }
    return NULL;
}

/**
 * banned_from_quest:
 * @quest the #quest being queried
 * @idnum the idnum of the potentially banned player
 *
 * Determines if the player is banned from a particular quest.
 *
 * Returns: %true if the player is banned.  %false if the player is
 * not banned.
 **/
bool
banned_from_quest(struct quest * quest, int id)
{
    return g_list_find(quest->bans, GINT_TO_POINTER(id));
}

/**
 * add_quest_player:
 * @quest the #quest to which the player is to be added
 * @id the idnum of the player
 *
 * Adds the player to the quest.
 **/
void
add_quest_player(struct quest *quest, int id)
{
    struct qplayer_data *player;

    CREATE(player, struct qplayer_data, 1);
    player->idnum = id;
    quest->players = g_list_prepend(quest->players, player);
}

/**
 * remove_quest_player:
 * @quest the #quest from which the player is to be removed
 * @id the idnum of the player
 *
 * Removes the player from the quest, updating the quest property in
 * the character record.  The character is loaded from file if not
 * present in the game world.
 *
 * Returns: %true if the removal succeeded
 **/
bool
remove_quest_player(struct quest * quest, int id)
{
    struct qplayer_data *player = NULL;
    struct creature *vict = NULL;

    player = quest_player_by_idnum(quest, id);
    if (!player)
        return false;

    quest->players = g_list_remove(quest->players, player);

    vict = get_char_in_world_by_idnum(player->idnum);

    if (vict) {
        GET_QUEST(vict) = 0;
        crashsave(vict);
    } else {
        // load the char from file
        vict = load_player_from_xml(id);
        if (vict) {
            GET_QUEST(vict) = 0;
            save_player_to_xml(vict);
            free_creature(vict);
        } else {
            errlog("Error loading player %d from file for removal from quest %d.\r\n",
                   id, quest->vnum);
            return false;
        }
    }

    return true;
}

/**
 * can_join_quest:
 * @quest the #quest being queried
 * @ch the #creature being queried about
 *
 * Determines if @ch can join the @quest, and sends a message to @ch
 * if unable to join.
 *
 * Returns: %true if @ch can join the @quest.  %false if @ch cannot
 * join @quest.
 **/
bool
can_join_quest(struct quest * quest, struct creature * ch)
{
    if (QUEST_FLAGGED(quest, QUEST_NOJOIN)) {
        send_to_char(ch, "This quest is open by invitation only.\r\n"
            "Contact the wizard in charge of the quest for an invitation.\r\n");
        return false;
    }
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
        return true;

    if (GET_REMORT_GEN(ch) > quest->maxgen) {
        send_to_char(ch, "Your generation is too high for this quest.\r\n");
        return false;
    }

    if (GET_LEVEL(ch) > quest->maxlevel) {
        send_to_char(ch, "Your level is too high for this quest.\r\n");
        return false;
    }
    if (GET_REMORT_GEN(ch) < quest->mingen) {
        send_to_char(ch, "Your generation is too low for this quest.\r\n");
        return false;
    }

    if (GET_LEVEL(ch) < quest->minlevel) {
        send_to_char(ch, "Your level is too low for this quest.\r\n");
        return false;
    }

    if (banned_from_quest(quest, GET_IDNUM(ch))) {
        send_to_char(ch, "Sorry, you have been banned from this quest.\r\n");
        return false;
    }

    if (ch->account->quest_banned) {
        send_to_char(ch, "Sorry, you have been banned from all quests.\r\n");
        return false;
    }

    return true;
}

/**
 * load_quests:
 *
 * Loads all quest information from the saved quest file.
 **/
void
load_quests(void)
{
    g_list_foreach(quests, (GFunc) free_quest, NULL);
    g_list_free(quests);
    quests = NULL;

    xmlDocPtr doc = xmlParseFile(QUEST_PATH);
    if (doc == NULL) {
        errlog("Quesst load FAILED.");
        return;
    }
    // discard root node
    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xmlFreeDoc(doc);
        return;
    }

    cur = cur->xmlChildrenNode;
    // Load all the nodes in the file
    while (cur != NULL) {
        // But only question nodes
        if ((xmlMatches(cur->name, "Quest")))
            quests = g_list_prepend(quests, load_quest(cur, doc));
        cur = cur->next;
    }
    xmlFreeDoc(doc);
}

/**
 * save_quests:
 *
 * Saves all quest information to the saved quest file.
 **/
void
save_quests(void)
{
    FILE *out;

    out = fopen(QUEST_PATH, "w");
    if (!out) {
        errlog("Cannot open quest file: %s", QUEST_PATH);
        return;
    }
    fputs("<Quests>", out);
    for (GList * qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        save_quest(quest, out);
    }
    fputs("</Quests>", out);
    fclose(out);
}

char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
FILE *qlogfile = NULL;

const char *
list_inactive_quests(void)
{
    int timediff;
    char timestr_a[128];
    int questCount = 0;
    const char *msg =
        "Finished Quests:\r\n"
        "-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

    if (!quests)
        return "There are no finished quests.\r\n";

    for (GList * qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        if (!quest->ended)
            continue;
        questCount++;
        timediff = quest->ended - quest->started;
        snprintf(timestr_a, 127, "%02d:%02d", timediff / 3600,
            (timediff / 60) % 60);

        msg = tmp_sprintf("%s %3d  %-10s  %-8s  %-24s %6s    %d\r\n", msg,
            quest->vnum, player_name_by_idnum(quest->owner_id),
            qtype_abbrevs[(int)quest->type], quest->name, timestr_a,
            g_list_length(quest->players));
    }

    if (!questCount)
        return "There are no finished quests.\r\n";

    return tmp_sprintf("%s%d visible quest%s finished.\r\n", msg,
        questCount, questCount == 1 ? "" : "s");
}

void
list_quest_bans(struct creature *ch, struct quest *quest)
{
    const char *name;
    int num = 0;

    acc_string_clear();
    acc_strcat("  -Banned Players------------------------------------\r\n",
        NULL);

    for (GList * bit = quest->bans; bit; bit = bit->next) {
        int id = GPOINTER_TO_INT(bit->data);
        name = player_name_by_idnum(id);
        if (name) {
            acc_sprintf("  %2d. %-20s\r\n", ++num, name);
        } else {
            acc_sprintf("BOGUS player idnum %d!\r\n", id);
            errlog("bogus player idnum %d in list_quest_bans.", id);
        }
    }

    page_string(ch->desc, acc_get_string());
}

char *
compose_qcomm_string(struct creature *ch, struct creature *vict,
    struct quest *quest, int mode, const char *str)
{
    if (mode == QCOMM_SAY && ch) {
        if (ch == vict) {
            return tmp_sprintf("%s %2d] You quest-say,%s '%s'\r\n",
                CCYEL_BLD(vict, C_NRM), quest->vnum, CCNRM(vict, C_NRM), str);
        } else {
            return tmp_sprintf("%s %2d] %s quest-says,%s '%s'\r\n",
                CCYEL_BLD(vict, C_NRM), quest->vnum,
                PERS(ch, vict), CCNRM(vict, C_NRM), str);
        }
    } else {                    // quest echo
        return tmp_sprintf("%s %2d] %s%s\r\n",
            CCYEL_BLD(vict, C_NRM), quest->vnum, str, CCNRM(vict, C_NRM));
    }
}

void
send_to_quest(struct creature *ch,
    const char *str, struct quest *quest, int level, int mode)
{
    struct creature *vict = NULL;

    for (GList * pit = quest->players; pit; pit = pit->next) {
        struct qplayer_data *player = pit->data;
        if ((player->flags & QP_IGNORE) && level < LVL_AMBASSADOR)
            continue;

        if ((vict = get_char_in_world_by_idnum(player->idnum))) {
            if (!PLR_FLAGGED(vict, PLR_WRITING) &&
                vict->desc && GET_LEVEL(vict) >= level) {
                send_to_char(vict, "%s", compose_qcomm_string(ch, vict, quest,
                        mode, str));
            }
        }
    }
}

/*************************************************************************
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * UTILITY FUNCTIONS                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *************************************************************************/

/*************************************************************************
 * function to find a quest                                              *
 * argument is the vnum of the quest as a string                         *
 *************************************************************************/
struct quest *
find_quest(struct creature *ch, char *argument)
{
    int vnum;
    struct quest *quest = NULL;

    if (*argument)
        vnum = atoi(argument);
    else
        vnum = GET_QUEST(ch);

    if ((quest = quest_by_vnum(vnum)))
        return quest;

    send_to_char(ch, "There is no quest number %d.\r\n", vnum);

    return NULL;
}

/*************************************************************************
 * low level function to return a quest                                  *
 *************************************************************************/
struct quest *
quest_by_vnum(int vnum)
{
    for (GList * qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        if (quest->vnum == vnum)
            return quest;
    }
    return NULL;
}

/*************************************************************************
 * function to list active quests to both mortals and gods               *
 *************************************************************************/

const char *
list_active_quests(struct creature *ch)
{
    int timediff;
    int questCount = 0;
    char timestr_a[32];
    const char *msg =
        "Active quests:\r\n"
        "-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

    if (!quests)
        return "There are no active quests.\r\n";

    for (GList * qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        if (quest->ended)
            continue;
        if (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            continue;
        questCount++;

        timediff = time(NULL) - quest->started;
        snprintf(timestr_a, 16, "%02d:%02d", timediff / 3600,
            (timediff / 60) % 60);

        msg = tmp_sprintf("%s %3d  %-10s  %-8s  %-24s %6s    %d\r\n", msg,
            quest->vnum, player_name_by_idnum(quest->owner_id),
            qtype_abbrevs[(int)quest->type], quest->name, timestr_a,
            g_list_length(quest->players));
    }
    if (!questCount)
        return "There are no visible quests.\r\n";

    return tmp_sprintf("%s%d visible quest%s active.\r\n\r\n", msg,
        questCount, questCount == 1 ? "" : "s");
}

void
list_quest_players(struct creature *ch, struct quest *quest, char *outbuf, size_t size)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    int num_online, num_offline;
    struct creature *vict = NULL;
    char bitbuf[1024];

    strcpy_s(buf, sizeof(buf), "  -Online Players------------------------------------\r\n");
    strcpy_s(buf2, sizeof(buf2), "  -Offline Players-----------------------------------\r\n");

    num_online = num_offline = 0;
    for (GList * pit = quest->players; pit; pit = pit->next) {
        struct qplayer_data *player = pit->data;
        const char *name = player_name_by_idnum(player->idnum);

        if (!name) {
            strcat_s(buf, sizeof(buf), "BOGUS player idnum!\r\n");
            strcat_s(buf2, sizeof(buf2), "BOGUS player idnum!\r\n");
            errlog("bogus player idnum in list_quest_players.");
            break;
        }

        if (player->flags) {
            sprintbit(player->flags, qp_bits, bitbuf, sizeof(bitbuf));
        } else {
            strcpy_s(bitbuf, sizeof(bitbuf), "");
        }

        // player is in world and visible
        if ((vict = get_char_in_world_by_idnum(player->idnum))
            && can_see_creature(ch, vict)) {

            // see if we can see the locations of the players
            if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
                snprintf(buf, sizeof(buf),
                    "%s  %2d. %-15s - %-10s %s[%5d] D%d/MK%d/PK%d %s\r\n", buf,
                    ++num_online, name, bitbuf, vict->in_room->name,
                    vict->in_room->number, player->deaths, player->mobkills,
                    player->pkills, vict->desc ? "" : "   (linkless)");
            } else if (QUEST_FLAGGED(quest, QUEST_WHOWHERE)) {
                snprintf(buf, sizeof(buf), "%s  %2d. %-15s - %s\r\n", buf,
                    ++num_online, name, vict->in_room->name);
            } else {
                snprintf(buf, sizeof(buf), "%s  %2d. %-15s - %-10s\r\n", buf, ++num_online,
                    name, bitbuf);
            }

        }
        // player is either offline or invisible
        else if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
            snprintf(buf2, sizeof(buf2), "%s  %2d. %-15s - %-10s\r\n", buf2, ++num_offline,
                name, bitbuf);
        }
    }

    // only gods may see the offline players
    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        strcat_s(buf, sizeof(buf), buf2);

    if (outbuf)
        strcpy_s(outbuf, size, buf);
    else
        page_string(ch->desc, buf);

}

void
qlog(struct creature *ch, const char *str, int type, int min_level, int file)
{
    // Mortals don't need to be seeing logs
    if (min_level < LVL_IMMORT)
        min_level = LVL_IMMORT;

    if (type) {
        struct descriptor_data *d = NULL;

        for (d = descriptor_list; d; d = d->next) {
            if (d->input_mode == CXN_PLAYING
                && !PLR_FLAGGED(d->creature, PLR_WRITING)) {
                int level = (d->original) ?
                    GET_LEVEL(d->original) : GET_LEVEL(d->creature);
                int qlog_level = (d->original) ?
                    GET_QLOG_LEVEL(d->original) : GET_QLOG_LEVEL(d->creature);

                if (level >= min_level && qlog_level >= type)
                    d_printf(d, "&Y[&g QLOG: %s %s &Y]&n\r\n",
                        ch ? PERS(ch, d->creature) : "", str);
            }
        }
    }

    if (file) {
        fprintf(qlogfile, "%-19.19s _ %s %s\n",
            tmp_ctime(time(NULL)), ch ? GET_NAME(ch) : "", str);
        fflush(qlogfile);
    }
}

struct creature *
check_char_vis(struct creature *ch, char *name)
{
    struct creature *vict;

    if (!(vict = get_char_vis(ch, name))) {
        send_to_char(ch, "No-one by the name of '%s' around.\r\n", name);
    }
    return (vict);
}

int
boot_quests(void)
{
    qlogfile = fopen(QLOGFILENAME, "a");
    if (!qlogfile) {
        errlog("unable to open qlogfile.");
        safe_exit(1);
    }
    load_quests();
    return 1;
}

ACMD(do_qlog)
{
    int i;

    skip_spaces(&argument);

    GET_QLOG_LEVEL(ch) = MIN(MAX(GET_QLOG_LEVEL(ch), 0), QLOG_COMP);

    if (!*argument) {
        send_to_char(ch, "You current qlog level is: %s.\r\n",
            qlog_types[(int)GET_QLOG_LEVEL(ch)]);
        return;
    }

    if ((i = search_block(argument, qlog_types, false)) < 0) {
        buf[0] = '\0';
        for (i = 0; *qlog_types[i] != '\n'; i++) {
            strcat_s(buf, sizeof(buf), qlog_types[i]);
            strcat_s(buf, sizeof(buf), " ");
        }
        send_to_char(ch, "Unknown qlog type '%s'.  Options are: %s\r\n",
            argument, buf);
        return;
    }

    GET_QLOG_LEVEL(ch) = i;
    send_to_char(ch, "Qlog level set to: %s.\r\n", qlog_types[i]);
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

const char *quest_commands[][2] = {
    {"list", "shows currently active quests"},
    {"info", "get info about a specific quest"},
    {"join", "join an active quest"},
    {"leave", "leave a quest"},

    // Go back and set this in the player file when you get time!
    {"status", "list the quests you are participating in"},
    {"who", "list all players in a specific quest"},
    {"current", "specify which quest you are currently active in"},
    {"ignore", "ignore most qsays from specified quest."},
    {"\n", "\n"}
    // "\n" terminator must be here
};

ACMD(do_quest)
{
    int i = 0;

    if (IS_NPC(ch))
        return;

    skip_spaces(&argument);
    argument = one_argument(argument, arg1);

    if (!*arg1) {
        send_to_char(ch, "Quest options:\r\n");

        while (1) {
            if (*quest_commands[i][0] == '\n')
                break;
            send_to_char(ch, "  %-10s -- %s\r\n",
                quest_commands[i][0], quest_commands[i][1]);
            i++;
        }

        return;
    }

    for (i = 0;; i++) {
        if (*quest_commands[i][0] == '\n') {
            send_to_char(ch,
                "No such quest option, '%s'.  Type 'quest' for usage.\r\n",
                arg1);
            return;
        }
        if (is_abbrev(arg1, quest_commands[i][0]))
            break;
    }

    switch (i) {
    case 0:                    // list
        do_quest_list(ch);
        break;
    case 1:                    // info
        do_quest_info(ch, argument);
        break;
    case 2:                    // join
        do_quest_join(ch, argument);
        break;
    case 3:                    // leave
        do_quest_leave(ch, argument);
        break;
    case 4:                    // status
        do_quest_status(ch);
        break;
    case 5:                    // who
        do_quest_who(ch, argument);
        break;
    case 6:                    // current
        do_quest_current(ch, argument);
        break;
    case 7:                    // ignore
        do_quest_ignore(ch, argument);
        break;
    default:
        break;
    }
}

void
do_quest_list(struct creature *ch)
{
    send_to_char(ch, "%s", list_active_quests(ch));
}

void
do_quest_join(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Join which quest?\r\n");
        return;
    }

    if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    if (is_playing_quest(quest, GET_IDNUM(ch))) {
        send_to_char(ch, "You are already in that quest, fool.\r\n");
        return;
    }

    if (!can_join_quest(quest, ch))
        return;

    add_quest_player(quest, GET_IDNUM(ch));

    GET_QUEST(ch) = quest->vnum;
    crashsave(ch);

    snprintf(buf, sizeof(buf), "joined quest %d '%s'.", quest->vnum, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, true);

    send_to_char(ch, "You have joined quest '%s'.\r\n", quest->name);

    snprintf(buf, sizeof(buf), "%s has joined the quest.", GET_NAME(ch));
    send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(ch), 0), QCOMM_ECHO);
}

bool
can_leave_quest(struct quest * quest, struct creature * ch)
{
    if (QUEST_FLAGGED(quest, QUEST_NOLEAVE)) {
        send_to_char(ch, "Sorry, you cannot leave the quest right now.\r\n");
        return false;
    }
    return true;
}

void
do_quest_leave(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char(ch, "Leave which quest?\r\n");
            return;
        }
    }

    else if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    if (!is_playing_quest(quest, GET_IDNUM(ch))) {
        send_to_char(ch, "You are not in that quest, fool.\r\n");
        return;
    }

    if (!can_leave_quest(quest, ch))
        return;

    if (!remove_quest_player(quest, GET_IDNUM(ch))) {
        send_to_char(ch, "Error removing char from quest.\r\n");
        return;
    }

    GET_QUEST(ch) = 0;
    crashsave(ch);

    qlog(ch, tmp_sprintf("%s left quest %d '%s'.",
                         GET_NAME(ch), quest->vnum, quest->name),
         QLOG_COMP, 0, true);
    send_to_char(ch, "You have left quest '%s'.\r\n", quest->name);
    send_to_quest(NULL, tmp_sprintf("%s has left the quest.", GET_NAME(ch)),
                  quest, MAX(GET_INVIS_LVL(ch), 0), QCOMM_ECHO);
}

void
do_quest_info(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;
    int timediff;
    char timestr_a[128];
    char *timestr_s;

    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char(ch, "Get info on which quest?\r\n");
            return;
        }
    } else if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    timediff = time(NULL) - quest->started;
    snprintf(timestr_a, sizeof(timestr_a), "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

    time_t started = quest->started;
    timestr_s = asctime(localtime(&started));
    timestr_s[strlen(timestr_s) - 1] = '\0';

    page_string(ch->desc,
                tmp_sprintf(
                    "Quest [%d] info:\r\n"
                    "Owner:  %s\r\n"
                    "Name:   %s\r\n"
                    "Description:\r\n%s"
                    "Updates:\r\n%s"
                    "  Type:            %s\r\n"
                    "  Started:         %s\r\n"
                    "  Age:             %s\r\n"
                    "  Min Level:   Gen %-2d, Level %2d\r\n"
                    "  Max Level:   Gen %-2d, Level %2d\r\n"
                    "  Num Players:     %d\r\n"
                    "  Max Players:     %d\r\n",
                    quest->vnum,
                    player_name_by_idnum(quest->owner_id), quest->name,
                    quest->description ? quest->description : "None.\r\n",
                    quest->updates ? quest->updates : "None.\r\n",
                    qtypes[(int)quest->type], timestr_s, timestr_a,
                    quest->mingen, quest->minlevel,
                    quest->maxgen, quest->maxlevel,
                    g_list_length(quest->players), quest->max_players));
}

void
do_quest_status(struct creature *ch)
{
    char timestr_a[128];
    int timediff;
    bool found = false;

    const char *msg = "You are participating in the following quests:\r\n"
        "-Vnum--Owner-------Type------Name----------------------Age------Players\r\n";

    for (GList * qit = quests; qit; qit = qit->next) {
        struct quest *quest = qit->data;
        if (quest->ended)
            continue;
        if (is_playing_quest(quest, GET_IDNUM(ch))) {
            timediff = time(NULL) - quest->started;
            snprintf(timestr_a, 128, "%02d:%02d", timediff / 3600,
                (timediff / 60) % 60);

            char *line =
                tmp_sprintf(" %s%3d  %-10s  %-8s  %-24s %6s    %d\r\n",
                quest->vnum == GET_QUEST(ch) ? "*" : " ",
                quest->vnum, player_name_by_idnum(quest->owner_id),
                qtype_abbrevs[(int)quest->type],
                quest->name, timestr_a,
                g_list_length(quest->players));
            msg = tmp_strcat(msg, line, NULL);
            found = true;
        }
    }
    if (!found)
        msg = tmp_strcat(msg, "None.\r\n", NULL);
    page_string(ch->desc, msg);
}

void
do_quest_who(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;
    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char(ch, "List the players for which quest?\r\n");
            return;
        }
    } else if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    if (QUEST_FLAGGED(quest, QUEST_NOWHO)) {
        send_to_char(ch,
            "Sorry, you cannot get a who listing for this quest.\r\n");
        return;
    }

    if (QUEST_FLAGGED(quest, QUEST_NO_OUTWHO) &&
        !is_playing_quest(quest, GET_IDNUM(ch))) {
        send_to_char(ch,
            "Sorry, you cannot get a who listing from outside this quest.\r\n");
        return;
    }

    list_quest_players(ch, quest, NULL, 0);

}

void
do_quest_current(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char(ch, "You are not current on any quests.\r\n");
            return;
        }
        send_to_char(ch, "You are current on quest %d, '%s'\r\n", quest->vnum,
            quest->name);
        return;
    }

    if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    if (!is_playing_quest(quest, GET_IDNUM(ch))) {
        send_to_char(ch, "You are not even in that quest.\r\n");
        return;
    }

    GET_QUEST(ch) = quest->vnum;
    crashsave(ch);

    send_to_char(ch, "Ok, you are now currently active in '%s'.\r\n",
        quest->name);
}

void
do_quest_ignore(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        if (!(quest = quest_by_vnum(GET_QUEST(ch)))) {
            send_to_char(ch, "Ignore which quest?\r\n");
            return;
        }
    }

    else if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended || (QUEST_FLAGGED(quest, QUEST_HIDE)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
        send_to_char(ch, "No such quest is running.\r\n");
        return;
    }

    struct qplayer_data *player = quest_player_by_idnum(quest, GET_IDNUM(ch));

    if (!player) {
        send_to_char(ch, "You are not even in that quest.\r\n");
        return;
    }

    TOGGLE_BIT(player->flags, QP_IGNORE);

    send_to_char(ch, "Ok, you are %s ignoring '%s'.\r\n",
        IS_SET(player->flags, QP_IGNORE) ? "now" : "no longer", quest->name);
}

ACMD(do_qsay)
{
    struct quest *quest = NULL;
    struct qplayer_data *player = NULL;

    quest = quest_by_vnum(GET_QUEST(ch));
    if (quest)
        player = quest_player_by_idnum(quest, GET_IDNUM(ch));

    if (!quest || !player) {
        send_to_char(ch, "You are not currently active on any quest.\r\n");
        return;
    }

    if (IS_SET(player->flags, QP_MUTE)) {
        send_to_char(ch, "You have been quest-muted.\r\n");
        return;
    }

    if (IS_SET(player->flags, QP_IGNORE)) {
        send_to_char(ch, "You can't quest-say while ignoring the quest.\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Qsay what?\r\n");
        return;
    }

    send_to_quest(ch, argument, quest, 0, QCOMM_SAY);
}

ACMD(do_qecho)
{
    struct quest *quest = NULL;
    struct qplayer_data *player = NULL;

    quest = quest_by_vnum(GET_QUEST(ch));
    if (quest)
        player = quest_player_by_idnum(quest, GET_IDNUM(ch));

    if (!quest || !player) {
        send_to_char(ch, "You are not currently active on any quest.\r\n");
        return;
    }

    if (IS_SET(player->flags, QP_MUTE)) {
        send_to_char(ch, "You have been quest-muted.\r\n");
        return;
    }

    if (IS_SET(player->flags, QP_IGNORE)) {
        send_to_char(ch, "You can't quest-echo while ignoring the quest.\r\n");
        return;
    }


    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Qecho what?\r\n");
        return;
    }

    send_to_quest(ch, argument, quest, 0, QCOMM_ECHO);
}

void
qp_reload(int sig __attribute__ ((unused)))
{
    struct creature *immortal;
    int online = 0;

    //
    // Check if the imm is logged on
    //
    for (GList * cit = first_living(creatures); cit; cit = next_living(cit)) {
        immortal = cit->data;
        if (GET_LEVEL(immortal) >= LVL_AMBASSADOR
            && (!IS_NPC(immortal) && GET_QUEST_ALLOWANCE(immortal) > 0)) {
            slog("QP_RELOAD: Reset %s to %d QPs from %d. ( online )",
                GET_NAME(immortal), GET_QUEST_ALLOWANCE(immortal),
                GET_IMMORT_QP(immortal));

            GET_IMMORT_QP(immortal) = GET_QUEST_ALLOWANCE(immortal);
            send_to_char(immortal,
                "Your quest points have been restored!\r\n");
            crashsave(immortal);
            online++;
        }
    }
    mudlog(LVL_GRGOD, NRM, true,
        "QP's have been reloaded - %d reset online", online);
}

bool
remove_quest_ban(struct quest *quest, int id)
{
    quest->bans = g_list_remove(quest->bans, GINT_TO_POINTER(id));
    return true;
}

bool
add_quest_ban(struct quest * quest, int id)
{
    quest->bans = g_list_prepend(quest->bans, GINT_TO_POINTER(id));
    return true;
}

bool
is_playing_quest(struct quest * quest, int id)
{
    return (quest_player_by_idnum(quest, id) != NULL);
}

bool
quest_levelok(struct creature *ch __attribute__((unused)))
{
    return true;
}

void
tally_quest_death(struct quest *quest, int idnum)
{
    struct qplayer_data *player = quest_player_by_idnum(quest, idnum);
    if (player) {
        player->deaths += 1;
        save_quests();
    }
}

void
tally_quest_mobkill(struct quest *quest, int idnum)
{
    struct qplayer_data *player = quest_player_by_idnum(quest, idnum);
    if (player) {
        player->mobkills += 1;
        save_quests();
    }
}

void
tally_quest_pkill(struct quest *quest, int idnum)
{
    struct qplayer_data *player = quest_player_by_idnum(quest, idnum);
    if (player) {
        player->pkills += 1;
        save_quests();
    }
}

static void
do_qcontrol_options(struct creature *ch)
{
    int i = 0;

    strcpy_s(buf, sizeof(buf), "qcontrol options:\r\n");
    while (1) {
        if (!qc_options[i].keyword)
            break;
        snprintf(buf, sizeof(buf), "%s  %-15s %s\r\n", buf, qc_options[i].keyword,
            qc_options[i].usage);
        i++;
    }
    page_string(ch->desc, buf);
}

static void
do_qcontrol_usage(struct creature *ch, int com)
{
    if (com < 0)
        do_qcontrol_options(ch);
    else {
        send_to_char(ch, "Usage: qcontrol %s %s\r\n",
            qc_options[com].keyword, qc_options[com].usage);
    }
}

void                            //Load mobile.
do_qcontrol_mload(struct creature *ch, char *argument, int com)
{
    struct creature *mob;
    struct quest *quest = NULL;
    char arg1[MAX_INPUT_LENGTH];
    int number;

    two_arguments(argument, buf, arg1);

    if (!*buf || !isdigit(*buf) || !*arg1 || !isdigit(*arg1)) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1))) {
        return;
    }
    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }
    if (quest->ended) {
        send_to_char(ch, "Pay attention dummy! That quest is over!\r\n");
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
        send_to_char(ch, "The mobile couldn't be loaded.\r\n");
        return;
    }
    char_to_room(mob, ch->in_room, false);
    act("$n makes a quaint, magical gesture with one hand.", true, ch,
        NULL, NULL, TO_ROOM);
    act("$n has created $N!", false, ch, NULL, mob, TO_ROOM);
    act("You create $N.", false, ch, NULL, mob, TO_CHAR);

    snprintf(buf, sizeof(buf), "mloaded %s at %d.", GET_NAME(mob), ch->in_room->number);
    qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), true);

}

void                            // Set loadroom for quest participants
do_qcontrol_loadroom(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    char arg1[MAX_INPUT_LENGTH];
    int number;

    two_arguments(argument, buf, arg1);

    if (!*buf || !isdigit(*buf)) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, buf))) {
        return;
    }
    if (!*arg1) {
        send_to_char(ch, "Current quest loadroom is (%d)\r\n",
            quest->loadroom);
        return;
    }
    if (!isdigit(*arg1)) {
        do_qcontrol_usage(ch, com);
        return;
    }
    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }
    if (quest->ended) {
        send_to_char(ch, "Pay attention you dummy! That quest is over!\r\n");
        return;
    }

    if ((number = atoi(arg1)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (!real_room(number)) {
        send_to_char(ch, "That room doesn't exist!\r\n");
        return;
    }

    quest->loadroom = number;
    send_to_char(ch, "Okay, quest loadroom is now %d\r\n", number);

    snprintf(buf, sizeof(buf), "%s set quest loadroom to: (%d)", GET_NAME(ch), number);
    qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), true);
}

static void
do_qcontrol_oload_list(struct creature *ch)
{
    int i = 0;
    struct obj_data *obj;

    send_to_char(ch, "Valid Quest Objects:\r\n");
    for (i = MIN_QUEST_OBJ_VNUM; i <= MAX_QUEST_OBJ_VNUM; i++) {
        if (!(obj = real_object_proto(i)))
            continue;
        if (IS_OBJ_STAT2(obj, ITEM2_UNAPPROVED))
            continue;
        send_to_char(ch, "    %s%d. %s%s %s: %d qps\r\n", CCNRM(ch, C_NRM),
            i - MIN_QUEST_OBJ_VNUM, CCGRN(ch, C_NRM), obj->name,
            CCNRM(ch, C_NRM), obj->shared->cost);
    }
}

// Load Quest Object
static void
do_qcontrol_oload(struct creature *ch, char *argument, int com)
{
    struct obj_data *obj;
    struct quest *quest = NULL;
    int number;
    char arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, buf, arg2);

    if (!*buf || !isdigit(*buf)) {
        do_qcontrol_oload_list(ch);
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2))) {
        return;
    }
    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }
    if (quest->ended) {
        send_to_char(ch, "Pay attention you dummy! That quest is over!\r\n");
        return;
    }

    if ((number = atoi(buf)) < 0) {
        send_to_char(ch, "A NEGATIVE number??\r\n");
        return;
    }
    if (number > MAX_QUEST_OBJ_VNUM - MIN_QUEST_OBJ_VNUM) {
        send_to_char(ch, "Invalid item number.\r\n");
        do_qcontrol_oload_list(ch);
        return;
    }
    obj = read_object(number + MIN_QUEST_OBJ_VNUM);

    if (!obj) {
        send_to_char(ch, "Error, no object loaded\r\n");
        return;
    }
    if (obj->shared->cost < 0) {
        send_to_char(ch, "This object is messed up.\r\n");
        extract_obj(obj);
        return;
    }
    if (GET_LEVEL(ch) == LVL_AMBASSADOR && obj->shared->cost > 0) {
        send_to_char(ch, "You can only load objects with a 0 cost.\r\n");
        extract_obj(obj);
        return;
    }

    if (obj->shared->cost > GET_IMMORT_QP(ch)) {
        send_to_char(ch, "You do not have the required quest points.\r\n");
        extract_obj(obj);
        return;
    }

    GET_IMMORT_QP(ch) -= obj->shared->cost;
    obj_to_char(obj, ch);
    crashsave(ch);
    act("$n makes a quaint, magical gesture with one hand.", true, ch,
        NULL, NULL, TO_ROOM);
    act("$n has created $p!", false, ch, obj, NULL, TO_ROOM);
    act("You create $p.", false, ch, obj, NULL, TO_CHAR);

    snprintf(buf, sizeof(buf), "loaded %s at %d.", obj->name, ch->in_room->number);
    qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), true);

}

void                            //Purge mobile.
do_qcontrol_trans(struct creature *ch, char *argument)
{

    struct creature *vict;
    struct quest *quest = NULL;
    struct room_data *room = NULL;

    two_arguments(argument, arg1, arg2);
    if (!*arg1) {
        send_to_char(ch,
            "Usage: qcontrol trans <quest number> [room number]\r\n");
        return;
    }
    if (!(quest = find_quest(ch, arg1))) {
        return;
    }
    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }
    if (quest->ended) {
        send_to_char(ch, "Pay attention dummy! That quest is over!\r\n");
        return;
    }
    if (*arg2) {
        if (!is_number(arg2)) {
            send_to_char(ch, "No such room: '%s'\r\n", arg2);
            return;
        }
        room = real_room(atoi(arg2));
        if (room == NULL) {
            send_to_char(ch, "No such room: '%s'\r\n", arg2);
            return;
        }
    }

    if (room == NULL)
        room = ch->in_room;

    int transCount = 0;
    for (GList * pit = quest->players; pit; pit = pit->next) {
        long id = ((struct qplayer_data *)pit->data)->idnum;

        vict = get_char_in_world_by_idnum(id);
        if (vict == NULL || vict == ch)
            continue;
        if ((GET_LEVEL(ch) < GET_LEVEL(vict))) {
            send_to_char(ch, "%s ignores your summons.\r\n", GET_NAME(vict));
            continue;
        }
        ++transCount;
        act("$n disappears in a mushroom cloud.", false, vict, NULL, NULL, TO_ROOM);
        char_from_room(vict, false);
        char_to_room(vict, room, false);
        act("$n arrives from a puff of smoke.", false, vict, NULL, NULL, TO_ROOM);
        act("$n has transferred you!", false, ch, NULL, vict, TO_VICT);
        look_at_room(vict, room, 0);
    }
    char *buf =
        tmp_sprintf("has transferred %d questers to %s[%d] for quest %d.",
        transCount, room->name, room->number, quest->vnum);
    qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), true);

    send_to_char(ch, "%d players transferred.\r\n", transCount);
}

void                            //Purge mobile.
do_qcontrol_purge(struct creature *ch, char *argument)
{

    struct creature *vict;
    struct quest *quest = NULL;
    char arg1[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, buf);
    if (!*buf) {
        send_to_char(ch, "Purge what?\r\n");
        return;
    }
    if (!(quest = find_quest(ch, arg1))) {
        return;
    }
    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }
    if (quest->ended) {
        send_to_char(ch, "Pay attention dummy! That quest is over!\r\n");
        return;
    }

    if ((vict = get_char_room_vis(ch, buf))) {
        if (!IS_NPC(vict)) {
            send_to_char(ch, "You don't need a quest to purge them!\r\n");
            return;
        }
        act("$n disintegrates $N.", false, ch, NULL, vict, TO_NOTVICT);
        snprintf(buf, sizeof(buf), "has purged %s at %d.",
            GET_NAME(vict), vict->in_room->number);
        qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_IMMORT), true);
        if (vict->desc) {
            close_socket(vict->desc);
            vict->desc = NULL;
        }
        creature_purge(vict, false);
        send_to_char(ch, "%s", OK);
    } else {
        send_to_char(ch, "Purge what?\r\n");
        return;
    }

}

static void
do_qcontrol_show(struct creature *ch, char *argument)
{

    int timediff;
    struct quest *quest = NULL;
    char *timestr_e, *timestr_s;
    char timestr_a[16];

    if (!quests) {
        send_to_char(ch, "There are no quests to show.\r\n");
        return;
    }
    // show all quests
    if (!*argument) {
        const char *msg;

        msg = list_active_quests(ch);
        msg = tmp_strcat(msg, list_inactive_quests(), NULL);
        page_string(ch->desc, msg);
        return;
    }
    // list q specific quest
    if (!(quest = find_quest(ch, argument)))
        return;

    time_t started = quest->started;
    timestr_s = asctime(localtime(&started));
    timestr_s[strlen(timestr_s) - 1] = '\0';

    // quest is over, show summary information
    if (quest->ended) {

        timediff = quest->ended - quest->started;
        snprintf(timestr_a, sizeof(timestr_a), "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

        time_t started = quest->started;
        timestr_e = asctime(localtime(&started));
        timestr_e[strlen(timestr_e) - 1] = '\0';

        acc_string_clear();

        acc_sprintf("Owner:  %-30s [%2d]\r\n"
            "Name:   %s\r\n"
            "Status: COMPLETED\r\n"
            "Description:\r\n%s"
            "  Type:           %s\r\n"
            "  Started:        %s\r\n"
            "  Ended:          %s\r\n"
            "  Age:            %s\r\n"
            "  Min Level:   Gen %-2d, Level %2d\r\n"
            "  Max Level:   Gen %-2d, Level %2d\r\n"
            "  Max Players:    %d\r\n"
            "  Pts. Awarded:   %d\r\n",
            player_name_by_idnum(quest->owner_id), quest->owner_level,
            quest->name,
            quest->description ? quest->description : "None.\r\n",
            qtypes[(int)quest->type], timestr_s,
            timestr_e, timestr_a,
            quest->mingen, quest->minlevel,
            quest->maxgen, quest->maxlevel,
            quest->max_players, quest->awarded);

        page_string(ch->desc, acc_get_string());

        return;
    }
    // quest is still active

    timediff = time(NULL) - quest->started;
    snprintf(timestr_a, sizeof(timestr_a), "%02d:%02d", timediff / 3600, (timediff / 60) % 60);

    acc_sprintf("Owner:  %-30s [%2d]\r\n"
        "Name:   %s\r\n"
        "Status: ACTIVE\r\n"
        "Description:\r\n%s"
        "Updates:\r\n%s"
        "  Type:            %s\r\n"
        "  Flags:           %s\r\n"
        "  Started:         %s\r\n"
        "  Age:             %s\r\n"
        "  Min Level:   Gen %-2d, Level %2d\r\n"
        "  Max Level:   Gen %-2d, Level %2d\r\n"
        "  Num Players:     %d\r\n"
        "  Max Players:     %d\r\n"
        "  Pts. Awarded:    %d\r\n",
        player_name_by_idnum(quest->owner_id), quest->owner_level,
        quest->name,
        quest->description ? quest->description : "None.\r\n",
        quest->updates ? quest->updates : "None.\r\n",
        qtypes[(int)quest->type],
        tmp_printbits(quest->flags, quest_bits),
        timestr_s,
        timestr_a,
        quest->mingen, quest->minlevel,
        quest->maxgen, quest->maxlevel,
        g_list_length(quest->players), quest->max_players, quest->awarded);

    if (quest->players) {
        list_quest_players(ch, quest, buf2, sizeof(buf2));
        acc_strcat(buf2, NULL);
    }

    if (quest->bans) {
        list_quest_bans(ch, quest);
        acc_strcat(buf2, NULL);
    }

    page_string(ch->desc, acc_get_string());

}

int
find_quest_type(char *argument)
{
    int i = 0;

    while (1) {
        if (*qtypes[i] == '\n')
            break;
        if (is_abbrev(argument, qtypes[i]))
            return i;
        i++;
    }
    return (-1);
}

void
qcontrol_show_valid_types(struct creature *ch)
{
    char *msg = tmp_sprintf("  Valid Types:\r\n");
    int i = 0;
    while (1) {
        if (*qtypes[i] == '\n')
            break;
        msg = tmp_sprintf("%s    %2d. %s\r\n", msg, i, qtypes[i]);
        i++;
    }
    page_string(ch->desc, msg);
    return;
}

static void
do_qcontrol_create(struct creature *ch, char *argument, int com)
{
    int type;
    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*arg1 || !*argument) {
        do_qcontrol_usage(ch, com);
        qcontrol_show_valid_types(ch);
        return;
    }

    if ((type = find_quest_type(arg1)) < 0) {
        send_to_char(ch, "Invalid quest type '%s'.\r\n", arg1);
        qcontrol_show_valid_types(ch);
        return;
    }

    if (strlen(argument) >= MAX_QUEST_NAME) {
        send_to_char(ch, "Quest name too long.  Max length %d characters.\r\n",
            MAX_QUEST_NAME - 1);
        return;
    }

    struct quest *quest = make_quest(GET_IDNUM(ch), GET_LEVEL(ch),
                                     type, argument);
    quests = g_list_prepend(quests, quest);

    char *msg = tmp_sprintf("created quest [%d] type %s, '%s'",
        quest->vnum, qtypes[type], argument);
    qlog(ch, msg, QLOG_BRIEF, LVL_AMBASSADOR, true);
    send_to_char(ch, "Quest %d created.\r\n", quest->vnum);
    add_quest_player(quest, GET_IDNUM(ch));

    GET_QUEST(ch) = quest->vnum;
    crashsave(ch);
    save_quests();
}

static void
do_qcontrol_end(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    if (!*argument) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, argument)))
        return;

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended... duh.\r\n");
        return;
    }

    send_to_quest(ch, "Quest ending...", quest, 0, QCOMM_ECHO);

    qlog(ch, "Purging players from quest...", QLOG_COMP, 0, true);


    while (quest->players) {
        struct qplayer_data *player = quest->players->data;
        remove_quest_player(quest, player->idnum);
    }

    quest->ended = time(NULL);
    qlog(ch, tmp_sprintf("ended quest %d '%s'", quest->vnum, quest->name),
        QLOG_BRIEF, 0, true);
    send_to_char(ch, "Quest ended.\r\n");
    save_quests();
}

static void
do_qcontrol_add(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    struct creature *vict = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2)))
        return;

    if (!(vict = check_char_vis(ch, arg1)))
        return;

    if (IS_NPC(vict)) {
        send_to_char(ch, "You cannot add mobiles to quests.\r\n");
        return;
    }

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended, you wacko.\r\n");
        return;
    }

    if (is_playing_quest(quest, GET_IDNUM(vict))) {
        send_to_char(ch, "That person is already part of this quest.\r\n");
        return;
    }

    if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch) {
        send_to_char(ch, "You are not powerful enough to do this.\r\n");
        return;
    }

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    add_quest_player(quest, GET_IDNUM(vict));
    GET_QUEST(vict) = quest->vnum;

    qlog(ch, tmp_sprintf("added %s to quest %d '%s'.",
            GET_NAME(vict), quest->vnum, quest->name),
        QLOG_COMP, GET_INVIS_LVL(vict), true);

    send_to_char(ch, "%s added to quest %d.\r\n", GET_NAME(vict), quest->vnum);

    send_to_char(vict, "%s has added you to quest %d.\r\n", GET_NAME(ch),
        quest->vnum);

    snprintf(buf, sizeof(buf), "%s is now part of the quest.", GET_NAME(vict));
    send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(vict), LVL_AMBASSADOR),
        QCOMM_ECHO);
}

static void
do_qcontrol_kick(struct creature *ch, char *argument, int com)
{

    struct quest *quest = NULL;
    struct creature *vict = NULL;
    int idnum;
    const char *vict_name;
    int pid;
    int level = 0;
    char *arg1, *arg2;

    arg1 = tmp_getword(&argument);
    arg2 = tmp_getword(&argument);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2)))
        return;

    if ((idnum = player_idnum_by_name(arg1)) < 0) {
        send_to_char(ch, "There is no character named '%s'\r\n", arg1);
        return;
    }

    if (quest->ended) {
        send_to_char(ch,
            "That quest has already ended.. there are no players in it.\r\n");
        return;
    }

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if (!is_playing_quest(quest, idnum)) {
        send_to_char(ch, "That person not participating in this quest.\r\n");
        return;
    }

    if (!(vict = get_char_in_world_by_idnum(idnum))) {
        // load the char from file
        pid = player_idnum_by_name(arg1);
        if (pid > 0) {
            vict = load_player_from_xml(pid);
            level = GET_LEVEL(vict);
            vict_name = tmp_strdup(GET_NAME(vict));
            free_creature(vict);
            vict = NULL;
        } else {
            send_to_char(ch, "Error loading char from file.\r\n");
            return;
        }

    } else {
        level = GET_LEVEL(vict);
        vict_name = tmp_strdup(GET_NAME(vict));
    }

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
        send_to_char(ch, "You are not powerful enough to do this.\r\n");
        return;
    }

    if (!remove_quest_player(quest, idnum)) {
        send_to_char(ch, "Error removing char from quest.\r\n");
        return;
    }

    send_to_char(ch, "%s kicked from quest %d.\r\n", vict_name, quest->vnum);
    if (vict) {
        snprintf(buf, sizeof(buf), "kicked %s from quest %d '%s'.",
            vict_name, quest->vnum, quest->name);
        qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(vict), LVL_AMBASSADOR),
            true);

        send_to_char(vict, "%s kicked you from quest %d.\r\n",
            GET_NAME(ch), quest->vnum);

        snprintf(buf, sizeof(buf), "%s has been kicked from the quest.", vict_name);
        send_to_quest(NULL, buf, quest, MAX(GET_INVIS_LVL(vict),
                LVL_AMBASSADOR), QCOMM_ECHO);
    } else {
        snprintf(buf, sizeof(buf), "kicked %s from quest %d '%s'.",
            vict_name, quest->vnum, quest->name);
        qlog(ch, buf, QLOG_BRIEF, LVL_AMBASSADOR, true);

        snprintf(buf, sizeof(buf), "%s has been kicked from the quest.", vict_name);
        send_to_quest(NULL, buf, quest, LVL_AMBASSADOR, QCOMM_ECHO);
    }

    save_quests();
}

void
qcontrol_show_valid_flags(struct creature *ch)
{

    char *msg = tmp_sprintf("  Valid Quest Flags:\r\n");
    int i = 0;
    while (1) {
        if (*quest_bits[i] == '\n')
            break;
        msg = tmp_sprintf("%s    %2d. %s - %s\r\n",
            msg, i, quest_bits[i], quest_bit_descs[i]);
        i++;
    }
    page_string(ch->desc, msg);
    return;
}

static void
do_qcontrol_flags(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    int state, cur_flags = 0, tmp_flags = 0, flag = 0, old_flags = 0;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !*argument) {
        do_qcontrol_usage(ch, com);
        qcontrol_show_valid_flags(ch);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if (*arg2 == '+')
        state = 1;
    else if (*arg2 == '-')
        state = 2;
    else {
        do_qcontrol_usage(ch, com);
        qcontrol_show_valid_flags(ch);
        return;
    }

    argument = one_argument(argument, arg1);

    old_flags = cur_flags = quest->flags;

    while (*arg1) {
        if ((flag = search_block(arg1, quest_bits, false)) == -1) {
            send_to_char(ch, "Invalid flag %s, skipping...\r\n", arg1);
        } else
            tmp_flags = tmp_flags | (1 << flag);

        argument = one_argument(argument, arg1);
    }

    if (state == 1)
        cur_flags = cur_flags | tmp_flags;
    else {
        tmp_flags = cur_flags & tmp_flags;
        cur_flags = cur_flags ^ tmp_flags;
    }

    quest->flags = cur_flags;

    tmp_flags = old_flags ^ cur_flags;
    sprintbit(tmp_flags, quest_bits, buf2, sizeof(buf2));

    if (tmp_flags == 0) {
        send_to_char(ch, "Flags for quest %d not altered.\r\n", quest->vnum);
    } else {
        send_to_char(ch, "[%s] flags %s for quest %d.\r\n", buf2,
            state == 1 ? "added" : "removed", quest->vnum);

        snprintf(buf, sizeof(buf), "%s [%s] flags for quest %d '%s'.",
            state == 1 ? "added" : "removed", buf2, quest->vnum, quest->name);
        qlog(ch, buf, QLOG_COMP, LVL_AMBASSADOR, true);
    }
    save_quests();
}

static void
do_qcontrol_comment(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*argument || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    snprintf(buf, sizeof(buf), "comments on quest %d '%s': %s",
        quest->vnum, quest->name, argument);
    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "Comment logged as '%s'", argument);

    save_quests();
}

static void
do_qcontrol_desc(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, argument)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if (already_being_edited(ch, quest->description))
        return;

    act("$n begins to edit a quest description.\r\n", true, ch, NULL, NULL, TO_ROOM);

    if (quest->description) {
        snprintf(buf, sizeof(buf), "began editing description of quest '%s'", quest->name);
    } else {
        snprintf(buf, sizeof(buf), "began writing description of quest '%s'", quest->name);
    }

    start_editing_text(ch->desc, &quest->description, MAX_QUEST_DESC);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}

static void
do_qcontrol_update(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    skip_spaces(&argument);

    if (!*argument) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, argument)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if (already_being_edited(ch, quest->updates))
        return;

    if (quest->description) {
        snprintf(buf, sizeof(buf), "began editing update of quest '%s'", quest->name);
    } else {
        snprintf(buf, sizeof(buf), "began writing the update of quest '%s'", quest->name);
    }

    start_editing_text(ch->desc, &quest->updates, MAX_QUEST_UPDATE);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

    act("$n begins to edit a quest update.\r\n", true, ch, NULL, NULL, TO_ROOM);
    qlog(ch, buf, QLOG_COMP, LVL_AMBASSADOR, true);
}

static void
do_qcontrol_ban(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    struct creature *vict = NULL;
    unsigned int idnum, accountID;
    struct account *account = NULL;
    int pid;
    int level = 0;
    bool del_vict = false;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (player_idnum_by_name(arg1) < 0) {
        send_to_char(ch, "There is no character named '%s'\r\n", arg1);
        return;
    } else {
        idnum = player_idnum_by_name(arg1);
        accountID = player_account_by_name(arg1);
    }

    if (!(vict = get_char_in_world_by_idnum(idnum))) {
        // load the char from file
        pid = player_idnum_by_name(arg1);
        if (pid > 0) {
            vict = load_player_from_xml(pid);
            level = GET_LEVEL(vict);
            del_vict = true;
            account = account_by_idnum(accountID);
        } else {
            send_to_char(ch, "Error loading char from file.\r\n");
            return;
        }
    } else {
        level = GET_LEVEL(vict);
        account = vict->account;
    }

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
        send_to_char(ch, "You are not powerful enough to do this.\r\n");
        return;
    }

    if (!strcmp("all", arg2)) { //ban from all quests
        if (!is_authorized(ch, QUEST_BAN, NULL)) {
            send_to_char(ch, "You do not have this ability.\r\n");
        } else if (account->quest_banned) {
            send_to_char(ch,
                "That player is already banned from all quests.\r\n");
        } else {
            account_set_quest_banned(account, true);    //ban

            snprintf(buf, sizeof(buf), "banned %s from all quests.", GET_NAME(vict));
            qlog(ch, buf, QLOG_COMP, 0, true);

            send_to_char(ch, "%s is now banned from all quests.\r\n",
                GET_NAME(vict));
            send_to_char(vict, "You have been banned from all quests.\r\n");
        }
    } else {
        if (!(quest = find_quest(ch, arg2)))
            return;

        if (!is_authorized(ch, EDIT_QUEST, quest))
            return;

        if (quest->ended) {
            send_to_char(ch, "That quest has already , you psychopath!\r\n");
            return;
        }

        if (banned_from_quest(quest, idnum)) {
            send_to_char(ch,
                "That character is already banned from this quest.\r\n");
            return;
        }

        if (is_playing_quest(quest, idnum)) {
            if (!remove_quest_player(quest, idnum)) {
                send_to_char(ch, "Unable to auto-kick victim from quest!\r\n");
            } else {
                send_to_char(ch, "%s auto-kicked from quest.\r\n", arg1);

                snprintf(buf, sizeof(buf), "auto-kicked %s from quest %d '%s'.",
                    vict ? GET_NAME(vict) : arg1, quest->vnum, quest->name);
                qlog(ch, buf, QLOG_COMP, 0, true);
            }
        }

        if (!add_quest_ban(quest, idnum)) {
            send_to_char(ch, "Error banning char from quest.\r\n");
            return;
        }

        if (vict) {
            send_to_char(ch, "You have been banned from quest '%s'.\r\n",
                quest->name);
        }

        snprintf(buf, sizeof(buf), "banned %s from quest %d '%s'.",
            vict ? GET_NAME(vict) : arg1, quest->vnum, quest->name);
        qlog(ch, buf, QLOG_COMP, 0, true);

        send_to_char(ch, "%s banned from quest %d.\r\n",
            vict ? GET_NAME(vict) : arg1, quest->vnum);
    }
    if (del_vict)
        free_creature(vict);
}

static void
do_qcontrol_unban(struct creature *ch, char *argument, int com)
{

    struct quest *quest = NULL;
    struct creature *vict = NULL;
    unsigned int idnum, accountID;
    int level = 0;
    bool del_vict = false;
    struct account *account;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (player_idnum_by_name(arg1) < 0) {
        send_to_char(ch, "There is no character named '%s'\r\n", arg1);
        return;
    } else {
        idnum = player_idnum_by_name(arg1);
        accountID = player_account_by_name(arg1);
    }

    if (!(vict = get_char_in_world_by_idnum(idnum))) {
        // load the char from file
        if (idnum > 0) {
            vict = load_player_from_xml(idnum);
            level = GET_LEVEL(vict);
            del_vict = true;
            account = account_by_idnum(accountID);
        } else {
            send_to_char(ch, "Error loading char from file.\r\n");
            return;
        }
    } else {
        level = GET_LEVEL(vict);
        account = vict->account;
    }

    if (level >= GET_LEVEL(ch) && vict && ch != vict) {
        send_to_char(ch, "You are not powerful enough to do this.\r\n");
        return;
    }

    if (!strcmp("all", arg2)) { //unban from all quests
        if (!is_authorized(ch, QUEST_BAN, NULL)) {
            send_to_char(ch, "You do not have this ability.\r\n");
        } else if (!account->quest_banned) {
            send_to_char(ch,
                "That player is not banned from all quests... maybe you should ban him!\r\n");
        } else {

            account_set_quest_banned(account, false);   //unban

            snprintf(buf, sizeof(buf), "unbanned %s from all quests.", GET_NAME(vict));
            qlog(ch, buf, QLOG_COMP, 0, true);

            send_to_char(ch, "%s unbanned from all quests.\r\n",
                GET_NAME(vict));
            send_to_char(vict,
                "You are no longer banned from all quests.\r\n");
        }
    } else {
        if (!(quest = find_quest(ch, arg2)))
            return;

        if (!is_authorized(ch, EDIT_QUEST, quest))
            return;

        if (quest->ended) {
            send_to_char(ch,
                "That quest has already ended, you psychopath!\r\n");
            return;
        }

        if (!banned_from_quest(quest, idnum)) {
            send_to_char(ch,
                "That player is not banned... maybe you should ban him!\r\n");
            return;
        }

        if (!remove_quest_ban(quest, idnum)) {
            send_to_char(ch, "Error unbanning char from quest.\r\n");
            return;
        }

        snprintf(buf, sizeof(buf), "unbanned %s from %d quest '%s'.",
            vict ? GET_NAME(vict) : arg1, quest->vnum, quest->name);
        qlog(ch, buf, QLOG_COMP, 0, true);

        send_to_char(ch, "%s unbanned from quest %d.\r\n",
            vict ? GET_NAME(vict) : arg1, quest->vnum);
    }

    if (del_vict)
        free_creature(vict);
}

static void
do_qcontrol_level(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);

    if (!*argument || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    quest->owner_level = atoi(arg2);

    snprintf(buf, sizeof(buf), "set quest %d '%s' access level to %d",
        quest->vnum, quest->name, quest->owner_level);
    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "%s", buf);
}

static void
do_qcontrol_minlev(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    quest->minlevel = MIN(LVL_GRIMP, MAX(0, atoi(arg2)));

    snprintf(buf, sizeof(buf), "set quest %d '%s' minimum level to %d",
        quest->vnum, quest->name, quest->minlevel);

    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "%s", buf);
}

static void
do_qcontrol_maxlev(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    quest->maxlevel = MIN(LVL_GRIMP, MAX(0, atoi(arg2)));

    snprintf(buf, sizeof(buf), "set quest %d '%s' maximum level to %d",
        quest->vnum, quest->name, quest->maxlevel);
    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "%s", buf);
}

static void
do_qcontrol_mingen(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    quest->mingen = MIN(10, MAX(0, atoi(arg2)));

    snprintf(buf, sizeof(buf), "set quest %d '%s' minimum gen to %d",
        quest->vnum, quest->name, quest->mingen);
    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "%s", buf);
}

static void
do_qcontrol_maxgen(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;

    two_arguments(argument, arg1, arg2);

    if (!*arg2 || !*arg1) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    quest->maxgen = MIN(10, MAX(0, atoi(arg2)));

    snprintf(buf, sizeof(buf), "set quest %d '%s' maximum gen to %d",
        quest->vnum, quest->name, quest->maxgen);
    qlog(ch, buf, QLOG_NORM, LVL_AMBASSADOR, true);
    send_to_char(ch, "%s", buf);
}

static void
do_qcontrol_mute(struct creature *ch, char *argument, int com)
{

    struct quest *quest = NULL;
    long idnum;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if ((idnum = player_idnum_by_name(arg1)) < 0) {
        send_to_char(ch, "There is no character named '%s'\r\n", arg1);
        return;
    }

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
        return;
    }

    struct qplayer_data *player = quest_player_by_idnum(quest, idnum);

    if (!player) {
        send_to_char(ch, "That player is not in the quest.\r\n");
        return;
    }

    if (IS_SET(player->flags, QP_MUTE)) {
        send_to_char(ch, "That player is already muted.\r\n");
        return;
    }

    SET_BIT(player->flags, QP_MUTE);

    snprintf(buf, sizeof(buf), "muted %s in %d quest '%s'.", arg1, quest->vnum, quest->name);
    qlog(ch, buf, QLOG_COMP, 0, true);

    send_to_char(ch, "%s muted for quest %d.\r\n", arg1, quest->vnum);

}

static void
do_qcontrol_unmute(struct creature *ch, char *argument, int com)
{

    struct quest *quest = NULL;
    long idnum;

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg2)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    if ((idnum = player_idnum_by_name(arg1)) < 0) {
        send_to_char(ch, "There is no character named '%s'\r\n", arg1);
        return;
    }

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
        return;
    }

    struct qplayer_data *player = quest_player_by_idnum(quest, idnum);

    if (!player) {
        send_to_char(ch, "That player is not in the quest.\r\n");
        return;
    }

    if (!IS_SET(player->flags, QP_MUTE)) {
        send_to_char(ch, "That player is not muted.\r\n");
        return;
    }

    REMOVE_BIT(player->flags, QP_MUTE);

    snprintf(buf, sizeof(buf), "unmuted %s in quest %d '%s'.", arg1, quest->vnum,
        quest->name);
    qlog(ch, buf, QLOG_COMP, 0, true);

    send_to_char(ch, "%s unmuted for quest %d.\r\n", arg1, quest->vnum);

}

static void
do_qcontrol_switch(struct creature *ch, char *argument)
{
    struct quest *quest = NULL;

    if ((!(quest = quest_by_vnum(GET_QUEST(ch))) ||
            !is_playing_quest(quest, GET_IDNUM(ch)))
        && (GET_LEVEL(ch) < LVL_ELEMENT)) {
        send_to_char(ch, "You are not currently active on any quest.\r\n");
        return;
    }
    do_switch(ch, argument, 0, SCMD_QSWITCH);
}

static void
do_qcontrol_title(struct creature *ch, char *argument)
{
    struct quest *quest;
    char *quest_str;

    quest_str = tmp_getword(&argument);
    if (isdigit(*quest_str)) {
        quest = quest_by_vnum(atoi(quest_str));
    } else {
        send_to_char(ch,
            "You must specify a quest to change the title of!\r\n");
        return;
    }

    if (!quest) {
        send_to_char(ch, "Quest #%s is not active!\r\n", quest_str);
        return;
    }

    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        send_to_char(ch, "Piss off, beanhead.  Permission DENIED!\r\n");
        return;
    }

    if (!*argument) {
        send_to_char(ch,
            "You might wanna put a new title in there, partner.\r\n");
        return;
    }

    free(quest->name);
    quest->name = strdup(argument);
    send_to_char(ch, "Quest #%d's title has been changed to %s\r\n",
        quest->vnum, quest->name);
}

static void
do_qcontrol_award(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    struct creature *vict = NULL;
    char arg3[MAX_INPUT_LENGTH];    // Awarded points
    int award;
    int idnum;

    argument = two_arguments(argument, arg1, arg2);
    argument = one_argument(argument, arg3);
    award = atoi(arg3);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1))) {
        return;
    }

    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }

    if ((idnum = player_idnum_by_name(arg2)) < 0) {
        send_to_char(ch, "There is no character named '%s'.\r\n", arg2);
        return;
    }

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
        return;
    }

    if ((vict = get_char_in_world_by_idnum(idnum))) {
        if (!is_playing_quest(quest, idnum)) {
            send_to_char(ch, "No such player in the quest.\r\n");
            return;
        }
        if (!vict->desc) {
            send_to_char(ch,
                "You can't award quest points to a linkless player.\r\n");
            return;
        }
    }

    if ((award <= 0)) {
        send_to_char(ch, "The award must be greater than zero.\r\n");
        return;
    }

    if ((award > GET_IMMORT_QP(ch))) {
        send_to_char(ch, "You do not have the required quest points.\r\n");
        return;
    }

    if (!(vict)) {
        send_to_char(ch, "No such player in the quest.\r\n");
        return;
    }

    if ((ch) && (vict)) {
        GET_IMMORT_QP(ch) -= award;
        account_set_quest_points(vict->account,
            vict->account->quest_points + award);
        quest->awarded += award;
        crashsave(ch);
        crashsave(vict);
        snprintf(buf, sizeof(buf), "awarded player %s %d qpoints.", GET_NAME(vict), award);
        send_to_char(vict, "Congratulations! You have been awarded %d qpoint%s!\r\n", award, PLURAL(award));
        send_to_char(ch, "You award %d qpoint%s.\r\n", award, PLURAL(award));
        qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
            true);
        if (*argument) {
            snprintf(buf, sizeof(buf), "'s Award Comments: %s", argument);
            qlog(ch, buf, QLOG_COMP, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
                true);
        }
    }

}

static void
do_qcontrol_penalize(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    struct creature *vict = NULL;
    char arg3[MAX_INPUT_LENGTH];    // Penalized points
    int penalty;
    int idnum;

    argument = two_arguments(argument, arg1, arg2);
    argument = one_argument(argument, arg3);
    penalty = atoi(arg3);

    if (!*arg1 || !*arg2) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, arg1))) {
        return;
    }

    if (!is_authorized(ch, EDIT_QUEST, quest)) {
        return;
    }

    if ((idnum = player_idnum_by_name(arg2)) < 0) {
        send_to_char(ch, "There is no character named '%s'.\r\n", arg2);
        return;
    }

    if (quest->ended) {
        send_to_char(ch, "That quest has already ended, you psychopath!\r\n");
        return;
    }

    if ((vict = get_char_in_world_by_idnum(idnum))) {
        if (!is_playing_quest(quest, idnum)) {
            send_to_char(ch, "No such player in the quest.\r\n");
            return;
        }
    }

    if (!(vict)) {
        send_to_char(ch, "No such player in the quest.\r\n");
        return;
    }

    if ((penalty <= 0)) {
        send_to_char(ch, "The penalty must be greater than zero.\r\n");
        return;
    }

    if ((penalty > vict->account->quest_points)) {
        send_to_char(ch, "They do not have the required quest points.\r\n");
        return;
    }

    if ((ch) && (vict)) {
        account_set_quest_points(vict->account,
            vict->account->quest_points - penalty);
        GET_IMMORT_QP(ch) += penalty;
        quest->penalized += penalty;
        crashsave(vict);
        crashsave(ch);
        send_to_char(vict,
            "%d of your quest points have been taken by %s!\r\n", penalty,
            GET_NAME(ch));
        send_to_char(ch, "%d quest points transferred from %s.\r\n", penalty,
            GET_NAME(vict));
        snprintf(buf, sizeof(buf), "penalized player %s %d qpoints.", GET_NAME(vict),
            penalty);
        qlog(ch, buf, QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
            true);
        if (*argument) {
            snprintf(buf, sizeof(buf), "'s Penalty Comments: %s", argument);
            qlog(ch, buf, QLOG_COMP, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR),
                true);
        }
    }
}

static void
do_qcontrol_restore(struct creature *ch, char *argument, int com)
{
    struct quest *quest = NULL;
    struct creature *vict = NULL;
    char *str;

    str = tmp_getword(&argument);

    if (!str) {
        do_qcontrol_usage(ch, com);
        return;
    }

    if (!(quest = find_quest(ch, str)))
        return;

    if (!is_authorized(ch, EDIT_QUEST, quest))
        return;

    for (GList * pit = quest->players; pit; pit = pit->next) {
        struct qplayer_data *player = pit->data;
        if ((vict = get_char_in_world_by_idnum(player->idnum))) {
            restore_creature(vict);
            if (!PLR_FLAGGED(vict, PLR_WRITING) && vict->desc) {
                send_to_char(vict, "%s",
                    compose_qcomm_string(ch, vict, quest, QCOMM_ECHO,
                        "You have been restored!"));
            }
        }
    }
}

static void
do_qcontrol_save(struct creature *ch)
{
    save_quests();
    send_to_char(ch, "Quests saved.\r\n");
}

ACMD(do_qcontrol)
{
    char arg1[MAX_INPUT_LENGTH];
    int com;

    argument = one_argument(argument, arg1);
    skip_spaces(&argument);
    if (!*arg1) {
        do_qcontrol_options(ch);
        return;
    }

    for (com = 0;; com++) {
        if (!qc_options[com].keyword) {
            send_to_char(ch, "Unknown qcontrol option, '%s'.\r\n", arg1);
            return;
        }
        if (is_abbrev(arg1, qc_options[com].keyword))
            break;
    }
    if (qc_options[com].level > GET_LEVEL(ch)) {
        send_to_char(ch, "You are not godly enough to do this!\r\n");
        return;
    }

    switch (com) {
    case 0:                    // show
        do_qcontrol_show(ch, argument);
        break;
    case 1:                    // create
        do_qcontrol_create(ch, argument, com);
        break;
    case 2:                    // end
        do_qcontrol_end(ch, argument, com);
        break;
    case 3:                    // add
        do_qcontrol_add(ch, argument, com);
        break;
    case 4:                    // kick
        do_qcontrol_kick(ch, argument, com);
        break;
    case 5:                    // flags
        do_qcontrol_flags(ch, argument, com);
        break;
    case 6:                    // comment
        do_qcontrol_comment(ch, argument, com);
        break;
    case 7:                    // desc
        do_qcontrol_desc(ch, argument, com);
        break;
    case 8:                    // update
        do_qcontrol_update(ch, argument, com);
        break;
    case 9:                    // ban
        do_qcontrol_ban(ch, argument, com);
        break;
    case 10:                   // unban
        do_qcontrol_unban(ch, argument, com);
        break;
    case 11:                   // mute
        do_qcontrol_mute(ch, argument, com);
        break;
    case 12:                   // unnute
        do_qcontrol_unmute(ch, argument, com);
        break;
    case 13:                   // level
        do_qcontrol_level(ch, argument, com);
        break;
    case 14:                   // minlev
        do_qcontrol_minlev(ch, argument, com);
        break;
    case 15:                   // maxlev
        do_qcontrol_maxlev(ch, argument, com);
        break;
    case 16:                   // maxgen
        do_qcontrol_mingen(ch, argument, com);
        break;
    case 17:                   // mingen
        do_qcontrol_maxgen(ch, argument, com);
        break;
    case 18:                   // Load Mobile
        do_qcontrol_mload(ch, argument, com);
        break;
    case 19:                   // Purge Mobile
        do_qcontrol_purge(ch, argument);
        break;
    case 20:
        do_qcontrol_save(ch);
        break;
    case 21:                   // help - in help_collection.cc
        do_qcontrol_help(ch, argument);
        break;
    case 22:
        do_qcontrol_switch(ch, argument);
        break;
    case 23:                   // title
        do_qcontrol_title(ch, argument);
        break;
    case 24:                   // oload
        do_qcontrol_oload(ch, argument, com);
        break;
    case 25:
        do_qcontrol_trans(ch, argument);
        break;
    case 26:                   // award
        do_qcontrol_award(ch, argument, com);
        break;
    case 27:                   // penalize
        do_qcontrol_penalize(ch, argument, com);
        break;
    case 28:                   // restore
        do_qcontrol_restore(ch, argument, com);
        break;
    case 29:
        do_qcontrol_loadroom(ch, argument, com);
        break;
    default:
        send_to_char(ch,
            "Sorry, this qcontrol option is not implemented.\r\n");
        break;
    }
}
