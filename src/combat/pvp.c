#include "room_data.h"
#include "creature.h"
#include "utils.h"
#include "quest.h"
#include "bomb.h"
#include "handler.h"
#include "spells.h"
#include "char_class.h"
#include "comm.h"
#include "security.h"
#include "players.h"

bool
is_arena_combat(struct creature *ch, struct creature *vict)
{
    if (!vict->in_room)
        return false;

    if (ROOM_FLAGGED(vict->in_room, ROOM_ARENA))
        return true;

    //mobs don't quest
    if (!IS_NPC(vict)) {
        if (GET_QUEST(vict)) {
            struct quest *quest;

            quest = quest_by_vnum(GET_QUEST(vict));
            if (QUEST_FLAGGED(quest, QUEST_ARENA))
                return true;
        }
    }

    if (!ch || !ch->in_room)
        return false;

    if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
        return true;

    //mobs don't quest
    if (!IS_NPC(ch)) {
        if (GET_QUEST(ch)) {
            struct quest *quest;

            quest = quest_by_vnum(GET_QUEST(ch));
            if (QUEST_FLAGGED(quest, QUEST_ARENA))
                return true;
        }
    }

    return false;
}

bool
is_npk_combat(struct creature * ch, struct creature * vict)
{
    if (!ch || !vict) {
        return false;
    }

    if (IS_NPC(ch) || IS_NPC(vict))
        return false;

    if (PLR_FLAGGED(vict, PLR_HARDCORE))
        return false;

    if (vict->in_room->zone->pk_style == ZONE_NEUTRAL_PK) {
        return true;
    }

    return false;
}

// Calculates the amount of reputation gained by the killer if they
// were to pkill the victim
int
pk_reputation_gain(struct creature *perp, struct creature *victim)
{
    if (perp == victim
        || IS_NPC(perp)
        || IS_NPC(victim)
        || !g_list_find(perp->fighting, victim))
        return 0;

    // Start with 10 for causing hassle
    int gain = 10;

    // adjust for level/gen difference
    gain += ((GET_LEVEL(perp) + GET_REMORT_GEN(perp) * 50)
             - (GET_LEVEL(victim) + GET_REMORT_GEN(victim) * 50)) / 5;

    // Additional adjustment for killing an innocent
    if (GET_REPUTATION(victim) == 0)
        gain *= 2;

    // Additional adjustment for killing a lower gen
    if (GET_REMORT_GEN(perp) > GET_REMORT_GEN(victim))
        gain += (GET_REMORT_GEN(perp) - GET_REMORT_GEN(victim)) * 9;

    if (IS_CRIMINAL(victim))
        gain /= 4;

    gain = MAX(1, gain);

    return gain;
}

struct creature *
find_responsible_party(struct creature *attacker, struct creature *victim)
{
    struct creature *perp = NULL;
    struct creature *ch;

    // if the victim is charmed, their master is at fault for letting
    // them die.  Charming can potentially be chained, so we find the
    // "top" master who is a PC in the same room here.
    for (ch = victim; AFF_FLAGGED(ch, AFF_CHARM)
        && ch->master && ch->in_room == ch->master->in_room; ch = ch->master)
        if (IS_PC(ch))
            perp = ch;
    if (perp)
        return perp;

    // if the attacker is charmed, their master is responsible for
    // their behavior.  We also handle chained charms here.
    for (ch = attacker; AFF_FLAGGED(ch, AFF_CHARM)
        && ch->master && ch->in_room == ch->master->in_room; ch = ch->master)
        if (IS_PC(ch))
            perp = ch;
    if (perp)
        return perp;

    // if neither was charmed, the attacker acted alone and is solely
    // responsible

    return attacker;
}

void
create_grievance(struct creature *ch,
    struct creature *perp, int gain, enum grievance_kind kind)
{
    struct grievance *grievance;

    CREATE(grievance, struct grievance, 1);
    grievance->time = time(NULL);
    grievance->player_id = GET_IDNUM(perp);
    grievance->rep = gain;
    grievance->grievance = kind;
    GET_GRIEVANCES(ch) = g_list_prepend(GET_GRIEVANCES(ch), grievance);
}

void
check_attack(struct creature *attacker, struct creature *victim)
{
    bool is_bountied(struct creature *hunter, struct creature *vict);
    struct creature *perp;

    // No reputation for attacking in arena
    if (is_arena_combat(attacker, victim))
        return;

    perp = find_responsible_party(attacker, victim);

    // no reputation for attacking a bountied person
    if (is_bountied(perp, victim))
        return;

    int gain = pk_reputation_gain(perp, victim);

    if (!gain)
        return;

    gain = MAX(1, gain / 5);
    gain_reputation(perp, gain);

    mudlog(LVL_IMMORT, CMP, true,
        "%s gained %d reputation for attacking %s", GET_NAME(perp),
        gain, GET_NAME(victim));
    create_grievance(victim, perp, gain, ATTACK);
}

void
count_pkill(struct creature *killer, struct creature *victim)
{
    bool award_bounty(struct creature *, struct creature *);
    struct creature *perp;

    if (is_arena_combat(killer, victim))
        return;

    perp = find_responsible_party(killer, victim);

    GET_PKILLS(perp)++;

    if (award_bounty(perp, victim))
        return;

    int gain = pk_reputation_gain(perp, victim);

    if (!gain)
        return;

    gain_reputation(perp, gain);

    mudlog(LVL_IMMORT, CMP, true,
        "%s gained %d reputation for murdering %s", GET_NAME(perp),
        gain, GET_NAME(victim));
    create_grievance(victim, perp, gain, MURDER);
}


void
check_thief(struct creature *ch, struct creature *victim)
{
    struct creature *perp;

    // First we need to find the perp
    perp = find_responsible_party(ch, victim);

    int gain = pk_reputation_gain(perp, victim);

    if (!gain)
        return;

    gain = MAX(1, gain / 10);
    gain_reputation(perp, gain);

    mudlog(LVL_IMMORT, CMP, true,
        "%s gained %d reputation for stealing from %s", GET_NAME(perp),
        gain, GET_NAME(victim));
    create_grievance(victim, perp, gain, THEFT);

    if (is_arena_combat(ch, victim))
        mudlog(LVL_POWER, CMP, true,
            "%s pstealing from %s in arena", GET_NAME(perp), GET_NAME(victim));
}

GList *
g_list_remove_if(GList * list, GCompareFunc func, gpointer user_data)
{
    GList *cur = list;
    GList *next;

    while (cur) {
        next = g_list_next(cur);
        if (!func(cur->data, user_data))
            list = g_list_delete_link(list, cur);
        cur = next;
    }
    return list;
}

int
matches_player(struct creature *tch, gpointer idnum_ptr)
{
    return (GET_IDNUM(tch) == GPOINTER_TO_INT(idnum_ptr)) ? 0 : -1;
}

void
perform_pardon(struct creature *ch, struct creature *pardoned)
{
    for (GList *it = GET_GRIEVANCES(ch);it;it = it->next) {
        struct grievance *grievance = it->data;

        if (grievance->player_id == GET_IDNUM(pardoned)) {

            if (grievance->grievance == MURDER) {
                mudlog(LVL_IMMORT, CMP, true,
                    "%s recovered %d reputation for murdering %s",
                    GET_NAME(pardoned), grievance->rep, GET_NAME(ch));
            } else if (grievance->grievance == ATTACK) {
                mudlog(LVL_IMMORT, CMP, true,
                    "%s recovered %d reputation for attacking %s",
                    GET_NAME(pardoned), grievance->rep, GET_NAME(ch));
            } else {
                mudlog(LVL_IMMORT, CMP, true,
                    "%s recovered %d reputation for stealing from %s",
                    GET_NAME(pardoned), grievance->rep, GET_NAME(ch));
            }

            gain_reputation(pardoned, -(grievance->rep));
        }
    }

    GET_GRIEVANCES(ch) = g_list_remove_if(GET_GRIEVANCES(ch),
        (GCompareFunc) matches_player, GINT_TO_POINTER(GET_IDNUM(pardoned)));
}

gint
grievance_expired(struct grievance *g, gpointer user_data)
{
    int min_time = GPOINTER_TO_INT(user_data);

    return (g->time < min_time) ? 0 : -1;
}

// Expire old grievances after 24 hours.
void
expire_old_grievances(struct creature *ch)
{
    time_t min_time = time(NULL) - 86400;
    GET_GRIEVANCES(ch) = g_list_remove_if(GET_GRIEVANCES(ch),
                                          (GCompareFunc) grievance_expired, GINT_TO_POINTER(min_time));
}

ACMD(do_pardon)
{
    if (!*argument) {
        send_to_char(ch, "You must specify someone to pardon!\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM)) {
        send_to_char(ch, "You don't seem quite in your right mind...\r\n");
        return;
    }
    // Find who they're accusing
    char *pardoned_name = tmp_getword(&argument);
    if (!player_name_exists(pardoned_name)) {
        send_to_char(ch, "There's no one of that name to pardon.\r\n");
        return;
    }
    // Get the pardoned character
    int pardoned_idnum = player_idnum_by_name(pardoned_name);
    struct creature *pardoned = get_char_in_world_by_idnum(pardoned_idnum);
    bool loaded_pardoned = false;
    if (!pardoned) {
        loaded_pardoned = true;
        CREATE(pardoned, struct creature, 1);
        pardoned = load_player_from_xml(pardoned_idnum);
    }
    // Do the imm pardon
    if (IS_IMMORT(ch) && is_authorized(ch, PARDON, pardoned)) {
        perform_pardon(ch, pardoned);
        send_to_char(ch, "Pardoned.\r\n");
        send_to_char(pardoned, "You have been pardoned by the Gods!\r\n");
        mudlog(MAX(LVL_GOD, GET_INVIS_LVL(ch)), NRM, true,
            "(GC) %s pardoned by %s", GET_NAME(pardoned), GET_NAME(ch));
    } else {

        // Find out if the player has a valid grievance against the pardoned
        expire_old_grievances(ch);
        if (g_list_find_custom(GET_GRIEVANCES(ch),
                GINT_TO_POINTER(GET_IDNUM(pardoned)),
                (GCompareFunc) matches_player)) {
            // If no grievance, increase the reputation of the pardoner
            send_to_char(ch, "%s has done nothing for you to pardon.\r\n",
                GET_NAME(pardoned));
            return;
        }
        send_to_char(ch, "You pardon %s of %s crimes against you.\r\n",
            GET_NAME(pardoned), HSHR(pardoned));
        send_to_char(pardoned, "%s pardons your crimes against %s.\r\n",
            GET_NAME(ch), HMHR(ch));
        perform_pardon(ch, pardoned);
    }

    save_player_to_xml(ch);
    save_player_to_xml(pardoned);
    if (loaded_pardoned)
        free_creature(pardoned);
}

void
check_object_killer(struct obj_data *obj, struct creature *vict)
{
    struct creature *killer = NULL;
    bool loaded_killer = false;
    int obj_id;

    if (ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)) {
        return;
    }
    if (IS_NPC(vict))
        return;

    slog("Checking object killer %s -> %s. ", obj->name, GET_NAME(vict));

    if (IS_BOMB(obj))
        obj_id = BOMB_IDNUM(obj);
    else if (GET_OBJ_SIGIL_IDNUM(obj))
        obj_id = GET_OBJ_SIGIL_IDNUM(obj);
    else {
        errlog("unknown damager in check_object_killer.");
        return;
    }

    if (!obj_id)
        return;

    killer = get_char_in_world_by_idnum(obj_id);

    // load the bastich from file.
    if (!killer) {
        killer = load_player_from_xml(obj_id);
        if (killer) {
            killer->account =
                account_by_idnum(player_account_by_idnum(obj_id));
            loaded_killer = true;
        }
    }
    // the piece o shit has a bogus killer idnum on it!
    if (!killer) {
        errlog("bogus idnum %d on object %s damaging %s.",
            obj_id, obj->name, GET_NAME(vict));
        return;
    }

    count_pkill(killer, vict);

    // save the sonuvabitch to file
    save_player_to_xml(killer);

    if (loaded_killer)
        free_creature(killer);
}
