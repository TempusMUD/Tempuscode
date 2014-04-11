/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.informative.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

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
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "players.h"
#include "tmpstr.h"
#include "accstr.h"
#include "account.h"
#include "spells.h"
#include "vehicle.h"
#include "materials.h"
#include "bomb.h"
#include "fight.h"
#include "obj_data.h"
#include "specs.h"
#include "strutil.h"
#include "actions.h"
#include "language.h"
#include "weather.h"
#include "smokes.h"

/* extern variables */
extern int mini_mud;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;

extern struct obj_data *object_list;
//extern const struct command_info cmd_info[];
extern struct zone_data *zone_table;

extern const int8_t eq_pos_order[];
extern const int exp_scale[];

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *ansi_motd;
extern char *imotd;
extern char *ansi_imotd;
extern char *wizlist;
extern char *ansi_wizlist;
extern char *immlist;
extern char *ansi_immlist;
extern char *policies;
extern char *handbook;
extern char *areas_low;
extern char *areas_mid;
extern char *areas_high;
extern char *areas_remort;
extern const char *dirs[];
extern const char *where[];
extern const char *color_liquid[];
extern const char *color_chemical[];
extern const char *fullness[];
extern const char *char_class_abbrevs[];
extern const char *level_abbrevs[];
extern const char *class_names[];
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *from_dirs[];
extern const char *material_names[];
extern const int rev_dir[];
extern const char *wear_implantpos[];
extern const char *moon_sky_types[];
extern const char *soilage_bits[];
extern const char *wear_description[];
extern const char *logtypes[];
extern const struct weap_spec_info weap_spec_char_class[];

int isbanned(char *hostname, char *blocking_hostname);

ACMD(do_stand);

#define HID_OBJ_PROB(ch, obj) \
(GET_LEVEL(ch) + GET_INT(ch) +                    \
 (affected_by_spell(ch, SKILL_HYPERSCAN) ? 40 : 0)+\
 (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 30 : 0) +\
 (AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) ? 60 : 0) +  \
 (PRF_FLAGGED(ch, PRF_HOLYLIGHT) ? 500 : 0) +     \
 (IS_OBJ_STAT(obj, ITEM_GLOW) ? -20 : 0) +        \
 (IS_OBJ_STAT(obj, ITEM_HUM) ? -20 : 0) +         \
 ((IS_CIGARETTE(obj) && SMOKE_LIT(obj)) ? 10 : 0)+\
 ((IS_OBJ_STAT(obj, ITEM_MAGIC) &&                \
   AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) ? 20 : 0))

static void
show_obj_extra(struct obj_data *object, struct creature *ch)
{
    if (IS_OBJ_TYPE(object, ITEM_NOTE)) {
        if (object->action_desc)
            acc_strcat(object->action_desc, NULL);
        else
            act("It's blank.", false, ch, NULL, NULL, TO_CHAR);
        return;
    } else if (IS_OBJ_TYPE(object, ITEM_DRINKCON))
        acc_strcat("It looks like a drink container.", NULL);
    else if (IS_OBJ_TYPE(object, ITEM_FOUNTAIN))
        acc_strcat("It looks like a source of drink.", NULL);
    else if (IS_OBJ_TYPE(object, ITEM_FOOD))
        acc_strcat("It looks edible.", NULL);
    else if (IS_OBJ_TYPE(object, ITEM_HOLY_SYMB))
        acc_strcat("It looks like the symbol of some deity.", NULL);
    else if (IS_OBJ_TYPE(object, ITEM_CIGARETTE) ||
        IS_OBJ_TYPE(object, ITEM_PIPE)) {
        if (GET_OBJ_VAL(object, 3))
            acc_strcat("It appears to be lit and smoking.", NULL);
        else
            acc_strcat("It appears to be unlit.", NULL);
    } else if (IS_OBJ_TYPE(object, ITEM_CONTAINER)) {
        if (GET_OBJ_VAL(object, 3)) {
            acc_strcat("It looks like a corpse.\r\n", NULL);
        } else {
            acc_strcat("It looks like a container.\r\n", NULL);
            if (CAR_CLOSED(object) && CAR_OPENABLE(object)) /*macro maps to containers too */
                acc_strcat("It appears to be closed.\r\n", NULL);
            else if (CAR_OPENABLE(object)) {
                acc_strcat("It appears to be open.\r\n", NULL);
                if (object->contains)
                    acc_strcat("There appears to be something inside.\r\n",
                        NULL);
                else
                    acc_strcat("It appears to be empty.\r\n", NULL);
            }
        }
    } else if (IS_OBJ_TYPE(object, ITEM_SYRINGE)) {
        if (GET_OBJ_VAL(object, 0)) {
            acc_strcat("It is full.", NULL);
        } else {
            acc_strcat("It's empty.", NULL);
        }
    } else if (GET_OBJ_MATERIAL(object) > MAT_NONE &&
        GET_OBJ_MATERIAL(object) < TOP_MATERIAL) {
        acc_sprintf("It appears to be composed of %s.",
            material_names[GET_OBJ_MATERIAL(object)]);
    } else {
        acc_strcat("You see nothing special.", NULL);
    }
}

static void
show_obj_bits(struct obj_data *object, struct creature *ch)
{
    if (object->engraving)
        acc_sprintf(" \"%s\"", object->engraving);
    if (IS_OBJ_STAT2(object, ITEM2_BROKEN))
        acc_sprintf(" %s<broken>", CCNRM(ch, C_NRM));
    if (object->in_obj && IS_CORPSE(object->in_obj) && IS_IMPLANT(object)
        && !CAN_WEAR(object, ITEM_WEAR_TAKE))
        acc_strcat(" (implanted)", NULL);

    if (ch == object->carried_by || ch == object->worn_by) {
        if (OBJ_REINFORCED(object))
            acc_sprintf(" %s[%sreinforced%s]%s",
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
                CCNRM(ch, C_NRM));
        if (OBJ_ENHANCED(object))
            acc_sprintf(" %s|enhanced|%s", CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
    }

    if (IS_OBJ_TYPE(object, ITEM_DEVICE) && ENGINE_STATE(object))
        acc_strcat(" (active)", NULL);

    if (((IS_OBJ_TYPE(object, ITEM_CIGARETTE) ||
                IS_OBJ_TYPE(object, ITEM_PIPE)) &&
            GET_OBJ_VAL(object, 3)) ||
        (IS_BOMB(object) && object->contains && IS_FUSE(object->contains)
            && FUSE_STATE(object->contains)))
        acc_sprintf(" %s(lit)%s", CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
        acc_sprintf(" %s(invisible)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT(object, ITEM_TRANSPARENT))
        acc_sprintf(" %s(transparent)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT2(object, ITEM2_HIDDEN))
        acc_sprintf(" %s(hidden)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    if (AFF3_FLAGGED(ch, AFF3_DETECT_POISON)
        && (((IS_OBJ_TYPE(object, ITEM_FOOD)
                    || IS_OBJ_TYPE(object, ITEM_DRINKCON)
                    || IS_OBJ_TYPE(object, ITEM_FOUNTAIN))
                && GET_OBJ_VAL(object, 3))
            || obj_has_affect(object, SPELL_ENVENOM)))
        acc_sprintf(" %s(poisoned)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) ||
        (IS_CLERIC(ch) && AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))) {
        if (IS_OBJ_STAT(object, ITEM_BLESS))
            acc_sprintf(" %s(holy aura)%s",
                CCBLU_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
        else if (IS_OBJ_STAT(object, ITEM_DAMNED))
            acc_sprintf(" %s(unholy aura)%s",
                CCRED_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
    }
    if ((AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
            || AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        && IS_OBJ_STAT(object, ITEM_MAGIC))
        acc_sprintf(" %s(yellow aura)%s",
            CCYEL_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
    if ((AFF_FLAGGED(ch, AFF_DETECT_MAGIC)
            || AFF2_FLAGGED(ch, AFF2_TRUE_SEEING)
            || PRF_FLAGGED(ch, PRF_HOLYLIGHT))
        && GET_OBJ_SIGIL_IDNUM(object))
        acc_sprintf(" %s(%ssigil%s)%s",
            CCYEL(ch, C_NRM), CCMAG(ch, C_NRM), CCYEL(ch, C_NRM),
            CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT(object, ITEM_GLOW))
        acc_sprintf(" %s(glowing)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT(object, ITEM_HUM))
        acc_sprintf(" %s(humming)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT2(object, ITEM2_ABLAZE))
        acc_sprintf(" %s*burning*%s", CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
    if (IS_OBJ_STAT3(object, ITEM3_HUNTED))
        acc_sprintf(" %s(hunted)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    if (OBJ_SOILED(object, SOIL_BLOOD))
        acc_sprintf(" %s(bloody)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    if (OBJ_SOILED(object, SOIL_WATER))
        acc_sprintf(" %s(wet)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (OBJ_SOILED(object, SOIL_MUD))
        acc_sprintf(" %s(muddy)%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    if (OBJ_SOILED(object, SOIL_ACID))
        acc_sprintf(" %s(acid covered)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (object->shared->owner_id != 0)
        acc_sprintf(" %s(protected)%s",
            CCYEL_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
    if (object->tmp_affects != NULL) {
        if (obj_has_affect(object, SPELL_ITEM_REPULSION_FIELD) != NULL) {
            acc_sprintf(" %s(repulsive)%s",
                CCCYN(ch, C_SPR), CCNRM(ch, C_SPR));
        } else if (obj_has_affect(object, SPELL_ITEM_ATTRACTION_FIELD) != NULL) {
            acc_sprintf(" %s(attractive)%s",
                CCCYN(ch, C_SPR), CCNRM(ch, C_SPR));
        }
        if (obj_has_affect(object, SPELL_ELEMENTAL_BRAND) != NULL) {
            acc_sprintf(" %s(%sbranded%s)%s",
                CCRED(ch, C_SPR),
                CCGRN(ch, C_SPR), CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
        }
    }
    if ((GET_LEVEL(ch) >= LVL_IMMORT || is_tester(ch)) &&
        PRF2_FLAGGED(ch, PRF2_DISP_VNUMS)) {
        acc_sprintf(" %s<%s%d%s>%s",
            CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_OBJ_VNUM(object), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    }
}

static void
show_room_obj(struct obj_data *object, struct creature *ch, int count)
{
    if (object->line_desc)
        acc_strcat(object->line_desc, NULL);
    else if (IS_IMMORT(ch))
        acc_sprintf("%s exists here.", tmp_capitalize(object->name));
    else
        return;

    show_obj_bits(object, ch);
    acc_strcat(CCGRN(ch, C_NRM), NULL);

    if (count > 1)
        acc_sprintf(" [%d]", count);
    acc_strcat("\r\n", NULL);
}

static void
show_obj_to_char(struct obj_data *object, struct creature *ch,
    int mode, int count)
{
    if (mode == SHOW_OBJ_ROOM) {
        show_room_obj(object, ch, count);
        return;
    } else if ((mode == SHOW_OBJ_INV || mode == SHOW_OBJ_CONTENT)
        && object->name) {
        acc_strcat(object->name, NULL);
    } else if (mode == SHOW_OBJ_EXTRA)
        show_obj_extra(object, ch);

    if (mode != SHOW_OBJ_NOBITS)
        show_obj_bits(object, ch);

    if (count > 1)
        acc_sprintf(" [%d]", count);
    acc_strcat("\r\n", NULL);

    if (IS_OBJ_TYPE(object, ITEM_VEHICLE) && mode == SHOW_OBJ_BITS) {
        if (CAR_OPENABLE(object)) {
            if (CAR_CLOSED(object))
                act("The door of $p is closed.", true, ch, object, NULL, TO_CHAR);
            else
                act("The door of $p is open.", true, ch, object, NULL, TO_CHAR);
        }
    }
}

void
list_obj_to_char(struct obj_data *list, struct creature *ch, int mode,
    bool show)
{
    struct obj_data *i = NULL, *o = NULL;
    bool found = false, corpse = false;
    int count = 0;

    if (list && list->in_obj && IS_CORPSE(list->in_obj))
        corpse = true;

    for (i = list; i; i = o) {
        if (!can_see_object(ch, i) ||
            OBJ_IS_SOILAGE(i) ||
            (IS_OBJ_STAT2(i, ITEM2_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)
                && number(50, 120) > HID_OBJ_PROB(ch, i))) {
            o = i->next_content;
            continue;
        }

        if (corpse && IS_IMPLANT(i) && !CAN_WEAR(i, ITEM_WEAR_TAKE) &&
            ((CHECK_SKILL(ch, SKILL_CYBERSCAN) +
                    (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 50 : 0))
                < number(80, 150) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
            o = i->next_content;
            continue;
        }

        if ((i->shared->proto &&
                i->name != i->shared->proto->name) ||
            IS_OBJ_STAT2(i, ITEM2_BROKEN))
            o = i->next_content;
        else {
            count = 1;
            for (o = i->next_content; o; o = o->next_content) {
                if (same_obj(o, i) && (!IS_BOMB(o) || !IS_BOMB(i)
                        || same_obj(o->contains, i->contains))) {
                    if (can_see_object(ch, o))
                        count++;
                } else {
                    break;
                }
            }
        }
        show_obj_to_char(i, ch, mode, count);
        count = 0;
        found = true;
    }
    if (!found && show)
        acc_sprintf(" Nothing.\r\n");
}

static void
list_obj_to_char_GLANCE(struct obj_data *list, struct creature *ch,
    struct creature *vict, int mode, bool show, int glance)
{
    struct obj_data *i = NULL, *o = NULL;
    bool found = false;
    int count;

    for (i = list; i; i = o) {
        if (!can_see_object(ch, i) ||
            (IS_OBJ_STAT2(i, ITEM2_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)
                && number(50, 120) > HID_OBJ_PROB(ch, i))) {
            o = i->next_content;
            continue;
        }
        if ((GET_LEVEL(ch) < LVL_AMBASSADOR &&
                GET_LEVEL(ch) + (GET_SKILL(ch,
                        SKILL_GLANCE) * ((glance) ? 4 : 0)) < (number(0,
                        50) + CHECK_SKILL(vict, SKILL_CONCEAL)))
            || (GET_LEVEL(ch) >= LVL_AMBASSADOR
                && GET_LEVEL(vict) > GET_LEVEL(ch))) {
            o = i->next_content;
            continue;
        }

        count = 1;
        if (IS_CORPSE(i) ||
            (i->shared->proto && i->name != i->shared->proto->name) ||
            IS_OBJ_STAT2(i, ITEM2_BROKEN))
            o = i->next_content;
        else {
            for (o = i->next_content; o; o = o->next_content) {
                if (same_obj(o, i) && (!IS_BOMB(o) || !IS_BOMB(i)
                        || same_obj(o->contains, i->contains))) {
                    if (can_see_object(ch, o))
                        count++;
                } else
                    break;
            }
        }

        show_obj_to_char(i, ch, mode, count);
        found = true;
    }

    if (!found && show)
        acc_sprintf("You can't see anything.\r\n");
}

static void
diag_char_to_char(struct creature *i, struct creature *ch)
{
    int percent;

    if (GET_MAX_HIT(i) > 0)
        percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
    else
        percent = -1;           /* How could MAX_HIT be < 1?? */

    strcpy(buf, CCYEL(ch, C_NRM));

    strcpy(buf2, PERS(i, ch));
    CAP(buf2);

    strcat(buf, buf2);

    if (percent >= 100)
        strcat(buf, " is in excellent condition.\r\n");
    else if (percent >= 90)
        strcat(buf, " has a few scratches.\r\n");
    else if (percent >= 75)
        strcat(buf, " has some small wounds and bruises.\r\n");
    else if (percent >= 50)
        strcat(buf, " has quite a few wounds.\r\n");
    else if (percent >= 30)
        strcat(buf, " has some big nasty wounds and scratches.\r\n");
    else if (percent >= 15)
        strcat(buf, " looks pretty hurt.\r\n");
    else if (percent >= 5)
        strcat(buf, " is in awful condition.\r\n");
    else if (percent >= 0)
        strcat(buf, " is on the brink of death.\r\n");
    else
        strcat(buf, " is bleeding awfully from big wounds.\r\n");

    strcat(buf, CCNRM(ch, C_NRM));

    send_to_char(ch, "%s", buf);
}

char *
diag_conditions(struct creature *ch)
{

    if (!ch) {
        strcpy(buf, "(NULL)");
        return buf;
    }
    int percent;
    if (GET_MAX_HIT(ch) > 0)
        percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    else
        percent = -1;           /* How could MAX_HIT be < 1?? */

    if (percent >= 100)
        strcpy(buf, "excellent");
    else if (percent >= 90)
        strcpy(buf, "few scratches");
    else if (percent >= 75)
        strcpy(buf, "small wounds");
    else if (percent >= 50)
        strcpy(buf, "quite a few");
    else if (percent >= 30)
        strcpy(buf, "big nasty");
    else if (percent >= 15)
        strcpy(buf, "pretty hurt");
    else if (percent >= 5)
        strcpy(buf, "awful");
    else if (percent >= 0)
        strcpy(buf, "the brink of death");
    else
        strcpy(buf, "bleeding awfully");
    return (buf);
}

static void
desc_char_trailers(struct creature *ch, struct creature *i)
{
    if (affected_by_spell(i, SPELL_QUAD_DAMAGE))
        acc_strcat("...", HSSH(i),
            " is glowing with a bright blue light!\r\n", NULL);

    if (AFF2_FLAGGED(i, AFF2_ABLAZE))
        acc_strcat("...", HSHR(i),
            " body is blazing like a bonfire!\r\n", NULL);
    if (AFF_FLAGGED(i, AFF_BLIND))
        acc_strcat("...", HSSH(i), " is groping around blindly!\r\n", NULL);

    if (AFF_FLAGGED(i, AFF_SANCTUARY)) {
        if (IS_EVIL(i))
            acc_strcat("...", HSSH(i),
                " is surrounded by darkness!\r\n", NULL);
        else
            acc_strcat("...", HSSH(i),
                " glows with a bright light!\r\n", NULL);
    }

    if (AFF_FLAGGED(i, AFF_CONFUSION))
        acc_strcat("...", HSSH(i),
            " is looking around in confusion!\r\n", NULL);
    if (AFF3_FLAGGED(i, AFF3_SYMBOL_OF_PAIN))
        acc_strcat("...a symbol of pain burns bright on ",
            HSHR(i), " forehead!\r\n", NULL);
    if (AFF_FLAGGED(i, AFF_BLUR))
        acc_strcat("...", HSHR(i),
            " form appears to be blurry and shifting.\r\n", NULL);
    if (AFF2_FLAGGED(i, AFF2_FIRE_SHIELD))
        acc_strcat("...a blazing sheet of fire floats before ",
            HSHR(i), " body!\r\n", NULL);
    if (AFF2_FLAGGED(i, AFF2_BLADE_BARRIER))
        acc_strcat("...", HSSH(i),
            " is surrounded by whirling blades!\r\n", NULL);
    if (AFF2_FLAGGED(i, AFF2_ENERGY_FIELD))
        acc_strcat("...", HSSH(i),
            " is covered by a crackling field of energy!\r\n", NULL);

    if (IS_SOULLESS(i))
        acc_strcat("...a deep red pentagram has been burned into ",
            HSHR(i), " forehead!\r\n", NULL);
    else if (AFF3_FLAGGED(i, AFF3_TAINTED))
        acc_strcat("...the mark of the tainted has been burned into ",
            HSHR(i), " forehead!\r\n", NULL);

    if (AFF3_FLAGGED(i, AFF3_PRISMATIC_SPHERE))
        acc_strcat("...", HSSH(i),
            " is surrounded by a prismatic sphere of light!\r\n", NULL);

    if (affected_by_spell(i, SPELL_SKUNK_STENCH) ||
        affected_by_spell(i, SPELL_TROG_STENCH))
        acc_strcat("...", HSSH(i),
            " is followed by a malodorous stench...\r\n", NULL);

    if (affected_by_spell(i, SPELL_ENTANGLE)) {
        if (i->in_room->sector_type == SECT_CITY
            || i->in_room->sector_type == SECT_CRACKED_ROAD)
            acc_strcat("...", HSSH(i),
                " is hopelessly tangled in the weeds and sparse vegetation.\r\n",
                NULL);
        else
            acc_strcat("...", HSSH(i),
                " is hopelessly tangled in the undergrowth.\r\n", NULL);
    }

    if (AFF2_FLAGGED(i, AFF2_DISPLACEMENT) &&
        AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        acc_strcat("...the image of ", HSHR(i),
            " body is strangely displaced.\r\n", NULL);

    if (AFF_FLAGGED(i, AFF_INVISIBLE))
        acc_strcat("...", HSSH(i),
            " is invisible to the unaided eye.\r\n", NULL);

    if (AFF2_FLAGGED(i, AFF2_TRANSPARENT))
        acc_strcat("...", HSHR(i),
            " body is completely transparent.\r\n", NULL);

    if (affected_by_spell(i, SKILL_KATA) && skill_bonus(i, SKILL_KATA) >= 50)
        acc_strcat("...", HSHR(i), " hands are glowing eerily.\r\n", NULL);

    if (affected_by_spell(i, SPELL_GAUSS_SHIELD))
        acc_strcat("...", HSSH(i), " is protected by a swirling ",
            "shield of energy.\r\n", NULL);
    if (affected_by_spell(i, SPELL_THORN_SKIN))
        acc_strcat("...thorns protrude painfully from ",
            HSHR(i), " skin.\r\n", NULL);
    if (affected_by_spell(i, SONG_WOUNDING_WHISPERS))
        acc_strcat("...", HSSH(i),
            " is surrounded by whirling slivers of sound.\r\n", NULL);
    if (affected_by_spell(i, SONG_MIRROR_IMAGE_MELODY))
        acc_strcat("...", HSSH(i), " is surrounded by mirror images.\r\n",
            NULL);

    if (affected_by_spell(i, SPELL_DIMENSIONAL_SHIFT))
        acc_strcat("...", HSSH(i),
            " is shifted into a parallel dimension.\r\n", NULL);
}

static void
look_at_char(struct creature *i, struct creature *ch, int cmd)
{
    int j, found = 0, app_height, h, k, pos;
    float app_weight;
    char *description = NULL;
    struct affected_type *af = NULL;
    struct creature *mob = NULL;

    if ((af = affected_by_spell(i, SKILL_DISGUISE))) {
        if ((mob = real_mobile_proto(af->modifier)))
            description = mob->player.description;
    }
    if (!CMD_IS("glance")) {
        if (description)
            send_to_char(ch, "%s", description);
        else if (!mob && i->player.description)
            send_to_char(ch, "%s", i->player.description);
        else
            act("You see nothing special about $m.", false, i, NULL, ch, TO_VICT);

        app_height = GET_HEIGHT(i) - number(1, 6) + number(1, 6);
        app_weight = GET_WEIGHT(i) - number(1, 6) + number(1, 6);
        if (!IS_NPC(i) && !mob) {
            send_to_char(ch, "%s appears to be a %s tall, %s %s %s.\r\n",
                         i->player.name,
                         format_distance(app_height, USE_METRIC(ch)),
                         format_weight(app_weight, USE_METRIC(ch)),
                         genders[(int)GET_SEX(i)],
                         race_name_by_idnum(GET_RACE(i)));
        }
    }

    diag_char_to_char(i, ch);

    if (CMD_IS("look")) {
        acc_string_clear();
        desc_char_trailers(ch, i);
        send_to_char(ch, "%s", acc_get_string());
    }

    if (!CMD_IS("glance") && !IS_NPC(i)) {

        buf[0] = '\0';

        for (h = 0; h < NUM_WEARS; h++) {
            pos = (int)eq_pos_order[h];

            if (!GET_EQ(i, pos) && CHAR_SOILAGE(i, pos)) {
                if (ILLEGAL_SOILPOS(pos)) {
                    CHAR_SOILAGE(i, pos) = 0;
                    continue;
                }
                found = false;
                sprintf(buf2, "%s %s %s ", HSHR(i),
                    wear_description[pos],
                    pos == WEAR_FEET ? "are" : ISARE(wear_description[pos]));

                for (k = 0, j = 0; j < TOP_SOIL; j++)
                    if (CHAR_SOILED(i, pos, (1 << j)))
                        k++;

                for (j = 0; j < TOP_SOIL; j++) {
                    if (CHAR_SOILED(i, pos, (1 << j))) {
                        found++;
                        if (found > 1) {
                            strcat(buf2, ", ");
                            if (found == k)
                                strcat(buf2, "and ");
                        }
                        strcat(buf2, soilage_bits[j]);
                    }
                }
                strcat(buf, strcat(CAP(buf2), ".\r\n"));
            }
        }
        if (found)
            send_to_char(ch, "%s", buf);
    }

    if (CMD_IS("examine") || CMD_IS("glance")) {
        found = false;
        for (j = 0; !found && j < NUM_WEARS; j++)
            if ((GET_EQ(i, j) && can_see_object(ch, GET_EQ(i, j)))
                || GET_TATTOO(i, j))
                found = true;

        acc_string_clear();
        if (found) {
            acc_sprintf("\r\n%s is using:\r\n", tmp_capitalize(PERS(i, ch)));
            for (j = 0; j < NUM_WEARS; j++)
                if (GET_EQ(i, (int)eq_pos_order[j]) &&
                    can_see_object(ch, GET_EQ(i, (int)eq_pos_order[j])) &&
                    (!IS_OBJ_STAT2(GET_EQ(i, (int)eq_pos_order[j]),
                            ITEM2_HIDDEN)
                        || number(50, 120) < HID_OBJ_PROB(ch, GET_EQ(i,
                                (int)eq_pos_order[j])))) {
                    if (CMD_IS("glance")
                        && number(50, 100) > CHECK_SKILL(ch, SKILL_GLANCE))
                        continue;
                    acc_sprintf("%s%s%s",
                        CCGRN(ch, C_NRM),
                        where[(int)eq_pos_order[j]], CCNRM(ch, C_NRM));
                    show_obj_to_char(GET_EQ(i, (int)eq_pos_order[j]), ch,
                        SHOW_OBJ_INV, 0);
                } else if (GET_TATTOO(i, (int)eq_pos_order[j])) {
                    acc_sprintf("%s%s%s",
                        CCGRN(ch, C_NRM),
                        tattoo_pos_descs[(int)eq_pos_order[j]],
                        CCNRM(ch, C_NRM));
                    show_obj_to_char(GET_TATTOO(i, (int)eq_pos_order[j]), ch,
                        SHOW_OBJ_INV, 0);

                }
        }
        if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR)) {
            acc_sprintf("\r\nYou attempt to peek at %s inventory:\r\n",
                HSHR(i));
            list_obj_to_char_GLANCE(i->carrying, ch, i, SHOW_OBJ_INV, true,
                (GET_LEVEL(ch) >= LVL_AMBASSADOR));
        }
        page_string(ch->desc, acc_get_string());
    }
}

struct creature *random_opponent(struct creature *ch);

static const char *
desc_one_char(struct creature *ch, struct creature *i, bool is_group)
{
    const char *positions[] = {
        " is lying here, dead.",
        " is lying here, badly wounded.",
        " is lying here, incapacitated.",
        " is lying here, stunned.",
        " is sleeping here.",
        " is resting here.",
        " is sitting here.",
        "!FIGHTING!",
        " is standing here.",
        " is hovering here.",
        "!MOUNTED!",
        " is swimming here."
    };
    const char *align = "";
    const char *desc = "";
    const char *appr = "";
    const char *vnum = "";
    const char *poisoned = "";

    if (AFF2_FLAGGED(i, AFF2_MOUNTED))
        return "";

    if (!IS_NPC(ch) && NPC2_FLAGGED(i, NPC2_UNAPPROVED) &&
        !(PRF_FLAGGED(ch, PRF_HOLYLIGHT) || is_tester(ch)))
        return "";

    if (IS_NPC(i)) {
        desc = tmp_capitalize(i->player.short_descr);
    } else if (affected_by_spell(i, SKILL_DISGUISE)) {
        desc = tmp_capitalize(GET_DISGUISED_NAME(ch, i));
    } else
        desc = tmp_strcat(i->player.name, GET_TITLE(i), NULL);

    if (!IS_NPC(i)) {
        if (!i->desc)
            desc = tmp_sprintf("%s %s(linkless)%s", desc,
                CCMAG(ch, C_NRM), CCYEL(ch, C_NRM));
        if (PLR_FLAGGED(i, PLR_OLC))
            desc = tmp_sprintf("%s %s(creating)%s", desc,
                CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
        else if (PLR_FLAGGED(i, PLR_WRITING))
            desc = tmp_sprintf("%s %s(writing)%s", desc,
                CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
        if (PLR_FLAGGED(i, PLR_AFK)) {
            if (AFK_REASON(i))
                desc = tmp_sprintf("%s %s(afk: %s)%s", desc,
                    CCGRN(ch, C_NRM), AFK_REASON(i), CCYEL(ch, C_NRM));
            else
                desc = tmp_sprintf("%s %s(afk)%s", desc,
                    CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
        }
    }

    if (IS_NPC(i) && GET_POSITION(i) == GET_DEFAULT_POS(i)) {
        if (i->player.long_descr)
            desc = i->player.long_descr;
        else
            desc = tmp_strcat(tmp_capitalize(desc), " exists here.", NULL);
    } else if (GET_POSITION(i) == POS_FIGHTING) {
        if (!is_fighting(i))
            desc = tmp_sprintf("%s is here, fighting thin air!", desc);
        else if (random_opponent(i) == ch)
            desc = tmp_sprintf("%s is here, fighting YOU!", desc);
        else if (random_opponent(i)->in_room == i->in_room)
            desc = tmp_sprintf("%s is here, fighting %s!", desc,
                PERS(random_opponent(i), ch));
        else
            desc =
                tmp_sprintf("%s is here, fighting someone who already left!",
                desc);
    } else if (GET_POSITION(i) == POS_MOUNTED) {
        if (!MOUNTED_BY(i))
            desc = tmp_sprintf("%s is here, mounted on thin air!", desc);
        else if (MOUNTED_BY(i) == ch)
            desc =
                tmp_sprintf("%s is here, mounted on YOU.  Heh heh...", desc);
        else if (MOUNTED_BY(i)->in_room == i->in_room)
            desc = tmp_sprintf("%s is here, mounted on %s.", desc,
                PERS(MOUNTED_BY(i), ch));
        else
            desc =
                tmp_sprintf("%s is here, mounted on someone who already left!",
                desc);
    } else if (AFF2_FLAGGED(i, AFF2_MEDITATE)
        && GET_POSITION(i) == POS_SITTING)
        desc = tmp_strcat(desc, " is meditating here.", NULL);
    else if (AFF_FLAGGED(i, AFF_HIDE))
        desc = tmp_strcat(desc, " is hiding here.", NULL);
    else if (AFF3_FLAGGED(i, AFF3_STASIS)
        && GET_POSITION(i) == POS_SLEEPING)
        desc = tmp_strcat(desc, " is lying here in a static state.", NULL);
    else if ((SECT_TYPE(i->in_room) == SECT_WATER_NOSWIM ||
            SECT_TYPE(i->in_room) == SECT_WATER_SWIM ||
            SECT_TYPE(i->in_room) == SECT_FIRE_RIVER) &&
        (!AFF_FLAGGED(i, AFF_WATERWALK)
            || GET_POSITION(ch) < POS_STANDING))
        desc = tmp_strcat(desc, " is swimming here.", NULL);
    else if (room_is_underwater(i->in_room) && GET_POSITION(i) > POS_RESTING)
        desc = tmp_strcat(desc, " is swimming here.", NULL);
    else if (SECT_TYPE(i->in_room) == SECT_PITCH_PIT
        && GET_POSITION(i) < POS_FLYING)
        desc = tmp_strcat(desc, " struggles in the pitch.", NULL);
    else if (SECT_TYPE(i->in_room) == SECT_PITCH_SUB)
        desc = tmp_strcat(desc, " struggles blindly in the pitch.", NULL);
    else
        desc = tmp_strcat(desc, positions[(int)MAX(0, MIN(GET_POSITION(i),
                        POS_SWIMMING))], NULL);

    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
        const char *align_color;

        if (IS_EVIL(i))
            align_color = CCRED(ch, C_NRM);
        else if (IS_GOOD(i))
            align_color = CCBLU(ch, C_NRM);
        else
            align_color = CCNRM(ch, C_NRM);

        align = tmp_sprintf(" %s%s(%da)%s",
            align_color, CCBLD(ch, C_CMP), GET_ALIGNMENT(i), CCNRM(ch, C_CMP));
    } else if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN) ||
        (IS_CLERIC(ch) && AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))) {
        if (IS_EVIL(i))
            align = tmp_sprintf(" %s%s(Red Aura)%s",
                CCRED(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
        else if (IS_GOOD(i))
            align = tmp_sprintf(" %s%s(Blue Aura)%s",
                CCBLU(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
    }

    if (AFF3_FLAGGED(ch, AFF3_DETECT_POISON)) {
        if (HAS_POISON_1(i) || HAS_POISON_2(i) || HAS_POISON_3(i))
            poisoned = tmp_sprintf(" %s(poisoned)%s",
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    }

    if (NPC_FLAGGED(i, NPC_UTILITY))
        appr = tmp_sprintf(" %s<util>", CCCYN(ch, C_NRM));
    // If they can see it, they probably need to know it's unapproved
    if (NPC2_FLAGGED(i, NPC2_UNAPPROVED))
        appr = tmp_sprintf(" %s(!appr)", CCRED(ch, C_NRM));

    if ((IS_NPC(i) && (GET_LEVEL(ch) >= LVL_IMMORT || is_tester(ch))) &&
        PRF2_FLAGGED(ch, PRF2_DISP_VNUMS)) {
        vnum = tmp_sprintf(" %s%s<%s%d%s>%s",
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            GET_NPC_VNUM(i), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    }

    desc = tmp_strcat(CCYEL(ch, C_NRM), (is_group) ? CCBLD(ch, C_CMP) : "",
        desc, align, poisoned, appr, vnum, CCNRM(ch, C_NRM), "\r\n", NULL);

    return desc;
}

static void
list_char_to_char(GList * list, struct creature *ch)
{
    struct creature *i;
    bool is_group = false;
    const char *desc;
    int unseen = 0;
    int hide_prob, hide_roll;

    if (list == NULL)
        return;

    for (GList * it = first_living(list); it; it = next_living(it)) {
        i = (struct creature *)it->data;
        is_group = false;
        if (ch == i)
            continue;

        if (ch->in_room != i->in_room && AFF_FLAGGED(i, AFF_HIDE | AFF_SNEAK)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
            continue;

        if (room_is_dark(ch->in_room)
            && !has_dark_sight(ch)
            && AFF_FLAGGED(i, AFF_INFRAVISION)) {
            switch (number(0, 2)) {
            case 0:
                acc_sprintf
                    ("You see a pair of glowing red eyes looking your way.\r\n");
                break;
            case 1:
                acc_sprintf("A pair of eyes glow red in the darkness.\r\n");
                break;
            case 2:
                unseen++;
                break;
            }

            continue;
        }
        // skip those with no ldesc
        if (!IS_IMMORT(ch) && IS_NPC(i) && !i->player.long_descr)
            continue;

        // skip those you can't see for in-game reasons
        if (!can_see_creature(ch, i)) {
            if (!IS_IMMORT(i) && !(IS_NPC(i) && NPC_FLAGGED(i, NPC_UTILITY)))
                unseen++;
            continue;
        }

        if (AFF_FLAGGED(i, AFF_HIDE)
            && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)
            && !PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
            hide_prob = number(0, skill_bonus(i, SKILL_HIDE));
            hide_roll = number(0, level_bonus(ch, true));
            if (affected_by_spell(ch, ZEN_AWARENESS))
                hide_roll += skill_bonus(ch, ZEN_AWARENESS) / 4;

            if (hide_prob > hide_roll) {
                unseen++;
                continue;
            }

            if (can_see_creature(i, ch))
                send_to_char(i, "%s seems to have seen you.\r\n",
                    GET_NAME(ch));
        }

        if (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(i, AFF_GROUP)) {
            is_group = (ch->master == i
                || i->master == ch || (ch->master && i->master == ch->master));
        }

        desc = desc_one_char(ch, i, is_group);
        if (*desc) {
            acc_strcat(desc, NULL);
            if (!PRF2_FLAGGED(ch, PRF2_NOTRAILERS)
                && ch->in_room == i->in_room)
                desc_char_trailers(ch, i);
        }
    }

    if (unseen &&
        (AFF_FLAGGED(ch, AFF_SENSE_LIFE)
            || affected_by_spell(ch, SKILL_HYPERSCAN))) {
        acc_sprintf("%s", CCMAG(ch, C_NRM));
        if (unseen == 1)
            acc_sprintf("You sense an unseen presence.\r\n");
        else if (unseen < 4)
            acc_sprintf("You sense a few unseen presences.\r\n");
        else if (unseen < 7)
            acc_sprintf("You sense many unseen presences.\r\n");
        else
            acc_sprintf("You sense a crowd of unseen presences.\r\n");
        acc_sprintf("%s", CCNRM(ch, C_NRM));
    }
}

static void
do_auto_exits(struct creature *ch, struct room_data *room)
{
    int door;
    bool found = false;

    if (room == NULL)
        room = ch->in_room;

    acc_sprintf("%s[ Exits: ", CCCYN(ch, C_NRM));
    for (door = 0; door < NUM_OF_DIRS; door++) {
        if (!room->dir_option[door] || !room->dir_option[door]->to_room)
            continue;

        if (IS_SET(room->dir_option[door]->exit_info, EX_HIDDEN | EX_SECRET))
            continue;

        if (IS_SET(room->dir_option[door]->exit_info, EX_CLOSED))
            acc_sprintf("|%c| ", tolower(*dirs[door]));
        else
            acc_sprintf("%c ", tolower(*dirs[door]));
        found = true;
    }

    acc_sprintf("%s]%s   ", found ? "" : "None obvious ", CCNRM(ch, C_NRM));

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        *buf = '\0';

        acc_sprintf("%s[ Hidden Doors: ", CCCYN(ch, C_NRM));
        found = false;
        for (door = 0; door < NUM_OF_DIRS; door++) {
            if (ABS_EXIT(room, door) && ABS_EXIT(room, door)->to_room != NULL
                && IS_SET(ABS_EXIT(room, door)->exit_info,
                    EX_SECRET | EX_HIDDEN)) {
                acc_sprintf("%c ", tolower(*dirs[door]));
                found = true;
            }
        }

        acc_sprintf("%s]%s\r\n", found ? "" : "None ", CCNRM(ch, C_NRM));
    } else
        acc_sprintf("\r\n");
}

/* functions and macros for 'scan' command */
static void
list_scanned_chars(GList * list, struct creature *ch, int distance, int door)
{
    const char *how_far[] = {
        "close by",
        "a ways off",
        "far off to the"
    };

    int count = 0;

    if (!list)
        return;

    /* this loop is a quick, easy way to help make a grammatical sentence
       (i.e., "You see x, x, y, and z." with commas, "and", etc.) */
    for (GList * it = first_living(list); it; it = next_living(it)) {
        struct creature *tch = (struct creature *)it->data;

        /* put any other conditions for scanning someone in this if statement -
           i.e., if (can_see_creature(ch, i) && condition2 && condition3) or whatever */

        if (can_see_creature(ch, tch) && ch != tch &&
            (!AFF_FLAGGED(tch, AFF_SNEAK | AFF_HIDE) ||
                PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
            count++;
    }

    if (!count)
        return;

    acc_string_clear();
    for (GList * it = first_living(list); it; it = next_living(it)) {
        struct creature *tch = (struct creature *)it->data;
        /* make sure to add changes to the if statement above to this
           one also, using or's to join them.. i.e.,
           if (!can_see_creature(ch, i) || !condition2 || !condition3) */

        if (!can_see_creature(ch, tch) || ch == tch ||
            ((AFF_FLAGGED(tch, AFF_SNEAK | AFF_HIDE)) &&
                !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
            continue;
        acc_sprintf("%s%s%s",
            CCYEL(ch, C_NRM), GET_DISGUISED_NAME(ch, tch), CCNRM(ch, C_NRM));

        if (--count > 1)
            acc_strcat(", ", NULL);
        else if (count == 1)
            acc_strcat(" and ", NULL);
        else {
            acc_sprintf(" %s %s.", how_far[distance], dirs[door]);
        }
    }
    send_to_char(ch, "You see %s\r\n", acc_get_string());
}

ACMD(do_scan)
{
/* >scan
   You quickly scan the area.
   You see John, a large horse and Frank close by north.
   You see a small rabbit a ways off south.
   You see a huge dragon and a griffon far off to the west.

   *make scan a skill (ranger?) with a prof. check in each dir. ?
   */
    int door;

    *buf = '\0';

    if (AFF_FLAGGED(ch, AFF_BLIND) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
        send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
        return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "The room is too smoky to see very far.\r\n");
        return;
    }
    /* may want to add more restrictions here, too */
    send_to_char(ch, "You quickly scan the area.\r\n");
    act("$n quickly scans $s surroundings.", true, ch, NULL, NULL, TO_ROOM);

    for (door = 0; door < NUM_OF_DIRS - 4; door++)  /* don't scan up/down */
        if (EXIT(ch, door) && EXIT(ch, door)->to_room != NULL &&
            EXIT(ch, door)->to_room != ch->in_room &&
            !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
            !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
            !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR) &&
            !IS_SET(EXIT(ch, door)->exit_info, EX_NOSCAN)) {
            if (EXIT(ch, door)->to_room->people) {
                list_scanned_chars(EXIT(ch, door)->to_room->people, ch, 0,
                    door);
                gain_skill_prof(ch, SKILL_SCANNING);
            } else if (_2ND_EXIT(ch, door) &&
                _2ND_EXIT(ch, door)->to_room != NULL &&
                !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_CLOSED) &&
                !ROOM_FLAGGED(_2ND_EXIT(ch, door)->to_room, ROOM_DEATH) &&
                !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_NOSCAN) &&
                !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_ISDOOR)) {
                /* check the second room away */
                if (_2ND_EXIT(ch, door)->to_room->people) {
                    if (CHECK_SKILL(ch, SKILL_SCANNING) > number(-20, 80)) {
                        list_scanned_chars(_2ND_EXIT(ch,
                                door)->to_room->people, ch, 1, door);
                        gain_skill_prof(ch, SKILL_SCANNING);
                    }
                } else if (_3RD_EXIT(ch, door) &&
                    _3RD_EXIT(ch, door)->to_room != NULL &&
                    !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_CLOSED) &&
                    !ROOM_FLAGGED(_3RD_EXIT(ch, door)->to_room, ROOM_DEATH) &&
                    !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_NOSCAN) &&
                    !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_ISDOOR)) {
                    /* check the third room */
                    if (_3RD_EXIT(ch, door)->to_room->people) {
                        if (CHECK_SKILL(ch, SKILL_SCANNING) > number(10, 90)) {
                            list_scanned_chars(_3RD_EXIT(ch,
                                    door)->to_room->people, ch, 2, door);
                            gain_skill_prof(ch, SKILL_SCANNING);
                        }
                    }
                }
            }
        }
}

ACMD(do_exits)
{
    int door;

    if (!PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
        if (!check_sight_self(ch)) {
            send_to_char(ch,
                "You can't see a damned thing, you're blind!\r\n");
            return;
        }

        if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
            !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
            send_to_char(ch,
                "The thick smoke is too disorienting to tell.\r\n");
            return;
        }
    }

    bool found = false;
    for (door = 0; door < NUM_OF_DIRS; door++) {
        if (!EXIT(ch, door) || !EXIT(ch, door)->to_room)
            continue;

        found = true;
        if (!IS_IMMORT(ch) &&
            IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN | EX_SECRET))
            continue;

        if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
            send_to_char(ch, "%s%s%-8s%s - %s[%s%5d%s] %s%s%s%s%s%s%s\r\n",
                CCBLD(ch, C_SPR), CCBLU(ch, C_NRM), tmp_capitalize(dirs[door]),
                CCNRM(ch, C_NRM), CCGRN(ch, C_NRM),
                CCNRM(ch, C_NRM), EXIT(ch, door)->to_room->number,
                CCCYN(ch, C_NRM), EXIT(ch, door)->to_room->name,
                CCNRM(ch, C_SPR),
                IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ?
                " (closed)" : "",
                IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN) ?
                " (hidden)" : "",
                IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ?
                " (secret)" : "",
                IS_SET(EXIT(ch, door)->exit_info, EX_NOPASS) ?
                " (nopass)" : "",
                (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
                    !EXIT(ch, door)->keyword) ? " *NEEDS KEYWORD*" : "");
        } else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
            send_to_char(ch, "%s%s%-8s%s - Closed %s\r\n",
                CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
                tmp_capitalize(dirs[door]),
                (EXIT(ch, door)->keyword) ? fname(EXIT(ch,
                        door)->keyword) : "TYPO ME>", CCNRM(ch, C_SPR));
        } else if ((AFF_FLAGGED(ch, AFF_BLIND)
                || ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED))
            && AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
            send_to_char(ch, "%s%s%-8s%s - Out of range\r\n", CCBLD(ch, C_SPR),
                CCBLU(ch, C_NRM), tmp_capitalize(dirs[door]), CCNRM(ch,
                    C_SPR));
        } else if (!check_sight_room(ch, EXIT(ch, door)->to_room)
            && !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH)) {
            send_to_char(ch, "%s%s%-8s%s - Too dark to tell\r\n", CCBLD(ch,
                    C_SPR), CCBLU(ch, C_NRM), tmp_capitalize(dirs[door]),
                CCNRM(ch, C_SPR));
        } else {
            send_to_char(ch, "%s%s%-8s%s - %s%s%s\r\n",
                CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
                tmp_capitalize(dirs[door]), CCNRM(ch, C_SPR),
                CCCYN(ch, C_NRM),
                (EXIT(ch, door)->to_room->name) ?
                (EXIT(ch, door)->to_room->name) : "<TYPO ME>",
                CCNRM(ch, C_SPR));
        }
    }

    if (!found)
        send_to_char(ch, "There are no obvious exits.\r\n");
}

void
look_at_room(struct creature *ch, struct room_data *room, int ignore_brief)
{

    struct room_affect_data *aff = NULL;
    struct obj_data *o = NULL;

    int ice_shown = 0;          // 1 if ice has already been shown to room...same for blood
    int blood_shown = 0;

    if (room == NULL)
        room = ch->in_room;

    if (!ch->desc)
        return;

    if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
        send_to_char(ch, "It is pitch black...\r\n");
        return;
    }

    if (!check_sight_self(ch)) {
        send_to_char(ch, "You can't see anything around you...\r\n");
        return;
    }

    acc_string_clear();
    acc_sprintf("%s", CCCYN(ch, C_NRM));
    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS) ||
        (ch->desc->original
            && PRF_FLAGGED(ch->desc->original, PRF_ROOMFLAGS))) {
        acc_sprintf("[%5d] %s [ %s ] [ %s ]", room->number,
            room->name,
            ROOM_FLAGS(room) ? tmp_printbits(ROOM_FLAGS(room),
                room_bits) : "NONE", strlist_aref(room->sector_type,
                sector_types));
        if (room->max_occupancy < 256)
            acc_sprintf(" [ Max: %d ]", room->max_occupancy);

        struct house *house = find_house_by_room(room->number);
        if (house)
            acc_sprintf(" [ House: %d ]", house->id);
    } else {
        acc_sprintf("%s", room->name);
    }

    acc_sprintf("%s\r\n", CCNRM(ch, C_NRM));

    if ((!PRF_FLAGGED(ch, PRF_BRIEF) &&
            (!ch->desc->original
                || !PRF_FLAGGED(ch->desc->original, PRF_BRIEF)))
        || ignore_brief || ROOM_FLAGGED(room, ROOM_DEATH)) {
        // We need to show them something...
        if (ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) &&
            !(PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
                ROOM_FLAGGED(room, ROOM_DEATH)) &&
            !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
            acc_sprintf("The smoke swirls around you...\r\n");
        else if (room->description)
            acc_sprintf("%s", room->description);
    }

    for (aff = room->affects; aff; aff = aff->next)
        if (aff->description)
            acc_sprintf("%s", aff->description);

    /* Zone PK type */
    switch (room->zone->pk_style) {
    case ZONE_NO_PK:
        acc_sprintf("%s[ %s!PK%s ] ", CCCYN(ch, C_NRM),
            CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));
        break;
    case ZONE_NEUTRAL_PK:
        acc_sprintf("%s[ %s%sNPK%s%s ] ", CCCYN(ch, C_NRM),
            CCBLD(ch, C_CMP), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM));
        break;
    case ZONE_CHAOTIC_PK:
        acc_sprintf("%s[ %s%sCPK%s%s ] ", CCCYN(ch, C_NRM),
            CCBLD(ch, C_CMP), CCRED(ch, C_NRM), CCNRM(ch, C_NRM),
            CCCYN(ch, C_NRM));
        break;
    }

    if ((GET_LEVEL(ch) >= LVL_AMBASSADOR ||
            !ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) ||
            AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))) {

        /* autoexits */
        if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
            do_auto_exits(ch, room);
        else
            acc_sprintf("\r\n");

        /* now list characters & objects */
        for (o = room->contents; o; o = o->next_content) {
            if (GET_OBJ_VNUM(o) == BLOOD_VNUM) {
                if (!blood_shown) {
                    acc_sprintf("%s%s.%s\r\n",
                        CCRED(ch, C_NRM),
                        GET_OBJ_TIMER(o) < 10 ?
                        "Some spots of blood have been splattered around" :
                        GET_OBJ_TIMER(o) < 20 ?
                        "Small pools of blood are here" :
                        GET_OBJ_TIMER(o) < 30 ?
                        "Large pools of blood are here" :
                        GET_OBJ_TIMER(o) < 40 ?
                        "Blood is pooled and splattered all over everything"
                        : "Dark red blood covers everything in sight",
                        CCNRM(ch, C_NRM));
                    blood_shown = 1;
                }
            }

            if (GET_OBJ_VNUM(o) == ICE_VNUM) {
                if (!ice_shown) {
                    acc_sprintf("%s%s.%s\r\n",
                        CCCYN(ch, C_NRM),
                        GET_OBJ_TIMER(o) < 10 ?
                        "A few patches of ice are scattered around" :
                        GET_OBJ_TIMER(o) < 20 ?
                        "A thin coating of ice covers everything" :
                        GET_OBJ_TIMER(o) < 30 ?
                        "A thick coating of ice covers everything" :
                        "Everything is covered with a thick coating of ice",
                        CCNRM(ch, C_NRM));
                    break;
                }
            }
        }

        acc_sprintf("%s", CCGRN(ch, C_NRM));
        list_obj_to_char(room->contents, ch, SHOW_OBJ_ROOM, false);
        acc_sprintf("%s", CCYEL(ch, C_NRM));
        list_char_to_char(room->people, ch);
        acc_sprintf("%s", CCNRM(ch, C_NRM));
    } else {
        acc_sprintf("%s\r\n", CCNRM(ch, C_NRM));
    }

    send_to_char(ch, "%s", acc_get_string());
}

static void
look_in_direction(struct creature *ch, int dir)
{
#define EXNUMB EXIT(ch, dir)->to_room
    struct room_affect_data *aff;

    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
        GET_LEVEL(ch) < LVL_AMBASSADOR) {
        send_to_char(ch, "The thick smoke limits your vision.\r\n");
        return;
    }
    if (EXIT(ch, dir)) {
        acc_string_clear();
        if (EXIT(ch, dir)->general_description) {
            acc_sprintf("%s", EXIT(ch, dir)->general_description);
        } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR | EX_CLOSED) &&
            EXIT(ch, dir)->keyword) {
            acc_sprintf("You see %s %s.\r\n", AN(fname(EXIT(ch,
                            dir)->keyword)), fname(EXIT(ch, dir)->keyword));
        } else if (EXNUMB) {
            if ((IS_SET(EXIT(ch, dir)->exit_info, EX_NOPASS) &&
                    GET_LEVEL(ch) < LVL_AMBASSADOR) ||
                IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN))
                acc_sprintf("You see nothing special.\r\n");

            else if (EXNUMB->name) {
                acc_sprintf("%s", CCCYN(ch, C_NRM));
                if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
                    sprintbit((long)ROOM_FLAGS(EXNUMB), room_bits, buf);
                    acc_sprintf("[%5d] %s [ %s] [ %s ]", EXNUMB->number,
                        EXNUMB->name, buf, sector_types[EXNUMB->sector_type]);
                } else
                    acc_sprintf("%s", EXNUMB->name);
                acc_sprintf("%s", CCNRM(ch, C_NRM));
                acc_sprintf("\r\n");
                for (aff = ch->in_room->affects; aff; aff = aff->next)
                    if (aff->type == dir && aff->description)
                        acc_sprintf("%s", aff->description);
            } else
                acc_sprintf("You see nothing special.\r\n");
        }
        if (EXNUMB && (!IS_SET(EXIT(ch, dir)->exit_info,
                    EX_ISDOOR | EX_CLOSED | EX_HIDDEN | EX_NOPASS) ||
                PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {

            if (room_is_dark(EXNUMB) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
                CHECK_SKILL(ch, SKILL_NIGHT_VISION) < number(GET_LEVEL(ch),
                    101))
                send_to_char(ch,
                    "It's too dark there to see anything special.\r\n");
            else {
                /* now list characters & objects */
                acc_sprintf("%s", CCGRN(ch, C_NRM));
                list_obj_to_char(EXNUMB->contents, ch, SHOW_OBJ_ROOM, false);
                acc_sprintf("%s", CCYEL(ch, C_NRM));
                list_char_to_char(EXNUMB->people, ch);
                acc_sprintf("%s", CCNRM(ch, C_NRM));
            }
        }

        if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN | EX_NOPASS)) {
            if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)
                && EXIT(ch, dir)->keyword) {
                acc_sprintf("The %s %s closed.\r\n", fname(EXIT(ch,
                            dir)->keyword), ISARE(fname(EXIT(ch,
                                dir)->keyword)));
            } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) &&
                EXIT(ch, dir)->keyword) {
                acc_sprintf("The %s %s open.\r\n", fname(EXIT(ch,
                            dir)->keyword), ISARE(fname(EXIT(ch,
                                dir)->keyword)));
                if (EXNUMB != NULL && room_is_dark(EXNUMB) &&
                    !EXIT(ch, dir)->general_description) {
                    sprintf(buf,
                        "It's too dark through the %s to see anything.\r\n",
                        fname(EXIT(ch, dir)->keyword));
                    acc_sprintf("%s", buf);
                }
            }
        }
        page_string(ch->desc, acc_get_string());
    } else {

        if (ch->in_room->sector_type == SECT_PITCH_SUB) {
            send_to_char(ch,
                "You cannot see anything through the black pitch.\r\n");
            return;
        }
        switch (dir) {
        case FUTURE:
            if (ch->in_room->zone->plane >= PLANE_HELL_1 &&
                ch->in_room->zone->plane <= PLANE_HELL_9)
                send_to_char(ch,
                    "From here, it looks like things are only going to get worse.\r\n");
            else if (ch->in_room->zone->plane == PLANE_ASTRAL)
                send_to_char(ch, "You gaze into infinity.\r\n");
            else
                send_to_char(ch,
                    "You try and try, but cannot see into the future.\r\n");
            break;
        case PAST:
            if (ch->in_room->zone->plane == PLANE_ASTRAL)
                send_to_char(ch, "You gaze into infinity.\r\n");
            else
                send_to_char(ch,
                    "You try and try, but cannot see into the past.\r\n");
            break;
        case UP:
            if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
                send_to_char(ch, "You see the ceiling above you.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_8)
                send_to_char(ch, "Snow and sleet rage across the sky.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_6 ||
                GET_PLANE(ch->in_room) == PLANE_HELL_7)
                send_to_char(ch,
                    "The sky is alight with clouds of blood-red steam.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_5)
                send_to_char(ch,
                    "You see the jet black sky above you, lit only by flashes of lightning.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_4)
                send_to_char(ch,
                    "The sky is a deep red, covered by clouds of dark ash.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_3)
                send_to_char(ch, "The sky is cold and grey.\r\n"
                    "It looks like the belly of one giant thunderstorm.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_2)
                send_to_char(ch,
                    "The sky is dull green, flickering with lightning.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_HELL_1)
                send_to_char(ch, "The dark sky is crimson and starless.\r\n");
            else if (GET_PLANE(ch->in_room) == PLANE_COSTAL)
                send_to_char(ch,
                    "Great swirls of pink, green, and blue cover the sky.\r\n");
            else if ((ch->in_room->sector_type == SECT_CITY)
                || (ch->in_room->sector_type == SECT_FOREST)
                || (ch->in_room->sector_type == SECT_HILLS)
                || (ch->in_room->sector_type == SECT_FIELD)
                || (ch->in_room->sector_type == SECT_FARMLAND)
                || (ch->in_room->sector_type == SECT_SWAMP)
                || (ch->in_room->sector_type == SECT_DESERT)
                || (ch->in_room->sector_type == SECT_JUNGLE)
                || (ch->in_room->sector_type == SECT_ROCK)
                || (ch->in_room->sector_type == SECT_MUDDY)
                || (ch->in_room->sector_type == SECT_TRAIL)
                || (ch->in_room->sector_type == SECT_TUNDRA)
                || (ch->in_room->sector_type == SECT_CRACKED_ROAD)
                || (ch->in_room->sector_type == SECT_ROAD)) {
                if (ch->in_room->zone->weather->sunlight == SUN_DARK) {
                    if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
                        send_to_char(ch,
                            "Lightning flashes across the dark sky above you.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_RAINING)
                        send_to_char(ch,
                            "Rain pelts your face as you look to the night sky.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_CLOUDLESS)
                        send_to_char(ch,
                            "Glittering stars shine like jewels upon the sea.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_CLOUDY)
                        send_to_char(ch,
                            "Thick dark clouds obscure the night sky.\r\n");
                    else
                        send_to_char(ch, "You see the sky above you.\r\n");
                } else {
                    if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
                        send_to_char(ch,
                            "Lightning flashes across the sky above you.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_RAINING)
                        send_to_char(ch,
                            "Rain pelts your face as you look to the sky.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_CLOUDLESS)
                        send_to_char(ch,
                            "You see the expansive blue sky, not a cloud in sight.\r\n");
                    else if (ch->in_room->zone->weather->sky == SKY_CLOUDY)
                        send_to_char(ch,
                            "Thick dark clouds obscure the sky above you.\r\n");
                    else
                        send_to_char(ch, "You see the sky above you.\r\n");
                }
            } else if (ch->in_room->sector_type == SECT_DEEP_OCEAN)
                send_to_char(ch, "You see dark waters above you.\r\n");
            else if (room_is_underwater(ch->in_room))
                send_to_char(ch, "You see water above you.\r\n");
            else
                send_to_char(ch, "Nothing special there...\r\n");
            break;
        case DOWN:
            if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
                send_to_char(ch, "You see the floor below you.\r\n");
            else if ((ch->in_room->sector_type == SECT_CITY) ||
                (ch->in_room->sector_type == SECT_FOREST) ||
                (ch->in_room->sector_type == SECT_HILLS) ||
                (ch->in_room->sector_type == SECT_FIELD) ||
                (ch->in_room->sector_type == SECT_FARMLAND) ||
                (ch->in_room->sector_type == SECT_JUNGLE) ||
                (ch->in_room->sector_type == SECT_TRAIL) ||
                (ch->in_room->sector_type == SECT_TUNDRA) ||
                (ch->in_room->sector_type == SECT_CRACKED_ROAD) ||
                (ch->in_room->sector_type == SECT_JUNGLE) ||
                (ch->in_room->sector_type == SECT_ROAD))
                send_to_char(ch, "You see the ground below you.\r\n");
            else if ((ch->in_room->sector_type == SECT_MOUNTAIN) ||
                (ch->in_room->sector_type == SECT_ROCK) ||
                (ch->in_room->sector_type == SECT_CATACOMBS))
                send_to_char(ch, "You see the rocky ground below you.\r\n");
            else if (ch->in_room->sector_type == SECT_SWAMP)
                send_to_char(ch,
                    "You see the swampy ground below your feet.\r\n");
            else if (room_is_watery(ch->in_room))
                send_to_char(ch, "You see the water below you.\r\n");
            else if (room_is_open_air(ch->in_room)) {
                if (ch->in_room->sector_type == SECT_FLYING ||
                    ch->in_room->sector_type == SECT_ELEMENTAL_AIR)
                    send_to_char(ch, "Below your feet is thin air.\r\n");
                else if (ch->in_room->sector_type == SECT_ELEMENTAL_LIGHTNING)
                    send_to_char(ch,
                        "Elemental lightning flickers beneath your feet.\r\n");
                else
                    send_to_char(ch,
                        "Nothing substantial lies below your feet.\r\n");
            } else if (ch->in_room->sector_type == SECT_DESERT)
                send_to_char(ch, "You see the sands below your feet.\r\n");
            else if (ch->in_room->sector_type == SECT_MUDDY)
                send_to_char(ch, "You see the mud underneath your feet.\r\n");
            else
                send_to_char(ch, "Nothing special there...\r\n");
            break;
        default:
            send_to_char(ch, "Nothing special there...\r\n");
            break;
        }
    }
}

static void
look_in_obj(struct creature *ch, char *arg)
{
    struct obj_data *obj = NULL;
    int amt, bits;
    struct room_data *room_was_in = NULL;

    acc_string_clear();
    if (!*arg)
        acc_sprintf("Look in what?\r\n");
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
                FIND_OBJ_EQUIP, ch, NULL, &obj))) {
        acc_sprintf("There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
        (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
        (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
        (GET_OBJ_TYPE(obj) != ITEM_PIPE) &&
        (GET_OBJ_TYPE(obj) != ITEM_VEHICLE))
        acc_sprintf("There's nothing inside that!\r\n");
    else {
        if (IS_OBJ_TYPE(obj, ITEM_CONTAINER)) {
            if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED) &&
                !GET_OBJ_VAL(obj, 3) && GET_LEVEL(ch) < LVL_GOD)
                acc_sprintf("It is closed.\r\n");
            else {
                acc_sprintf("%s", obj->name);
                switch (bits) {
                case FIND_OBJ_INV:
                    acc_sprintf(" (carried): \r\n");
                    break;
                case FIND_OBJ_ROOM:
                    acc_sprintf(" (here): \r\n");
                    break;
                case FIND_OBJ_EQUIP:
                case FIND_OBJ_EQUIP_CONT:
                    acc_sprintf(" (used): \r\n");
                    break;
                }

                list_obj_to_char(obj->contains, ch, SHOW_OBJ_CONTENT, true);
            }
        } else if (IS_OBJ_TYPE(obj, ITEM_VEHICLE)) {
            if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
                act("The door of $p is closed, and you can't see in.",
                    false, ch, obj, NULL, TO_CHAR);
            else if (real_room(ROOM_NUMBER(obj)) != NULL) {
                acc_sprintf("Inside %s you see:\r\n", OBJS(obj, ch));
                room_was_in = ch->in_room;
                char_from_room(ch, false);
                char_to_room(ch, real_room(ROOM_NUMBER(obj)), false);
                list_char_to_char(ch->in_room->people, ch);
                act("$n looks in from the outside.", false, ch, NULL, NULL, TO_ROOM);
                char_from_room(ch, false);
                char_to_room(ch, room_was_in, false);
            }
        } else if (IS_OBJ_TYPE(obj, ITEM_PIPE)) {
            if (GET_OBJ_VAL(obj, 0))
                acc_sprintf("There appears to be some tobacco in it.\r\n");
            else
                acc_sprintf("There is nothing in it.\r\n");

        } else {                /* item must be a fountain or drink container */
            if (GET_OBJ_VAL(obj, 1) == 0)
                acc_sprintf("It is empty.\r\n");
            else {
                if (GET_OBJ_VAL(obj, 0) < 0 || GET_OBJ_VAL(obj, 1) < 0)
                    amt = 3;
                else
                    amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
                acc_sprintf("It's %sfull of a %s liquid.\r\n", fullness[amt],
                    color_liquid[GET_OBJ_VAL(obj, 2)]);
            }
        }
    }
    page_string(ch->desc, acc_get_string());
}

char *
find_exdesc(char *word, struct extra_descr_data *list, bool find_exact)
{
    struct extra_descr_data *i;

    if (!find_exact) {
        for (i = list; i; i = i->next)
            if (isname(word, i->keyword))
                return (i->description);
    } else {
        for (i = list; i; i = i->next)
            if (isname_exact(word, i->keyword))
                return (i->description);
    }

    return NULL;
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void
look_at_target(struct creature *ch, char *arg, int cmd)
{
    int bits, found = 0, j;
    struct creature *found_char = NULL;
    struct obj_data *obj = NULL, *found_obj = NULL, *car;
    char *desc;

    if (!*arg) {
        send_to_char(ch, "Look at what?\r\n");
        return;
    }
    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
        FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
        look_at_char(found_char, ch, cmd);
        if (ch != found_char) {
            if (can_see_creature(found_char, ch)) {
                if (CMD_IS("examine"))
                    act("$n examines you.", true, ch, NULL, found_char, TO_VICT);
                else
                    act("$n looks at you.", true, ch, NULL, found_char, TO_VICT);
            }
            act("$n looks at $N.", true, ch, NULL, found_char, TO_NOTVICT);
        }
        return;
    }

    if (strchr(arg, '.'))
        arg = strchr(arg, '.') + 1;

    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, ch->in_room->ex_description, false)) != NULL) {
        page_string(ch->desc, desc);
        return;
    }
    /* Does the argument match a door keyword anywhere in the room? */
    for (j = 0; j < 8; j++) {
        if (EXIT(ch, j) && EXIT(ch, j)->keyword
            && isname(arg, EXIT(ch, j)->keyword)) {
            send_to_char(ch, "The %s %s %s.\r\n", fname(EXIT(ch, j)->keyword),
                PLUR(fname(EXIT(ch, j)->keyword)) ? "are" : "is",
                IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED) ? "closed" : "open");
            return;
        }
    }
    /* Does the argument match an extra desc in the char's equipment? */
    for (j = 0; j < NUM_WEARS && !found; j++)
        if (GET_EQ(ch, j) && can_see_object(ch, GET_EQ(ch, j)) &&
            (GET_OBJ_VAL(GET_EQ(ch, j), 3) != -999 ||
                isname(arg, GET_EQ(ch, j)->aliases)))
            if ((desc =
                    find_exdesc(arg, GET_EQ(ch, j)->ex_description,
                        false)) != NULL) {
                page_string(ch->desc, desc);
                found = bits = 1;
                found_obj = GET_EQ(ch, j);
            }
    /* Does the argument match an extra desc in the char's inventory? */
    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
        if (can_see_object(ch, obj) &&
            (GET_OBJ_VAL(obj, 3) != -999 || isname(arg, obj->aliases)))
            if ((desc = find_exdesc(arg, obj->ex_description, false)) != NULL) {
                page_string(ch->desc, desc);
                found = bits = 1;
                found_obj = obj;
                break;
            }
    }

    /* Does the argument match an extra desc of an object in the room? */
    for (obj = ch->in_room->contents; obj && !found; obj = obj->next_content)
        if (can_see_object(ch, obj) &&
            (GET_OBJ_VAL(obj, 3) != -999 || isname(arg, obj->aliases)))
            if ((desc = find_exdesc(arg, obj->ex_description, false)) != NULL) {
                page_string(ch->desc, desc);
                found = bits = 1;
                found_obj = obj;
                break;
            }

    if (bits) {                 /* If an object was found back in
                                 * generic_find */
        if (found_obj) {
            if (IS_OBJ_TYPE(found_obj, ITEM_WINDOW)) {
                if (!CAR_CLOSED(found_obj)) {
                    if (real_room(GET_OBJ_VAL(found_obj, 0)) != NULL)
                        look_at_room(ch, real_room(GET_OBJ_VAL(found_obj, 0)),
                            1);
                } else
                    act("$p is closed right now.", false, ch, found_obj, NULL,
                        TO_CHAR);
            } else if (IS_V_WINDOW(found_obj)) {

                for (car = object_list; car; car = car->next)
                    if (GET_OBJ_VNUM(car) == V_CAR_VNUM(found_obj) &&
                        ROOM_NUMBER(car) == ROOM_NUMBER(found_obj) &&
                        car->in_room)
                        break;

                if (car) {
                    act("You look through $p.", false, ch, found_obj, NULL,
                        TO_CHAR);
                    look_at_room(ch, car->in_room, 1);
                }
            } else if (!found) {
                acc_string_clear();
                show_obj_to_char(found_obj, ch, SHOW_OBJ_EXTRA, 0); /* Show no-description */
                page_string(ch->desc, acc_get_string());
            } else {
                acc_string_clear();
                show_obj_to_char(found_obj, ch, SHOW_OBJ_BITS, 0);  /* Find hum, glow etc */
                send_to_char(ch, "%s", acc_get_string());
            }
        }

    } else if (!found)
        send_to_char(ch, "You do not see that here.\r\n");
}

static void
glance_at_target(struct creature *ch, char *arg, int cmd)
{
    struct creature *found_char = NULL;
    struct obj_data *found_obj = NULL;

    if (!*arg) {
        send_to_char(ch, "Look at what?\r\n");
        return;
    }
    (void)generic_find(arg, FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
        look_at_char(found_char, ch, cmd);  /** CMD_IS("glance") !! **/
        if (ch != found_char) {
            if (can_see_creature(found_char, ch) &&
                ((GET_SKILL(ch, SKILL_GLANCE) + GET_LEVEL(ch)) <
                    (number(0, 101) + GET_LEVEL(found_char)))) {
                act("$n glances sidelong at you.", true, ch, NULL, found_char,
                    TO_VICT);
                act("$n glances sidelong at $N.", true, ch, NULL, found_char,
                    TO_NOTVICT);

                if (IS_NPC(found_char) && !(is_fighting(found_char))
                    && AWAKE(found_char) && (!found_char->master
                        || found_char->master != ch)) {
                    if (IS_ANIMAL(found_char) || IS_BUGBEAR(found_char)
                        || GET_INT(found_char) < number(3, 5)) {
                        act("$N growls at you.", false, ch, NULL, found_char,
                            TO_CHAR);
                        act("$N growls at $n.", false, ch, NULL, found_char,
                            TO_NOTVICT);
                    } else if (IS_UNDEAD(found_char)) {
                        act("$N regards you with an icy glare.",
                            false, ch, NULL, found_char, TO_CHAR);
                        act("$N regards $n with an icy glare.",
                            false, ch, NULL, found_char, TO_NOTVICT);
                    } else if (IS_MINOTAUR(found_char) || IS_DEVIL(found_char)
                        || IS_DEMON(found_char) || IS_MANTICORE(found_char)) {
                        act("$N roars at you.", false, ch, NULL, found_char,
                            TO_CHAR);
                        act("$N roars at $n.", false, ch, NULL, found_char,
                            TO_NOTVICT);
                    } else {
                        const char *response = "";
                        switch (number(0, 4)) {
                        case 0:
                            response = tmp_sprintf("Piss off, %s.",
                                GET_DISGUISED_NAME(found_char, ch));
                            break;
                        case 1:
                            response = tmp_sprintf("Take a hike, %s.",
                                GET_DISGUISED_NAME(found_char, ch));
                            break;
                        case 2:
                            response = "You lookin' at me?";
                            break;
                        case 3:
                            response = tmp_sprintf("Hit the road, %s.",
                                GET_DISGUISED_NAME(found_char, ch));
                            break;
                        case 4:
                            response = tmp_sprintf("Get lost, %s.",
                                GET_DISGUISED_NAME(found_char, ch));
                            break;
                        }
                        perform_say_to(found_char, ch, response);
                    }

                    if (NPC_FLAGGED(found_char, NPC_AGGRESSIVE) &&
                        (GET_MORALE(found_char) >
                            number(GET_LEVEL(ch) / 2, GET_LEVEL(ch))) &&
                        !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
                        if (GET_POSITION(found_char) >= POS_SITTING) {
                            if (ok_to_attack(found_char, ch, false))
                                hit(found_char, ch, TYPE_UNDEFINED);
                        } else
                            do_stand(found_char, tmp_strdup(""), 0, 0);
                    }
                }
            } else
                gain_skill_prof(ch, SKILL_GLANCE);
        }
    } else
        send_to_char(ch, "No-one around by that name.\r\n");
    return;
}

static gint
found_fighting(struct creature *tch, gpointer ignore __attribute__((unused)))
{
    return (is_fighting(tch)) ? 0 : -1;
}

ACMD(do_listen)
{
    struct creature *fighting_vict = NULL;
    struct obj_data *noisy_obj = NULL;
    int i;

    if (ch->in_room->sounds) {
        send_to_char(ch, "%s", ch->in_room->sounds);
        return;
    }
    for (GList * it = first_living(ch->in_room->people); it; it = next_living(it)) {
        struct creature *tch = (struct creature *)it->data;

        if (is_fighting(tch)) {
            fighting_vict = tch;
            break;
        }
    }

    if (!fighting_vict) {
        for (i = 0; i < NUM_WEARS; i++)
            if (GET_EQ(ch, i) && IS_OBJ_STAT(GET_EQ(ch, i), ITEM_HUM)) {
                noisy_obj = NULL;
                break;
            }

        if (!noisy_obj) {
            for (noisy_obj = ch->carrying; noisy_obj;
                noisy_obj = noisy_obj->next_content)
                if (IS_OBJ_STAT(noisy_obj, ITEM_HUM))
                    break;

            if (!noisy_obj)
                for (noisy_obj = ch->in_room->contents; noisy_obj;
                    noisy_obj = noisy_obj->next_content)
                    if (IS_OBJ_STAT(noisy_obj, ITEM_HUM))
                        break;
        }
    }

    switch (ch->in_room->sector_type) {
    case SECT_CITY:
        if (!PRIME_MATERIAL_ROOM(ch->in_room) ||
            ch->in_room->zone->number == 262) {
            send_to_char(ch, "You hear nothing special.\r\n");
            break;
        }
        if (ch->in_room->zone->weather->sunlight == SUN_DARK) {
            if (ch->in_room->zone->number >= 500
                && ch->in_room->zone->number < 510)
                send_to_char(ch,
                    "Dark, foreboding and silent, the city itself lies in ambush for you.\r\n");
            send_to_char(ch, "You hear the sounds of the city at night.\r\n");
        } else {
            if (ch->in_room->zone->weather->sky == SKY_RAINING) {
                if (ch->in_room->zone->number >= 500
                    && ch->in_room->zone->number < 510)
                    send_to_char(ch,
                        "Luckily the sound of falling acid rain will cover *your* footsteps too...\r\n");
                else
                    send_to_char(ch,
                        "You hear the sounds of a city on a rainy day.\r\n");
            } else if (ch->in_room->zone->number >= 500
                && ch->in_room->zone->number < 510)
                send_to_char(ch,
                    "The quiet of a sleeping city seems almost peaceful until the shots\r\nring out, extolling another death.\r\n");
            else
                send_to_char(ch,
                    "You hear the sounds of a bustling city.\r\n");
        }
        break;
    case SECT_FOREST:
        if (ch->in_room->zone->weather->sky == SKY_RAINING)
            send_to_char(ch,
                "You hear the rain on the leaves of the trees.\r\n");
        else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
            send_to_char(ch, "You hear peals of thunder in the distance.\r\n");
        else
            send_to_char(ch,
                "You hear the breeze rustling through the trees.\r\n");
        break;
    default:
        if (OUTSIDE(ch)) {
            if (ch->in_room->zone->weather->sky == SKY_RAINING)
                send_to_char(ch,
                    "You hear the falling rain all around you.\r\n");
            else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
                send_to_char(ch,
                    "You hear peals of thunder in the distance.\r\n");
            else if (fighting_vict)
                send_to_char(ch, "You hear the sounds of battle.\r\n");
            else if (noisy_obj)
                act("You hear a low humming coming from $p.",
                    false, ch, noisy_obj, NULL, TO_CHAR);
            else
                send_to_char(ch, "You hear nothing special.\r\n");
        } else if (fighting_vict)
            send_to_char(ch, "You hear the sounds of battle.\r\n");
        else if (noisy_obj)
            act("You hear a low humming coming from $p.",
                false, ch, noisy_obj, NULL, TO_CHAR);
        else {
            for (i = 0; i < NUM_DIRS; i++) {
                if (ch->in_room->dir_option[i] &&
                    ch->in_room->dir_option[i]->to_room &&
                    ch->in_room->dir_option[i]->to_room != ch->in_room &&
                    !IS_SET(ch->in_room->dir_option[i]->exit_info,
                        EX_CLOSED)) {

                    GList *found =
                        g_list_find_custom(ch->in_room->dir_option[i]->
                        to_room->people, NULL, (GCompareFunc) found_fighting);

                    if (found && !number(0, 1)) {
                        send_to_char(ch,
                            "You hear sounds of battle from %s.\r\n",
                            from_dirs[rev_dir[i]]);
                        return;
                    }
                }
            }
            send_to_char(ch, "You hear nothing special.\r\n");
        }
    }
}

ACMD(do_look)
{
    static char arg[MAX_INPUT_LENGTH];
    static char arg2[MAX_INPUT_LENGTH];
    int look_type;

    if (!ch->desc)
        return;

    if (GET_POSITION(ch) < POS_SLEEPING)
        send_to_char(ch, "You can't see anything but stars!\r\n");
    else if (!check_sight_self(ch))
        send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    else if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
        send_to_char(ch, "It is pitch black...\r\n");
        acc_string_clear();
        list_char_to_char(ch->in_room->people, ch); // glowing red eyes
        send_to_char(ch, "%s", acc_get_string());
    } else {
        half_chop(argument, arg, arg2);

        if (!*arg)              /* "look" alone, without an argument at all */
            look_at_room(ch, ch->in_room, 1);
        else if (is_abbrev(arg, "into"))
            look_in_obj(ch, arg2);
        /* did the char type 'look <direction>?' */
        else if ((look_type = search_block(arg, dirs, false)) >= 0)
            look_in_direction(ch, look_type);
        else if (is_abbrev(arg, "at"))
            look_at_target(ch, arg2, cmd);
        else
            look_at_target(ch, arg, cmd);
    }
}

ACMD(do_glance)
{
    if (!ch->desc)
        return;

    if (GET_POSITION(ch) < POS_SLEEPING)
        send_to_char(ch, "You can't see anything but stars!\r\n");
    else if (AFF_FLAGGED(ch, AFF_BLIND)
        && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
        send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    else if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
        send_to_char(ch, "It is pitch black...\r\n");
        acc_string_clear();
        list_char_to_char(ch->in_room->people, ch); // glowing red eyes
        send_to_char(ch, "%s", acc_get_string());
    } else {
        char *arg = tmp_getword(&argument);

        if (!*arg)              /* "look" alone, without an argument at all */
            send_to_char(ch, "Glance at who?\r\n");
        else if (is_abbrev(arg, "at"))
            glance_at_target(ch, argument, cmd);
        else
            glance_at_target(ch, arg, cmd);
    }
}

ACMD(do_examine)
{
    struct creature *tmp_char;
    struct obj_data *tmp_object;

    if (GET_POSITION(ch) < POS_SLEEPING) {
        send_to_char(ch, "You can't see anything but stars!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_BLIND)
        && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
        send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
        return;
    }

    if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
        send_to_char(ch, "It is pitch black, and you can't see anything.\r\n");
        return;
    }

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Examine what?\r\n");
        return;
    }
    look_at_target(ch, arg, cmd);

    (void)generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
        FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

    if (tmp_object) {
        if (OBJ_REINFORCED(tmp_object))
            act("$p appears to be structurally reinforced.",
                false, ch, tmp_object, NULL, TO_CHAR);
        if (OBJ_ENHANCED(tmp_object))
            act("$p looks like it has been enhanced.",
                false, ch, tmp_object, NULL, TO_CHAR);

        sprintf(buf, "$p seems to be in %s condition.",
                obj_cond_color(tmp_object, COLOR_LEV(ch)));
        act(buf, false, ch, tmp_object, NULL, TO_CHAR);

        if (IS_OBJ_TYPE(tmp_object, ITEM_CIGARETTE)) {
            send_to_char(ch, "It seems to have about %d drags left on it.\r\n",
                GET_OBJ_VAL(tmp_object, 0));
        }
        if ((IS_OBJ_TYPE(tmp_object, ITEM_DRINKCON)) ||
            (IS_OBJ_TYPE(tmp_object, ITEM_FOUNTAIN)) ||
            (IS_OBJ_TYPE(tmp_object, ITEM_PIPE)) ||
            (IS_OBJ_TYPE(tmp_object, ITEM_VEHICLE)) ||
            (IS_OBJ_TYPE(tmp_object, ITEM_CONTAINER))) {
            look_in_obj(ch, arg);
        }
    }
}

ACMD(do_qpoints)
{
    int qp;

    if (IS_IMMORT(ch)) {
        qp = GET_IMMORT_QP(ch);
        send_to_char(ch, "You have %d quest point%s to award.\r\n", qp,
            (qp == 1) ? "" : "s");
    } else if (IS_PC(ch)) {
        qp = ch->account->quest_points;
        send_to_char(ch, "You have %d quest point%s.\r\n", qp,
            (qp == 1) ? "" : "s");
    } else {
        send_to_char(ch, "Mobiles don't have quest points!\r\n");
    }
}

ACMD(do_gold)
{
    if (GET_GOLD(ch) == 0)
        send_to_char(ch, "You're broke!\r\n");
    else if (GET_GOLD(ch) == 1)
        send_to_char(ch, "You have one miserable little gold coin.\r\n");
    else {
        send_to_char(ch, "You have %'" PRId64 " gold coins.\r\n", GET_GOLD(ch));
    }
}

ACMD(do_cash)
{
    if (GET_CASH(ch) == 0)
        send_to_char(ch, "You're broke!\r\n");
    else if (GET_CASH(ch) == 1)
        send_to_char(ch, "You have one almighty credit.\r\n");
    else {
        send_to_char(ch, "You have %'" PRId64 " credits.\r\n", GET_CASH(ch));
    }
}

ACMD(do_encumbrance)
{
    int encumbr;

    encumbr = ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 4) / CAN_CARRY_W(ch);

    if (IS_CARRYING_W(ch) + IS_WEARING_W(ch) <= 0) {
        send_to_char(ch, "You aren't carrying a damn thing.\r\n");
        return;
    } else {
        send_to_char(ch, "You are carrying a total %s of stuff.\r\n",
                     format_weight(IS_WEARING_W(ch) + IS_CARRYING_W(ch), USE_METRIC(ch)));
        send_to_char(ch, "%s of which you are wearing or equipped with.\r\n",
                     format_weight(IS_WEARING_W(ch), USE_METRIC(ch)));
    }
    if (encumbr > 3)
        send_to_char(ch, "You are heavily encumbered.\r\n");
    else if (encumbr == 3)
        send_to_char(ch, "You are moderately encumbered.\r\n");
    else if (encumbr == 2)
        send_to_char(ch, "You are lightly encumbered.\r\n");

}

//if like me you noticed this mode thing and had no idea what it was meant for
//it may interest you to know that mode=1 means we should only show bad things
//IMPORTANT: Add negative messages ABOVE the mode check, positive messages BELOW
void
acc_append_affects(struct creature *ch, int8_t mode)
{

    struct affected_type *af = NULL;
    struct creature *mob = NULL;
    const char *name = NULL;
    const char *str = "";

    if (affected_by_spell(ch, SPELL_FIRE_BREATHING)) {
        acc_strcat("You are empowered with breath of FIRE!\r\n", NULL);
    } else if (affected_by_spell(ch, SPELL_FROST_BREATHING)) {
        acc_strcat("You are empowered with breath of FROST!\r\n", NULL);
    }
    if (AFF_FLAGGED(ch, AFF_BLIND))
        acc_strcat("You have been blinded!\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_POISON)
        || AFF3_FLAGGED(ch, AFF3_POISON_2)
        || AFF3_FLAGGED(ch, AFF3_POISON_3))
        acc_strcat("You are poisoned!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_PETRIFY))
        acc_sprintf("You are turning into %sSTONE%s!\r\n",
                    CCNRM_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
    if (AFF3_FLAGGED(ch, AFF3_RADIOACTIVE))
        acc_strcat("You are radioactive.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_GAMMA_RAY))
        acc_strcat("You have been irradiated.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_ABLAZE))
        acc_sprintf("You are %s%sON FIRE!!%s\r\n",
            CCRED_BLD(ch, C_SPR), CCBLK(ch, C_CMP), CCNRM(ch, C_SPR));
    if (affected_by_spell(ch, SPELL_QUAD_DAMAGE))
        acc_sprintf("You are dealing out %squad damage%s.\r\n",
            CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    if (affected_by_spell(ch, SPELL_BLACKMANTLE))
        acc_strcat("You are covered by the blackmantle.\r\n", NULL);

    if (affected_by_spell(ch, SPELL_ENTANGLE))
        acc_strcat("You are entangled in the undergrowth!\r\n", NULL);

    if ((af = affected_by_spell(ch, SKILL_DISGUISE))) {
        if ((mob = real_mobile_proto(af->modifier)))
            acc_sprintf("You are disguised as %s at a level of %d.\r\n",
                GET_NAME(mob), af->duration);
    }
    // radiation sickness

    if ((af = affected_by_spell(ch, TYPE_RAD_SICKNESS))) {
        if (!number(0, 2))
            acc_strcat("You feel nauseous.\r\n", NULL);
        else if (!number(0, 1))
            acc_strcat("You feel sick and your skin is dry.\r\n", NULL);
        else
            acc_strcat("You feel sick and your hair is falling out.\r\n",
                NULL);
    }

    if (AFF2_FLAGGED(ch, AFF2_SLOW))
        acc_strcat("You feel unnaturally slowed.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_CHARM))
        acc_strcat("You have been charmed!\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_MANA_LEAK) && !AFF3_FLAGGED(ch, AFF3_MANA_TAP))
        acc_strcat(str,
            "You are slowly being drained of your spiritual energy.\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_ENERGY_LEAK)
        && !AFF3_FLAGGED(ch, AFF3_ENERGY_TAP))
        acc_strcat(str,
            "Your body is slowly being drained of physical energy.\r\n", NULL);

    if (AFF3_FLAGGED(ch, AFF3_SYMBOL_OF_PAIN))
        acc_strcat("Your mind burns with the symbol of pain!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_WEAKNESS))
        acc_strcat("You feel unusually weakened.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH))
        acc_strcat("You feel a psychic force crushing your mind!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_FEAR))
        acc_strcat("The world is a terribly frightening place!\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_ACIDITY))
        acc_strcat("Your body is producing self-corroding acids!\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_GRAVITY_WELL))
        acc_strcat
            ("Spacetime is bent around you in a powerful gravity well!\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_HAMSTRUNG))
        acc_sprintf
            ("%sThe gash on your leg is %sBLEEDING%s%s all over!!%s\r\n",
            CCRED(ch, C_SPR), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCRED(ch,
                C_SPR), CCNRM(ch, C_SPR));
    if (IS_SICK(ch))
        acc_strcat("You are afflicted with a terrible sickness!\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_HAMSTRUNG))
        acc_sprintf
            ("%sThe gash on your leg is %sBLEEDING%s%s all over!!%s\r\n",
            CCRED(ch, C_SPR), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCRED(ch,
                C_SPR), CCNRM(ch, C_SPR));
    if (AFF_FLAGGED(ch, AFF_CONFUSION))
        acc_strcat("You are very confused.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_MOTOR_SPASM))
        acc_strcat("Your muscles are convulsing uncontrollably!\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_VERTIGO))
        acc_strcat("You are lost in a sea of vertigo.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_TAINTED))
        acc_strcat("The very essence of your being has been tainted.\r\n",
            NULL);
    if (affected_by_spell(ch, SONG_INSIDIOUS_RHYTHM))
        acc_strcat("Your senses have been dulled by insidious melodies.\r\n",
            NULL);
    if (affected_by_spell(ch, SONG_VERSE_OF_VULNERABILITY))
        acc_strcat("You feel more vulnerable to attack.\r\n", NULL);

    // vampiric regeneration

    if ((af = affected_by_spell(ch, SPELL_VAMPIRIC_REGENERATION))) {
        if ((name = player_name_by_idnum(af->modifier)))
            acc_sprintf
                ("You are under the effects of %s's vampiric regeneration.\r\n",
                name);
        else
            acc_strcat
                ("You are under the effects of vampiric regeneration from an unknown source.\r\n",
                NULL);
    }

    if ((af = affected_by_spell(ch, SPELL_LOCUST_REGENERATION))) {
        if ((name = player_name_by_idnum(af->modifier)))
            acc_strcat("You are under the effects of ", name,
                "'s locust regeneration.\r\n", NULL);
        else
            acc_strcat(str,
                "You are under the effects of locust regeneration from an unknown source.\r\n",
                NULL);
    }

    if (mode)                   /* Only asked for bad affs? */
        return;
    if (IS_SOULLESS(ch))
        acc_strcat("A deep despair clouds your soulless mind.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_SNEAK) && !AFF3_FLAGGED(ch, AFF3_INFILTRATE))
        acc_strcat("You are sneaking.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_INFILTRATE))
        acc_strcat("You are infiltrating.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_INVISIBLE))
        acc_strcat("You are invisible.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_TRANSPARENT))
        acc_strcat("You are transparent.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
        acc_strcat(str,
            "You are sensitive to the presence of invisible things.\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_DETECT_POISON))
        acc_strcat(str,
            "You are sensitive to the presence of poisons.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_TRUE_SEEING))
        acc_strcat("You are seeing truly.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_SANCTUARY))
        acc_strcat("You are protected by Sanctuary.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_ARMOR))
        acc_strcat("You feel protected.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_BARKSKIN))
        acc_strcat("Your skin is thick and tough like tree bark.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_STONESKIN))
        acc_strcat("Your skin is as hard as granite.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_SPIRIT_TRACK))
        acc_strcat("You can track by sensing the spirit of your prey.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_INFRAVISION))
        acc_strcat("Your eyes are glowing red.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_REJUV))
        acc_strcat("You feel like your body will heal with a good rest.\r\n",
            NULL);
    if (AFF_FLAGGED(ch, AFF_REGEN))
        acc_strcat("Your body is regenerating itself rapidly.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_GLOWLIGHT))
        acc_strcat("You are followed by a ghostly illumination.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_BLUR))
        acc_strcat("Your image is blurred and shifting.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_DISPLACEMENT)) {
        if (affected_by_spell(ch, SPELL_REFRACTION))
            acc_strcat("Your body is irregularly refractive.\r\n", NULL);
        else
            acc_strcat("Your image is displaced.\r\n", NULL);
    }
    if (affected_by_spell(ch, SPELL_ELECTROSTATIC_FIELD))
        acc_strcat("You are surrounded by an electrostatic field.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_FIRE_SHIELD))
        acc_strcat("You are protected by a shield of fire.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_BLADE_BARRIER))
        acc_strcat("You are protected by whirling blades.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_ENERGY_FIELD))
        acc_strcat("You are surrounded by a field of energy.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_PRISMATIC_SPHERE))
        acc_strcat("You are surrounded by a prismatic sphere of light.\r\n",
            NULL);
    if (AFF2_FLAGGED(ch, AFF2_FLUORESCENT))
        acc_strcat("The atoms in your vicinity are fluorescent.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_DIVINE_ILLUMINATION)) {
        if (IS_EVIL(ch))
            acc_strcat("An unholy light is following you.\r\n", NULL);
        else if (IS_GOOD(ch))
            acc_strcat("A holy light is following you.\r\n", NULL);
        else
            acc_strcat("A sickly light is following you.\r\n", NULL);
    }
    if (AFF2_FLAGGED(ch, AFF2_BERSERK))
        acc_strcat("You are BERSERK!\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
        acc_strcat("You are protected from good.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
        acc_strcat("You are protected from evil.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROT_DEVILS))
        acc_strcat("You are protected from devils.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROT_DEMONS))
        acc_strcat("You are protected from demons.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROTECT_UNDEAD))
        acc_strcat("You are protected from the undead.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROT_LIGHTNING))
        acc_strcat("You are protected from lightning.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROT_FIRE))
        acc_strcat("You are protected from fire.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_MAGICAL_PROT))
        acc_strcat("You are protected against magic.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_ENDURE_COLD))
        acc_strcat("You can endure extreme cold.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_SENSE_LIFE))
        acc_strcat(str,
            "You are sensitive to the presence of living creatures.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_EMPOWER))
        acc_strcat("You are empowered.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_TELEKINESIS))
        acc_strcat("You are feeling telekinetic.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_HASTE))
        acc_strcat("You are moving very fast.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_KATA))
        acc_strcat("You feel focused from your kata.\r\n", NULL);
    if (AFF2_FLAGGED(ch, AFF2_OBLIVITY))
        acc_strcat("You are oblivious to pain.\r\n", NULL);
    if (affected_by_spell(ch, ZEN_MOTION))
        acc_strcat("The zen of motion is one with your body.\r\n", NULL);
    if (affected_by_spell(ch, ZEN_TRANSLOCATION))
        acc_strcat("You are as one with the zen of translocation.\r\n", NULL);
    if (affected_by_spell(ch, ZEN_CELERITY))
        acc_strcat("You are under the effects of the zen of celerity.\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_MANA_TAP) && !AFF3_FLAGGED(ch, AFF3_MANA_LEAK))
        acc_strcat(str,
            "You have a direct tap to the spiritual energies of the universe.\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_ENERGY_TAP)
        && !AFF3_FLAGGED(ch, AFF3_ENERGY_LEAK))
        acc_strcat(str,
            "Your body is absorbing physical energy from the universe.\r\n",
            NULL);
    if (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
        acc_strcat("You are perceiving sonic images.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_PROT_HEAT))
        acc_strcat("You are protected from heat.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_RIGHTEOUS_PENETRATION))
        acc_strcat("You feel overwhelmingly righteous!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_PRAY))
        acc_strcat("You feel guided by divine forces.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_BLESS))
        acc_strcat("You feel blessed.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DEATH_KNELL))
        acc_strcat("You feel giddy from the absorption of a life.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_MALEFIC_VIOLATION))
        acc_strcat("You feel overwhelmingly wicked!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_MANA_SHIELD))
        acc_strcat("You are protected by a mana shield.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS))
        acc_strcat("You are surrounded by a shield of righteousness.\r\n",
            NULL);
    if (affected_by_spell(ch, SPELL_ANTI_MAGIC_SHELL))
        acc_strcat("You are enveloped in an anti-magic shell.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_SANCTIFICATION))
        acc_strcat("You have been sanctified!\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DIVINE_INTERVENTION))
        acc_strcat("You are shielded by divine intervention.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_SPHERE_OF_DESECRATION))
        acc_strcat(str,
            "You are surrounded by a black sphere of desecration.\r\n", NULL);

    /* psionic affections */
    if (affected_by_spell(ch, SPELL_POWER))
        acc_strcat("Your physical strength is augmented.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_INTELLECT))
        acc_strcat("Your mental faculties are augmented.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_NOPAIN))
        acc_strcat("Your mind is ignoring pain.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_RETINA))
        acc_strcat("Your retina is especially sensitive.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_ADRENAL_MAXIMIZER))
        acc_strcat("Shukutei Adrenal Maximizations are active.\r\n", NULL);
    else if (AFF_FLAGGED(ch, AFF_ADRENALINE))
        acc_strcat("Your adrenaline is pumping.\r\n", NULL);
    if (AFF_FLAGGED(ch, AFF_CONFIDENCE))
        acc_strcat("You feel very confident.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DERMAL_HARDENING))
        acc_strcat("Your dermal surfaces are hardened.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_LATTICE_HARDENING))
        acc_strcat("Your molecular lattice has been strengthened.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_NOBREATHE))
        acc_strcat("You are not breathing.\r\n", NULL);
    if (AFF3_FLAGGED(ch, AFF3_PSISHIELD))
        acc_strcat("You are protected by a psionic shield.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_METABOLISM))
        acc_strcat("Your metabolism is racing.\r\n", NULL);
    else if (affected_by_spell(ch, SPELL_RELAXATION))
        acc_strcat("You feel very relaxed.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_ENDURANCE))
        acc_strcat("Your endurance is increased.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_CAPACITANCE_BOOST))
        acc_strcat("Your energy capacitance is boosted.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
        acc_strcat("Your mind is resistant to external energies.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_PSYCHIC_FEEDBACK))
        acc_strcat("You are providing psychic feedback to your attackers.\r\n",
            NULL);

    /* physic affects */
    if (AFF3_FLAGGED(ch, AFF3_ATTRACTION_FIELD))
        acc_strcat("You are emitting an attraction field.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_REPULSION_FIELD))
        acc_strcat("You are emitting a repulsion field.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_VACUUM_SHROUD))
        acc_strcat("You are existing in a total vacuum.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_CHEMICAL_STABILITY))
        acc_strcat("You feel chemically inert.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_ALBEDO_SHIELD))
        acc_strcat("You are protected from electromagnetic attacks.\r\n",
            NULL);
    if (affected_by_spell(ch, SPELL_GAUSS_SHIELD))
        acc_strcat("You feel somewhat protected from metal.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DIMENSIONAL_SHIFT))
        acc_strcat("You are traversing a parallel dimension.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DIMENSIONAL_VOID))
        acc_strcat
            ("You are disoriented from your foray into the interdimensional void!\r\n",
            NULL);

    /*cyborg */
    if (AFF3_FLAGGED(ch, AFF3_DAMAGE_CONTROL))
        acc_strcat("Your Damage Control process is running.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_DEFENSIVE_POS))
        acc_strcat("You are postured defensively.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_OFFENSIVE_POS))
        acc_strcat("You are postured offensively.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_NEURAL_BRIDGING))
        acc_strcat("Your neural pathways have been bridged.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC))
        acc_strcat("Melee Combat Tactics are in effect.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_REFLEX_BOOST))
        acc_strcat("Your Reflex Boosters are active.\r\n", NULL);

    if (AFF3_FLAGGED(ch, AFF3_SHROUD_OBSCUREMENT))
        acc_strcat(str,
            "You are surrounded by an magical obscurement shroud.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_DETECT_SCRYING))
        acc_strcat("You are sensitive to attempts to magical scrying.\r\n",
            NULL);
    if (affected_by_spell(ch, SKILL_ELUSION))
        acc_strcat("You are attempting to hide your tracks.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_TELEPATHY))
        acc_strcat("Your telepathic abilities are greatly enhanced.\r\n",
            NULL);
    if (affected_by_spell(ch, SPELL_ANIMAL_KIN))
        acc_strcat("You are feeling a strong bond with animals.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_THORN_SKIN))
        acc_strcat("There are thorns protruding from your skin.\r\n", NULL);
    if (affected_by_spell(ch, SKILL_NANITE_RECONSTRUCTION))
        acc_strcat("Your implants are undergoing nanite reconstruction.\r\n",
            NULL);
    if (AFF2_FLAGGED(ch, AFF2_PROT_RAD))
        acc_strcat("You are immune to the effects of radiation.\r\n", NULL);

    /* bard affects */
    if (affected_by_spell(ch, SONG_MISDIRECTION_MELISMA))
        acc_strcat("Your path is cloaked in the tendrils of song.\r\n", NULL);
    if (affected_by_spell(ch, SONG_ARIA_OF_ARMAMENT))
        acc_strcat("You feel protected by song.\r\n", NULL);
    if (affected_by_spell(ch, SONG_MELODY_OF_METTLE))
        acc_strcat("Your vitality is boosted by the Melody of Mettle.\r\n",
            NULL);
    if (affected_by_spell(ch, SONG_DEFENSE_DITTY))
        acc_strcat
            ("Harmonic resonance protects you from deleterious effects.\r\n",
            NULL);
    if (affected_by_spell(ch, SONG_ALRONS_ARIA))
        acc_strcat("Alron guides your hands.\r\n", NULL);
    if (affected_by_spell(ch, SONG_VERSE_OF_VALOR))
        acc_strcat("The spirit of fallen heroes fills your being.\r\n", NULL);
    if (affected_by_spell(ch, SONG_DRIFTERS_DITTY))
        acc_strcat("A pleasant tune gives you a pep in your step.\r\n", NULL);
    if (affected_by_spell(ch, SONG_CHANT_OF_LIGHT))
        acc_strcat("You are surrounded by a warm glow.\r\n", NULL);
    if (affected_by_spell(ch, SONG_ARIA_OF_ASYLUM))
        acc_strcat("You are enveloped by a gossamer shield.\r\n", NULL);
    if (affected_by_spell(ch, SONG_RHYTHM_OF_RAGE))
        acc_strcat("You are consumed by a feral rage!\r\n", NULL);
    if (affected_by_spell(ch, SONG_POWER_OVERTURE))
        acc_strcat("Your strength is bolstered by song.\r\n", NULL);
    if (affected_by_spell(ch, SONG_GUIHARIAS_GLORY))
        acc_strcat("The power of deities is rushing through your veins.\r\n",
            NULL);
    if ((af = affected_by_spell(ch, SONG_MIRROR_IMAGE_MELODY)))
        acc_strcat(tmp_sprintf
            ("You are being accompanied by %d mirror image%s.\r\n",
                af->modifier, af->modifier > 1 ? "s" : ""), NULL);
    if (affected_by_spell(ch, SONG_UNLADEN_SWALLOW_SONG))
        acc_strcat("You are under the effect of an uplifting tune!\r\n", NULL);
    if (affected_by_spell(ch, SONG_IRRESISTABLE_DANCE))
        acc_strcat("You are feet are dancing out of your control!\r\n", NULL);
    if (affected_by_spell(ch, SONG_WEIGHT_OF_THE_WORLD))
        acc_strcat
            ("The weight of the world rests lightly upon your shoulders.\r\n",
            NULL);
    if (affected_by_spell(ch, SONG_EAGLES_OVERTURE))
        acc_strcat("Other are impressed by your beautiful voice.\r\n", NULL);
    if (affected_by_spell(ch, SONG_FORTISSIMO))
        acc_strcat("Your voice reverberates with vigor!\r\n", NULL);
    if (affected_by_spell(ch, SONG_REGALERS_RHAPSODY))
        acc_strcat("A tune has soothed your hunger and thirst.\r\n", NULL);
    if (affected_by_spell(ch, SONG_WOUNDING_WHISPERS))
        acc_strcat("You are surrounded by whirling slivers of sound.\r\n",
            NULL);

}

ACMD(do_affects)
{
    acc_string_clear();
    acc_strcat("Current affects:\r\n", NULL);
    acc_append_affects(ch, 0);
    if (strlen(acc_get_string()) == 18)
        send_to_char(ch, "You feel pretty normal.\r\n");
    else
        page_string(ch->desc, acc_get_string());
}

ACMD(do_gen_points)
{
    char cbuf[MAX_INPUT_LENGTH];

    switch (subcmd) {
    case SCMD_ALIGNMENT:
        if (GET_ALIGNMENT(ch) < -300) {
            sprintf(cbuf, "%s", CCRED(ch, C_NRM));
        } else if (GET_ALIGNMENT(ch) > 300) {
            sprintf(cbuf, "%s", CCCYN(ch, C_NRM));
        } else {
            sprintf(cbuf, "%s", CCYEL(ch, C_NRM));
        }
        send_to_char(ch, "%sYour alignment is%s %s%d%s.\r\n", CCWHT(ch, C_NRM),
                     CCNRM(ch, C_NRM), cbuf, GET_ALIGNMENT(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_ARMOR:
        send_to_char(ch, "Your armor class is %s%d%s.\r\n",
                     CCGRN(ch, C_NRM), GET_AC(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_EXPERIENCE:
        if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
            send_to_char(ch, "You are very experienced.\r\n");
            return;
        }
        send_to_char(ch, "You need %s%'d%s exp to level.\r\n",
                     CCCYN(ch, C_NRM), ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)),
                     CCNRM(ch, C_NRM));
        break;
    case SCMD_HITPOINTS:
        send_to_char(ch, "You have %s%d%s out of %s%d%s hitpoints.\r\n",
                     CCGRN(ch, C_NRM), GET_HIT(ch), CCNRM(ch, C_NRM),
                     CCGRN(ch, C_NRM), GET_MAX_HIT(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_LIFEPOINTS:
        send_to_char(ch, "You have %s%d%s life points.\r\n",
                     CCCYN(ch, C_NRM), GET_LIFE_POINTS(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_MANA:
        send_to_char(ch, "You have %s%d%s out of %s%d%s mana.\r\n",
                     CCMAG(ch, C_NRM), GET_MANA(ch), CCNRM(ch, C_NRM),
                     CCMAG(ch, C_NRM), GET_MAX_MANA(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_MOVE:
        send_to_char(ch, "You have %s%d%s out of %s%d%s move.\r\n",
                     CCCYN(ch, C_NRM), GET_MOVE(ch), CCNRM(ch, C_NRM),
                     CCCYN(ch, C_NRM), GET_MAX_MOVE(ch), CCNRM(ch, C_NRM));
        break;
    case SCMD_REPUTATION:
        strcpy(cbuf, reputation_msg[GET_REPUTATION_RANK(ch)]);
        send_to_char(ch, "You have %s %s reputation.\r\n", AN(cbuf), cbuf);
        break;
    default:
        errlog("Bad do_gen_points subcmd %d", subcmd);
        send_to_char(ch, "You don't seem to be able to do that.\r\n");
    }
}

void
do_blind_score(struct creature *ch)
{
	struct time_info_data playing_time;
	struct time_info_data real_time_passed(time_t t2, time_t t1);
	int get_hunted_id(int hunter_id);

    acc_string_clear();
    if (IS_PC(ch)) {
        acc_sprintf("You are known as %s %s\r\n",
                    GET_NAME(ch), GET_TITLE(ch));
    }
    acc_sprintf("You are a %d level, %d year old %s %s %s.\r\n",
                GET_LEVEL(ch), GET_AGE(ch), genders[(int)GET_SEX(ch)],
                (GET_RACE(ch) >= 0 && GET_RACE(ch) < NUM_RACES) ?
                race_name_by_idnum(GET_RACE(ch)) : "BAD RACE",
                (GET_CLASS(ch) >= 0 && GET_CLASS(ch) < TOP_CLASS) ?
                class_names[(int)GET_CLASS(ch)] : "BAD CLASS");
	if (!IS_NPC(ch) && IS_REMORT(ch))
		acc_sprintf("You have remortalized as a %s (generation %d).\r\n",
			class_names[(int)GET_REMORT_CLASS(ch)],
			GET_REMORT_GEN(ch));
	if ((age(ch).month == 0) && (age(ch).day == 0))
		acc_strcat("  It's your birthday today!\r\n\r\n", NULL);
	else
		acc_strcat("\r\n", NULL);

	acc_sprintf("Hit Points:  (%4d/%4d)\r\n",
                GET_HIT(ch), GET_MAX_HIT(ch));
	acc_sprintf("Mana Points: (%4d/%4d)\r\n",
                GET_MANA(ch), GET_MAX_MANA(ch));
	acc_sprintf("Move Points: (%4d/%4d)\r\n",
                GET_MOVE(ch), GET_MAX_MOVE(ch));
	acc_sprintf("Armor Class: %d/10, ", GET_AC(ch));
	acc_sprintf("Alignment: %d, ", GET_ALIGNMENT(ch));
	acc_sprintf("Experience: %'d\r\n", GET_EXP(ch));
	acc_sprintf("Kills: %'d, PKills: %'d, Arena: %'d\r\n",
                (GET_MOBKILLS(ch) + GET_PKILLS(ch) + GET_ARENAKILLS(ch)),
                GET_PKILLS(ch),
                GET_ARENAKILLS(ch));
	acc_sprintf("Life points: %d\r\n", GET_LIFE_POINTS(ch));

    acc_sprintf("You are %s tall, and weigh %s.\r\n",
                format_distance(GET_HEIGHT(ch), USE_METRIC(ch)),
                format_weight(GET_WEIGHT(ch), USE_METRIC(ch)));
	if (IS_PC(ch)) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			acc_sprintf("Next level in %'d more experience\r\n",
                        ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)));
		} else {
			if (ch->player_specials->poofout) {
				acc_sprintf("%sPoofout:%s  %s\r\n",
					CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
					ch->player_specials->poofout);
			}
			if (ch->player_specials->poofin) {
				acc_sprintf("%sPoofin :%s  %s\r\n",
					CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
					ch->player_specials->poofin);
			}
		}
		playing_time = real_time_passed((time(NULL) - ch->player.time.logon) +
                                        ch->player.time.played, 0);
		acc_sprintf("Playing time: %d days and %d hours.\r\n",
                    playing_time.day, playing_time.hours);

		acc_strcat("You have a reputation of being -",
			reputation_msg[GET_REPUTATION_RANK(ch)], "-\r\n", NULL);
		if (get_hunted_id(GET_IDNUM(ch)))
			acc_sprintf("You are registered to bounty hunt %s.\r\n",
				player_name_by_idnum(get_hunted_id(GET_IDNUM(ch))));

	}
	acc_sprintf("You are currently speaking %s.\r\n",
                tongue_name(GET_TONGUE(ch)));
	acc_sprintf(
		"You carry %s%'" PRId64 "%s gold coins.  You have %s%'" PRId64 "%s cash credits.\r\n",
		CCCYN(ch, C_NRM), GET_GOLD(ch), CCNRM(ch, C_NRM), CCCYN(ch,
			C_NRM), GET_CASH(ch), CCNRM(ch, C_NRM));

	switch (GET_POSITION(ch)) {
	case POS_DEAD:
		acc_strcat("You are DEAD!\r\n", NULL);
		break;
	case POS_MORTALLYW:
		acc_strcat("You are mortally wounded!  You should seek help!\r\n", NULL);
		break;
	case POS_INCAP:
		acc_strcat("You are incapacitated, slowly fading away...\r\n", NULL);
		break;
	case POS_STUNNED:
		acc_strcat("You are stunned!  You can't move!\r\n", NULL);
		break;
	case POS_SLEEPING:
		if (AFF3_FLAGGED(ch, AFF3_STASIS))
			acc_strcat("You are inactive in a static state.\r\n", NULL);
		else
			acc_strcat("You are sleeping.\r\n", NULL);
		break;
	case POS_RESTING:
		acc_strcat("You are resting.\r\n", NULL);
		break;
	case POS_SITTING:
		if (AFF2_FLAGGED(ch, AFF2_MEDITATE))
			acc_strcat("You are meditating.\r\n", NULL);
		else
			acc_strcat("You are sitting.\r\n", NULL);
		break;
	case POS_FIGHTING:
		if (is_fighting(ch))
			acc_strcat("You are fighting \r\n", CCNRM(ch, C_NRM), "\r\n", NULL);
		else
			acc_strcat("You are fighting thin air.\r\n", NULL);
		break;
	case POS_MOUNTED:
		if (GET_POSITION(ch) == POS_MOUNTED)
			acc_strcat("You are mounted on \r\n", CCNRM(ch, C_NRM), "\r\n", NULL);
		else
			acc_strcat("You are mounted on the thin air!?\r\n", NULL);
		break;
	case POS_STANDING:
		acc_strcat("You are standing.\r\n", NULL);
		break;
	case POS_FLYING:
		acc_strcat("You are hovering in midair.\r\n", NULL);
		break;
	default:
		acc_strcat("You are floating.\r\n", NULL);
		break;
	}

	if (GET_COND(ch, DRUNK) > 10)
		acc_strcat("You are intoxicated\r\n", NULL);
	if (GET_COND(ch, FULL) == 0)
		acc_strcat("You are hungry.\r\n", NULL);
	if (GET_COND(ch, THIRST) == 0) {
		if (IS_VAMPIRE(ch))
			acc_strcat("You have an intense thirst for blood.\r\n", NULL);
		else
			acc_strcat("You are thirsty.\r\n", NULL);
	} else if (IS_VAMPIRE(ch) && GET_COND(ch, THIRST) < 4)
		acc_strcat("You feel the onset of your bloodlust.\r\n", NULL);

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR && PLR_FLAGGED(ch, PLR_MORTALIZED))
		acc_strcat("You are mortalized.\r\n", NULL);

	page_string(ch->desc, acc_get_string());
}

ACMD(do_score)
{

    struct time_info_data playing_time;
    struct time_info_data real_time_passed(time_t t2, time_t t1);
    int get_hunted_id(int hunter_id);

    if (ch->desc && ch->desc->is_blind) {
        do_blind_score(ch);
        return;
    }

    acc_string_clear();
    acc_sprintf
        ("%s*****************************************************************\r\n",
        CCRED(ch, C_NRM));
    acc_sprintf
        ("%s***************************%sS C O R E%s%s*****************************\r\n",
        CCYEL(ch, C_NRM), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCYEL(ch,
            C_NRM));
    acc_sprintf
        ("%s*****************************************************************%s\r\n",
        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

    acc_sprintf("%s, %d year old %s %s %s.  Your level is %d.\r\n",
        GET_NAME(ch),
        GET_AGE(ch), genders[(int)GET_SEX(ch)],
        (GET_RACE(ch) >= 0 && GET_RACE(ch) < NUM_RACES) ?
        race_name_by_idnum(GET_RACE(ch)) : "BAD RACE",
        (GET_CLASS(ch) >= 0 && GET_CLASS(ch) < TOP_CLASS) ?
        class_names[(int)GET_CLASS(ch)] : "BAD CLASS", GET_LEVEL(ch));
    if (!IS_NPC(ch) && IS_REMORT(ch))
        acc_sprintf("You have remortalized as a %s (generation %d).\r\n",
            class_names[(int)GET_REMORT_CLASS(ch)], GET_REMORT_GEN(ch));
    if ((age(ch).month == 0) && (age(ch).day == 0))
        acc_strcat("  It's your birthday today!\r\n\r\n", NULL);
    else
        acc_strcat("\r\n", NULL);

    acc_sprintf("Hit Points:  %11s           Armor Class: %s%d/10%s\r\n",
        tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_HIT(ch),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_HIT(ch),
            CCNRM(ch, C_NRM)), CCGRN(ch, C_NRM), GET_AC(ch), CCNRM(ch, C_NRM));
    acc_sprintf("Mana Points: %11s           Alignment: %s%d%s\r\n",
        tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MANA(ch),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(ch),
            CCNRM(ch, C_NRM)),
        CCGRN(ch, C_NRM), GET_ALIGNMENT(ch), CCNRM(ch, C_NRM));
    acc_sprintf("Move Points: %11s           Experience: %s%'d%s\r\n",
        tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MOVE(ch),
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MOVE(ch),
            CCNRM(ch, C_NRM)),
        CCGRN(ch, C_NRM), GET_EXP(ch), CCNRM(ch, C_NRM));
    acc_sprintf
        ("                                   %sKills%s: %'d, %sPKills%s: %'d, %sArena%s: %'d\r\n",
        CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
        (GET_MOBKILLS(ch) + GET_PKILLS(ch) + GET_ARENAKILLS(ch)), CCRED(ch,
            C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(ch), CCGRN(ch, C_NRM),
        CCNRM(ch, C_NRM), GET_ARENAKILLS(ch));

    acc_sprintf
        ("%s*****************************************************************%s\r\n",
        CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    acc_sprintf("You have %s%d%s life points.\r\n", CCCYN(ch, C_NRM),
        GET_LIFE_POINTS(ch), CCNRM(ch, C_NRM));

    acc_sprintf("You are %s%s%s tall, and weigh %s%s%s.\r\n",
                CCCYN(ch, C_NRM), format_distance(GET_HEIGHT(ch), USE_METRIC(ch)), CCNRM(ch, C_NRM),
                CCCYN(ch, C_NRM), format_weight(GET_WEIGHT(ch), USE_METRIC(ch)), CCNRM(ch, C_NRM));

    if (!IS_NPC(ch)) {
        if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
            acc_sprintf("You need %s%'d%s exp to reach your next level.\r\n",
                CCCYN(ch, C_NRM),
                ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)), CCNRM(ch,
                    C_NRM));
        } else {
            if (ch->player_specials->poofout) {
                acc_sprintf("%sPoofout:%s  %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    ch->player_specials->poofout);
            }
            if (ch->player_specials->poofin) {
                acc_sprintf("%sPoofin :%s  %s\r\n",
                    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                    ch->player_specials->poofin);
            }
        }
        playing_time = real_time_passed((time(NULL) - ch->player.time.logon) +
            ch->player.time.played, 0);
        acc_sprintf("You have existed here for %d days and %d hours.\r\n",
            playing_time.day, playing_time.hours);

        acc_sprintf("You are known as %s%s%s.%s\r\n",
            CCYEL(ch, C_NRM), GET_NAME(ch), GET_TITLE(ch), CCNRM(ch, C_NRM));
        acc_strcat("You have a reputation of being -",
            reputation_msg[GET_REPUTATION_RANK(ch)], "-\r\n", NULL);
        if (get_hunted_id(GET_IDNUM(ch)))
            acc_sprintf("You are registered to bounty hunt %s.\r\n",
                player_name_by_idnum(get_hunted_id(GET_IDNUM(ch))));

    }
    acc_sprintf("You are currently speaking %s.\r\n",
        tongue_name(GET_TONGUE(ch)));
    acc_sprintf
        ("You carry %s%'" PRId64 "%s gold coins.  You have %s%'" PRId64 "%s cash credits.\r\n",
        CCCYN(ch, C_NRM), GET_GOLD(ch), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
        GET_CASH(ch), CCNRM(ch, C_NRM));

    switch (GET_POSITION(ch)) {
    case POS_DEAD:
        acc_strcat(CCRED(ch, C_NRM), "You are DEAD!", CCNRM(ch, C_NRM),
            "\r\n", NULL);
        break;
    case POS_MORTALLYW:
        acc_strcat(CCRED(ch, C_NRM),
            "You are mortally wounded!  You should seek help!",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_INCAP:
        acc_strcat(CCRED(ch, C_NRM),
            "You are incapacitated, slowly fading away...", CCNRM(ch, C_NRM),
            "\r\n", NULL);
        break;
    case POS_STUNNED:
        acc_strcat(CCRED(ch, C_NRM), "You are stunned!  You can't move!",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_SLEEPING:
        if (AFF3_FLAGGED(ch, AFF3_STASIS))
            acc_strcat(CCGRN(ch, C_NRM), "You are inactive in a static state.",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        else
            acc_strcat(CCGRN(ch, C_NRM), "You are sleeping.",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_RESTING:
        acc_strcat(CCGRN(ch, C_NRM), "You are resting.",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_SITTING:
        if (AFF2_FLAGGED(ch, AFF2_MEDITATE))
            acc_strcat(CCGRN(ch, C_NRM), "You are meditating.",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        else
            acc_strcat(CCGRN(ch, C_NRM), "You are sitting.",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_FIGHTING:
        if ((is_fighting(ch)))
            acc_strcat(CCYEL(ch, C_NRM),
                "You are fighting ", PERS(random_opponent(ch), ch), ".",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        else
            acc_strcat(CCYEL(ch, C_NRM),
                "You are fighting thin air.", CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_MOUNTED:
        if (MOUNTED_BY(ch))
            acc_strcat(CCGRN(ch, C_NRM), "You are mounted on ",
                PERS(MOUNTED_BY(ch), ch), ".", CCNRM(ch, C_NRM), "\r\n", NULL);
        else
            acc_strcat(CCGRN(ch, C_NRM), "You are mounted on the thin air!?",
                CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_STANDING:
        acc_strcat(CCGRN(ch, C_NRM), "You are standing.",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    case POS_FLYING:
        acc_strcat(CCGRN(ch, C_NRM), "You are hovering in midair.",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    default:
        acc_strcat(CCGRN(ch, C_NRM), "You are floating.",
            CCNRM(ch, C_NRM), "\r\n", NULL);
        break;
    }

    if (GET_COND(ch, DRUNK) > 10)
        acc_strcat("You are intoxicated\r\n", NULL);
    if (GET_COND(ch, FULL) == 0)
        acc_strcat("You are hungry.\r\n", NULL);
    if (GET_COND(ch, THIRST) == 0) {
        if (IS_VAMPIRE(ch))
            acc_strcat("You have an intense thirst for blood.\r\n", NULL);
        else
            acc_strcat("You are thirsty.\r\n", NULL);
    } else if (IS_VAMPIRE(ch) && GET_COND(ch, THIRST) < 4)
        acc_strcat("You feel the onset of your bloodlust.\r\n", NULL);

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR && PLR_FLAGGED(ch, PLR_MORTALIZED))
        acc_strcat("You are mortalized.\r\n", NULL);

    acc_append_affects(ch, PRF2_FLAGGED(ch, PRF2_NOAFFECTS));

    page_string(ch->desc, acc_get_string());
}

ACMD(do_inventory)
{
    acc_string_clear();
    acc_sprintf("You are carrying:\r\n");
    list_obj_to_char(ch->carrying, ch, SHOW_OBJ_INV, true);
    page_string(ch->desc, acc_get_string());
}

ACMD(do_equipment)
{
    int idx, pos;
    bool found = false;
    struct obj_data *obj = NULL;
    const char *str;
    const char *active_buf[2] = { "(inactive)", "(active)" };
    bool show_all = false;
    skip_spaces(&argument);

    acc_string_clear();

    if (subcmd == SCMD_EQ) {
        if (*argument && is_abbrev(argument, "status")) {
            acc_sprintf("Equipment status:\r\n");
            for (idx = 0; idx < NUM_WEARS; idx++) {
                pos = eq_pos_order[idx];

                if (!(obj = GET_EQ(ch, pos)))
                    continue;
                found = 1;
                acc_sprintf("-%s- is in %s condition.\r\n",
                            obj->name, obj_cond_color(obj, COLOR_LEV(ch)));
            }
            if (found)
                page_string(ch->desc, acc_get_string());
            else
                send_to_char(ch, "You're totally naked!\r\n");
            return;
        } else if (*argument && is_abbrev(argument, "all")) {
            show_all = true;
        }

        if (show_all) {
            acc_sprintf("You are using:\r\n");
        }
        for (idx = 0; idx < NUM_WEARS; idx++) {
            pos = eq_pos_order[idx];
            if (GET_EQ(ch, pos)) {
                if (!found && !show_all) {
                    acc_sprintf("You are using:\r\n");
                    found = true;
                }
                if (can_see_object(ch, GET_EQ(ch, pos))) {
                    acc_sprintf("%s%s%s", CCGRN(ch, C_NRM),
                        where[pos], CCNRM(ch, C_NRM));
                    show_obj_to_char(GET_EQ(ch, pos), ch, SHOW_OBJ_INV, 0);
                } else {
                    acc_sprintf("%sSomething.\r\n", where[pos]);
                }
            } else if (show_all && pos != WEAR_ASS) {
                acc_sprintf("%sNothing!\r\n", where[pos]);
            }
        }

        if (!found && !show_all) {
            acc_sprintf("You're totally naked!\r\n");
        }
    } else if (subcmd == SCMD_TATTOOS) {
        show_all = (*argument && is_abbrev(argument, "all"));

        if (show_all)
            acc_sprintf("You have the following tattoos:\r\n");
        for (idx = 0; idx < NUM_WEARS; idx++) {
            pos = tattoo_pos_order[idx];
            if (ILLEGAL_TATTOOPOS(pos))
                continue;

            if (GET_TATTOO(ch, pos)) {
                if (!found && !show_all) {
                    acc_sprintf("You have the following tattoos:\r\n");
                    found = true;
                }
                acc_sprintf("%s%s%s", CCGRN(ch, C_NRM),
                    tattoo_pos_descs[pos], CCNRM(ch, C_NRM));
                show_obj_to_char(GET_TATTOO(ch, pos), ch, SHOW_OBJ_INV, 0);
            } else if (show_all) {
                acc_sprintf("%sNothing!\r\n", where[pos]);
            }
        }

        if (!found && !show_all)
            acc_sprintf("You're a tattoo virgin!\r\n");
    } else if (subcmd == SCMD_IMPLANTS) {
        if (*argument && is_abbrev(argument, "status")) {
            acc_sprintf("Implant status:\r\n");
            for (idx = 0; idx < NUM_WEARS; idx++) {
                pos = implant_pos_order[idx];

                if (!(obj = GET_IMPLANT(ch, pos)))
                    continue;
                found = true;
                acc_sprintf("-%s- is in %s condition.\r\n",
                            obj->name, obj_cond_color(obj, COLOR_LEV(ch)));
            }
            if (found)
                page_string(ch->desc, acc_get_string());
            else
                send_to_char(ch, "You don't have any implants.\r\n");
            return;
        } else if (*argument && is_abbrev(argument, "all")) {
            show_all = true;
        }

        if (show_all) {
            acc_sprintf("You are implanted with:\r\n");
        }
        for (idx = 0; idx < NUM_WEARS; idx++) {
            pos = implant_pos_order[idx];
            if (ILLEGAL_IMPLANTPOS(pos))
                continue;

            if (GET_IMPLANT(ch, pos)) {
                if (!found && !show_all) {
                    acc_sprintf("You are implanted with:\r\n");
                    found = 1;
                }
                if (can_see_object(ch, GET_IMPLANT(ch, pos))) {

                    if (IS_DEVICE(GET_IMPLANT(ch, pos)) ||
                        IS_COMMUNICATOR(GET_IMPLANT(ch, pos)))
                        str = tmp_sprintf(" %10s %s(%s%'d%s/%s%'d%s)%s",
                            active_buf[(ENGINE_STATE(GET_IMPLANT(ch,
                                            pos))) ? 1 : 0], CCGRN(ch, C_NRM),
                            CCNRM(ch, C_NRM), CUR_ENERGY(GET_IMPLANT(ch, pos)),
                            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                            MAX_ENERGY(GET_IMPLANT(ch, pos)), CCGRN(ch, C_NRM),
                            CCNRM(ch, C_NRM));
                    else
                        str = "";

                    acc_sprintf("%s[%12s]%s - %-30s%s\r\n",
                        CCCYN(ch, C_NRM), wear_implantpos[pos],
                        CCNRM(ch, C_NRM), GET_IMPLANT(ch, pos)->name, str);
                } else {
                    acc_sprintf("%s[%12s]%s - (UNKNOWN)\r\n", CCCYN(ch,
                            C_NRM), wear_implantpos[pos], CCNRM(ch, C_NRM));
                }
            } else if (show_all && pos != WEAR_ASS && pos != WEAR_HOLD) {
                acc_sprintf("[%12s] - Nothing!\r\n", wear_implantpos[pos]);
            }
        }
        if (!found && !show_all) {
            acc_sprintf("You don't have any implants.\r\n");
        }
    }

    page_string(ch->desc, acc_get_string());
}

void set_local_time(struct zone_data *zone, struct time_info_data *local_time);

ACMD(do_time)
{
    const char *suf;
    int weekday, day;
    struct time_info_data local_time;
    extern const char *weekdays[7];
    extern const char *month_name[16];

    if (ch->in_room->zone->time_frame == TIME_TIMELESS) {
        send_to_char(ch, "Time has no meaning here.\r\n");
        return;
    }

    set_local_time(ch->in_room->zone, &local_time);

    send_to_char(ch, "It is %d o'clock %s, on ",
        ((local_time.hours % 12 == 0) ? 12 : ((local_time.hours) % 12)),
        ((local_time.hours >= 12) ? "pm" : "am"));

    /* 35 days in a month */
    weekday = ((35 * local_time.month) + local_time.day + 1) % 7;

    strcat(buf, weekdays[weekday]);
    strcat(buf, "\r\n");

    day = local_time.day + 1;   /* day in [1..35] */

    if (day == 1)
        suf = "st";
    else if (day == 2)
        suf = "nd";
    else if (day == 3)
        suf = "rd";
    else if (day < 20)
        suf = "th";
    else if ((day % 10) == 1)
        suf = "st";
    else if ((day % 10) == 2)
        suf = "nd";
    else if ((day % 10) == 3)
        suf = "rd";
    else
        suf = "th";

    send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
        day, suf, month_name[(int)local_time.month],
        ch->in_room->zone->time_frame == TIME_MODRIAN ? local_time.year :
        ch->in_room->zone->time_frame == TIME_ELECTRO ? local_time.year +
        3567 : local_time.year);

    send_to_char(ch, "The moon is currently %s.\r\n",
        lunar_phases[get_lunar_phase(lunar_day)]);
}

void
show_mud_date_to_char(struct creature *ch)
{
    const char *suf;
    struct time_info_data local_time;
    extern const char *month_name[16];
    int day;

    if (!ch->in_room) {
        errlog("!ch->in_room in show_mud_date_to_char");
        return;
    }

    set_local_time(ch->in_room->zone, &local_time);

    day = local_time.day + 1;   /* day in [1..35] */

    if (day == 1)
        suf = "st";
    else if (day == 2)
        suf = "nd";
    else if (day == 3)
        suf = "rd";
    else if (day < 20)
        suf = "th";
    else if ((day % 10) == 1)
        suf = "st";
    else if ((day % 10) == 2)
        suf = "nd";
    else if ((day % 10) == 3)
        suf = "rd";
    else
        suf = "th";

    send_to_char(ch, "It is the %d%s Day of the %s, Year %d.\r\n",
        day, suf, month_name[(int)local_time.month], local_time.year);

}

ACMD(do_weather)
{
    static const char *sky_look[] = {
        "cloudless",
        "cloudy",
        "rainy",
        "lit by flashes of lightning"
    };

    if (OUTSIDE(ch) && !ZONE_FLAGGED(ch->in_room->zone, ZONE_NOWEATHER)) {
        send_to_char(ch, "The sky is %s and %s.\r\n",
            sky_look[(int)ch->in_room->zone->weather->sky],
            (ch->in_room->zone->weather->change >= 0 ?
                "you feel a warm wind from the south" :
                "your foot tells you bad weather is due"));
        if (ch->in_room->zone->weather->sky < SKY_RAINING) {
            send_to_char(ch, "The %s moon is %s%s.\r\n",
                lunar_phases[get_lunar_phase(lunar_day)],
                (ch->in_room->zone->weather->moonlight &&
                    ch->in_room->zone->weather->moonlight < MOON_SKY_SET) ?
                "visible " : "",
                moon_sky_types[(int)ch->in_room->zone->weather->moonlight]);
        }
    } else {
        send_to_char(ch, "You have no feeling about the weather at all.\r\n");
    }
}

//generates a formatted string representation of a player for the who list
void
who_string(struct creature *ch, struct creature *target)
{
    int len = strlen(BADGE(target));

    //show badge
    if (GET_LEVEL(target) >= LVL_AMBASSADOR) {
        acc_sprintf("%s%s[%s%s%s%s%s]",
            CCBLD(ch, C_NRM), CCYEL(ch, C_NRM), CCGRN(ch, C_NRM),
            tmp_pad(' ', (MAX_BADGE_LENGTH - len) / 2),
            BADGE(target),
            tmp_pad(' ', (MAX_BADGE_LENGTH - len + 1) / 2), CCYEL(ch, C_NRM));
    } else if (is_tester(target)) {
        acc_sprintf("%s%s[%sTESTING%s]",
            CCBLD(ch, C_NRM), CCYEL(ch, C_NRM), CCGRN(ch, C_NRM),
            CCYEL(ch, C_NRM));
    } else {
        //show level/class
        if (is_authorized(ch, SEE_FULL_WHOLIST, NULL)) {
            char *col = CCNRM(ch, C_NRM);

            if (PRF2_FLAGGED(target, PRF2_ANONYMOUS))
                col = CCRED(ch, C_NRM);
            acc_sprintf("%s[%s%2d%s(%s%2d%s) ",
                CCGRN(ch, C_NRM), col, GET_LEVEL(target),
                CCCYN(ch, C_NRM),
                CCNRM(ch, C_NRM), GET_REMORT_GEN(target), CCCYN(ch, C_NRM));
        } else if (PRF2_FLAGGED(target, PRF2_ANONYMOUS)) {
            acc_sprintf("%s[%s--", CCGRN(ch, C_NRM), CCCYN(ch, C_NRM));
        } else {
            acc_sprintf("%s[%s%2d",
                CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(target));
        }
        acc_sprintf(" %s%s%s%s]",
            get_char_class_color_code(ch, target, GET_CLASS(target)),
            char_class_abbrevs[(int)GET_CLASS(target)],
            CCNRM(ch, C_NRM), CCGRN(ch, C_NRM));
    }

    // name
    if (GET_LEVEL(target) >= LVL_AMBASSADOR) {
        acc_sprintf("%s%s %s%s",
            CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_NAME(target), GET_TITLE(target));
    } else {
        acc_sprintf("%s %s%s",
            CCNRM(ch, C_NRM), GET_NAME(target), GET_TITLE(target));
    }
}

//generates a formatted string representation of a player for the who list
void
who_flags(struct creature *ch, struct creature *target)
{
    //hardcore
    if (PLR_FLAGGED(target, PLR_HARDCORE)) {
        acc_strcat(CCRED(ch, C_NRM), " (hardcore)", CCNRM(ch, C_NRM), NULL);
    }
    //nowho
    if (PRF2_FLAGGED(target, PRF2_NOWHO)) {
        acc_strcat(CCRED(ch, C_NRM), " (nowho)", CCNRM(ch, C_NRM), NULL);
    }
    //clan badge
    if (real_clan(GET_CLAN(target))) {
        if (!PRF2_FLAGGED(target, PRF2_CLAN_HIDE)) {
            acc_sprintf(" %s%s%s",
                CCCYN(ch, C_NRM),
                real_clan(GET_CLAN(target))->badge, CCNRM(ch, C_NRM));
        } else if (is_authorized(ch, SEE_FULL_WHOLIST, NULL)) {
            acc_sprintf(" %s)%s(%s",
                        CCCYN(ch, C_NRM),
                        real_clan(GET_CLAN(target))->name, CCNRM(ch, C_NRM));
        }
    }
    //imm invis
    if (GET_INVIS_LVL(target) && IS_IMMORT(ch)) {
        acc_sprintf(" %s(%s%d%s)",
            CCBLU(ch, C_NRM),
            CCMAG(ch, C_NRM), GET_INVIS_LVL(target), CCBLU(ch, C_NRM));
    }

    if (PLR_FLAGGED(target, PLR_MAILING)) {
        acc_sprintf(" %s(mailing)", CCGRN(ch, C_NRM));
    } else if (PLR_FLAGGED(target, PLR_OLC)) {
        acc_sprintf(" %s(creating)", CCGRN(ch, C_NRM));
    } else if (PLR_FLAGGED(target, PLR_WRITING)) {  //writing
        acc_sprintf(" %s(writing)", CCGRN(ch, C_NRM));
    }
    //deaf
    if (PRF_FLAGGED(target, PRF_DEAF)) {
        acc_sprintf(" %s(deaf)", CCBLU(ch, C_NRM));
    }
    //notell
    if (PRF_FLAGGED(target, PRF_NOTELL)) {
        acc_sprintf(" %s(notell)", CCBLU(ch, C_NRM));
    }
    //questing
    if (GET_QUEST(target)) {
        acc_sprintf(" %s(quest)%s",
                    CCYEL_BLD(ch, C_NRM),
                    CCNRM(ch, C_NRM));
    }
    //afk
    if (PLR_FLAGGED(target, PLR_AFK)) {
        acc_sprintf(" %s(afk)", CCGRN(ch, C_NRM));
    }
}

void
who_kills(struct creature *ch, struct creature *target)
{
    acc_sprintf(" %s*%d KILLS*", CCRED_BLD(ch, C_NRM), GET_PKILLS(target));
    acc_sprintf(" -%s-%s", reputation_msg[GET_REPUTATION_RANK(target)], CCNRM(ch, C_NRM));
}

static int
cmp(int a, int b)
{
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

gint
who_list_compare(struct creature *a, struct creature *b)
{
    time_t now, time_a, time_b;

    // immorts on top by level,
    // then testers, then by gen, then by level
    // then by time played
    if (IS_IMMORT(a) || IS_IMMORT(b))
        return cmp(GET_LEVEL(b), GET_LEVEL(a));
    if (is_tester(a) && is_tester(b))
        return 0;
    if (is_tester(a) && !is_tester(b))
        return -1;
    if (!is_tester(a) && is_tester(b))
        return 1;
    if (GET_REMORT_GEN(a) != GET_REMORT_GEN(b))
        return cmp(GET_REMORT_GEN(b), GET_REMORT_GEN(a));
    if (GET_LEVEL(a) != GET_LEVEL(b))
        return cmp(GET_LEVEL(b), GET_LEVEL(a));

    now = time(NULL);
    time_a = now - a->player.time.logon + a->player.time.played;
    time_b = now - b->player.time.logon + b->player.time.played;
    return cmp(time_b, time_a);
}

ACMD(do_who)
{

    struct descriptor_data *d;
    struct creature *curr;
    int playerTotal = 0, testerTotal = 0, immTotal = 0;
    bool zone = false, plane = false, time = false, kills = false, noflags =
        false;
    bool classes = false, clan = false;
    bool mage = false, thief = false, ranger = false, knight = false, cleric =
        false, barbarian = false;
    bool bard = false, monk = false, physic = false, cyborg = false, psionic =
        false, mercenary = false;
    struct clan_data *real_clan = NULL;
    char *arg;
    GList *immortals = NULL, *testers = NULL, *players = NULL;

    for (arg = tmp_getword(&argument); *arg; arg = tmp_getword(&argument)) {
        if (!strcmp(arg, "zone")) {
            zone = true;
        }
        if (!strcmp(arg, "plane")) {
            plane = true;
        }
        if (!strcmp(arg, "time")) {
            time = true;
        }
        if (!strcmp(arg, "kills")) {
            kills = true;
        }
        if (!strcmp(arg, "noflags")) {
            noflags = true;
        }
        if (!strcmp(arg, "class")) {
            classes = true;
            if (!strcmp(arg, "mag")) {
                mage = true;
            }
            if (!strcmp(arg, "thi")) {
                thief = true;
            }
            if (!strcmp(arg, "ran")) {
                ranger = true;
            }
            if (!strcmp(arg, "kni")) {
                knight = true;
            }
            if (!strcmp(arg, "cle")) {
                cleric = true;
            }
            if (!strcmp(arg, "barb")) {
                barbarian = true;
            }
            if (!strcmp(arg, "bard")) {
                bard = true;
            }
            if (!strcmp(arg, "mon")) {
                monk = true;
            }
            if (!strcmp(arg, "phy")) {
                physic = true;
            }
            if (!strcmp(arg, "cyb") || !strcmp(arg, "borg")) {
                cyborg = true;
            }
            if (!strcmp(arg, "psi")) {
                psionic = true;
            }
            if (!strcmp(arg, "mer")) {
                mercenary = true;
            }
        }
        if (!strcmp(arg, "clan")) {
            arg = tmp_getword(&argument);
            if (arg) {
                real_clan = clan_by_name(arg);
                if (real_clan)
                    clan = true;
            }
        }
    }

    acc_string_clear();

    for (d = descriptor_list; d; d = d->next) {
        if (d->original) {
            curr = d->original;
        } else if (d->creature) {
            curr = d->creature;
        } else {
            continue;
        }

        //Must be in the game.
        if (!curr || !curr->in_room) {
            continue;
        }
        //update the total number of players first
        if (GET_LEVEL(curr) >= LVL_AMBASSADOR) {
            immTotal++;
        } else if (is_tester(curr)) {
            testerTotal++;
        } else  {
            playerTotal++;
        }

        /////////////////BEGIN CONDITION CHECKING//////////////////////
        //zone
        if (zone && ch->in_room->zone != curr->in_room->zone) {
            continue;
        }
        //plane
        if (plane && ch->in_room->zone->plane != curr->in_room->zone->plane) {
            continue;
        }
        //time
        if (time
            && ch->in_room->zone->time_frame !=
            curr->in_room->zone->time_frame) {
            continue;
        }
        //kills
        if (kills && GET_PKILLS(curr) == 0) {
            continue;
        }
        //classes
        if (classes && !((mage && GET_CLASS(curr) == CLASS_MAGE) ||
                (thief && GET_CLASS(curr) == CLASS_THIEF) ||
                (ranger && GET_CLASS(curr) == CLASS_RANGER) ||
                (knight && GET_CLASS(curr) == CLASS_KNIGHT) ||
                (barbarian && GET_CLASS(curr) == CLASS_BARB) ||
                (bard && GET_CLASS(curr) == CLASS_BARD) ||
                (cleric && GET_CLASS(curr) == CLASS_CLERIC) ||
                (monk && GET_CLASS(curr) == CLASS_MONK) ||
                (physic && GET_CLASS(curr) == CLASS_PHYSIC) ||
                (cyborg && GET_CLASS(curr) == CLASS_CYBORG) ||
                (psionic && GET_CLASS(curr) == CLASS_PSIONIC) ||
                (mercenary && GET_CLASS(curr) == CLASS_MERCENARY))) {
            continue;
        }
        //clans
        if (clan && real_clan) {
            //not in clan
            if (real_clan->number != GET_CLAN(curr) ||
                //not able to see the clan
                (PRF2_FLAGGED(curr, PRF2_CLAN_HIDE)
                    && !is_authorized(ch, SEE_FULL_WHOLIST, NULL))) {
                continue;
            }
        }
        //nowho
        if ((PRF2_FLAGGED(curr, PRF2_NOWHO) || PLR_FLAGGED(curr, PLR_AFK))
            && GET_LEVEL(curr) >= LVL_AMBASSADOR
            && GET_LEVEL(ch) < LVL_AMBASSADOR) {
            continue;
        }
        //imm invis
        if (GET_INVIS_LVL(curr) > GET_LEVEL(ch)) {
            continue;
        }
        /////////////////END CONDITIONS/////////////////////////

        if (GET_LEVEL(curr) >= LVL_AMBASSADOR)
            immortals = g_list_prepend(immortals, curr);
        else if (is_tester(curr))
            testers = g_list_prepend(testers, curr);
        else
            players = g_list_prepend(players, curr);
    }

    immortals = g_list_sort(immortals, (GCompareFunc) who_list_compare);
    testers = g_list_sort(testers, (GCompareFunc) who_list_compare);
    players = g_list_sort(players, (GCompareFunc) who_list_compare);

    acc_strcat(CCNRM(ch, C_SPR), CCBLD(ch, C_CMP), "**************      ",
        CCGRN(ch, C_NRM), "Visible Players of TEMPUS",
        CCNRM(ch, C_NRM), CCBLD(ch, C_CMP), "      **************",
        CCNRM(ch, C_SPR), "\r\n",
        (IS_NPC(ch) || ch->account->compact_level > 1) ? "" : "\r\n", NULL);
    for (GList *cit = first_living(immortals); cit; cit = next_living(cit)) {
        curr = (struct creature *)cit->data;
        who_string(ch, curr);
        if (!noflags) {
            who_flags(ch, curr);
        }
        if (kills) {
            who_kills(ch, curr);
        }
        acc_strcat("\r\n", NULL);
    }
    if (IS_IMMORT(ch) || is_tester(ch)) {
        for (GList * cit = first_living(testers); cit; cit = next_living(cit)) {
            curr = (struct creature *)cit->data;
            who_string(ch, curr);
            if (!noflags) {
                who_flags(ch, curr);
            }
            if (kills) {
                who_kills(ch, curr);
            }
            acc_strcat("\r\n", NULL);
        }
    }
    for (GList * cit = first_living(players); cit; cit = next_living(cit)) {
        curr = (struct creature *)cit->data;
        who_string(ch, curr);
        if (!noflags) {
            who_flags(ch, curr);
        }
        if (kills) {
            who_kills(ch, curr);
        }
        acc_strcat("\r\n", NULL);
    }

    if (IS_PC(ch) && ch->account->compact_level <= 1)
        acc_strcat("\r\n", NULL);
    acc_sprintf("%s%d of %d immortal%s, ",
                CCNRM(ch, C_NRM),
                g_list_length(immortals),
                immTotal, (immTotal == 1) ? "" : "s");
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR || is_tester(ch)) {
        acc_sprintf("%d of %d tester%s, ", g_list_length(testers),
            testerTotal, (testerTotal == 1) ? "" : "s");
    }
    acc_sprintf("and %d of %d player%s displayed.\r\n", g_list_length(players),
        playerTotal, (playerTotal == 1) ? "" : "s");
    page_string(ch->desc, acc_get_string());
    g_list_free(immortals);
    g_list_free(testers);
    g_list_free(players);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
    switch (subcmd) {
    case SCMD_CREDITS:
        page_string(ch->desc, credits);
        break;
    case SCMD_INFO:
        page_string(ch->desc, info);
        break;
    case SCMD_MOTD:
        if (clr(ch, C_NRM))
            page_string(ch->desc, ansi_motd);
        else
            page_string(ch->desc, motd);
        break;
    case SCMD_IMOTD:
        if (clr(ch, C_NRM))
            page_string(ch->desc, ansi_imotd);
        else
            page_string(ch->desc, imotd);
        break;
    case SCMD_CLEAR:
        send_to_char(ch, "\033[H\033[J");
        break;
    case SCMD_VERSION:
        send_to_char(ch, "%s", circlemud_version);
        break;
    case SCMD_WHOAMI:
        send_to_char(ch, "%s\r\n", GET_NAME(ch));
        break;
    default:
        return;
        break;
    }
}

void
acc_print_object_location(int num, struct obj_data *obj,
    struct creature *ch, int recur)
{
    if ((obj->carried_by && GET_INVIS_LVL(obj->carried_by) > GET_LEVEL(ch)) ||
        (obj->in_obj && obj->in_obj->carried_by &&
            GET_INVIS_LVL(obj->in_obj->carried_by) > GET_LEVEL(ch)) ||
        (obj->worn_by && GET_INVIS_LVL(obj->worn_by) > GET_LEVEL(ch)) ||
        (obj->in_obj && obj->in_obj->worn_by &&
            GET_INVIS_LVL(obj->in_obj->worn_by) > GET_LEVEL(ch)))
        return;

    if (num > 0)
        acc_sprintf("%sO%s%3d. %s%-25s%s - ", CCGRN_BLD(ch, C_NRM), CCNRM(ch,
                C_NRM), num, CCGRN(ch, C_NRM), obj->name, CCNRM(ch, C_NRM));
    else
        acc_sprintf("%33s", " - ");

    if (obj->in_room != NULL) {
        acc_sprintf("%s[%s%5d%s] %s%s%s\r\n",
            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            obj->in_room->number, CCGRN(ch, C_NRM), CCCYN(ch, C_NRM),
            obj->in_room->name, CCNRM(ch, C_NRM));
    } else if (obj->carried_by) {
        acc_sprintf("carried by %s%s%s [%s%5d%s]%s\r\n",
            CCYEL(ch, C_NRM), PERS(obj->carried_by, ch),
            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
            obj->carried_by->in_room->number, CCGRN(ch, C_NRM), CCNRM(ch,
                C_NRM));
    } else if (obj->worn_by) {
        acc_sprintf("worn by %s%s%s [%s%5d%s]%s\r\n",
            CCYEL(ch, C_NRM), PERS(obj->worn_by, ch),
            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->worn_by->in_room->number,
            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
    } else if (obj->in_obj) {
        acc_sprintf("inside %s%s%s%s\r\n",
            CCGRN(ch, C_NRM), obj->in_obj->name,
            CCNRM(ch, C_NRM), (recur ? ", which is" : " "));
        if (recur)
            acc_print_object_location(0, obj->in_obj, ch, recur);
        return;
    } else {
        acc_sprintf("%sin an unknown location%s\r\n",
            CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    }
}

/**
 * Checks to see that the given object matches the search criteria
 * @param req The list of required search parameters
 * @param exc The list of excluded search parameters
 * @param thing The struct obj_data of the object to be reviewed
 *
 * @return a boolean value representing if the object passed the search criteria
 *
**/

bool
object_matches_terms(GList * req, GList * exc, struct obj_data *obj)
{
    GList *it;

    if (!obj->aliases)
        return false;

    for (it = req; it; it = it->next) {
        if (!isname((char *)it->data, obj->aliases)) {
            return false;
        }
    }
    for (it = exc; it; it = it->next) {
        if (isname((char *)it->data, obj->aliases)) {
            return false;
        }
    }
    return true;
}

/**
 * Checks to see that the given object matches the search criteria
 * @param req The list of required search parameters
 * @param exc The list of excluded search parameters
 * @param mob The struct creature struct of the mobile to be reviewed
 *
 * @return a boolean value representing if the object passed the search criteria
 *
**/
bool
creature_matches_terms(GList * req, GList * exc, struct creature * mob)
{
    GList *it;

    for (it = req; it; it = it->next) {
        if (!isname((char *)it->data, mob->player.name)) {
            return false;
        }
    }
    for (it = exc; it; it = it->next) {
        if (isname((char *)it->data, mob->player.name)) {
            return false;
        }
    }
    return true;
}

/**
 *  Recursively checks to see if an object is located in a house
 *
 *  @param obj the object data of the object in question
 *
 *  @return a boolean value reflecting the search results
**/

bool
obj_in_house(struct obj_data * obj)
{

    if (obj->in_obj)
        return obj_in_house(obj->in_obj);

    if (!obj->in_room)
        return false;

    return ROOM_FLAGGED(obj->in_room, ROOM_HOUSE);
}

void
perform_immort_where(struct creature *ch, char *arg, bool show_morts)
{
    register struct obj_data *k;
    struct descriptor_data *d;
    int num = 0, found = 0;
    bool house_only, no_house, no_mob, no_object;
    GList *required = NULL, *excluded = NULL;
    char *arg1;

    house_only = no_house = no_mob = no_object = false;

    acc_string_clear();
    skip_spaces(&arg);
    if (!*arg || !show_morts) {
        acc_strcat("Players\r\n-------\r\n", NULL);
        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CXN_PLAYING)
                continue;
            struct creature *player = (d->original ? d->original : d->creature);
            struct creature *form = d->creature;

            if (player && player->in_room && (show_morts || IS_IMMORT(player))) {
                const char *notes = "";

                if (player != form) {
                    notes = tmp_sprintf("%s%s (in %s)", notes, CCGRN(ch, C_CMP),
                                        GET_NAME(form));
                }
                if (ROOM_FLAGGED(form->in_room, ROOM_HOUSE)) {
                    notes = tmp_sprintf("%s%s (house)", notes, CCMAG(ch, C_CMP));
                }
                if (ROOM_FLAGGED(form->in_room, ROOM_ARENA)) {
                    notes = tmp_sprintf("%s%s (arena)", notes, CCYEL(ch, C_CMP));
                }
                if (!IS_APPR(form->in_room->zone)) {
                    notes = tmp_sprintf("%s%s (!appr)", notes, CCRED(ch, C_CMP));
                }
                if (ROOM_FLAGGED(form->in_room, ROOM_CLAN_HOUSE)) {
                    notes = tmp_sprintf("%s%s (clan)", notes, CCCYN(ch, C_CMP));
                }
                acc_sprintf("%s%-20s%s - %s[%s%5d%s]%s %s%s%s%s\r\n",
                            (GET_LEVEL(player) >= LVL_AMBASSADOR ? CCGRN(ch, C_NRM) : ""),
                            GET_NAME(player),
                            (GET_LEVEL(player) >= LVL_AMBASSADOR ? CCNRM(ch, C_NRM) : ""),
                            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                            form->in_room->number,
                            CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                            CCCYN(ch, C_NRM), form->in_room->name,
                            notes, CCNRM(ch, C_NRM));
            }
        }
        page_string(ch->desc, acc_get_string());
    } else {
        register struct creature *i = NULL;
        for (arg1 = tmp_getword(&arg);*arg1;arg1 = tmp_getword(&arg)) {
            if (arg1[0] == '!') {
                excluded = g_list_prepend(excluded, tmp_strdup(arg1 + 1));
            } else if (arg1[0] == '-') {
                if (is_abbrev(arg1, "-nomob"))
                    no_mob = true;
                if (is_abbrev(arg1, "-noobject"))
                    no_object = true;
                if (is_abbrev(arg1, "-nohouse"))
                    no_house = true;
                if (is_abbrev(arg1, "-house"))
                    house_only = true;
            } else {
                required = g_list_prepend(required, tmp_strdup(arg1));
            }
        }

        if (!required) {        //if there are no required fields don't search
            send_to_char(ch,
                "You're going to have to be a bit more specific than that.\r\n");
            return;
        }

        if (house_only && no_house) {
            send_to_char(ch,
                "Nothing exists both inside and outside a house.\r\n");
            return;
        }

        if (!no_mob) {
            GList *cit;
            for (cit = first_living(creatures); cit; cit = next_living(cit)) {
                i = cit->data;
                if (can_see_creature(ch, i)
                    && i->in_room
                    && creature_matches_terms(required, excluded, i)
                    && (GET_NPC_SPEC(i) != fate
                        || GET_LEVEL(ch) >= LVL_SPIRIT)) {
                    found = 1;
                    acc_sprintf
                        ("%sM%s%3d. %s%-25s%s - %s[%s%5d%s]%s %s%s%s\r\n",
                        CCRED_BLD(ch, NRM), CCNRM(ch, NRM), ++num, CCYEL(ch,
                            C_NRM), GET_NAME(i), CCNRM(ch, C_NRM), CCGRN(ch,
                            C_NRM), CCNRM(ch, C_NRM), i->in_room->number,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
                        i->in_room->name, CCNRM(ch, C_NRM));
                }
            }
        }

        if (!no_object) {
            for (num = 0, k = object_list; k; k = k->next) {
                if (!can_see_object(ch, k))
                    continue;
                if (house_only && !obj_in_house(k))
                    continue;
                if (no_house && obj_in_house(k))
                    continue;
                if (object_matches_terms(required, excluded, k)) {
                    found = 1;
                    acc_print_object_location(++num, k, ch, true);
                }
            }
        }

        if (found) {
            page_string(ch->desc, acc_get_string());
        } else {
            send_to_char(ch, "Couldn't find any such thing.\r\n");
        }
    }
}

ACMD(do_where)
{
    skip_spaces(&argument);

    if (is_authorized(ch, FULL_IMMORT_WHERE, NULL)) {
        perform_immort_where(ch, argument, true);
        return;
    }
    if (IS_IMMORT(ch)) {
        perform_immort_where(ch, argument, false);
        return;
    }

    if (!can_see_room(ch, ch->in_room)) {
        if (room_is_dark(ch->in_room)) {
            send_to_char(ch, "Looks like you're in the dark!\r\n");
        } else {
            send_to_char(ch, "You can't tell where you are.\r\n");
        }
        return;
    }

    send_to_char(ch, "You are located: %s%s%s\r\n",
                 CCGRN(ch, C_NRM),
                 ch->in_room->name,
                 CCNRM(ch, C_NRM));
    send_to_char(ch, "In the area: %s%s%s\r\n",
                 CCGRN(ch, C_NRM),
                 ch->in_room->zone->name,
                 CCNRM(ch, C_NRM));
    if (ch->in_room->zone->author) {
        send_to_char(ch, "Authored by: %s%s%s\r\n",
                     CCGRN(ch, C_NRM),
                     ch->in_room->zone->author,
                     CCNRM(ch, C_NRM));
    }
    if (ZONE_FLAGGED(ch->in_room->zone, ZONE_NOLAW))
        send_to_char(ch, "This place is beyond the reach of law.\r\n");
    if (!IS_APPR(ch->in_room->zone)) {
        send_to_char(ch, "This zone is %sUNAPPROVED%s!\r\n",
                     CCRED(ch, C_NRM),
                     CCNRM(ch, C_NRM));
    }
    if (ch->in_room->zone->public_desc)
        send_to_char(ch, "%s", ch->in_room->zone->public_desc);
    act("$n ponders the implications of $s location.", true, ch, NULL, NULL,
        TO_ROOM);
}

void
print_attributes_to_buf(struct creature *ch, char *buff)
{

    int8_t str, intel, wis, dex, con, cha;
    str = GET_STR(ch);
    intel = GET_INT(ch);
    wis = GET_WIS(ch);
    dex = GET_DEX(ch);
    con = GET_CON(ch);
    cha = GET_CHA(ch);
    sprintf(buf2, " %s%s(augmented)%s",
        CCBLD(ch, C_SPR), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));

    sprintf(buff, "      %s%sStrength:%s ",
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d]", str));

    if (str <= 3)
        strcat(buff, "You can barely stand up under your own weight.");
    else if (str <= 4)
        strcat(buff, "You couldn't beat your way out of a paper bag.");
    else if (str <= 5)
        strcat(buff, "You are laughed at by ten year olds.");
    else if (str <= 6)
        strcat(buff, "You are a weakling.");
    else if (str <= 7)
        strcat(buff,
            "You can pick up large rocks without breathing too hard.");
    else if (str <= 8)
        strcat(buff, "You are not very strong.");
    else if (str <= 10)
        strcat(buff, "You are of average strength.");
    else if (str <= 12)
        strcat(buff, "You are fairly strong.");
    else if (str <= 15)
        strcat(buff, "You are a nice specimen.");
    else if (str <= 16)
        strcat(buff, "You are a damn nice specimen.");
    else if (str < 18)
        strcat(buff, "You are very strong.");
    else if (str <= 20)
        strcat(buff, "You are extremely strong.");
    else if (str <= 23)
        strcat(buff, "You are exceptionally strong.");
    else if (str <= 25)
        strcat(buff, "Your strength is awe inspiring.");
    else if (str <= 27)
        strcat(buff, "Your strength is super-human.");
    else if (str == 28)
        strcat(buff, "Your strength is at the human peak!");
    else if (str == 29)
        strcat(buff, "You have the strength of a hill giant!");
    else if (str == 30)
        strcat(buff, "You have the strength of a stone giant!");
    else if (str == 31)
        strcat(buff, "You have the strength of a frost giant!");
    else if (str == 32)
        strcat(buff, "You can toss boulders with ease!");
    else if (str == 33)
        strcat(buff, "You have the strength of a cloud giant!");
    else if (str == 34)
        strcat(buff, "You possess a herculean might!");
    else if (str >= 35)
        strcat(buff, "You have the strength of a god!");
    else
        strcat(buff, "Your strength is SKREWD.");

    if (str != ch->real_abils.str)
        strcat(buff, buf2);
    strcat(buff, "\r\n");

    sprintf(buff, "%s  %s%sIntelligence:%s ", buff,
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d]", intel));
    if (intel <= 5)
        strcat(buff, "You lose arguments with inanimate objects.");
    else if (intel <= 8)
        strcat(buff, "You're about as smart as a rock.");
    else if (intel <= 10)
        strcat(buff, "You are a bit slow-witted.");
    else if (intel <= 12)
        strcat(buff, "Your intelligence is average.");
    else if (intel <= 13)
        strcat(buff, "You are fairly intelligent.");
    else if (intel <= 14)
        strcat(buff, "You are very smart.");
    else if (intel <= 16)
        strcat(buff, "You are exceptionally smart.");
    else if (intel <= 17)
        strcat(buff, "You possess a formidable intellect.");
    else if (intel <= 18)
        strcat(buff, "You are a powerhouse of logic.");
    else if (intel <= 19)
        strcat(buff, "You are an absolute genius!");
    else if (intel <= 20)
        strcat(buff, "You are a suuuuper-geniuus!");
    else
        strcat(buff,
            "You solve nonlinear higher dimensional systems in your sleep.");
    if (intel != ch->real_abils.intel)
        strcat(buff, buf2);
    strcat(buff, "\r\n");

    sprintf(buff, "%s        %s%sWisdom:%s ", buff,
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d]", wis));
    if (wis <= 5)
        strcat(buff, "Yoda you are not.");
    else if (wis <= 6)
        strcat(buff, "You are pretty foolish.");
    else if (wis <= 8)
        strcat(buff, "You are fairly foolhardy.");
    else if (wis <= 10)
        strcat(buff, "Your wisdom is average.");
    else if (wis <= 12)
        strcat(buff, "You are fairly wise.");
    else if (wis <= 15)
        strcat(buff, "You are very wise.");
    else if (wis <= 18)
        strcat(buff, "You are exceptionally wise.");
    else if (wis <= 19)
        strcat(buff, "You have the wisdom of a god!");
    else
        strcat(buff, "God himself comes to you for advice.");

    if (wis != ch->real_abils.wis)
        strcat(buff, buf2);
    strcat(buff, "\r\n");

    sprintf(buff, "%s     %s%sDexterity:%s ", buff,
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d]", dex));
    if (dex <= 5)
        strcat(buff, "I wouldn't walk too fast if I were you.");
    else if (dex <= 8)
        strcat(buff, "You're pretty clumsy.");
    else if (dex <= 10)
        strcat(buff, "Your agility is pretty average.");
    else if (dex <= 12)
        strcat(buff, "You are fairly agile.");
    else if (dex <= 15)
        strcat(buff, "You are very agile.");
    else if (dex <= 18)
        strcat(buff, "You are exceptionally agile.");
    else
        strcat(buff, "You have the agility of a god!");
    if (dex != ch->real_abils.dex)
        strcat(buff, buf2);
    strcat(buff, "\r\n");

    sprintf(buff, "%s  %s%sConstitution:%s ", buff,
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d] ", con));
    if (con <= 3)
        strcat(buff, "You are dead, but haven't realized it yet.");
    else if (con <= 5)
        strcat(buff, "You're as healthy as a rabid dog.");
    else if (con <= 7)
        strcat(buff,
            "A child poked you once, and you have the scars to prove it.");
    else if (con <= 8)
        strcat(buff, "You're pretty skinny and sick looking.");
    else if (con <= 10)
        strcat(buff, "Your health is average.");
    else if (con <= 12)
        strcat(buff, "You are fairly healthy.");
    else if (con <= 15)
        strcat(buff, "You are very healthy.");
    else if (con <= 18)
        strcat(buff, "You are exceptionally healthy.");
    else if (con <= 19)
        strcat(buff, "You are practically immortal!");
    else
        strcat(buff, "You can eat pathogens for breakfast.");

    if (con != ch->real_abils.con)
        strcat(buff, buf2);
    strcat(buff, "\r\n");

    sprintf(buff, "%s      %s%sCharisma:%s ", buff,
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (mini_mud)
        strcat(buff, tmp_sprintf(" [%d] ", cha));
    if (cha <= 5)
        strcat(buff, "U-G-L-Y");
    else if (cha <= 6)
        strcat(buff, "Your face could turn a family of elephants to stone.");
    else if (cha <= 7)
        strcat(buff, "Small children run away from you screaming.");
    else if (cha <= 8)
        strcat(buff, "You are totally unattractive.");
    else if (cha <= 9)
        strcat(buff, "You are slightly unattractive.");
    else if (cha <= 10)
        strcat(buff, "You are not too unpleasant to deal with.");
    else if (cha <= 12)
        strcat(buff, "You are a pleasant person.");
    else if (cha <= 15)
        strcat(buff, "You are exceptionally attractive.");
    else if (cha <= 16)
        strcat(buff, "You have a magnetic personality.");
    else if (cha <= 17)
        strcat(buff, "Others eat from the palm of your hand.");
    else if (cha <= 18)
        strcat(buff, "Your image should be chiseled in marble!");
    else if (cha <= 22)
        strcat(buff, "Others eat from the palm of your hand.  Literally.");
    else
        strcat(buff,
            "If the gods made better they'd have kept it for themselves.");
    if (cha != ch->real_abils.cha)
        strcat(buff, buf2);
    strcat(buff, "\r\n");
}

ACMD(do_attributes)
{
    sprintf(buf,
        "\r\n%s********************************** %sAttributes %s*********************************%s\r\n",
        CCBLU(ch, C_SPR), CCYEL(ch, C_SPR), CCBLU(ch, C_SPR), CCWHT(ch,
            C_SPR));
    send_to_char(ch, "%s", buf);
    send_to_char(ch, "        Name:  %s%20s%s        Race: %s%10s%s\r\n",
        CCRED(ch, C_SPR), GET_NAME(ch), CCWHT(ch, C_SPR), CCRED(ch, C_SPR),
        race_name_by_idnum(GET_RACE(ch)), CCWHT(ch, C_SPR));
    send_to_char(ch, "        Class: %s%20s%s        Level: %s%9d%s\r\n\r\n",
        CCRED(ch, C_SPR), class_names[(int)GET_CLASS(ch)], CCWHT(ch, C_SPR),
        CCRED(ch, C_SPR), GET_LEVEL(ch), CCWHT(ch, C_SPR));

    print_attributes_to_buf(ch, buf);

    send_to_char(ch, "%s", buf);
    sprintf(buf,
        "\r\n%s*******************************************************************************%s\r\n",
        CCBLU(ch, C_SPR), CCWHT(ch, C_SPR));
    send_to_char(ch, "%s", buf);
}

ACMD(do_consider)
{
    struct creature *victim;
    int diff, ac;

    one_argument(argument, buf);

    if (!(victim = get_char_room_vis(ch, buf))) {
        send_to_char(ch, "Consider killing who?\r\n");
        return;
    }
    if (victim == ch) {
        send_to_char(ch, "Easy!  Very easy indeed!\r\n");
        return;
    }
    if (!IS_NPC(victim)) {
        send_to_char(ch,
            "Well, if you really want to kill another player...\r\n");
    }
    diff = level_bonus(victim, true) - level_bonus(ch, true);

    if (diff <= -30)
        send_to_char(ch, "It's not even worth the effort...\r\n");
    else if (diff <= -22)
        send_to_char(ch, "You could do it without any hands!\r\n");
    else if (diff <= -19)
        send_to_char(ch, "You may not even break a sweat.\r\n");
    else if (diff <= -16)
        send_to_char(ch, "You've thrashed far worse.\r\n");
    else if (diff <= -13)
        send_to_char(ch, "It may be quite easy...\r\n");
    else if (diff <= -10)
        send_to_char(ch, "Should be fairly easy.\r\n");
    else if (diff <= -6)
        send_to_char(ch, "You should be able to handle it.\r\n");
    else if (diff <= -4)
        send_to_char(ch, "It looks like you have a definite advantage.\r\n");
    else if (diff <= -2)
        send_to_char(ch, "You seem to have a slight advantage.\r\n");
    else if (diff <= -1)
        send_to_char(ch, "The odds here are just barely in your favor.\r\n");
    else if (diff == 0)
        send_to_char(ch, "It's a pretty even match.\r\n");
    else if (diff <= 1)
        send_to_char(ch, "Keep your wits about you...\r\n");
    else if (diff <= 2)
        send_to_char(ch, "With a bit of luck, you could win.\r\n");
    else if (diff <= 4)
        send_to_char(ch, "You should probably stretch out first.\r\n");
    else if (diff <= 6)
        send_to_char(ch, "You probably couldn't do it bare handed...\r\n");
    else if (diff <= 8)
        send_to_char(ch, "You won't catch me betting on you.\r\n");
    else if (diff <= 10)
        send_to_char(ch, "You may get your ass whipped!\r\n");
    else if (diff <= 14)
        send_to_char(ch, "You better have some nice equipment!\r\n");
    else if (diff <= 18)
        send_to_char(ch, "You better spend some more time in the gym!\r\n");
    else if (diff <= 22)
        send_to_char(ch, "Well, you only live once...\r\n");
    else if (diff <= 26)
        send_to_char(ch, "You must be out of your freakin' mind.\r\n");
    else if (diff <= 30)
        send_to_char(ch, "What?? Are you STUPID or something?!!\r\n");
    else
        send_to_char(ch,
            "Would you like to borrow a cross and a shovel for your grave?\r\n");

    if (GET_SKILL(ch, SKILL_CONSIDER) > 70) {
        diff = (GET_MAX_HIT(victim) - GET_MAX_HIT(ch));
        if (diff <= -300)
            act("$E looks puny, and weak.", false, ch, NULL, victim, TO_CHAR);
        else if (diff <= -200)
            act("$E would die ten times before you would be killed.", false,
                ch, NULL, victim, TO_CHAR);
        else if (diff <= -100)
            act("You could beat $M to death with your forehead.", false, ch, NULL,
                victim, TO_CHAR);
        else if (diff <= -50)
            act("$E can take almost as much as you.", false, ch, NULL, victim,
                TO_CHAR);
        else if (diff <= 50)
            send_to_char(ch,
                "You can both take pretty much the same abuse.\r\n");
        else if (diff <= 200)
            act("$E looks like $E could take a lickin'.", false, ch, NULL, victim,
                TO_CHAR);
        else if (diff <= 600)
            act("Haven't you seen $M breaking bricks on $S head?", false, ch,
                NULL, victim, TO_CHAR);
        else if (diff <= 900)
            act("You would bet $E eats high voltage cable for breakfast.",
                false, ch, NULL, victim, TO_CHAR);
        else if (diff <= 1200)
            act("$E probably isn't very scared of bulldozers.", false, ch, NULL,
                victim, TO_CHAR);
        else if (diff <= 1800)
            act("A blow from a house-sized meteor MIGHT do $M in.", false, ch,
                NULL, victim, TO_CHAR);
        else
            act("Maybe if you threw $N into the sun...", false, ch, NULL, victim,
                TO_CHAR);

        ac = GET_AC(victim);
        if (ac <= -100)
            act("$E makes battleships look silly.", false, ch, NULL, victim,
                TO_CHAR);
        else if (ac <= -50)
            act("$E is about as defensible as a boulder.", false, ch, NULL,
                victim, TO_CHAR);
        else if (ac <= 0)
            act("$E has better defenses than most small cars.", false, ch, NULL,
                victim, TO_CHAR);
        else if (ac <= 50)
            act("$S defenses are pretty damn good.", false, ch, NULL, victim,
                TO_CHAR);
        else if (ac <= 70)
            act("$S body appears to be well protected.", false, ch, NULL, victim,
                TO_CHAR);
        else if (ac <= 90)
            act("Well, $E's better off than a naked person.", false, ch, NULL,
                victim, TO_CHAR);
        else
            act("$S armor SUCKS!", false, ch, NULL, victim, TO_CHAR);
    }
}

ACMD(do_diagnose)
{
    struct creature *vict;

    one_argument(argument, buf);

    if (*buf) {
        if (!(vict = get_char_room_vis(ch, buf))) {
            send_to_char(ch, "%s", NOPERSON);
            return;
        } else
            diag_char_to_char(vict, ch);
    } else {
        if (is_fighting(ch))
            diag_char_to_char(random_opponent(ch), ch);
        else
            send_to_char(ch, "Diagnose who?\r\n");
    }
}

ACMD(do_pkiller)
{
    if (IS_NPC(ch))
        return;

    char *arg = tmp_getword(&argument);

    if (*arg) {
        if (strcasecmp(arg, "on") == 0) {
            if (reputation_of(ch) <= 0) {
                arg = tmp_getword(&argument);
                if (strcasecmp(arg, "yes")) {
                    send_to_char(ch,
                        "Your reputation is 0.  You must type PK ON YES "
                        "to enter the world of PK.\r\n");
                    return;
                }

                gain_reputation(ch, 5);
            }
            SET_BIT(PRF2_FLAGS(ch), PRF2_PKILLER);
        } else if (strcasecmp(arg, "off") == 0) {
            REMOVE_BIT(PRF2_FLAGS(ch), PRF2_PKILLER);
        } else {
            send_to_char(ch, "Usage: pkiller { Off | On [yes]}\r\n");
            return;
        }
    }

    const char *color = CCCYN(ch, C_NRM);
    if (PRF2_FLAGGED(ch, PRF2_PKILLER))
        color = CCRED(ch, C_SPR);

    send_to_char(ch, "Your current pkiller status is: %s%s%s\r\n",
        color, ONOFF(PRF2_FLAGGED(ch, PRF2_PKILLER)), CCNRM(ch, C_NRM));
}

ACMD(do_compact)
{
    int tp;

    if (IS_NPC(ch))
        return;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Your current compact level is %s.\r\n",
            compact_levels[ch->account->compact_level]);
        return;
    }
    if (((tp = search_block(arg, compact_levels, false)) == -1)) {
        send_to_char(ch,
            "Usage: compact { off | minimal | partial | full }\r\n");
        return;
    }
    ch->account->compact_level = tp;

    send_to_char(ch, "Your %scompact setting%s is now %s%s%s%s.\r\n", CCRED(ch,
            C_SPR), CCNRM(ch, C_OFF), CCYEL(ch, C_NRM), CCBLD(ch, C_CMP),
        compact_levels[tp], CCNRM(ch, C_NRM));
}

ACMD(do_color)
{
    int tp;

    if (IS_NPC(ch))
        return;

    char *arg = tmp_getword(&argument);

    if (!*arg) {
        send_to_char(ch, "Your current color level is %s.\r\n",
            strlist_aref(COLOR_LEV(ch), ansi_levels));
        return;
    }
    if (((tp = search_block(tmp_tolower(arg), ansi_levels, false)) == -1)) {
        send_to_char(ch,
            "Usage: color { none | sparse | normal | complete }\r\n");
        return;
    }
    ch->account->ansi_level = tp;

    send_to_char(ch, "Your color is now %s%s%s%s.\r\n",
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), ansi_levels[tp], CCNRM(ch, C_NRM));
}

void
show_all_toggles(struct creature *ch)
{
    bool gets_clanmail = false;
    int tp;

    if (IS_NPC(ch) || !ch->desc || !ch->desc->account)
        return;
    if (GET_WIMP_LEV(ch) == 0)
        strcpy(buf2, "OFF");
    else
        sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

    if (GET_CLAN(ch)) {
        struct clan_data *clan = real_clan(GET_CLAN(ch));
        if (clan) {
            struct clanmember_data *member =
                real_clanmember(GET_IDNUM(ch), clan);
            if (member)
                gets_clanmail = !member->no_mail;
        }
    }
    send_to_char(ch,
        "-- DISPLAY -------------------------------------------------------------------\r\n"
        "Display Hit Pts: %-3s    "
        "   Display Mana: %-3s    "
        "   Display Move: %-3s\r\n"
        "       Autoexit: %-3s    "
        "   Autodiagnose: %-3s    "
        "     Autoprompt: %-3s\r\n"
        "     Brief Mode: %-3s    "
        "     See misses: %-3s    "
        "Compact Display: %s\r\n"
        "  Terminal Size: %-7s"
        "     Notrailers: %-3s    "
        "    Color Level: %s\r\n"
        "   Metric Units: %-3s\r\n\r\n"
        "-- CHANNELS ------------------------------------------------------------------\r\n"
        "       Autopage: %-3s    "
        " Newbie Helper?: %-3s    "
        "    Projections: %-3s\r\n"
        " Gossip Channel: %-3s    "
        "Auction Channel: %-3s    "
        "  Grats Channel: %-3s\r\n"
        "   Spew Channel: %-3s    "
        "  Music Channel: %-3s    "
        "  Dream Channel: %-3s\r\n"
        "  Guild Channel: %-3s    "
        "   Clan Channel: %-3s    "
        " Haggle Channel: %-3s\r\n"
        "   Nasty Speech: %-3s\r\n"
        "\r\n"
        "-- GAMEPLAY ------------------------------------------------------------------\r\n"
        "      Autosplit: %-3s    "
        "       Autoloot: %-3s    "
        "     Wimp Level: %-3s\r\n"
        "           Deaf: %-3s    "
        "         NoTell: %-3s    "
        "       On Quest: %-3s\r\n"
        "   Show Affects: %-3s    "
        "      Clan hide: %-3s    "
        "      Clan mail: %-3s\r\n"
        "      Anonymous: %-3s    "
        "        PKILLER: %-3s\r\n"
        "\r\n",
        ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
        ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
        ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
        ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_AUTO_DIAGNOSE)),
        YESNO(PRF2_FLAGGED(ch, PRF2_AUTOPROMPT)),
        ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
        YESNO(!PRF_FLAGGED(ch, PRF_GAGMISS)),
        (IS_NPC(ch) ? "mob" :
            compact_levels[ch->account->compact_level]),
        tmp_sprintf("%dx%d",
            GET_PAGE_LENGTH(ch),
            GET_PAGE_WIDTH(ch)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_NOTRAILERS)),
        ansi_levels[COLOR_LEV(ch)],
        YESNO(USE_METRIC(ch)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOPAGE)),
        YESNO(PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOPROJECT)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOSPEW)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOMUSIC)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NODREAM)),
        ONOFF(!PRF2_FLAGGED(ch, PRF2_NOGUILDSAY)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOCLANSAY)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOHAGGLE)),
        ONOFF(PRF_FLAGGED(ch, PRF_NASTY)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOLOOT)),
        buf2,
        YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
        ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
        YESNO(GET_QUEST(ch)),
        YESNO(!PRF2_FLAGGED(ch, PRF2_NOAFFECTS)),
        YESNO(PRF2_FLAGGED(ch, PRF2_CLAN_HIDE)),
        YESNO(gets_clanmail),
        YESNO(PRF2_FLAGGED(ch, PRF2_ANONYMOUS)),
        YESNO(PRF2_FLAGGED(ch, PRF2_PKILLER)));

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
        tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
            (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
        send_to_char(ch,
        "-- IMMORTAL  -----------------------------------------------------------------\r\n"
        "    Imm Channel: %-3s    "
        "       Petition: %-3s    "
        "         Holler: %-3s\r\n"
        "       God Echo: %-3s    "
        "  Display Vnums: %-3s    "
        "Show Room Flags: %-3s\r\n"
        "       Nohassle: %-3s    "
        "          Snoop: %-3s    "
        "          Debug: %-3s\r\n"
        "   Syslog Level: %-8s\r\n",
        ONOFF(!PRF2_FLAGGED(ch, PRF2_NOIMMCHAT)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOPETITION)),
        ONOFF(!PRF2_FLAGGED(ch, PRF2_NOHOLLER)),
        ONOFF(!PRF2_FLAGGED(ch, PRF2_NOGECHO)),
        YESNO(PRF2_FLAGGED(ch, PRF2_DISP_VNUMS)),
        YESNO(PRF_FLAGGED(ch, PRF_ROOMFLAGS)),
        ONOFF(!PRF_FLAGGED(ch, PRF_NOHASSLE)),
        YESNO(!PRF_FLAGGED(ch, PRF_NOSNOOP)),
        ONOFF(PRF2_FLAGGED(ch, PRF2_DEBUG)),
        logtypes[tp]);
    }

    if (IS_MAGE(ch)) {
        send_to_char(ch, "((Mana shield)) Low:[%ld], Percent:[%ld]\n",
            GET_MSHIELD_LOW(ch), GET_MSHIELD_PCT(ch));
    }
}

ACMD(do_toggle)
{
    ACMD(do_gen_tog);
    char *arg;

    arg = tmp_getword(&argument);
    if (!*arg) {
        // Display toggles
        show_all_toggles(ch);
        return;
    }

    if (is_abbrev(arg, "gossip"))
        do_gen_tog(ch, "", cmd, SCMD_NOGOSSIP);
    else if (is_abbrev(arg, "spew"))
        do_gen_tog(ch, "", cmd, SCMD_NOSPEW);
    else if (is_abbrev(arg, "guild"))
        do_gen_tog(ch, "", cmd, SCMD_NOGUILDSAY);
    else if (is_abbrev(arg, "clan"))
        do_gen_tog(ch, "", cmd, SCMD_NOCLANSAY);
    else if (is_abbrev(arg, "sing"))
        do_gen_tog(ch, "", cmd, SCMD_NOMUSIC);
    else if (is_abbrev(arg, "auction"))
        do_gen_tog(ch, "", cmd, SCMD_NOAUCTION);
    else if (is_abbrev(arg, "grats"))
        do_gen_tog(ch, "", cmd, SCMD_NOGRATZ);
    else if (is_abbrev(arg, "newbie"))
        do_gen_tog(ch, "", cmd, SCMD_NEWBIE_HELP);
    else if (is_abbrev(arg, "dream"))
        do_gen_tog(ch, "", cmd, SCMD_NODREAM);
    else if (is_abbrev(arg, "project"))
        do_gen_tog(ch, "", cmd, SCMD_NOPROJECT);
    else if (is_abbrev(arg, "brief"))
        do_gen_tog(ch, "", cmd, SCMD_BRIEF);
    else
        send_to_char(ch, "What is it that you want to toggle?\r\n");
    return;
}

ACMD(do_commands)
{
    int no, i, cmd_num;
    int wizhelp = 0, socials = 0, moods = 0, level = 0;
    struct creature *vict = NULL;
    char *arg = tmp_getword(&argument);

    if (*arg) {
        if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
            if (is_number(arg)) {
                level = atoi(arg);
            } else {
                send_to_char(ch, "Who is that?\r\n");
                return;
            }
        } else
            level = GET_LEVEL(vict);

        if (level < 0) {
            send_to_char(ch, "What a comedian.\r\n");
            return;
        }
        if (GET_LEVEL(ch) < level) {
            send_to_char(ch,
                "You can't see the commands of people above your level.\r\n");
            return;
        }
    } else {
        vict = ch;
        level = GET_LEVEL(ch);
    }

    if (subcmd == SCMD_SOCIALS)
        socials = 1;
    else if (subcmd == SCMD_MOODS)
        moods = 1;
    else if (subcmd == SCMD_WIZHELP)
        wizhelp = 1;

    sprintf(buf, "The following %s%s are available to %s:\r\n",
        wizhelp ? "privileged " : "",
        socials ? "socials" : moods ? "moods" : "commands",
        (vict && vict == ch) ? "you" : vict ? GET_NAME(vict) : "that level");

    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
        i = cmd_sort_info[cmd_num].sort_pos;
        if (cmd_info[i].minimum_level < 0)
            continue;
        if (level < cmd_info[i].minimum_level)
            continue;
        if (!is_authorized(ch, COMMAND, &cmd_info[i]))
            continue;
        if (wizhelp && (cmd_info[i].minimum_level < LVL_AMBASSADOR))
            continue;
        if (!wizhelp && socials != cmd_sort_info[i].is_social)
            continue;
        if (moods != cmd_sort_info[i].is_mood)
            continue;
        sprintf(buf + strlen(buf), "%-16s", cmd_info[i].command);
        if (!(no % 5))
            strcat(buf, "\r\n");
        no++;
    }

    strcat(buf, "\r\n");
    page_string(ch->desc, buf);
}

ACMD(do_soilage)
{
    int i, j, k, pos, found = 0;

    strcpy(buf, "Soilage status:\r\n");

    for (i = 0; i < NUM_WEARS; i++) {
        pos = (int)eq_pos_order[i];

        if (GET_EQ(ch, pos) && OBJ_SOILAGE(GET_EQ(ch, pos))) {
            found = 0;
            sprintf(buf2, "Your %s%s%s %s ", CCGRN(ch, C_NRM),
                OBJS(GET_EQ(ch, pos), ch), CCNRM(ch, C_NRM),
                ISARE(OBJS(GET_EQ(ch, pos), ch)));

            for (k = 0, j = 0; j < TOP_SOIL; j++)
                if (OBJ_SOILED(GET_EQ(ch, pos), (1 << j)))
                    k++;

            for (j = 0; j < TOP_SOIL; j++) {
                if (OBJ_SOILED(GET_EQ(ch, pos), (1 << j))) {
                    found++;
                    if (found > 1) {
                        strcat(buf2, ", ");
                        if (found == k)
                            strcat(buf2, "and ");
                    }
                    strcat(buf2, soilage_bits[j]);
                }
            }
            strcat(buf, strcat(buf2, ".\r\n"));

        } else if (CHAR_SOILAGE(ch, pos)) {
            if (ILLEGAL_SOILPOS(pos)) {
                CHAR_SOILAGE(ch, pos) = 0;
                continue;
            }
            found = false;
            sprintf(buf2, "Your %s %s ", wear_description[pos],
                pos == WEAR_FEET ? "are" : ISARE(wear_description[pos]));

            for (k = 0, j = 0; j < TOP_SOIL; j++)
                if (CHAR_SOILED(ch, pos, (1 << j)))
                    k++;

            for (j = 0; j < TOP_SOIL; j++) {
                if (CHAR_SOILED(ch, pos, (1 << j))) {
                    found++;
                    if (found > 1) {
                        strcat(buf2, ", ");
                        if (found == k)
                            strcat(buf2, "and ");
                    }
                    strcat(buf2, soilage_bits[j]);
                }
            }
            strcat(buf, strcat(buf2, ".\r\n"));

        }
    }
    page_string(ch->desc, buf);

}

ACMD(do_skills)
{
    int parse_player_class(char *arg);
    int parse_char_class(char *arg);
    void show_char_class_skills(struct creature *ch, int con, int immort,
        int bits);
    void list_skills(struct creature *ch, int mode, int type);
    int char_class = 0;
    char *arg;

    arg = tmp_getword(&argument);

    if (*arg && is_abbrev(arg, "list")) {
        arg = tmp_getword(&argument);
        if (*arg) {
            if (GET_LEVEL(ch) < LVL_IMMORT)
                char_class = parse_player_class(arg);
            else
                char_class = parse_char_class(arg);
            if (char_class < 0) {
                send_to_char(ch, "'%s' isn't a character class!\r\n", arg);
                return;
            }
        } else
            char_class = GET_CLASS(ch);

        show_char_class_skills(ch, char_class, 0,
            (subcmd ? (char_class == CLASS_PSIONIC ? TRIG_BIT :
                    char_class == CLASS_PHYSIC ? ALTER_BIT :
                    char_class == CLASS_MONK ? ZEN_BIT :
                    char_class == CLASS_BARD ? SONG_BIT : SPELL_BIT) : 0));

        return;
    }

    list_skills(ch, 0, subcmd == SCMD_SKILLS_ONLY ? 2 : 1);
}

ACMD(do_specializations)
{
    int i, j, char_class;
    struct obj_data *obj = NULL;

    for (i = 0; i < MAX_WEAPON_SPEC; i++) {
        if (!GET_WEAP_SPEC(ch, i).level ||
            (GET_WEAP_SPEC(ch, i).vnum > 0 &&
                !real_object_proto(GET_WEAP_SPEC(ch, i).vnum))) {
            for (j = i; j < MAX_WEAPON_SPEC; j++) {
                if (j == MAX_WEAPON_SPEC - 1) {
                    GET_WEAP_SPEC(ch, j).vnum = 0;
                    GET_WEAP_SPEC(ch, j).level = 0;
                    break;
                }
                GET_WEAP_SPEC(ch, j).vnum = GET_WEAP_SPEC(ch, j + 1).vnum;
                GET_WEAP_SPEC(ch, j).level = GET_WEAP_SPEC(ch, j + 1).level;
            }
        }
    }

    if ((char_class = GET_CLASS(ch)) >= NUM_CLASSES)
        char_class = CLASS_WARRIOR;
    if (IS_REMORT(ch) && GET_REMORT_CLASS(ch) < NUM_CLASSES) {
        if (weap_spec_char_class[char_class].max <
            weap_spec_char_class[GET_REMORT_CLASS(ch)].max)
            char_class = GET_REMORT_CLASS(ch);
    }

    send_to_char(ch, "As a %s you can specialize in %d weapons.\r\n",
        class_names[char_class], weap_spec_char_class[char_class].max);
    for (i = 0; i < MAX_WEAPON_SPEC; i++) {
        if (!GET_WEAP_SPEC(ch, i).level)
            break;

        if (GET_WEAP_SPEC(ch, i).vnum <= 0)
            break;

        obj = real_object_proto(GET_WEAP_SPEC(ch, i).vnum);
        if (!obj)
            break;

        send_to_char(ch, " %2d. %-30s [%d]\r\n", i + 1,
            obj->name, GET_WEAP_SPEC(ch, i).level);
    }
}

ACMD(do_wizlist)
{
    acc_string_clear();

    acc_sprintf("\r\n                   %sThe Immortals of TempusMUD\r\n"
        "                   %s--------------------------%s\r\n",
        CCBLU(ch, C_NRM), CCBLU_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
    send_role_member_list(role_by_name("Wizlist_Architects"), ch,
        "Architects", "OLCAdmin");
    send_role_member_list(role_by_name("Wizlist_Blders"), ch,
        "Builders", NULL);
    send_role_member_list(role_by_name("Wizlist_Coders"), ch,
        "Implementors", "CoderAdmin");
    send_role_member_list(role_by_name("Wizlist_Quests"), ch,
        "Questors", "QuestorAdmin");
    send_role_member_list(role_by_name("Wizlist_Elders"), ch,
        "Elder Gods", "GroupsAdmin");
    send_role_member_list(role_by_name("Wizlist_Founders"), ch,
        "Founders", NULL);

    acc_strcat("\r\n\r\n", NULL);

    page_string(ch->desc, acc_get_string());
}

ACMD(do_areas)
{
    struct zone_data *zone;
    bool found_one = false;

    acc_string_clear();
    acc_sprintf
        ("%s%s                    --- Areas appropriate for your level ---%s\r\n",
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    for (zone = zone_table; zone; zone = zone->next) {
        if (!zone->public_desc || !zone->min_lvl)
            continue;
        if (IS_IMMORT(ch) || (zone->min_lvl <= GET_LEVEL(ch)
                && zone->max_lvl >= GET_LEVEL(ch)
                && zone->min_gen <= GET_REMORT_GEN(ch)
                && zone->max_gen >= GET_REMORT_GEN(ch))) {
            // name
            acc_strcat((found_one) ? "\r\n" : "", CCCYN(ch, C_NRM),
                zone->name, CCNRM(ch, C_NRM), "\r\n", NULL);

            // min/max level
            if (zone->min_lvl == 1 && zone->max_lvl == 49 &&
                zone->min_gen == 0 && zone->max_gen == 10) {
                acc_strcat("[ All Levels ]\r\n", NULL);
            } else if (zone->min_gen > 0 || zone->max_gen > 0) {
                // [ Level 12 Generation 2 to Level 20 Generation 10 ]
                acc_sprintf("[ Level %d Gen %d to Level %d Gen %d ]\r\n",
                    zone->min_lvl, zone->min_gen,
                    zone->max_lvl, zone->max_gen);
            } else {
                // [ Level 12 to Level 20 ]
                acc_sprintf("[ Level %d to Level %d ]\r\n",
                    zone->min_lvl, zone->max_lvl);
            }
            // desc
            acc_strcat(zone->public_desc, NULL);
            found_one = true;
        }
    }

    if (found_one)
        page_string(ch->desc, acc_get_string());
    else {
        send_to_char(ch,
            "Bug the immortals about adding zones appropriate for your level!\r\n");
        mudlog(GET_INVIS_LVL(ch), NRM, true,
            "%s (%d:%d) didn't have any appropriate areas listed.",
            GET_NAME(ch), GET_LEVEL(ch), GET_REMORT_GEN(ch));
    }
}
