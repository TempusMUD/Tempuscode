#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
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
#include "creature.h"
#include "db.h"
#include "house.h"
#include "clan.h"
#include "players.h"
#include "tmpstr.h"
#include "account.h"
#include "spells.h"
#include "fight.h"
#include "obj_data.h"
#include "weather.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "paths.h"

void extract_norents(struct obj_data *obj);
void char_arrest_pardoned(struct creature *ch);
extern struct descriptor_data *descriptor_list;
struct player_special_data dummy_mob;   /* dummy spec area for mobs         */

struct creature *
make_creature(bool pc)
{
    struct creature *ch;

    CREATE(ch, struct creature, 1);

    if (pc) {
        CREATE(ch->player_specials, struct player_special_data, 1);
        ch->player_specials->desc_mode = CXN_UNKNOWN;
    } else {
        ch->player_specials = &dummy_mob;
        SET_BIT(NPC_FLAGS(ch), NPC_ISNPC);
    }

    return ch;
}

void
reset_creature(struct creature *ch)
{
    void free_alias(struct alias_data *a);

    struct creature *tmp_mob;
    struct alias_data *a;
    bool is_pc;

    //
    // first make sure the char is no longer in the world
    //
    if (ch->in_room != NULL
        || ch->carrying != NULL
        || is_fighting(ch) || ch->followers != NULL || ch->master != NULL) {
        errlog("attempted clear of creature: %p %p %p %p %p",
            ch->in_room,
               ch->carrying, ch->fighting, ch->followers, ch->master);
        raise(SIGSEGV);
    }
    //
    // also check equipment and implants
    //
    for (int pos = 0; pos < NUM_WEARS; pos++) {
        if (ch->equipment[pos]
            || ch->implants[pos]
            || ch->tattoos[pos]) {
            errlog
                ("attempted clear of creature who is still connected to the world.");
            raise(SIGSEGV);
        }
    }

    //
    // next remove and free all alieases
    //

    while ((a = GET_ALIASES(ch)) != NULL) {
        GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
        free_alias(a);
    }

    //
    // now remove all affects
    //

    while (ch->affected)
        affect_remove(ch, ch->affected);

    //
    // free mob strings:
    // free strings only if the string is not pointing at proto
    //
    if (ch->mob_specials.shared && GET_NPC_VNUM(ch) > -1) {

        tmp_mob = real_mobile_proto(GET_NPC_VNUM(ch));

        if (ch->player.name != tmp_mob->player.name)
            free(ch->player.name);
        if (ch->player.title != tmp_mob->player.title)
            free(ch->player.title);
        if (ch->player.short_descr != tmp_mob->player.short_descr)
            free(ch->player.short_descr);
        if (ch->player.long_descr != tmp_mob->player.long_descr)
            free(ch->player.long_descr);
        if (ch->player.description != tmp_mob->player.description)
            free(ch->player.description);
        free(ch->mob_specials.func_data);
        prog_state_free(ch->prog_state);
    } else {
        //
        // otherwise ch is a player, so free all
        //

        free(ch->player.name);
        free(ch->player.title);
        free(ch->player.short_descr);
        free(ch->player.long_descr);
        free(ch->player.description);
        free(ch->mob_specials.func_data);
        prog_state_free(ch->prog_state);
    }

    // remove player_specials
    is_pc = !IS_NPC(ch);

    if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
        free(ch->player_specials->poofin);
        free(ch->player_specials->poofout);
        free(ch->player_specials->afk_reason);
        g_list_foreach(ch->player_specials->afk_notifies, (GFunc)free, NULL);
        g_list_free(ch->player_specials->afk_notifies);
        ch->player_specials->afk_notifies = NULL;
        g_list_foreach(GET_RECENT_KILLS(ch), (GFunc)free, NULL);
        g_list_free(GET_RECENT_KILLS(ch));
        GET_RECENT_KILLS(ch) = NULL;
        g_list_foreach(GET_GRIEVANCES(ch), (GFunc)free, NULL);
        g_list_free(GET_GRIEVANCES(ch));
        GET_GRIEVANCES(ch) = NULL;
        free(ch->player_specials);

        if (IS_NPC(ch)) {
            errlog("Mob had player_specials allocated!");
            raise(SIGSEGV);
        }
    }
    //
    // next remove all the combat ch creature might be involved in
    //
    remove_all_combat(ch);

    memset(ch, 0, sizeof(struct creature));

    // And we reset all the values to their initial settings
    ch->fighting = NULL;
    GET_POSITION(ch) = POS_STANDING;
    GET_CLASS(ch) = -1;
    GET_REMORT_CLASS(ch) = -1;

    GET_AC(ch) = 100;           /* Basic Armor */
    if (ch->points.max_mana < 100)
        ch->points.max_mana = 100;

    if (is_pc) {
        CREATE(ch->player_specials, struct player_special_data, 1);
        set_title(ch, "");
    } else {
        ch->player_specials = &dummy_mob;
        SET_BIT(NPC_FLAGS(ch), NPC_ISNPC);
        GET_TITLE(ch) = NULL;
    }
}

void
free_creature(struct creature *ch)
{
    reset_creature(ch);
    if (ch->player_specials != &dummy_mob) {
        free(ch->player_specials);
        free(ch->player.title);
    }
}

void
check_position(struct creature *ch)
{
    if (GET_HIT(ch) > 0) {
        if (GET_POSITION(ch) < POS_STUNNED)
            GET_POSITION(ch) = POS_RESTING;
        return;
    }

    if (GET_HIT(ch) > -3)
        GET_POSITION(ch) = POS_STUNNED;
    else if (GET_HIT(ch) > -6)
        GET_POSITION(ch) = POS_INCAP;
    else
        GET_POSITION(ch) = POS_MORTALLYW;
}

/**
 * Returns true if ch character is in the Testers access group.
**/
bool
is_tester(struct creature *ch)
{
    if (IS_NPC(ch))
        return false;
    return is_authorized(ch, TESTER, NULL);
}

bool
is_fighting(struct creature *ch)
{
    if (!ch->fighting)
        return false;

    return (first_living(ch->fighting) != NULL);
}

// Returns this creature's account id.
long
account_id(struct creature *ch)
{
    if (ch->account == NULL)
        return 0;
    return ch->account->id;
}

void
add_player_tag(struct creature *ch, const char *tag)
{
    if (!IS_PC(ch)) {
        return;
    }
    if (ch->player_specials->tags == NULL) {
        ch->player_specials->tags = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }
    g_hash_table_add(ch->player_specials->tags, g_strdup(tag));
}

void
remove_player_tag(struct creature *ch, const char *tag)
{
    if (!IS_PC(ch) || ch->player_specials->tags == NULL) {
        return;
    }
    
    g_hash_table_remove(ch->player_specials->tags, tag);
}

bool
player_has_tag(struct creature *ch, const char *tag)
{
    if (!IS_PC(ch) || ch->player_specials->tags == NULL) {
        return false;
    }
    
    return g_hash_table_contains(ch->player_specials->tags, tag);
}

/**
 * Modifies the given experience to be appropriate for ch character's
 *  level/gen and class.
 * if victim != NULL, assume that ch char is fighting victim to gain
 *  experience.
 *
**/
int
calc_penalized_exp(struct creature *ch, int experience,
    struct creature *victim)
{

    // Mobs are easily trained
    if (IS_NPC(ch))
        return experience;
    // Immortals are not
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR)
        return 0;

    if (victim != NULL) {
        if (GET_LEVEL(victim) >= LVL_AMBASSADOR)
            return 0;

        // good clerics & knights penalized for killing good
        if (IS_GOOD(victim) && IS_GOOD(ch) && (IS_CLERIC(ch) || IS_KNIGHT(ch))) {
            experience /= 2;
        }
    }
    // Slow remorting down a little without slowing leveling completely.
    // Ch penalty works out to:
    // gen lvl <=15 16->39  40>
    //  1     23.3%  33.3%  43.3%
    //  2     40.0%  50.0%  60.0%
    //  3     50.0%  60.0%  70.0%
    //  4     56.6%  66.6%  76.6%
    //  5     61.4%  71.4%  81.4%
    //  6     65.0%  75.0%  85.0%
    //  7     67.7%  77.7%  87.7%
    //  8     70.0%  80.0%  90.0%
    //  9     71.8%  81.8%  91.8%
    // 10     73.3%  83.3%  93.3%
    if (IS_REMORT(ch)) {
        float gen = GET_REMORT_GEN(ch);
        float multiplier = (gen / (gen + 2));

        if (GET_LEVEL(ch) <= 15)
            multiplier -= 0.10;
        else if (GET_LEVEL(ch) >= 40)
            multiplier += 0.10;

        experience -= (int)(experience * multiplier);
    }

    return experience;
}

/**
 * adjust_creature_money:
 * @ch: The creature whose money is adjusted
 * @amount: The amount of money to adjust by
 *
 * Adjusts the amount of money the creature is holding.  @amount may
 * be negative or positive.  @ch must be in the world.  The currency
 * to use is determined by the time frame of @ch's current location.
 * If the adjustment would result in a negative amount, the adjustment
 * does not take place.
 *
 * Returns: %true if the creature had enough money, %false if the
 * adjustment would result in a negative amount
 **/
bool
adjust_creature_money(struct creature *ch, money_t amount)
{
    if (ch->in_room->zone->time_frame == TIME_ELECTRO) {
        if (GET_CASH(ch) + amount < 0) {
            return false;
        }
        GET_CASH(ch) += amount;
    } else {
        if (GET_GOLD(ch) + amount < 0) {
            return false;
        }
        GET_GOLD(ch) += amount;
    }
    return true;
}

money_t
adjusted_price(struct creature *buyer, struct creature *seller, money_t base_price)
{
    int cost_modifier = (GET_CHA(seller) - GET_CHA(buyer)) * 2;
    money_t price = base_price + (base_price * cost_modifier) / 100;

    return MAX(1, price);
}

int
getSpeed(struct creature *ch)
{
    // if(IS_NPC(ch))
    if (ch->char_specials.saved.act & NPC_ISNPC)
        return 0;
    return (int)ch->player_specials->saved.speed;
}

void
setSpeed(struct creature *ch, int speed)
{
    if (ch->char_specials.saved.act & NPC_ISNPC)
        return;
    speed = MAX(speed, 0);
    speed = MIN(speed, 100);
    ch->player_specials->saved.speed = (char)(speed);
}

bool
is_newbie(struct creature *ch)
{
    if (ch->char_specials.saved.act & NPC_ISNPC)
        return false;
    if (ch->char_specials.saved.act & PLR_HARDCORE)
        return false;
    if ((ch->char_specials.saved.remort_generation) > 0)
        return false;
    if (ch->player.level > 24)
        return false;

    return true;
}

// Utility function to determine if a char should be affected by sanctuary
// on a hit by hit level... --N
bool
affected_by_sanctuary(struct creature * ch, struct creature * attacker)
{
    if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
        if (attacker && IS_EVIL(ch) &&
            affected_by_spell(attacker, SPELL_RIGHTEOUS_PENETRATION))
            return false;
        else if (attacker && IS_GOOD(ch) &&
            affected_by_spell(attacker, SPELL_MALEFIC_VIOLATION))
            return false;
        else
            return true;
    }

    return false;
}

// Pass in the attacker for conditional reduction such as PROT_GOOD and
// PROT_EVIL.  Or leave it blank for the characters base reduction --N
float
damage_reduction(struct creature *ch, struct creature *attacker)
{
    struct affected_type *af = NULL;
    float dam_reduction = 0;

    if (GET_CLASS(ch) == CLASS_CLERIC && IS_GOOD(ch)) {
        // good clerics get an alignment-based protection, up to 30% in the
        // full moon, up to 10% otherwise
        if (get_lunar_phase(lunar_day) == MOON_FULL)
            dam_reduction += GET_ALIGNMENT(ch) / 30;
        else
            dam_reduction += GET_ALIGNMENT(ch) / 100;
    }
    // *************************** Sanctuary ****************************
    // ******************************************************************
    if (affected_by_sanctuary(ch, attacker)) {
        if (IS_VAMPIRE(ch))
            dam_reduction += 0;
        else if ((IS_CLERIC(ch) || IS_KNIGHT(ch)) && !IS_NEUTRAL(ch))
            dam_reduction += 25;
        else if (GET_CLASS(ch) == CLASS_CYBORG
            || GET_CLASS(ch) == CLASS_PHYSIC)
            dam_reduction += 8;
        else
            dam_reduction += 15;
    }
    // ***************************** Oblivity ****************************
    // *******************************************************************
    // damage reduction ranges up to about 35%
    if (AFF2_FLAGGED(ch, AFF2_OBLIVITY) && IS_NEUTRAL(ch)) {
        dam_reduction += (((GET_LEVEL(ch) +
                    skill_bonus(ch, ZEN_OBLIVITY)) * 10) +
            (1000 - abs(GET_ALIGNMENT(ch))) +
            (CHECK_SKILL(ch, ZEN_OBLIVITY) * 10)) / 100;
    }
    // **************************** No Pain *****************************
    // ******************************************************************
    if (AFF_FLAGGED(ch, AFF_NOPAIN)) {
        dam_reduction += 25;
    }
    // **************************** Berserk *****************************
    // ******************************************************************
    if (AFF2_FLAGGED(ch, AFF2_BERSERK)) {
        if (IS_BARB(ch))
            dam_reduction += (skill_bonus(ch, SKILL_BERSERK)) / 6;
        else
            dam_reduction += 7;
    }
    // ************************** Damage Control ************************
    // ******************************************************************
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL)) {
        dam_reduction += (skill_bonus(ch, SKILL_DAMAGE_CONTROL)) / 5;
    }
    // **************************** ALCOHOLICS!!! ***********************
    // ******************************************************************
    if (GET_COND(ch, DRUNK) > 5)
        dam_reduction += GET_COND(ch, DRUNK);

    // ********************** Shield of Righteousness *******************
    // ******************************************************************
    if ((af = affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS))) {

        // Find the caster apply for the shield of righteousness spell
        while (af) {
            if (af->type == SPELL_SHIELD_OF_RIGHTEOUSNESS
                && af->location == APPLY_CASTER)
                break;
            af = af->next;
        }

        // We found the shield of righteousness caster
        if (af && af->modifier == GET_IDNUM(ch)) {
            dam_reduction +=
                (skill_bonus(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                + (GET_ALIGNMENT(ch) / 100);
        } else if (af && ch->in_room) {
            for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (IS_NPC(tch)
                    && af->modifier == (short int)-NPC_IDNUM(tch)) {
                    dam_reduction +=
                        (skill_bonus(tch, SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                        + (GET_ALIGNMENT(ch) / 100);
                    break;
                } else if (!IS_NPC(tch) && af->modifier == GET_IDNUM(tch)) {
                    dam_reduction +=
                        (skill_bonus(tch, SPELL_SHIELD_OF_RIGHTEOUSNESS) / 20)
                        + (GET_ALIGNMENT(ch) / 100);
                    break;
                }
            }
        }
    }
    // ************************** Aria of Asylum ************************
    // ******************************************************************
    // Ch should be very similar to Shield of Righteousness as it's also
    // a group thing
    if ((af = affected_by_spell(ch, SONG_ARIA_OF_ASYLUM))) {

        // Find the caster apply
        while (af) {
            if (af->type == SONG_ARIA_OF_ASYLUM
                && af->location == APPLY_CASTER)
                break;
            af = af->next;
        }

        // We found the aria of asylum singer
        if (af && af->modifier == GET_IDNUM(ch)) {
            dam_reduction += 5 + (((1000 - abs(GET_ALIGNMENT(ch))) / 100) +
                (skill_bonus(ch, SONG_ARIA_OF_ASYLUM) / 10));
        } else if (af && ch->in_room) {

            for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (IS_NPC(tch)
                    && af->modifier == (short int)-NPC_IDNUM(tch)) {
                    dam_reduction +=
                        5 + (((1000 - abs(GET_ALIGNMENT(tch))) / 100) +
                        (skill_bonus(tch, SONG_ARIA_OF_ASYLUM) / 10));
                    break;
                } else if (!IS_NPC(tch) && af->modifier == GET_IDNUM(tch)) {
                    dam_reduction +=
                        5 + (((1000 - abs(GET_ALIGNMENT(tch))) / 100) +
                        (skill_bonus(tch, SONG_ARIA_OF_ASYLUM) / 10));
                    break;
                }
            }
        }
    }
    // *********************** Lattice Hardening *************************
    // *******************************************************************
    if (affected_by_spell(ch, SPELL_LATTICE_HARDENING))
        dam_reduction += (skill_bonus(ch, SPELL_LATTICE_HARDENING)) / 6;

    // ************** Stoneskin Barkskin Dermal Hardening ****************
    // *******************************************************************
    struct affected_type *taf;
    if ((taf = affected_by_spell(ch, SPELL_STONESKIN)))
        dam_reduction += (taf->level) / 4;
    else if ((taf = affected_by_spell(ch, SPELL_BARKSKIN)))
        dam_reduction += (taf->level) / 6;
    else if ((taf = affected_by_spell(ch, SPELL_DERMAL_HARDENING)))
        dam_reduction += (taf->level) / 6;

    if (attacker) {
        /// ****************** Various forms of protection ***************
        if (IS_EVIL(attacker) && AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
            dam_reduction += 8;
        if (IS_GOOD(attacker) && AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
            dam_reduction += 8;
        if (IS_UNDEAD(attacker) && AFF2_FLAGGED(ch, AFF2_PROTECT_UNDEAD))
            dam_reduction += 8;
        if (IS_DEMON(attacker) && AFF2_FLAGGED(ch, AFF2_PROT_DEMONS))
            dam_reduction += 8;
        if (IS_DEVIL(attacker) && AFF2_FLAGGED(ch, AFF2_PROT_DEVILS))
            dam_reduction += 8;
    }

    dam_reduction += abs(MIN(0, GET_AC(ch) + 300) / 5);
    dam_reduction = MIN(dam_reduction, 75);

    return (dam_reduction / 100);
}

//
// Compute level bonus factor.
// Currently, a gen 4 level 49 secondary should equate to level 49 mort primary.
//
//   params: primary - Add in remort gen as a primary?
//   return: a number from 1-100 based on level and primary/secondary)

int
level_bonus(struct creature *ch, bool primary)
{
    int bonus = MIN(50, ch->player.level + 1);
    short gen = ch->char_specials.saved.remort_generation;

    if (gen == 0 && IS_NPC(ch)) {
        if ((ch->player.remort_char_class % NUM_CLASSES) == 0) {
            gen = 0;
        } else {
            gen = (GET_INT(ch) + GET_STR(ch) + GET_WIS(ch)) / 3;
            gen = MAX(0, gen - 18);
        }
    }

    if (gen == 0) {
        return bonus;
    } else {
        if (primary) {     // Primary. Give full remort bonus per gen.
            return bonus + (MIN(gen, 10)) * 5;
        } else { // Secondary. Give less level bonus and less remort bonus.
            return (bonus * 3 / 4) + (MIN(gen, 10) * 3);
        }
    }
}

//
// Compute level bonus factor.
// Should be used for a particular skill in general.
// Returns 50 for max mort, 100 for max remort.
// params: skill - the skill # to check bonus for.
// return: a number from 1-100 based on level/gen/can learn skill.

int
skill_bonus(struct creature *ch, int skill)
{
    // Immorts get full bonus.
    if (ch->player.level >= 50)
        return 100;

    // Irregular skill #s get 1
    if (skill > TOP_SPELL_DEFINE || skill < 0)
        return 1;

    if (IS_NPC(ch) && GET_CLASS(ch) >= NUM_CLASSES) {
        // Check to make sure they have the skill
        int skill_lvl = CHECK_SKILL(ch, skill);
        if (!skill_lvl)
            return 1;
        // Average the basic level bonus and the skill level
        return MIN(100, (level_bonus(ch, true) + skill_lvl) / 2);
    } else {
        int pclass = GET_CLASS(ch);
        int sclass = GET_REMORT_CLASS(ch);

        if (pclass < 0 || pclass >= NUM_CLASSES)
            pclass = CLASS_WARRIOR;

        int spell_lvl = SPELL_LEVEL(skill, pclass);
        int spell_gen = SPELL_GEN(skill, pclass);

        if (sclass >= NUM_CLASSES)
            sclass = CLASS_WARRIOR;

        if (spell_lvl <= ch->player.level && spell_gen <= GET_REMORT_GEN(ch)) {
            return level_bonus(ch, true);
        } else if (sclass >= 0
            && (SPELL_LEVEL(skill, sclass) <= ch->player.level)) {
            return level_bonus(ch, false);
        } else {
            return ch->player.level / 2;
        }
    }
}

/**
 *  attempts to set character's position.
 *
 *  @return success or failure
 *  @param new_position the enumerated int position to be set to.
**/
bool
creature_setPosition(struct creature * ch, int new_pos)
{
    if (new_pos == GET_POSITION(ch))
        return false;
    if (new_pos < BOTTOM_POS || new_pos > TOP_POS)
        return false;
    if (new_pos == POS_STANDING && is_fighting(ch)) {
        GET_POSITION(ch) = POS_FIGHTING;
    } else {
        GET_POSITION(ch) = new_pos;
    }
    return true;
}

// erase ch's memory
void
clear_memory(struct creature *ch)
{
    struct memory_rec *curr, *next;

    curr = MEMORY(ch);

    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }

    MEMORY(ch) = NULL;
}

void
dismount(struct creature *ch)
{
    if (!MOUNTED_BY(ch))
        return;

    ch->char_specials.mounted = NULL;
}

void
mount(struct creature *ch, struct creature *vict)
{
    if (MOUNTED_BY(ch))
        return;

    ch->char_specials.mounted = vict;
}

void
start_hunting(struct creature *ch, struct creature *vict)
{
    ch->char_specials.hunting = vict;
}

void
stop_hunting(struct creature *ch)
{
    ch->char_specials.hunting = NULL;
}

/**
 * Extract a ch completely from the world, and destroy his stuff
 * @param con_state the connection state to change the descriptor to, if one exists
**/
void
extract_creature(struct creature *ch, enum cxn_state con_state)
{
    ACMD(do_return);
    void die_follower(struct creature *ch);
    void verify_tempus_integrity(struct creature *);

    struct obj_data *obj;
    struct descriptor_data *t_desc;
    int idx;

    if (!IS_NPC(ch) && !ch->desc) {
        for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
            if (t_desc->original == ch)
                do_return(t_desc->creature, tmp_strdup(""), 0, SCMD_FORCED);
    }

    if (ch->desc && ch->desc->original) {
        do_return(ch->desc->creature, tmp_strdup(""), 0, SCMD_FORCED);
        return;
    }

    if (ch->in_room == NULL) {
        errlog("NOWHERE extracting char. (handler.c, extract_char)");
        slog("...extract char = %s", GET_NAME(ch));
        raise(SIGSEGV);
    }

    if (ch->followers || ch->master)
        die_follower(ch);
    // remove fighters, defenders, hunters and mounters
    GList *cit;
    for (cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *tch = cit->data;
        if (ch == DEFENDING(tch))
            stop_defending(tch);
        if (ch == MOUNTED_BY(tch)) {
            dismount(tch);
            if (GET_POSITION(tch) == POS_MOUNTED) {
                if (room_is_open_air(tch->in_room))
                    GET_POSITION(tch) = POS_FLYING;
                else
                    GET_POSITION(tch) = POS_STANDING;
            }
        }
        if (ch == NPC_HUNTING(tch))
            stop_hunting(tch);
        if (g_list_find(tch->fighting, ch))
            remove_combat(tch, ch);
    }

    destroy_attached_progs(ch);
    char_arrest_pardoned(ch);
    remove_all_combat(ch);

    if (MOUNTED_BY(ch)) {
        REMOVE_BIT(AFF2_FLAGS(MOUNTED_BY(ch)), AFF2_MOUNTED);
        dismount(ch);
    }
    // Make sure they aren't editing a help topic.
    if (GET_OLC_HELP(ch)) {
        GET_OLC_HELP(ch)->editor = NULL;
        GET_OLC_HELP(ch) = NULL;
    }
    // Forget snooping, if applicable
    if (ch->desc) {
        if (ch->desc->snooping) {
            ch->desc->snooping->snoop_by =
                g_list_remove(ch->desc->snooping->snoop_by, ch->desc);
            ch->desc->snooping = NULL;
        }
        if (ch->desc->snoop_by) {
            for (GList * it = ch->desc->snoop_by; it; it = it->next) {
                struct descriptor_data *d = it->data;
                d_printf(d, "Your victim is no longer among us.\r\n");
                d->snooping = NULL;
            }
            g_list_free(ch->desc->snoop_by);
        }
    }
    // destroy all that equipment
    for (idx = 0; idx < NUM_WEARS; idx++) {
        if (GET_EQ(ch, idx))
            extract_obj(raw_unequip_char(ch, idx, EQUIP_WORN));
        if (GET_IMPLANT(ch, idx))
            extract_obj(raw_unequip_char(ch, idx, EQUIP_IMPLANT));
        if (GET_TATTOO(ch, idx))
            extract_obj(raw_unequip_char(ch, idx, EQUIP_TATTOO));
    }

    // transfer inventory to room, if any
    while (ch->carrying) {
        obj = ch->carrying;
        obj_from_char(obj);
        extract_obj(obj);
    }

    if (ch->desc && ch->desc->original)
        do_return(ch, tmp_strdup(""), 0, SCMD_NOEXTRACT);

    char_from_room(ch, false);

    // remove any paths
    path_remove_object(ch);

    if (IS_NPC(ch)) {
        if (GET_NPC_VNUM(ch) > -1)  // if mobile
            ch->mob_specials.shared->number--;
        clear_memory(ch);       // Only NPC's can have memory
        free_creature(ch);
        return;
    }

    if (ch->desc) {             // PC's have descriptors. Take care of them
        set_desc_state(con_state, ch->desc);
    } else {                    // if a player gets purged from within the game
        free_creature(ch);
    }
}

// Retrieves the characters appropriate loadroom.
struct room_data *
player_loadroom(struct creature *ch)
{
    struct room_data *load_room = NULL;

    if (PLR_FLAGGED(ch, PLR_FROZEN)) {
        load_room = r_frozen_start_room;
    } else if (GET_LOADROOM(ch)) {
        if ((load_room = real_room(GET_LOADROOM(ch))) &&
            (!can_enter_house(ch, load_room->number) ||
                !clan_house_can_enter(ch, load_room))) {
            load_room = NULL;
        }
    } else if (GET_HOMEROOM(ch)) {
        if ((load_room = real_room(GET_HOMEROOM(ch))) &&
            (!can_enter_house(ch, load_room->number) ||
                !clan_house_can_enter(ch, load_room))) {
            load_room = NULL;
        }

    }

    if (load_room != NULL)
        return load_room;

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        load_room = r_immort_start_room;
    } else {
        if (GET_HOME(ch) == HOME_NEWBIE_SCHOOL) {
            if (GET_LEVEL(ch) > 5) {
                population_record[HOME_NEWBIE_SCHOOL]--;
                GET_HOME(ch) = HOME_MODRIAN;
                population_record[HOME_MODRIAN]--;
                load_room = r_mortal_start_room;
            } else {
                load_room = r_newbie_school_start_room;
            }
        } else if (GET_HOME(ch) == HOME_ELECTRO) {
            load_room = r_electro_start_room;
        } else if (GET_HOME(ch) == HOME_NEWBIE_TOWER) {
            if (GET_LEVEL(ch) > 5) {
                population_record[HOME_NEWBIE_TOWER]--;
                GET_HOME(ch) = HOME_MODRIAN;
                population_record[HOME_MODRIAN]--;
                load_room = r_mortal_start_room;
            } else
                load_room = r_tower_modrian_start_room;
        } else if (GET_HOME(ch) == HOME_NEW_THALOS) {
            load_room = r_new_thalos_start_room;
        } else if (GET_HOME(ch) == HOME_KROMGUARD) {
            load_room = r_kromguard_start_room;
        } else if (GET_HOME(ch) == HOME_ELVEN_VILLAGE) {
            load_room = r_elven_start_room;
        } else if (GET_HOME(ch) == HOME_ISTAN) {
            load_room = r_istan_start_room;
        } else if (GET_HOME(ch) == HOME_ARENA) {
            load_room = r_arena_start_room;
        } else if (GET_HOME(ch) == HOME_MONK) {
            load_room = r_monk_start_room;
        } else if (GET_HOME(ch) == HOME_SKULLPORT_NEWBIE) {
            load_room = r_skullport_newbie_start_room;
        } else if (GET_HOME(ch) == HOME_SOLACE_COVE) {
            load_room = r_solace_start_room;
        } else if (GET_HOME(ch) == HOME_MAVERNAL) {
            load_room = r_mavernal_start_room;
        } else if (GET_HOME(ch) == HOME_DWARVEN_CAVERNS) {
            load_room = r_dwarven_caverns_start_room;
        } else if (GET_HOME(ch) == HOME_HUMAN_SQUARE) {
            load_room = r_human_square_start_room;
        } else if (GET_HOME(ch) == HOME_SKULLPORT) {
            load_room = r_skullport_start_room;
        } else if (GET_HOME(ch) == HOME_DROW_ISLE) {
            load_room = r_drow_isle_start_room;
        } else if (GET_HOME(ch) == HOME_ASTRAL_MANSE) {
            load_room = r_astral_manse_start_room;
            // zul dane
        } else if (GET_HOME(ch) == HOME_ZUL_DANE) {
            // newbie start room for zul dane
            if (GET_LEVEL(ch) > 5)
                load_room = r_zul_dane_newbie_start_room;
            else
                load_room = r_zul_dane_start_room;
        } else {
            load_room = r_mortal_start_room;
        }
    }
    return load_room;
}

void
restore_creature(struct creature *ch)
{
    int i;

    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);

    if (GET_COND(ch, FULL) >= 0)
        GET_COND(ch, FULL) = 24;
    if (GET_COND(ch, THIRST) >= 0)
        GET_COND(ch, THIRST) = 24;

    if ((GET_LEVEL(ch) >= LVL_GRGOD)
        && (GET_LEVEL(ch) >= LVL_AMBASSADOR)) {
        for (i = 1; i <= MAX_SKILLS; i++)
            SET_SKILL(ch, i, 100);
        for (i = 0; i < MAX_TONGUES; i++)
            SET_TONGUE(ch, i, 100);
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            ch->real_abils.intel = 25;
            ch->real_abils.wis = 25;
            ch->real_abils.dex = 25;
            ch->real_abils.str = 25;
            ch->real_abils.con = 25;
            ch->real_abils.cha = 25;
        }
        ch->aff_abils = ch->real_abils;
    }
    update_pos(ch);
}

bool
creature_rent(struct creature *ch)
{
    remove_all_combat(ch);
    ch->player_specials->rentcode = RENT_RENTED;
    ch->player_specials->rent_per_day =
        (GET_LEVEL(ch) < LVL_IMMORT) ? calc_daily_rent(ch, 1, NULL, false) : 0;
    ch->player_specials->desc_mode = CXN_UNKNOWN;
    ch->player_specials->rent_currency = ch->in_room->zone->time_frame;
    GET_LOADROOM(ch) = ch->in_room->number;
    ch->player.time.logon = time(NULL);
    save_player_objects(ch);
    save_player_to_xml(ch);
    if (GET_LEVEL(ch) < 50)
        mlog(ROLE_ADMINBASIC, MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)),
            NRM, true,
            "%s has rented (%d/day, %'" PRId64 " %s)", GET_NAME(ch),
            ch->player_specials->rent_per_day, CASH_MONEY(ch) + BANK_MONEY(ch),
            (ch->player_specials->rent_currency ==
                TIME_ELECTRO) ? "gold" : "creds");
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    if (ch->desc) {
        ch->desc->creature = NULL;
        set_desc_state(CXN_MENU, ch->desc);
        ch->desc = NULL;
    }

    return true;
}

bool
creature_cryo(struct creature * ch)
{
    ch->player_specials->rentcode = RENT_CRYO;
    ch->player_specials->rent_per_day = 0;
    ch->player_specials->desc_mode = CXN_UNKNOWN;
    ch->player_specials->rent_currency = ch->in_room->zone->time_frame;
    GET_LOADROOM(ch) = ch->in_room->number;
    ch->player.time.logon = time(NULL);
    save_player_objects(ch);
    save_player_to_xml(ch);

    mlog(ROLE_ADMINBASIC, MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)),
        NRM, true, "%s has cryo-rented", GET_NAME(ch));
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    if (ch->desc) {
        ch->desc->creature = NULL;
        set_desc_state(CXN_MENU, ch->desc);
        ch->desc = NULL;
    }

    return true;
}

bool
creature_quit(struct creature * ch)
{
    struct obj_data *obj, *next_obj, *next_contained_obj;
    int pos;

    if (IS_NPC(ch))
        return false;

    for (pos = 0; pos < NUM_WEARS; pos++) {
        // Drop all non-cursed equipment worn
        if (GET_EQ(ch, pos)) {
            obj = GET_EQ(ch, pos);
            if (IS_OBJ_STAT(obj, ITEM_NODROP) ||
                IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) {
                for (obj = obj->contains; obj; obj = next_contained_obj) {
                    next_contained_obj = obj->next_content;
                    obj_from_obj(obj);
                    obj_to_room(obj, ch->in_room);
                }
            } else {
                obj = unequip_char(ch, pos, EQUIP_WORN);
                obj_to_room(obj, ch->in_room);
            }
        }
        // Drop all implanted items, breaking them
        if (GET_IMPLANT(ch, pos)) {
            obj = unequip_char(ch, pos, EQUIP_IMPLANT);
            GET_OBJ_DAM(obj) = GET_OBJ_MAX_DAM(obj) / 8 - 1;
            SET_BIT(GET_OBJ_EXTRA2(obj), ITEM2_BROKEN);
            obj_to_room(obj, ch->in_room);
            act("$p drops to the ground!", false, NULL, obj, NULL, TO_ROOM);
        }
    }

    // Drop all uncursed inventory items
    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)) {
            for (obj = obj->contains; obj; obj = next_contained_obj) {
                next_contained_obj = obj->next_content;
                obj_from_obj(obj);
                obj_to_room(obj, ch->in_room);
            }
        } else {
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
        }
    }

    ch->player_specials->rentcode = RENT_QUIT;
    ch->player_specials->rent_per_day = calc_daily_rent(ch, 3, NULL, false);
    ch->player_specials->desc_mode = CXN_UNKNOWN;
    ch->player_specials->rent_currency = ch->in_room->zone->time_frame;
    GET_LOADROOM(ch) = 0;
    ch->player.time.logon = time(NULL);
    save_player_objects(ch);
    save_player_to_xml(ch);
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    if (ch->desc) {
        ch->desc->creature = NULL;
        set_desc_state(CXN_MENU, ch->desc);
        ch->desc = NULL;
    }

    return true;
}

bool
creature_idle(struct creature * ch)
{
    if (IS_NPC(ch))
        return false;
    ch->player_specials->rentcode = RENT_FORCED;
    ch->player_specials->rent_per_day = calc_daily_rent(ch, 3, NULL, false);
    ch->player_specials->desc_mode = CXN_UNKNOWN;
    ch->player_specials->rent_currency = ch->in_room->zone->time_frame;
    GET_LOADROOM(ch) = 0;
    ch->player.time.logon = time(NULL);
    save_player_objects(ch);
    save_player_to_xml(ch);

    mlog(ROLE_ADMINBASIC, LVL_GOD, CMP, true,
        "%s force-rented and extracted (idle).", GET_NAME(ch));

    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    if (ch->desc) {
        ch->desc->creature = NULL;
        set_desc_state(CXN_MENU, ch->desc);
        ch->desc = NULL;
    }
    return true;
}

bool
creature_die(struct creature * ch)
{
    struct obj_data *obj, *next_obj;
    int pos;

    remove_all_combat(ch);

    // If their stuff hasn't been moved out, they dt'd, so we need to dump
    // their stuff to the room
    for (pos = 0; pos < NUM_WEARS; pos++) {
        if (GET_EQ(ch, pos)) {
            obj = unequip_char(ch, pos, EQUIP_WORN);
            obj_to_room(obj, ch->in_room);
        }
        if (GET_IMPLANT(ch, pos)) {
            obj = unequip_char(ch, pos, EQUIP_IMPLANT);
            obj_to_room(obj, ch->in_room);
        }
        if (GET_TATTOO(ch, pos)) {
            obj = unequip_char(ch, pos, EQUIP_TATTOO);
            extract_obj(obj);
        }
    }
    for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
    }

    if (!IS_NPC(ch)) {
        ch->player_specials->rentcode = RENT_QUIT;
        ch->player_specials->rent_per_day = 0;
        ch->player_specials->desc_mode = CXN_AFTERLIFE;
        ch->player_specials->rent_currency = 0;
        GET_LOADROOM(ch) = ch->in_room->zone->respawn_pt;
        ch->player.time.logon = time(NULL);
        save_player_objects(ch);

        if (PLR_FLAGGED(ch, PLR_HARDCORE))
            PLR2_FLAGS(ch) |= PLR2_BURIED;

        save_player_to_xml(ch);
    }
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);

    return true;
}

bool
creature_npk_die(struct creature * ch)
{
    remove_all_combat(ch);

    if (!IS_NPC(ch)) {
        ch->player_specials->rentcode = RENT_QUIT;
        ch->player_specials->rent_per_day = 0;
        ch->player_specials->desc_mode = CXN_AFTERLIFE;
        ch->player_specials->rent_currency = 0;
        GET_LOADROOM(ch) = ch->in_room->zone->respawn_pt;
        ch->player.time.logon = time(NULL);
        save_player_objects(ch);
        save_player_to_xml(ch);
    }
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);

    return true;
}

bool
creature_arena_die(struct creature * ch)
{
    // Remove any combat ch character might have been involved in
    // And make sure all defending creatures stop defending
    remove_all_combat(ch);

    // Rent them out
    if (!IS_NPC(ch)) {
        ch->player_specials->rentcode = RENT_RENTED;
        ch->player_specials->rent_per_day =
            (GET_LEVEL(ch) < LVL_IMMORT) ? calc_daily_rent(ch, 1, NULL,
            false) : 0;
        ch->player_specials->desc_mode = CXN_UNKNOWN;
        ch->player_specials->rent_currency = ch->in_room->zone->time_frame;
        GET_LOADROOM(ch) = ch->in_room->zone->respawn_pt;
        ch->player.time.logon = time(NULL);
        save_player_objects(ch);
        save_player_to_xml(ch);
        if (GET_LEVEL(ch) < 50)
            mudlog(MAX(LVL_AMBASSADOR, GET_INVIS_LVL(ch)), NRM, true,
                "%s has died in arena (%d/day, %'" PRId64 " %s)", GET_NAME(ch),
                ch->player_specials->rent_per_day,
                CASH_MONEY(ch) + BANK_MONEY(ch),
                (ch->player_specials->rent_currency ==
                    TIME_ELECTRO) ? "creds" : "gold");
    }
    // But extract them to afterlife
    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    return true;
}

bool
creature_purge(struct creature * ch, bool destroy_obj)
{
    struct obj_data *obj, *next_obj;

    if (!destroy_obj) {
        int pos;

        for (pos = 0; pos < NUM_WEARS; pos++) {
            if (GET_EQ(ch, pos)) {
                obj = unequip_char(ch, pos, EQUIP_WORN);
                obj_to_room(obj, ch->in_room);
            }
            if (GET_IMPLANT(ch, pos)) {
                obj = unequip_char(ch, pos, EQUIP_IMPLANT);
                obj_to_room(obj, ch->in_room);
            }
        }

        for (obj = ch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
        }
    }

    if (!IS_NPC(ch)) {
        ch->player_specials->rentcode = RENT_QUIT;
        ch->player_specials->rent_per_day = 0;
        ch->player_specials->desc_mode = CXN_UNKNOWN;
        ch->player_specials->rent_currency = 0;
        GET_LOADROOM(ch) = 0;
        ch->player.time.logon = time(NULL);
        save_player_objects(ch);
        save_player_to_xml(ch);
    }

    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    if (ch->desc)
        close_socket(ch->desc);
    return true;
}

bool
creature_remort(struct creature * ch)
{
    if (IS_NPC(ch))
        return false;
    ch->player_specials->rentcode = RENT_QUIT;
    ch->player_specials->rent_per_day = 0;
    ch->player_specials->desc_mode = CXN_UNKNOWN;
    ch->player_specials->rent_currency = 0;
    GET_LOADROOM(ch) = 0;
    ch->player.time.logon = time(NULL);
    save_player_objects(ch);
    save_player_to_xml(ch);

    GET_POSITION(ch) = POS_DEAD;
    destroy_attached_progs(ch);
    extract_creature(ch, CXN_REMORT_AFTERLIFE);
    return true;
}

bool
creature_trusts_idnum(struct creature * ch, long idnum)
{
    if (IS_NPC(ch))
        return false;

    return is_trusted(ch->account, idnum);
}

bool
creature_distrusts_idnum(struct creature * ch, long idnum)
{
    return !creature_trusts_idnum(ch, idnum);
}

bool
creature_trusts(struct creature * ch, struct creature * target)
{
    if (IS_NPC(ch))
        return false;

    if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master == target)
        return true;

    return creature_trusts_idnum(ch, GET_IDNUM(target));
}

bool
creature_distrusts(struct creature * ch, struct creature * target)
{
    return !creature_trusts(ch, target);
}

int
reputation_of(struct creature *ch)
{
    struct account *acct;

    if (IS_NPC(ch))
        return 0;

    acct = ch->account;
    if (!acct)
        acct = account_by_idnum(player_account_by_idnum(GET_IDNUM(ch)));
    if (acct && GET_LEVEL(ch) < LVL_AMBASSADOR)
        return MAX(0, MIN(1000,
                (ch->player_specials->saved.reputation * 95 / 100)
                + (acct->reputation * 5 / 100)));
    return ch->player_specials->saved.reputation;
}

void
gain_reputation(struct creature *ch, int amt)
{
    struct account *acct;

    if (IS_NPC(ch))
        return;

    acct = ch->account;
    if (!acct)
        acct = account_by_idnum(player_account_by_idnum(GET_IDNUM(ch)));
    if (acct && GET_LEVEL(ch) < LVL_AMBASSADOR)
        account_gain_reputation(acct, amt);

    ch->player_specials->saved.reputation += amt;
    if (ch->player_specials->saved.reputation < 0)
        ch->player_specials->saved.reputation = 0;
}

void
creature_set_reputation(struct creature *ch, int amt)
{
    ch->player_specials->saved.reputation = MIN(1000, MAX(0, amt));
}

bool
ok_to_attack(struct creature *ch, struct creature *vict, bool mssg)
{
    extern int get_hunted_id(int hunter_id);

    if (!vict) {
        errlog("ERROR:  NULL victim passed to ok_to_attack()");
        return false;
    }
    // Immortals over level LVL_GOD can always attack
    // anyone they want
    if (GET_LEVEL(ch) >= LVL_GOD) {
        return true;
    }
    // If they're already fighting, let them have at it!
    if (g_list_find(ch->fighting, vict) || g_list_find(vict->fighting, ch)) {
        return true;
    }
    // Charmed players can't attack their master
    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) {
        if (mssg)
            act("$N is just such a good friend, you simply can't hurt $M.",
                false, ch, NULL, vict, TO_CHAR);
        return false;
    }
    // If we have a bounty situation, we ignore NVZs and !PKs
    if (IS_PC(ch) && IS_PC(vict) &&
        get_hunted_id(GET_IDNUM(ch)) == GET_IDNUM(vict)) {
        return true;
    }
    // Now if we're in an arena room anbody can attack anybody
    if (is_arena_combat(ch, vict))
        return true;

    // If anyone is in an NVZ, no attacks are allowed
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        if (mssg) {
            send_to_char(ch, "The universal forces of order "
                "prevent violence here!\r\n");
            if (!number(0, 1))
                act("$n seems to be violently disturbed.", false,
                    ch, NULL, NULL, TO_ROOM);
            else
                act("$n becomes violently agitated for a moment.",
                    false, ch, NULL, NULL, TO_ROOM);
        }
        return false;
    }

    if (ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)) {
        if (mssg) {
            send_to_char(ch, "The universal forces of order "
                "prevent violence there!\r\n");
            if (!number(0, 1))
                act("$n seems to be violently disturbed.", false,
                    ch, NULL, NULL, TO_ROOM);
            else
                act("$n becomes violently agitated for a moment.",
                    false, ch, NULL, NULL, TO_ROOM);
        }
        return false;
    }
    // Disallow attacking members of your own group
    if (IS_PC(ch)
        && AFF_FLAGGED(ch, AFF_GROUP)
        && AFF_FLAGGED(vict, AFF_GROUP)
        && ch->master
        && vict->master
        && (ch->master == vict
            || vict->master == ch || ch->master == vict->master)) {
        send_to_char(ch, "You can't attack a member of your group!\r\n");
        return false;
    }
    // If either creature is a mob and we're not in an NVZ
    // It's always ok
    if (IS_NPC(vict) || IS_NPC(ch))
        return true;

    // At this point, we have to be dealing with PVP
    // Start checking killer prefs and zone restrictions
    if (!PRF2_FLAGGED(ch, PRF2_PKILLER)) {
        if (mssg) {
            send_to_char(ch, "A small dark shape flies in from the future "
                "and sticks to your tongue.\r\n");
        }
        return false;
    }
    // If a newbie is trying to attack someone, don't let it happen
    if (is_newbie(ch)) {
        if (mssg) {
            send_to_char(ch, "You are currently under new player "
                "protection, which expires at level 41\r\n");
            send_to_char(ch, "You cannot attack other players "
                "while under this protection.\r\n");
        }
        return false;
    }
    // If someone is trying to attack a newbie, also don't let it
    // happen
    if (is_newbie(vict)) {
        if (mssg) {
            act("$N is currently under new character protection.",
                false, ch, NULL, vict, TO_CHAR);
            act("You are protected by the gods against $n's attack!",
                false, ch, NULL, vict, TO_VICT);
            slog("%s protected against %s (ok_to_attack) at %d",
                GET_NAME(vict), GET_NAME(ch), vict->in_room->number);
        }
        return false;
    }
    // If they aren't in the same quest it's not ok to attack them
    if (GET_QUEST(ch) && GET_QUEST(ch) != GET_QUEST(vict)) {
        if (mssg)
            send_to_char(ch,
                "%s is not in your quest and may not be attacked!\r\n",
                PERS(vict, ch));
        qlog(ch,
            tmp_sprintf("%s has attacked non-questing PC %s",
                GET_NAME(ch), GET_NAME(vict)),
            QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR), true);

        return false;
    }

    if (GET_QUEST(vict) && GET_QUEST(vict) != GET_QUEST(ch)) {
        if (mssg)
            send_to_char(ch,
                "%s is on a godly quest and may not be attacked!\r\n",
                PERS(vict, ch));

        qlog(ch,
            tmp_sprintf("%s has attacked questing PC %s",
                GET_NAME(ch), GET_NAME(vict)),
            QLOG_BRIEF, MAX(GET_INVIS_LVL(ch), LVL_AMBASSADOR), true);

        return false;
    }
    // We're not in an NVZ, or an arena, and nobody is a newbie, so
    // check to see if we're in a !PK zone
    if (ch->in_room->zone->pk_style == ZONE_NO_PK ||
        vict->in_room->zone->pk_style == ZONE_NO_PK) {
        if (mssg) {
            send_to_char(ch, "You seem to be unable to bring "
                "your weapon to bear on %s.\r\n", GET_NAME(vict));
            act("$n shakes with rage as $e tries to bring $s "
                "weapon to bear.", false, ch, NULL, NULL, TO_ROOM);

        }
        return false;
    }

    if (reputation_of(ch) <= 0) {
        send_to_char(ch, "Your reputation is 0.  If you want to be "
            "a player killer, type PK on yes.\r\n");
        send_to_char(vict, "%s has just tried to attack you but was "
            "prevented by %s reputation being 0.\r\n", GET_NAME(ch), HSHR(ch));
        return false;
    }

    if (reputation_of(vict) <= 0) {
        send_to_char(ch, "%s's reputation is 0 and %s is immune to player "
            "versus player violence.\r\n", GET_NAME(vict), HSSH(vict));
        send_to_char(vict, "%s has just tried to attack you but was "
            "prevented by your reputation being 0.\r\n", GET_NAME(ch));
        return false;
    }
    return true;
}

void
add_combat(struct creature *attacker, struct creature *target, bool initiated __attribute__((unused)))
{
    if (attacker == target || attacker->in_room != target->in_room)
        return;

    if (!ok_to_attack(attacker, target, true))
        return;

    if (g_list_find(attacker->fighting, target))
        return;

    for (GList * it = first_living(attacker->in_room->people); it; it = next_living(it)) {
        struct creature *defender = it->data;
        if (defender != attacker
            && defender != target
            && DEFENDING(defender) == target
            && !is_fighting(defender)
            && !g_list_find(attacker->fighting, defender)
            && GET_POSITION(defender) > POS_RESTING) {

            send_to_char(defender,
                         "You defend %s from %s's vicious attack!\r\n",
                         PERS(target, defender), PERS(attacker, defender));
            send_to_char(DEFENDING(defender),
                         "%s defends you from %s's vicious attack!\r\n",
                         PERS(defender, target), PERS(attacker, target));
            act("$n comes to $N's defense!", false, defender, NULL,
                target, TO_NOTVICT);

            if (DEFENDING(defender) == attacker)
                stop_defending(defender);

            attacker->fighting = g_list_remove(attacker->fighting, defender);
            attacker->fighting = g_list_append(attacker->fighting, defender);

            update_pos(attacker);
            trigger_prog_fight(attacker, defender);

			//By not breaking here and not adding defenders to combatList we get
            //the desired effect of a) having new defenders kick in after the first dies
            //because they are on the attackers combat list, and b) still likely have
            //defenders left to defend from other potential attackers because they
            //won't actually begin combat until the attacker hits them.  This is the
            //theory at least.
        }
    }

    if (DEFENDING(target) == attacker)
        stop_defending(target);

    attacker->fighting = g_list_remove(attacker->fighting, target);
    attacker->fighting = g_list_append(attacker->fighting, target);
    trigger_prog_fight(attacker, target);
}

/*
   corrects position and removes combat related bits.
   Call ONLY from remove_combat()/remove_all_combat()
*/
static void
remove_fighting_affects(struct creature *ch)
{
    if (ch->in_room && room_is_open_air(ch->in_room)) {
        GET_POSITION(ch) = POS_FLYING;
    } else if (!IS_NPC(ch)) {
        if (GET_POSITION(ch) >= POS_FIGHTING)
            GET_POSITION(ch) = POS_STANDING;
        else if (GET_POSITION(ch) >= POS_RESTING)
            GET_POSITION(ch) = POS_SITTING;
    } else {
        if (AFF_FLAGGED(ch, AFF_CHARM) && IS_UNDEAD(ch))
            GET_POSITION(ch) = POS_STANDING;
        else if (GET_POSITION(ch) > POS_SITTING)
            GET_POSITION(ch) = POS_STANDING;
    }

    update_pos(ch);

}

void
remove_combat(struct creature *ch, struct creature *target)
{
    if (!target)
        return;

    if (!ch->fighting)
        return;

    ch->fighting = g_list_remove(ch->fighting, target);
    if (!is_fighting(ch))
        remove_fighting_affects(ch);
}

void
remove_all_combat(struct creature *ch)
{
    if (!ch->in_room)
        return;

    for (GList *it = first_living(ch->in_room->people);it;it = next_living(it)) {
        remove_combat((struct creature *)it->data, ch);
    }

    if (ch->fighting) {
        g_list_free(ch->fighting);
        ch->fighting = NULL;
        remove_fighting_affects(ch);
    }
}

struct creature *
random_opponent(struct creature *ch)
{
    GList *first = first_living(ch->fighting);

    if (!first)
        return NULL;

    for (GList *it = first;it;it = next_living(it)) {
        if (!random_fractional_10())
            return it->data;
    }

    return first->data;
}

void
ignite_creature(struct creature *ch, struct creature *igniter)
{
    struct affected_type af;

    init_affect(&af);

    memset(&af, 0x0, sizeof(struct affected_type));
    af.type = SPELL_ABLAZE;
    af.duration = -1;
    af.bitvector = AFF2_ABLAZE;
    af.aff_index = 2;

    if (igniter)
        af.owner = GET_IDNUM(igniter);

    affect_to_char(ch, &af);
}

void
extinguish_creature(struct creature *ch)
{
    affect_from_char(ch, SPELL_ABLAZE);
}

void
stop_defending(struct creature *ch)
{
    if (!DEFENDING(ch))
        return;

    act("You stop defending $N.", true, ch, NULL, DEFENDING(ch), TO_CHAR);
    if (ch->in_room == DEFENDING(ch)->in_room) {
        act("$n stops defending you against attacks.",
            false, ch, NULL, DEFENDING(ch), TO_VICT);
        act("$n stops defending $N.", false, ch, NULL, DEFENDING(ch), TO_NOTVICT);
    }

    ch->char_specials.defending = NULL;
}

void
start_defending(struct creature *ch, struct creature *vict)
{
    if (DEFENDING(ch))
        stop_defending(ch);

    ch->char_specials.defending = vict;

    act("You start defending $N against attacks.", true, ch, NULL, vict, TO_CHAR);
    act("$n starts defending you against attacks.",
        false, ch, NULL, vict, TO_VICT);
    act("$n starts defending $N against attacks.",
        false, ch, NULL, vict, TO_NOTVICT);
}

int
creature_breath_threshold(struct creature *ch)
{
    return GET_LEVEL(ch) / 32 + 2;
}

int
max_creature_attr(struct creature *ch, int mode)
{
    if (IS_NPC(ch) || IS_IMMORT(ch))
        return 50;

    struct race *race = race_by_idnum(GET_RACE(ch));
    int max_stat = 18;

    if (IS_REMORT(ch))
        max_stat += GET_REMORT_GEN(ch);

    if (race) {
        switch (mode) {
        case ATTR_STR:
            max_stat += race->str_mod;
            break;
        case ATTR_INT:
            max_stat += race->int_mod;
            break;
        case ATTR_WIS:
            max_stat += race->wis_mod;
            break;
        case ATTR_DEX:
            max_stat += race->dex_mod;
            break;
        case ATTR_CON:
            max_stat += race->con_mod;
            break;
        case ATTR_CHA:
            max_stat += race->cha_mod;
            break;
        }
    }

    return MIN(max_stat, 50);
}

int
strength_damage_bonus(int str)
{
    return (str * str) / 47 - str / 10 - 4;
}

int
strength_hit_bonus(int str)
{
    return str / 3 - 5;
}

float
strength_carry_weight(int str)
{
    float fstr = str;
    return fstr * fstr;
}

float
strength_wield_weight(int str)
{
    float fstr = str;
    return fstr * 12 / 10;
}

int
dexterity_defense_bonus(int dex)
{
    return 7 - dex / 2;
}

int
dexterity_hit_bonus(int dex)
{
    return dex / 2 - 5;
}

int
dexterity_damage_bonus(int dex)
{
    return dex - 5;
}

int
constitution_hitpoint_bonus(int con)
{
    return con * 3 / 4 - 4;
}


int
constitution_shock_bonus(int con)
{
    return con * 2 + 20;
}

int
wisdom_mana_bonus(int wis)
{
    return MAX(0, wis - 10);
}
