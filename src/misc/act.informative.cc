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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "vehicle.h"
#include "materials.h"
#include "clan.h"
#include "smokes.h"
#include "bomb.h"
#include "fight.h"
#include "specs.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern const struct title_type titles[NUM_CLASSES][LVL_GRIMP + 1];
//extern const struct command_info cmd_info[];
extern struct index_data *obj_index;
extern struct zone_data *zone_table;

extern const byte eq_pos_order[];
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
extern const char *connected_types[];
extern const char *char_class_abbrevs[];
extern const char *level_abbrevs[];
extern const char *player_race[];
extern const char *pc_char_class_types[];
extern const char *room_bits[];
extern const char *sector_types[];
extern const char *spells[];
extern const char *genders[];
extern const char *from_dirs[];
extern const char *material_names[];
extern const int rev_dir[];
extern const char *wear_implantpos[];
extern const char *evil_knight_titles[];
extern const char *moon_sky_types[];
extern const char *soilage_bits[];
extern const char *wear_description[];
extern const struct {double multiplier;int max;} weap_spec_char_class[];
long find_char_class_bitvector(char arg);

void gain_skill_prof(struct char_data *ch, int skillnum);
int isbanned(char *hostname, char *blocking_hostname);
char *obj_cond(struct obj_data *obj);  /** writes to buf2 **/
char *obj_cond_color(struct obj_data *obj, struct char_data *ch);  /**writes to buf2 **/
int same_obj(struct obj_data * obj1, struct obj_data * obj2);

ACMD(do_stand);
ACMD(do_say);

#define HID_OBJ_PROB(ch, obj) \
(GET_LEVEL(ch) + GET_INT(ch) +                    \
 (affected_by_spell(ch,SKILL_HYPERSCAN) ? 40 : 0)+\
 (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY) ? 30 : 0) +\
 (AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) ? 60 : 0) +  \
 (PRF_FLAGGED(ch, PRF_HOLYLIGHT) ? 500 : 0) +     \
 (IS_OBJ_STAT(obj, ITEM_GLOW) ? -20 : 0) +        \
 (IS_OBJ_STAT(obj, ITEM_HUM) ? -20 : 0) +         \
 ((IS_CIGARETTE(obj) && SMOKE_LIT(obj)) ? 10 : 0)+\
 ((IS_OBJ_STAT(obj, ITEM_MAGIC) &&                \
   AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) ? 20 : 0))

#define SHOW_OBJ_ROOM    0
#define SHOW_OBJ_INV     1
#define SHOW_OBJ_CONTENT 2
#define SHOW_OBJ_NOBITS  3
#define SHOW_OBJ_EXTRA   5
#define SHOW_OBJ_BITS    6

    void 
show_obj_to_char(struct obj_data * object, struct char_data * ch,
		 int mode, int count)
{
    bool found = false;

    *buf = '\0';
    if ((mode == SHOW_OBJ_ROOM) && object->description)
        strcpy(buf, object->description);
    else if (!object->description && GET_LEVEL(ch) >= LVL_AMBASSADOR )
        sprintf(buf,"%s exits here.\r\n",object->short_description);
    else if (object->short_description && 
	     ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
        strcpy(buf, object->short_description);
    else if (mode == 5) {
        if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
            if (object->action_description) {
                strcpy(buf, "There is something written upon it:\r\n\r\n");
                strcat(buf, object->action_description);
                page_string(ch->desc, buf, 1);
            } else {
                act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
            }
            return;
        } else if (GET_OBJ_TYPE(object) == ITEM_DRINKCON) 
            strcpy(buf, "It looks like a drink container.");
        else if (GET_OBJ_TYPE(object) == ITEM_FOUNTAIN)
            strcpy(buf, "It looks like a source of drink.");
        else if (GET_OBJ_TYPE(object) == ITEM_FOOD)
            strcpy(buf, "It looks edible.");
        else if (GET_OBJ_TYPE(object) == ITEM_HOLY_SYMB)
            strcpy(buf, "It looks like the symbol of some deity.");
        else if (GET_OBJ_TYPE(object) == ITEM_CIGARETTE ||
             GET_OBJ_TYPE(object) == ITEM_PIPE) {
            if (GET_OBJ_VAL(object, 3))
                strcpy(buf, "It appears to be lit and smoking.");
            else
                strcpy(buf, "It appears to be unlit.");
        } else if (GET_OBJ_TYPE(object) == ITEM_CONTAINER) {
            if (GET_OBJ_VAL(object, 3)) {
                strcpy(buf, "It looks like a corpse.\r\n");
            } else {
                strcpy(buf, "It looks like a container.\r\n");
                if (CAR_CLOSED(object) && CAR_OPENABLE(object)) /*macro maps to containers too */
                    strcat(buf, "It appears to be closed.\r\n");
                else if (CAR_OPENABLE(object)) {
                    strcat(buf, "It appears to be open.\r\n");
                    if (object->contains)
                        strcat(buf, "There appears to be something inside.\r\n");
                    else
                        strcat(buf, "It appears to be empty.\r\n");
                }
            }
        } else if (GET_OBJ_TYPE(object) == ITEM_SYRINGE) {
            if (GET_OBJ_VAL(object, 0)) {
                strcpy(buf, "It is full.");
            } else {
                strcpy(buf, "It's empty.");
            }
        } else if (GET_OBJ_MATERIAL(object) > MAT_NONE &&
               GET_OBJ_MATERIAL(object) < TOP_MATERIAL) {
            sprintf(buf, "It appears to be composed of %s.", 
                material_names[GET_OBJ_MATERIAL(object)]);
        } else {
            strcpy(buf, "You see nothing special.");
        }
    }
    if (mode != 3) {
        found = false;

	if (IS_OBJ_STAT2(object, ITEM2_BROKEN)) {
	    sprintf(buf2, " %s<broken>", CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (object->in_obj && IS_CORPSE(object->in_obj) && IS_IMPLANT(object) &&
	    !CAN_WEAR(object, ITEM_WEAR_TAKE)) {
	    sprintf(buf2, " (implanted)");
	    strcat(buf, buf2);
	    found = true;
	}

	if (ch == object->carried_by || ch == object->worn_by) {
	    if (OBJ_REINFORCED(object)) {
            sprintf(buf2, " %s[%sreinforced%s]%s", CCYEL(ch, C_NRM),
                CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
            strcat(buf, buf2);
	    }
	    if (OBJ_ENHANCED(object)) {
            sprintf(buf2, " %s|enhanced|%s", CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
            strcat(buf, buf2);
	    }
	}

	if (IS_OBJ_TYPE(object, ITEM_DEVICE) &&
	    ENGINE_STATE(object))
	    strcat(buf, " (active)");

	if (((GET_OBJ_TYPE(object) == ITEM_CIGARETTE ||
	      GET_OBJ_TYPE(object) == ITEM_PIPE) && 
	     GET_OBJ_VAL(object, 3)) ||
	    (IS_BOMB(object) && object->contains && IS_FUSE(object->contains) &&
	     FUSE_STATE(object->contains))) {
	    sprintf(buf2, " %s(lit)%s", CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
	    sprintf(buf2, " %s(invisible)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (IS_OBJ_STAT(object, ITEM_TRANSPARENT)) {
	    sprintf(buf2, " %s(transparent)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (IS_OBJ_STAT2(object, ITEM2_HIDDEN)) {
	    sprintf(buf2, " %s(hidden)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}

	if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) || 
	    IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) {
	    if (IS_OBJ_STAT(object, ITEM_BLESS)) {
            sprintf(buf2, " %s(holy aura)%s", 
                CCBLU_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
            strcat(buf, buf2);
            found = true;
	    } else if (IS_OBJ_STAT(object, ITEM_EVIL_BLESS)) {
            sprintf(buf2, " %s(unholy aura)%s", 
                CCRED_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
            strcat(buf, buf2);
            found = true;
	    }
	}
	if ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) || IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) &&
	    IS_OBJ_STAT(object, ITEM_MAGIC)) {
	    sprintf(buf2, " %s(yellow aura)%s", 
		    CCYEL_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
	    strcat(buf, buf2);
	    found = true;
	} 

	if ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) || IS_AFFECTED_2(ch, AFF2_TRUE_SEEING) || PRF_FLAGGED(ch, PRF_HOLYLIGHT)) &&
	    GET_OBJ_SIGIL_IDNUM(object)) {
	    sprintf(buf2, " %s(%ssigil%s)%s", CCYEL(ch, C_NRM), CCMAG(ch, C_NRM), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	}

	if (IS_OBJ_STAT(object, ITEM_GLOW)) {
	    sprintf(buf2, " %s(glowing)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (IS_OBJ_STAT(object, ITEM_HUM)) {
	    sprintf(buf2, " %s(humming)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (IS_OBJ_STAT2(object, ITEM2_ABLAZE)) {
	    sprintf(buf2, " %s*burning*%s", CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
	if (OBJ_SOILED(object, SOIL_BLOOD)) {
	    sprintf(buf2, " %s(bloody)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    strcat(buf, buf2);
	    found = true;
	}
        if ( OBJ_SOILED( object, SOIL_WATER ) ) {
            sprintf(buf2, " %s(wet)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM) );
            strcat(buf, buf2);
            found = true;
        }
        if ( OBJ_SOILED( object, SOIL_MUD ) ) {
            sprintf(buf2, " %s(muddy)%s", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM) );        
            strcat(buf, buf2);
            found = true;
        }
	if( OBJ_SOILED( object, SOIL_ACID ) ) {
            sprintf(buf2, " %s(acid covered)%s", CCGRN(ch, C_NRM), CCNRM( ch, C_NRM ) );
            strcat(buf, buf2);
            found = true;
        }

	if (mode == 0)
	    strcat(buf, CCGRN(ch, C_NRM));	/* This is a Fireball hack. */
    }

    if (!((mode == 0) && !object->description)) {
        if (count > 1) {
            sprintf(buf2, " [%d]", count);
            strcat(buf, buf2);
        } 
        strcat(buf, "\r\n");
    }

    page_string(ch->desc, buf, 1);

    if (GET_OBJ_TYPE(object) == ITEM_VEHICLE && mode == 6) {
        if (CAR_OPENABLE(object)) {
            if (CAR_CLOSED(object))
            act("The door of $p is closed.", TRUE, ch, object, 0, TO_CHAR);
            else
            act("The door of $p is open.", TRUE, ch, object, 0, TO_CHAR);
        }
    }
}

void 
list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		 bool show)
{
    struct obj_data *i = NULL, *o = NULL;
    bool found = false, corpse = false;
    int count = 0;

    if (list && list->in_obj && IS_CORPSE(list->in_obj))
	corpse = true;

    for (i = list; i; i = o) {
	if (!INVIS_OK_OBJ(ch, i) ||
	    GET_OBJ_VNUM(i) == BLOOD_VNUM || GET_OBJ_VNUM(i) == ICE_VNUM ||
	    (IS_OBJ_STAT2(i, ITEM2_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
	     number(50, 120) > HID_OBJ_PROB(ch, i))) {
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
	     i->short_description != i->shared->proto->short_description) ||
	    IS_OBJ_STAT2(i, ITEM2_BROKEN))
	    o = i->next_content; 
	else {
	    count = 1;
	    for (o = i->next_content; o; o = o->next_content) {
		if (same_obj(o, i) &&
		    (!IS_BOMB(o) || !IS_BOMB(i)||same_obj(o->contains,i->contains))) {
		    if (INVIS_OK_OBJ(ch, o))
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
	send_to_char(" Nothing.\r\n", ch);

}

void 
list_obj_to_char_GLANCE(struct obj_data * list, struct char_data * ch, 
			struct char_data *vict, int mode, bool show, 
			int glance)
{
    struct obj_data *i = NULL, *o = NULL;
    bool found = false;
    int count = 0;

    for (i = list; i; i = o) {
	if (!INVIS_OK_OBJ(ch, i) ||
	    (IS_OBJ_STAT2(i, ITEM2_HIDDEN) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
	     number(50, 120) > HID_OBJ_PROB(ch, i))) {
	    o = i->next_content;
	    continue;
	}
	if ((GET_LEVEL(ch) < LVL_AMBASSADOR && 
	     GET_LEVEL(ch) + (GET_SKILL(ch, SKILL_GLANCE) >> ((glance) ? 2 : 0)) < 
	     (number(0, 50) + CHECK_SKILL(vict, SKILL_CONCEAL))) ||
	    (GET_LEVEL(ch) >= LVL_AMBASSADOR && GET_LEVEL(vict) > GET_LEVEL(ch))) {
	    o = i->next_content;
	    continue;
	}
    
	if (IS_CORPSE(i) ||
	    (i->shared->proto &&
	     i->short_description != i->shared->proto->short_description) ||
	    IS_OBJ_STAT2(i, ITEM2_BROKEN))
	    o = i->next_content;
    
	else {
	    for (o = i; o; o = o->next_content) {
		if (same_obj(o, i) &&
		    (!IS_BOMB(o) || !IS_BOMB(i)||same_obj(o->contains,i->contains))) {
		    if (INVIS_OK_OBJ(ch, o))
			count++;
		} else
		    break;
	    }
	}

	show_obj_to_char(i, ch, mode, count);
	count = 0;
	found = true;
    }
    if (!found && show)
	send_to_char("You can't see anything.\r\n", ch);
}

void 
diag_char_to_char(struct char_data * i, struct char_data * ch)
{
    int percent;

    if (GET_MAX_HIT(i) > 0)
	percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
    else
	percent = -1;		/* How could MAX_HIT be < 1?? */

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

    send_to_char(buf, ch);
}
char *
diag_conditions(struct char_data *ch)
{
 
    int percent;
    if (GET_MAX_HIT(ch) > 0)
	percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
    else
	percent = -1;		/* How could MAX_HIT be < 1?? */

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
void
show_trailers_to_char(struct char_data *ch, struct char_data *i) 
{

    if (affected_by_spell(i, SPELL_QUAD_DAMAGE))
	act("...$e is glowing with a bright blue light!",
	    FALSE, i, 0, ch, TO_VICT);

    if (IS_AFFECTED_2(i, AFF2_ABLAZE))
	act("...$s body is blazing like a bonfire!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_BLIND))
	act("...$e is groping around blindly!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_SANCTUARY)) {
        if (IS_EVIL(i))
            act("...$e glows with a dark light!", 
            FALSE, i, 0, ch, TO_VICT);
        else
            act("...$e glows with a bright light!", 
            FALSE, i, 0, ch, TO_VICT);
    }
    else if (IS_AFFECTED(i, AFF_NOPAIN))
	act("...$e looks like $e could take on anything!", 
	    FALSE, i, 0, ch, TO_VICT);
    else if (IS_AFFECTED(i, AFF_GLOWLIGHT))
	act("...$e is followed by a ghostly light.", 
	    FALSE, i, 0, ch, TO_VICT);
    else if (IS_AFFECTED_2(i, AFF2_DIVINE_ILLUMINATION)) {
	if (IS_EVIL(i))
	    act("...$e is followed by an unholy light.", 
		FALSE, i, 0, ch, TO_VICT);
	else if (IS_GOOD(i))
	    act("...$e is followed by a holy light.", 
		FALSE, i, 0, ch, TO_VICT);
	else
	    act("...$e is followed by a sickly light.", 
		FALSE, i, 0, ch, TO_VICT);
    } else if (IS_AFFECTED_2(i, AFF2_FLUORESCENT))
	act("...$e is followed by a fluorescent light.", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_CONFUSION))
	act("...$e is looking around in confusion!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED_3(i, AFF3_SYMBOL_OF_PAIN))
	act("...a symbol of pain burns bright on $s forehead!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_BLUR))
	act("...$s form appears to be blurry and shifting.", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED_2(i, AFF2_FIRE_SHIELD))
	act("...a blazing sheet of fire floats before $s body!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED_2(i, AFF2_BLADE_BARRIER))
	act("...$e is surrounded by whirling blades!", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED_2(i, AFF2_ENERGY_FIELD))
	act("...$e is covered by a crackling field of energy!", 
	    FALSE, i, 0, ch, TO_VICT);

	if (IS_SOULLESS(i))
        act("...a deep red pentagram has been burned into $s forehead!", 
            FALSE, i, 0, ch, TO_VICT);
    else if(IS_AFFECTED_3(i,AFF3_TAINTED))
        act("...the mark of the tainted has been burned into $s forehead!",
        FALSE,i,0,ch,TO_VICT);

    if (IS_AFFECTED_3(i, AFF3_PRISMATIC_SPHERE))
	act("...$e is surrounded by a prismatic sphere of light!", 
	    FALSE, i, 0, ch, TO_VICT);
   
    if(affected_by_spell(i, SPELL_ELECTROSTATIC_FIELD))
        act("...$e is surrounded by a swarming sea of electrons!",
            FALSE, i, 0, ch, TO_VICT);

    if (affected_by_spell(i, SPELL_SKUNK_STENCH) ||
	affected_by_spell(i, SPELL_TROG_STENCH))
	act("...$e is followed by a malodorous stench...", 
	    FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED_2(i, AFF2_PETRIFIED))
	act("...$e is petrified into solid stone.", 
	    FALSE, i, 0, ch, TO_VICT);

    if (affected_by_spell(i, SPELL_ENTANGLE)) {
	if (i->in_room->sector_type == SECT_CITY) 
	    act("...$e is hopelessly entangled in the weeds and sparse vegetation!", 
		FALSE,i,0,ch,TO_VICT);
	else
	    act("...$e is hopelessly entangled in the undergrowth!", 
		FALSE,i,0,ch,TO_VICT);
    }
    if(IS_AFFECTED_3(i,AFF3_GRAVITY_WELL)) {
        act("...spacetime bends around $m in a powerful gravity well!",
        FALSE,i,0,ch,TO_VICT);
    }

    if (IS_AFFECTED_2(i, AFF2_DISPLACEMENT)) {
        if ( affected_by_spell( i, SPELL_REFRACTION ) )
            act( "...$s body is strangely refractive.", FALSE, i, 0, ch, TO_VICT );
    }
}

void 
look_at_char(struct char_data * i, struct char_data * ch, int cmd)
{
    int j, found = 0, app_height, app_weight, h, k, pos;
    char *description = NULL;
    struct affected_type *af = NULL;
    struct char_data *mob = NULL;

    if ((af = affected_by_spell(i, SKILL_DISGUISE))) {
	if ((mob = real_mobile_proto(af->modifier)))
	    description = mob->player.description;
    }
    if (!CMD_IS("glance")) {
	if (description)
	    send_to_char(description, ch);
	else if (!mob && i->player.description)
	    send_to_char(i->player.description, ch);
	else
	    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);
    
	app_height = GET_HEIGHT(i) - number(1,6) + number(1,6);
	app_weight = GET_WEIGHT(i) - number(1,6) + number(1,6);
	if (!IS_NPC(i) && !mob) {
	    sprintf(buf, "%s appears to be a %d cm tall, %d pound %s %s.\r\n", 
		    i->player.name, app_height, app_weight, 
		    genders[(int)GET_SEX(i)], player_race[(int)GET_RACE(i)]);
	    send_to_char(buf, ch);
	}
    }

    diag_char_to_char(i, ch);

    if (CMD_IS("look"))
	show_trailers_to_char(ch, i);

    if (!CMD_IS("glance") && !IS_NPC(i)) {
    
	buf[0] = '\0';

	for (h = 0; h < NUM_WEARS; h++) {
	    pos = (int) eq_pos_order[h];
      
	    if (!GET_EQ(i, pos) && CHAR_SOILAGE(i, pos)) {
		if (ILLEGAL_SOILPOS(pos)) {
		    CHAR_SOILAGE(i, pos) = 0;
		    continue;
		}
		found = FALSE;
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
	    send_to_char(buf, ch);
    }

    if (CMD_IS("examine") || CMD_IS("glance")) {
	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
	    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
		found = TRUE;
    
	if (found) {
	    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
	    for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(i, (int)eq_pos_order[j]) && 
		    CAN_SEE_OBJ(ch, GET_EQ(i, (int)eq_pos_order[j])) &&
		    (!IS_OBJ_STAT2(GET_EQ(i, (int)eq_pos_order[j]), ITEM2_HIDDEN) ||
		     number(50, 120) < 
		     HID_OBJ_PROB(ch, GET_EQ(i, (int)eq_pos_order[j])))) {
		    if (CMD_IS("glance")&&number(50, 100) > CHECK_SKILL(ch, SKILL_GLANCE))
			continue;
		    send_to_char(CCGRN(ch, C_NRM), ch);
		    send_to_char(where[(int)eq_pos_order[j]], ch);
		    send_to_char(CCNRM(ch, C_NRM), ch);
		    show_obj_to_char(GET_EQ(i, (int)eq_pos_order[j]), ch, 1, 0);
		}
	}
	if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR)) {
	    found = FALSE;
	    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
	    list_obj_to_char_GLANCE(i->carrying, ch, i, 1, TRUE, 
				    (GET_LEVEL(ch) >= LVL_AMBASSADOR));
	}
    }
}


void 
list_one_char(struct char_data * i, struct char_data * ch, byte is_group)
{
    char *positions[] = {
	" is lying here, dead.",
	" is lying here, mortally wounded.",
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

    if (AFF2_FLAGGED(i, AFF2_MOUNTED) || 
	(MOB2_FLAGGED(i, MOB2_UNAPPROVED) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
	 !IS_NPC(ch) && !PLR_FLAGGED(ch, PLR_TESTER)))
	return;

    if (AFF_FLAGGED(i, AFF_HIDE)) {

	if (!AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) &&
	    !PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
	    if (AFF_FLAGGED(ch, AFF_SENSE_LIFE) || 
		affected_by_spell(ch, SKILL_HYPERSCAN)) {
		send_to_char("You sense a hidden life form.\r\n", ch);
		if (CAN_SEE(i, ch) && AWAKE(i) && 
		    CHECK_SKILL(i, SKILL_HIDE) > number(30, 120))
		    act("$n seems to suspect your presence.", TRUE, ch, 0, i, TO_VICT);
	    }
	    return;
	} else if (CAN_SEE(i, ch))
	    act("$n seems to have seen you.", TRUE, ch, 0, i, TO_VICT);
    } 

    if (IS_NPC(i) && i->getPosition() == GET_DEFAULT_POS(i)) {
    
	if (!i->player.long_descr) {
	    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		sprintf(buf, "%s exists here.\r\n", i->player.short_descr);
		send_to_char(buf, ch);
	    }
	    return;
	}

	if (IS_AFFECTED(i, AFF_INVISIBLE))
	    strcpy(buf, "*");
	else if (IS_AFFECTED_2(i, AFF2_TRANSPARENT))
	    strcpy(buf, "/*");
	else
	    *buf = '\0';

	if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) ||
	    IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) {
	    if (IS_EVIL(i)) {
		sprintf(buf2, "%s%s(Red Aura)%s ", 
			CCBLD(ch, C_CMP), CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    } else if (IS_GOOD(i)) {
		sprintf(buf2, "%s%s(Blue Aura)%s ", 
			CCBLD(ch, C_CMP), CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		strcat(buf, buf2);
	    }
	}
	strcat(buf, CCYEL(ch, C_NRM));
	if (is_group)
	    strcat(buf, CCBLD(ch, C_CMP));
	strcat(buf, i->player.long_descr);
	send_to_char(buf, ch);

	if (IN_ROOM(ch) != IN_ROOM(i)) 
	    return;
    
	if (!PRF2_FLAGGED(ch, PRF2_NOTRAILERS))
	    show_trailers_to_char(ch, i);
	send_to_char(CCNRM(ch, C_NRM), ch);
	return;
    }
  
    strcpy(buf, CCYEL(ch, C_NRM));
    if (is_group)
	strcat(buf, CCBLD(ch, C_CMP));
  
    if (IS_NPC(i)) {
	strcpy(buf2, i->player.short_descr);
	CAP(buf2);
	strcat(buf, buf2);
    } else if (affected_by_spell(i, SKILL_DISGUISE)) {
	strcpy(buf2, GET_DISGUISED_NAME(ch, i));
	CAP(buf2);
	sprintf(buf, "%s%s", buf, buf2);
    } else
	sprintf(buf, "%s%s %s", buf, i->player.name, GET_TITLE(i));
  
    if (is_group)
	strcat(buf, CCNRM(ch, C_CMP));

    if (IS_AFFECTED(i, AFF_INVISIBLE)) {
	sprintf(buf2, " %s(invisible)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    } else if (IS_AFFECTED_2(i, AFF2_TRANSPARENT)) {
	sprintf(buf2, " %s(transparent)%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }
    if (IS_AFFECTED(i, AFF_HIDE)) {
	sprintf(buf2, " %s(hidden)%s", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }
    if (!IS_NPC(i) && !i->desc) {
	sprintf(buf2, " %s(linkless)%s", CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }
    if (PLR_FLAGGED(i, PLR_WRITING)) {
	sprintf(buf2, " %s(writing)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    } else if (PLR_FLAGGED(i, PLR_OLC)) {
	sprintf(buf2, " %s(creating)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    } else if (i->desc && i->desc->showstr_point && 
	       !PRF2_FLAGGED(i, PRF2_LIGHT_READ)) {
	sprintf(buf2, " %s(reading)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }
    if (PLR_FLAGGED(i, PLR_AFK)) {
	sprintf(buf2, " %s(afk)%s", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }

    strcat(buf, CCNRM(ch, C_CMP));
    strcat(buf, CCYEL(ch, C_NRM));	/* This is a Fireball hack. */
    if (is_group)
	strcat(buf, CCBLD(ch, C_CMP));


    if ((i->getPosition() != POS_FIGHTING) && (i->getPosition() != POS_MOUNTED)) {
	if (IS_AFFECTED_2(i, AFF2_MEDITATE) && i->getPosition() == POS_SITTING)
	    strcat(buf, " is meditating here.");
	else if (AFF3_FLAGGED(i, AFF3_STASIS) && i->getPosition() == POS_SLEEPING)
	    strcat(buf, " is lying here in a static state.");
	else if ((SECT_TYPE(i->in_room) == SECT_WATER_NOSWIM ||
		  SECT_TYPE(i->in_room) == SECT_WATER_SWIM   ||
		  SECT_TYPE(i->in_room) == SECT_FIRE_RIVER) &&
		 (!IS_AFFECTED(i, AFF_WATERWALK) || ch->getPosition() < POS_STANDING))
	    strcat(buf, " is swimming here.");
	else if(SECT_TYPE(i->in_room) ==SECT_UNDERWATER && i->getPosition() > POS_RESTING)
	    strcat(buf, " is swimming here.");
	else if (SECT_TYPE(i->in_room) == SECT_PITCH_PIT && i->getPosition() < POS_FLYING)
	    strcat(buf, " is struggling in the pitch.");
	else if (SECT_TYPE(i->in_room) == SECT_PITCH_SUB)
	    strcat(buf, " is struggling blindly in the pitch.");
	else
	    strcat(buf, positions[(int) MAX(0, MIN(i->getPosition(), POS_SWIMMING))]);
    } else {
	if (FIGHTING(i)) {
	    strcat(buf, " is here, fighting ");
	    if (FIGHTING(i) == ch)
		strcat(buf, "YOU!");
	    else {
		if (i->in_room == FIGHTING(i)->in_room)
		    strcat(buf, PERS(FIGHTING(i), ch));
		else
		    strcat(buf, "someone who has already left");
		strcat(buf, "!");
	    }
	} else if (MOUNTED(i)) {
	    strcat(buf, " is here, mounted on ");
	    if (MOUNTED(i) == ch)
		strcat(buf, "YOU!, heheheh...");
	    else {
		if (i->in_room == MOUNTED(i)->in_room)
		    strcat(buf, PERS(MOUNTED(i), ch));
		else {
		    strcat(buf, "someone who has already left");
		    i->setPosition( POS_STANDING );
		    MOUNTED(i) = NULL;
		}
		strcat(buf, ".");
	    }
	} else
	    strcat(buf, " is here struggling with thin air.");
    }

    if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) ||
	IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) {
	if (IS_EVIL(i))
	    sprintf(buf, "%s %s%s(Red Aura)%s", buf, 
		    CCRED(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
	else if (IS_GOOD(i))
	    sprintf(buf, "%s %s%s(Blue Aura)%s", buf,
		    CCBLU(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    if (!PRF2_FLAGGED(ch, PRF2_NOTRAILERS))
	show_trailers_to_char(ch, i);

    send_to_char(CCNRM(ch, C_NRM), ch);
    send_to_char(CCYEL(ch, C_NRM), ch);

}

void 
list_char_to_char(struct char_data * list, struct char_data * ch)
{
    struct char_data *i;
    byte is_group = FALSE;

    for (i = list; i; i = i->next_in_room) {
	is_group = 0;
	if (ch != i && 
	    INVIS_OK(ch, i) &&
	    !IS_AFFECTED_2(i, AFF2_MOUNTED) &&
	    (ch->in_room == i->in_room || PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
	     AFF2_FLAGGED(ch, AFF2_TRUE_SEEING) ||
	     !AFF_FLAGGED(i, AFF_HIDE | AFF_SNEAK))) {
	    if (CAN_SEE(ch, i)) {
		if (IS_AFFECTED(ch, AFF_GROUP) && IS_AFFECTED(i, AFF_GROUP)) {
		    if (ch->master) {
			if (i == ch->master || (i->master && i->master == ch->master))
			    is_group = 1;
		    } else {
			if (i->master && i->master == ch)
			    is_group = 1;
		    }
		}
		list_one_char(i, ch, is_group);

	    } else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
		       IS_AFFECTED(i, AFF_INFRAVISION)) {
		if (!number(0, 1))
		    send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
		else
		    send_to_char("A pair of eyes glow red in the darkness.\r\n", ch);
	    }
	}
    }
}


void 
do_auto_exits(struct char_data *ch, struct room_data *room)
{

    int door;

    *buf = '\0';

    if (room == NULL)
	room = ch->in_room;

    for (door = 0; door < NUM_OF_DIRS; door++)
	if (ABS_EXIT(room, door) && ABS_EXIT(room, door)->to_room != NULL &&
	    !IS_SET(ABS_EXIT(room, door)->exit_info, EX_CLOSED | EX_NOPASS))
	    sprintf(buf, "%s%c ", buf, LOWER(*dirs[door]));;

    sprintf(buf2, "%s[ Exits: %s]%s   ", CCCYN(ch, C_NRM),
	    *buf ? buf : "None obvious ", CCNRM(ch, C_NRM));

    send_to_char(buf2, ch);

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
	*buf = '\0';
    
	for (door = 0; door < NUM_OF_DIRS; door++)
	    if (ABS_EXIT(room, door) && ABS_EXIT(room, door)->to_room != NULL &&
		IS_SET(ABS_EXIT(room, door)->exit_info, EX_CLOSED | EX_NOPASS))
		sprintf(buf, "%s%c ", buf, LOWER(*dirs[door]));
    
	sprintf(buf2, "%s[ Closed Doors: %s]%s\r\n", CCCYN(ch, C_NRM),
		*buf ? buf : "None ", CCNRM(ch, C_NRM));
    
	send_to_char(buf2, ch);
    } else 
	send_to_char("\r\n", ch);
}

/* functions and macros for 'scan' command */
void 
list_scanned_chars(struct char_data * list, struct char_data * ch, 
		   int distance, int door)
{
    const char *how_far[] = {
	"close by",
	"a ways off",
	"far off to the"
    };

    struct char_data *i;
    int count = 0;
    *buf = '\0';

    /* this loop is a quick, easy way to help make a grammatical sentence
       (i.e., "You see x, x, y, and z." with commas, "and", etc.) */
  
    for (i = list; i; i = i->next_in_room) {

	/* put any other conditions for scanning someone in this if statement -
	   i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever */
    
	if (CAN_SEE(ch, i) && ch != i &&
	    (!AFF_FLAGGED(i, AFF_SNEAK | AFF_HIDE) || 
	     PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
	    count++;
    }

    if (!count)
	return;
  
    for (i = list; i; i = i->next_in_room) {
    
	/* make sure to add changes to the if statement above to this one also, using
	   or's to join them.. i.e., 
	   if (!CAN_SEE(ch, i) || !condition2 || !condition3) */
    
	if (!CAN_SEE(ch, i) || ch == i || 
	    ((AFF_FLAGGED(i, AFF_SNEAK | AFF_HIDE)) &&
	     !PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
	    continue; 
	if (!*buf)
	    sprintf(buf, "You see %s%s%s",
		    CCYEL(ch, C_NRM), GET_DISGUISED_NAME(ch, i), CCNRM(ch, C_NRM));
	else 
	    sprintf(buf, "%s%s%s%s", 
		    buf, CCYEL(ch, C_NRM), GET_DISGUISED_NAME(ch, i), 
		    CCNRM(ch, C_NRM));
	if (--count > 1)
	    strcat(buf, ", ");
	else if (count == 1)
	    strcat(buf, " and ");
	else {
	    sprintf(buf2, " %s %s.\r\n", how_far[distance], dirs[door]);
	    strcat(buf, buf2);      
	}
    }
    send_to_char(buf, ch);
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

    if (IS_AFFECTED(ch, AFF_BLIND) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
	send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
	return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) && 
	GET_LEVEL(ch) < LVL_AMBASSADOR) {
	send_to_char("The room is too smoky to see very far.\r\n", ch);
	return;
    }
    /* may want to add more restrictions here, too */
    send_to_char("You quickly scan the area.\r\n", ch);
    act("$n quickly scans $s surroundings.", TRUE, ch, 0, 0, TO_ROOM);
  
    for (door = 0; door < NUM_OF_DIRS - 4; door++) /* don't scan up/down */
	if (EXIT(ch, door) && EXIT(ch, door)->to_room != NULL &&
	    EXIT(ch, door)->to_room != ch->in_room &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
	    !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	    !IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR) &&
	    !IS_SET(EXIT(ch, door)->exit_info, EX_NOSCAN)) {
	    if (EXIT(ch, door)->to_room->people) {
		list_scanned_chars(EXIT(ch, door)->to_room->people, ch, 0, door);
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
			list_scanned_chars(_2ND_EXIT(ch, door)->to_room->people, 
					   ch, 1, door);
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
			    list_scanned_chars(_3RD_EXIT(ch, door)->to_room->people, 
					       ch, 2, door);
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
    char buf3[MAX_STRING_LENGTH];

    *buf = '\0';

    if (IS_AFFECTED(ch, AFF_BLIND)) {
	send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
	return;
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
	!PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
	send_to_char("The thick smoke is too disorienting to tell.\r\n", ch);
	return;
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
	if (EXIT(ch, door) && EXIT(ch, door)->to_room != NULL &&
	    (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ||
	     GET_LEVEL(ch) >= LVL_AMBASSADOR) &&
	    (!IS_SET(EXIT(ch, door)->exit_info, EX_NOPASS) ||
	     GET_LEVEL(ch) >= LVL_AMBASSADOR)) {

	    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		sprintf(buf2, "%s%s", CCBLD(ch, C_SPR), CCBLU(ch, C_NRM));
		sprintf(buf3, "%-8s%s - %s[%s%5d%s] %s%s%s%s%s%s\r\n",
			dirs[door], CCNRM(ch, C_NRM), 
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			EXIT(ch, door)->to_room->number, CCGRN(ch, C_NRM),
			CCCYN(ch, C_NRM), EXIT(ch, door)->to_room->name, 
			CCNRM(ch, C_SPR), 
			IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ?
			" (closed)" : "",
			IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ?
			" (secret)" : "",
			IS_SET(EXIT(ch, door)->exit_info, EX_NOPASS) ?
			" (nopass)" : "");
		strcat(buf2, CAP(buf3));
	    } else {
		sprintf(buf2, "%s%s", CCBLD(ch, C_SPR), CCBLU(ch, C_NRM));
		sprintf(buf3, "%-8s%s - ", dirs[door], CCNRM(ch, C_SPR));
		strcat(buf2, CAP(buf3));
		if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch) &&
		    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH))
		    strcat(buf2, "Too dark to tell\r\n");
		else {
		    strcat(buf2, CCCYN(ch, C_NRM));
			if(EXIT(ch, door)->to_room->name)
				strcat(buf2, EXIT(ch, door)->to_room->name);
			else
				strcat(buf2, "(null)");
		    strcat(buf2, CCNRM(ch, C_NRM));
		    strcat(buf2, "\r\n");
		}
	    }
	    strcat(buf, CAP(buf2));
	}
    sprintf(buf2, "%s%sObvious Exits:%s\r\n", CCRED(ch, C_NRM), CCBLD(ch, C_CMP),
	    CCNRM(ch, C_NRM));
    send_to_char(buf2, ch);

    if (*buf)
	send_to_char(buf, ch);
    else
	send_to_char(" None.\r\n", ch);
}



void 
look_at_room(struct char_data * ch, struct room_data *room, int ignore_brief)
{

    struct room_affect_data *aff = NULL;
    struct obj_data *o = NULL;

    int ice_shown = 0;    // 1 if ice has already been shown to room...same for blood
    int blood_shown = 0; 

    if (room == NULL)
		room = ch->in_room;

    if (!ch->desc)
		return;

    if (!LIGHT_OK(ch)) {
		send_to_char("It is pitch black...\r\n", ch);
		return;
    }
    send_to_char(CCCYN(ch, C_NRM), ch);
    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS) ||
	(ch->desc->original && PRF_FLAGGED(ch->desc->original, PRF_ROOMFLAGS))) {
		sprintbit((long) ROOM_FLAGS(room), room_bits, buf);
		sprintf(buf2, "[%5d] %s [ %s] [ %s ]", room->number,
			room->name, buf, 
			sector_types[room->sector_type]);
		send_to_char(buf2, ch);
    } else {
		send_to_char(room->name, ch);
	}

    send_to_char(CCNRM(ch, C_NRM), ch);
    send_to_char("\r\n", ch);

    if (((!PRF_FLAGGED(ch, PRF_BRIEF) &&
	  (!ch->desc->original || !PRF_FLAGGED(ch->desc->original, PRF_BRIEF))) ||
	 ignore_brief ||
	 ROOM_FLAGGED(room, ROOM_DEATH)) &&
	(!ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) || 
	 PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
	 ROOM_FLAGGED(room, ROOM_DEATH)))
	send_to_char(room->description, ch);

    for (aff = room->affects; aff; aff = aff->next)
		if (aff->description)
			send_to_char(aff->description, ch);

    if ((GET_LEVEL(ch) >= LVL_AMBASSADOR || 
	 !ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) ||
	 AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))) {
		
		/* autoexits */
		if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
			do_auto_exits(ch, room);
		/* now list characters & objects */
		
		// Why was this if here ?
		//if (ch->in_room == room) {
	    for (o = room->contents; o; o = o->next_content) {
			if (GET_OBJ_VNUM(o) == BLOOD_VNUM) {
				if(!blood_shown) {
				sprintf(buf,
					"%s%s.%s\r\n",
					CCRED(ch, C_NRM),
					GET_OBJ_TIMER(o) < 10 ?
					"Some blood is splattered around here" :
					GET_OBJ_TIMER(o) < 20 ?
					"Some small pools of blood are here" :
					GET_OBJ_TIMER(o) < 30 ?
					"Some large pools of blood are here" :
					GET_OBJ_TIMER(o) < 40 ?
					"Blood is pooled and splattered all over everything here":
					"Dark red blood covers everything in sight here",
					CCNRM(ch, C_NRM));
				blood_shown = 1;
				send_to_char(buf, ch);
				}
			}
		
			if (GET_OBJ_VNUM(o) == ICE_VNUM) {
				if(!ice_shown) {
				sprintf(buf,
					"%s%s.%s\r\n",
					CCCYN(ch, C_NRM),
					GET_OBJ_TIMER(o) < 10 ?
					"A few patches of ice are scattered around here" :
					GET_OBJ_TIMER(o) < 20 ?
					"A thin coating of ice covers everything here" :
					GET_OBJ_TIMER(o) < 30 ?
					"A thick coating of ice covers everything here" : 
					"Everything is covered with a thick coating of ice",
					CCNRM(ch, C_NRM));
				ice_shown = 1;
				send_to_char(buf, ch);
				break;
				}
			} 
	    }
			 
	    send_to_char(CCGRN(ch, C_NRM), ch);
	    list_obj_to_char(room->contents, ch, 0, FALSE);
	    send_to_char(CCYEL(ch, C_NRM), ch);
	    list_char_to_char(room->people, ch);
	    send_to_char(CCNRM(ch, C_NRM), ch);
    }
}

void 
look_in_direction(struct char_data * ch, int dir)
{
#define EXNUMB EXIT(ch, dir)->to_room
    struct room_affect_data *aff;

    if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
	GET_LEVEL(ch) < LVL_AMBASSADOR) {
	send_to_char("The thick smoke limits your vision.\r\n", ch);
	return;
    }
    if (EXIT(ch, dir)) {
	if (EXIT(ch, dir)->general_description) {
	    send_to_char(EXIT(ch, dir)->general_description, ch);
	}
	else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR | EX_CLOSED) &&
		 EXIT(ch, dir)->keyword) {
	    sprintf(buf, "You see %s %s.\r\n", AN(fname(EXIT(ch, dir)->keyword)),
		    fname(EXIT(ch, dir)->keyword));
	    send_to_char(buf, ch);
	}
	else if (EXNUMB) {
	    if ((IS_SET(EXIT(ch, dir)->exit_info, EX_NOPASS) && 
		 GET_LEVEL(ch) < LVL_AMBASSADOR) ||
		IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN))
		send_to_char("You see nothing special.\r\n", ch);

	    else if (EXNUMB->name) { 
		send_to_char(CCCYN(ch, C_NRM), ch);
		if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
		    sprintbit((long) ROOM_FLAGS(EXNUMB), room_bits, buf);
		    sprintf(buf2, "[%5d] %s [ %s] [ %s ]", EXNUMB->number,
			    EXNUMB->name, buf, 
			    sector_types[EXNUMB->sector_type]);
		    send_to_char(buf2, ch);
		} else
		    send_to_char(EXNUMB->name, ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
		send_to_char("\r\n", ch);
		for (aff = ch->in_room->affects; aff; aff = aff->next)
		    if (aff->type == dir && aff->description)
			send_to_char(aff->description, ch);
	    } else
		send_to_char("You see nothing special.\r\n", ch);
	}     
	if (EXNUMB && (!IS_SET(EXIT(ch, dir)->exit_info, 
			       EX_ISDOOR | EX_CLOSED | EX_HIDDEN | EX_NOPASS) ||
		       PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
      
	    if (IS_DARK(EXNUMB) && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) &&
		CHECK_SKILL(ch, SKILL_NIGHT_VISION) < number(GET_LEVEL(ch), 101))
		send_to_char("It's too dark there to see anything special.\r\n", ch);
	    else {
	
		/* now list characters & objects */
		send_to_char(CCGRN(ch, C_NRM), ch);
		list_obj_to_char(EXNUMB->contents, ch, 0, FALSE);
		send_to_char(CCYEL(ch, C_NRM), ch);
		list_char_to_char(EXNUMB->people, ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
	    }
	} 
	//      send_to_char("You see nothing special.\r\n", ch);
    
    
	if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN | EX_NOPASS)) {
	    if (IS_SET(EXIT(ch,dir)->exit_info,EX_CLOSED)&&EXIT(ch, dir)->keyword) {
		sprintf(buf, "The %s %s closed.\r\n", fname(EXIT(ch, dir)->keyword),
			ISARE(fname(EXIT(ch, dir)->keyword)));
		send_to_char(buf, ch);
	    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && 
		       EXIT(ch, dir)->keyword) {
		sprintf(buf, "The %s %s open.\r\n", fname(EXIT(ch, dir)->keyword),
			ISARE(fname(EXIT(ch, dir)->keyword)));
		send_to_char(buf, ch);
		if (EXNUMB != NULL && IS_DARK(EXNUMB) && 
		    !EXIT(ch, dir)->general_description) {
		    sprintf(buf, 
			    "It's too dark through the %s to see anything.\r\n", 
			    fname(EXIT(ch, dir)->keyword));
		    send_to_char(buf, ch);
		}
	    }
	}

    } else {

	if (ch->in_room->sector_type == SECT_PITCH_SUB) {
	    send_to_char("You cannot see anything through the black pitch.\r\n", ch);
	    return;
	}
	switch (dir) {
	case FUTURE:
	    if (ch->in_room->zone->plane >= PLANE_HELL_1 && 
		ch->in_room->zone->plane <= PLANE_HELL_9)
		send_to_char("From here, it looks like things are only going to get worse.\r\n", ch);
	    else if (ch->in_room->zone->plane == PLANE_ASTRAL)
		send_to_char("You gaze into infinity.\r\n", ch);
	    else
		send_to_char("You try and try, but cannot see into the future.\r\n", 
			     ch);
	    break;
	case PAST:
	    if (ch->in_room->zone->plane == PLANE_ASTRAL)
		send_to_char("You gaze into infinity.\r\n", ch);
	    else
		send_to_char("You try and try, but cannot see into the past.\r\n", ch);
	    break;
	case UP:
	    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS)) 
		send_to_char("You see the ceiling above you.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_8)
		send_to_char("Snow and sleet rage across the sky.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_6 ||
		     GET_PLANE(ch->in_room) == PLANE_HELL_7)
		send_to_char("The sky is alight with clouds of blood-red steam.\r\n", 
			     ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_5)
		send_to_char("You see the jet black sky above you, lit only by flashes of lightning.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_4)
		send_to_char("The sky is a deep red, covered by clouds of dark ash.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_3)
		send_to_char("The sky is cold and grey.\r\n"
			     "It looks like the belly of one giant thunderstorm.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_2)
		send_to_char("The sky is dull green, flickering with lightning.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_HELL_1)
		send_to_char("The dark sky is crimson and starless.\r\n", ch);
	    else if (GET_PLANE(ch->in_room) == PLANE_COSTAL)
		send_to_char("Great swirls of pink, green, and blue cover the sky.\r\n", ch);
	    else if ((ch->in_room->sector_type == SECT_CITY) ||
		     (ch->in_room->sector_type == SECT_FOREST)||
		     (ch->in_room->sector_type == SECT_HILLS) ||
		     (ch->in_room->sector_type == SECT_FIELD) ||
		     (ch->in_room->sector_type == SECT_CORNFIELD) ||
		     (ch->in_room->sector_type == SECT_SWAMP) ||
		     (ch->in_room->sector_type == SECT_DESERT) ||
		     (ch->in_room->sector_type == SECT_JUNGLE) ||
		     (ch->in_room->sector_type == SECT_ROAD))  {
		if (ch->in_room->zone->weather->sunlight == SUN_DARK) {
		    if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
			send_to_char("Lightning flashes across the dark sky above you.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_RAINING)
			send_to_char("Rain pelts your face as you look to the night sky.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_CLOUDLESS)
			send_to_char("Glittering stars shine like jewels upon the sea.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_CLOUDY)
			send_to_char("Thick dark clouds obscure the night sky.\r\n", ch);
		    else send_to_char("You see the sky above you.\r\n", ch);
		} else {
		    if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
			send_to_char("Lightning flashes across the sky above you.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_RAINING)
			send_to_char("Rain pelts your face as you look to the sky.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_CLOUDLESS)
			send_to_char("You see the expansive blue sky, not a cloud in sight.\r\n", ch);
		    else if (ch->in_room->zone->weather->sky == SKY_CLOUDY)
			send_to_char("Thick dark clouds obscure the sky above you.\r\n", ch);
		    else send_to_char("You see the sky above you.\r\n", ch);
		}
	    } else if ((ch->in_room->sector_type == SECT_UNDERWATER))
		send_to_char("You see water above you.\r\n", ch);
	    else 
		send_to_char("Nothing special there...\r\n", ch);
	    break;
	case DOWN:
	    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
		send_to_char("You see the floor below you.\r\n", ch);
	    else if ((ch->in_room->sector_type == SECT_CITY) ||
		     (ch->in_room->sector_type == SECT_FOREST)||
		     (ch->in_room->sector_type == SECT_HILLS) ||
		     (ch->in_room->sector_type == SECT_FIELD) || 
		     (ch->in_room->sector_type == SECT_CORNFIELD) || 
		     (ch->in_room->sector_type == SECT_JUNGLE) || 
		     (ch->in_room->sector_type == SECT_ROAD))  
		send_to_char("You see the ground below you.\r\n", ch);
	    else if (ch->in_room->sector_type == SECT_MOUNTAIN) 
		send_to_char("You see the rocky ground below you.\r\n", ch);
	    else if ((ch->in_room->sector_type == SECT_WATER_SWIM) ||
		     (ch->in_room->sector_type == SECT_WATER_NOSWIM))  
		send_to_char("You see the water below you.\r\n", ch);
	    else if ( ch->in_room->isOpenAir() ) {
		if (ch->in_room->sector_type == SECT_FLYING ||
		    ch->in_room->sector_type == SECT_ELEMENTAL_AIR )
		    send_to_char("Below your feet is thin air.\r\n", ch);
		else if ( ch->in_room->sector_type == SECT_ELEMENTAL_LIGHTNING )
		    send_to_char( "Elemental lightning flickers beneath your feet.\r\n", ch );
		else
		    send_to_char( "Nothing substantial lies below your feet.\r\n", ch );
	    }
	    else if (ch->in_room->sector_type == SECT_DESERT) 
		send_to_char("You see the sands below your feet.\r\n", ch);
	    else if (ch->in_room->sector_type == SECT_SWAMP) 
		send_to_char("You see the swampy ground below your feet.\r\n", ch);
	    else 
		send_to_char("Nothing special there...\r\n", ch);
	    break;
	default:
	    send_to_char("Nothing special there...\r\n", ch);
	    break;
	}
    }
}


void 
look_in_obj(struct char_data * ch, char *arg)
{
    struct obj_data *obj = NULL;
    struct char_data *dummy = NULL;
    int amt, bits;
    struct room_data * room_was_in = NULL;

    if (!*arg)
	send_to_char("Look in what?\r\n", ch);
    else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				   FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
	sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
	send_to_char(buf, ch);
    } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	       (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	       (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
	       (GET_OBJ_TYPE(obj) != ITEM_PIPE) &&
	       (GET_OBJ_TYPE(obj) != ITEM_VEHICLE))
	send_to_char("There's nothing inside that!\r\n", ch);
    else {
	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
	    if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED) &&
		!GET_OBJ_VAL(obj, 3))
		send_to_char("It is closed.\r\n", ch);
	    else {
		send_to_char(obj->short_description, ch);
		switch (bits) {
		case FIND_OBJ_INV:
		    send_to_char(" (carried): \r\n", ch);
		    break;
		case FIND_OBJ_ROOM:
		    send_to_char(" (here): \r\n", ch);
		    break;
		case FIND_OBJ_EQUIP:
		case FIND_OBJ_EQUIP_CONT:
		    send_to_char(" (used): \r\n", ch);
		    break;
		}

		list_obj_to_char(obj->contains, ch, 2, TRUE);
	    }
	} else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
	    if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
		act("The door of $p is closed, and you can't see in.", 
		    FALSE, ch, obj, 0, TO_CHAR);
	    else if (real_room(ROOM_NUMBER(obj)) != NULL) {
		act("Inside $p you see:", FALSE, ch, obj, 0, TO_CHAR);
		room_was_in = ch->in_room;
		char_from_room(ch);
		char_to_room(ch, real_room(ROOM_NUMBER(obj)));
		list_char_to_char(ch->in_room->people, ch);
		act("$n looks in from the outside.", FALSE,ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, room_was_in);
	    }
	} else if (GET_OBJ_TYPE(obj) == ITEM_PIPE) {
	    if (GET_OBJ_VAL(obj, 0))
		send_to_char("There appears to be some tobacco in it.\r\n", ch);
	    else
		send_to_char("There is nothing in it.\r\n", ch);

	} else {		/* item must be a fountain or drink container */
	    if (GET_OBJ_VAL(obj, 1) == 0)
		send_to_char("It is empty.\r\n", ch);
	    else {
		if (GET_OBJ_VAL(obj, 1) < 0)
		    amt = 3;
		else
		    amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
		sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt],
			color_liquid[GET_OBJ_VAL(obj, 2)]);
		send_to_char(buf, ch);
	    }
	}
    }
}



char *
find_exdesc(char *word, struct extra_descr_data * list, int find_exact=0)
{
    struct extra_descr_data *i;

	if(!find_exact) {
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
look_at_target(struct char_data * ch, char *arg, int cmd)
{
    int bits, found = 0, j;
    struct char_data *found_char = NULL;
    struct obj_data *obj = NULL, *found_obj = NULL, *car;
    char *desc;

    if (!*arg) {
	send_to_char("Look at what?\r\n", ch);
	return;
    }
    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
			FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
	look_at_char(found_char, ch, cmd);
	if (ch != found_char) {
	    if (CAN_SEE(found_char, ch)) {
		if (CMD_IS("examine"))
		    act("$n examines you.", TRUE, ch, 0, found_char, TO_VICT);
		else
		    act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
	    }
	    act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
	}
	return;
    }
    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, ch->in_room->ex_description)) != NULL) {
	page_string(ch->desc, desc, 1);
	return;
    } 
    /* Does the argument match a door keyword anywhere in the room? */
    for (j = 0; j < 8; j++) {
	if (EXIT(ch, j)&&EXIT(ch, j)->keyword &&isname(arg,EXIT(ch, j)->keyword)) {
	    sprintf(buf, "The %s %s %s.\r\n", fname(EXIT(ch, j)->keyword), 
		    PLUR( fname( EXIT(ch, j)->keyword ) ) ? "are" : "is",
		    IS_SET(EXIT(ch, j)->exit_info, EX_CLOSED) ? "closed" : "open");
	    send_to_char(buf, ch);
	    return;
	}
    }
    /* Does the argument match an extra desc in the char's equipment? */
    for (j = 0; j < NUM_WEARS && !found; j++)
	if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)) && 
	    (GET_OBJ_VAL(GET_EQ(ch, j), 3) != -999 || 
	     isname(arg, GET_EQ(ch, j)->name)))
	    if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) {
		page_string(ch->desc, desc, 1);
		found = 1;
		bits = 1;
		found_obj = GET_EQ(ch, j);
	    }
    /* Does the argument match an extra desc in the char's inventory? */
    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
	if (CAN_SEE_OBJ(ch, obj) && 
	    (GET_OBJ_VAL(obj, 3) != -999 || isname(arg, obj->name)))
	    if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
		page_string(ch->desc, desc, 1);
		found = bits = 1;
		found_obj = obj;
		break;
	    }
    }

    /* Does the argument match an extra desc of an object in the room? */
    for (obj = ch->in_room->contents;obj&&!found; obj = obj->next_content)
	if (CAN_SEE_OBJ(ch, obj) && 
	    (GET_OBJ_VAL(obj, 3) != -999 || isname(arg, obj->name)))
	    if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
		page_string(ch->desc, desc, 1);
		found = bits = 1;
		found_obj = obj;
		break;
	    }
  
    if (bits) {			/* If an object was found back in
				 * generic_find */
	if (found_obj) {
	    if (IS_OBJ_TYPE(found_obj, ITEM_WINDOW)) {
		if (!CAR_CLOSED(found_obj)) {
		    if (real_room(GET_OBJ_VAL(found_obj, 0)) != NULL)
			look_at_room(ch, real_room(GET_OBJ_VAL(found_obj, 0)), 1);
		} else
		    act("$p is closed right now.", FALSE, ch, found_obj, 0, TO_CHAR);
		found = 1;
	    } 
	    else if (IS_V_WINDOW(found_obj)) {
	
		for (car = object_list; car; car = car->next)
		    if (GET_OBJ_VNUM(car) == V_CAR_VNUM(found_obj) &&
			ROOM_NUMBER(car) == ROOM_NUMBER(found_obj) &&
			car->in_room)
			break;
	
		if (car) {
		    act("You look through $p.", FALSE, ch, found_obj, 0, TO_CHAR);
		    look_at_room(ch, car->in_room, 1);
		    found = 1;
		}
	    } else if (!found)
		show_obj_to_char(found_obj, ch, 5, 0);	/* Show no-description */
	    else 
		show_obj_to_char(found_obj, ch, 6, 0);	/* Find hum, glow etc */
	}

    } else if (!found)
	send_to_char("You do not see that here.\r\n", ch);
}

void 
glance_at_target(struct char_data * ch, char *arg, int cmd)
{
    int bits;
    struct char_data *found_char = NULL;
    struct obj_data  *found_obj = NULL;
    char glancebuf[512];

    if (!*arg) {
	send_to_char("Look at what?\r\n", ch);
	return;
    }
    bits = generic_find(arg, FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
	look_at_char(found_char, ch, cmd);  /** CMD_IS("glance") !! **/
	if (ch != found_char) {
	    if (CAN_SEE(found_char, ch) && 
		((GET_SKILL(ch, SKILL_GLANCE) + GET_LEVEL(ch)) < 
		 (number(0, 101) + GET_LEVEL(found_char)))) {
		act("$n glances sidelong at you.", TRUE, ch, 0, found_char, TO_VICT);
		act("$n glances sidelong at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
	
		if (IS_NPC(found_char) && !FIGHTING(found_char) && AWAKE(found_char)&&
		    (!found_char->master || found_char->master != ch)) {
		    if (IS_ANIMAL(found_char) || IS_BUGBEAR(found_char) ||
			GET_INT(found_char) < number(3, 5)) {
			act("$N growls at you.", FALSE, ch, 0, found_char, TO_CHAR);
			act("$N growls at $n.", FALSE, ch, 0, found_char, TO_NOTVICT);
		    } else if (IS_UNDEAD(found_char)) {
			act("$N regards you with an icy glare.",
			    FALSE,ch,0,found_char, TO_CHAR);
			act("$N regards $n with an icy glare.", 
			    FALSE, ch, 0, found_char, TO_NOTVICT);
		    } else if (IS_MINOTAUR(found_char) || IS_DEVIL(found_char) ||
			       IS_DEMON(found_char) || IS_MANTICORE(found_char)) {
			act("$N roars at you.", FALSE, ch, 0, found_char, TO_CHAR);
			act("$N roars at $n.", FALSE, ch, 0, found_char, TO_NOTVICT);
		    } else if (!number(0, 4)) {
			sprintf(glancebuf, "Piss off, %s.", 
				GET_DISGUISED_NAME(found_char, ch));
			do_say(found_char, glancebuf, 0, 0);
		    } else if (!number(0, 3)) {
			sprintf(glancebuf, "Take a hike, %s.",
				GET_DISGUISED_NAME(found_char, ch));
			do_say(found_char, glancebuf, 0, 0);
		    } else if (!number(0, 2)) {
			sprintf(glancebuf, "%s You lookin' at me?",
				GET_DISGUISED_NAME(found_char, ch));
			do_say(found_char, glancebuf, 0, SCMD_SAY_TO);
		    } else if (!number(0, 1)) {
			sprintf(glancebuf, "Hit the road, %s.",
				GET_DISGUISED_NAME(found_char, ch));
			do_say(found_char, glancebuf, 0, 0);
		    } else {
			sprintf(glancebuf, "Get lost, %s.",
				GET_DISGUISED_NAME(found_char, ch));
			do_say(found_char, glancebuf, 0, 0);
		    }

		    if (MOB_FLAGGED(found_char, MOB_AGGRESSIVE)  &&
			(GET_MORALE(found_char) > 
			 number(GET_LEVEL(ch) >> 1, GET_LEVEL(ch))) &&
			!PRF_FLAGGED(ch, PRF_NOHASSLE)) {
			if (found_char->getPosition() >= POS_SITTING) {
			    if (peaceful_room_ok(found_char, ch, false))
				hit(found_char, ch, TYPE_UNDEFINED);
			} else
			    do_stand(found_char, "", 0, 0);
		    }
		}
	    } else
		gain_skill_prof(ch, SKILL_GLANCE);
	}
    } else 
	send_to_char("No-one around by that name.\r\n", ch);
    return;
}

ACMD(do_listen)
{
    struct char_data *fighting_vict = NULL;
    struct obj_data *noisy_obj = NULL;
    int i;

    if (ch->in_room->sounds) {
	send_to_char(ch->in_room->sounds, ch);
	return;
    }

    for (fighting_vict = ch->in_room->people; fighting_vict;
	 fighting_vict = fighting_vict->next_in_room)
	if (FIGHTING(fighting_vict))
	    break;
  
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
	    ch->in_room->zone->number == 262){
	    send_to_char("You hear nothing special.\r\n", ch);
	    break;
	}
	if (ch->in_room->zone->weather->sunlight == SUN_DARK) {
	    if (ch->in_room->zone->number >= 500&&ch->in_room->zone->number < 510)
		send_to_char("Dark, foreboding and silent, the city itself lies in ambush for you.\r\n", ch);
	    send_to_char("You hear the sounds of the city at night.\r\n", ch);
	} else {
	    if (ch->in_room->zone->weather->sky == SKY_RAINING) {
		if (ch->in_room->zone->number>=500&&ch->in_room->zone->number < 510)
		    send_to_char("Luckily the sound of falling acid rain will cover *your* footsteps too...\r\n", ch);
		else 
		    send_to_char("You hear the sounds of a city on a rainy day.\r\n", ch);
	    } else if (ch->in_room->zone->number >= 500 && 
		       ch->in_room->zone->number < 510)
		send_to_char("The quiet of a sleeping city seems almost peaceful until the shots\r\nring out, extolling another death.\r\n", ch);
	    else
		send_to_char("You hear the sounds of a bustling city.\r\n", ch);
	}
	break;
    case SECT_FOREST:
	if (ch->in_room->zone->weather->sky == SKY_RAINING)
	    send_to_char("You hear the rain on the leaves of the trees.\r\n", ch);
	else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
	    send_to_char("You hear peals of thunder in the distance.\r\n", ch);
	else
	    send_to_char("You hear the breeze rustling through the trees.\r\n",ch);
	break;
    default:
	if (OUTSIDE(ch)) {
	    if (ch->in_room->zone->weather->sky == SKY_RAINING)  
		send_to_char("You hear the falling rain all around you.\r\n", ch);
	    else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING) 
		send_to_char("You hear peals of thunder in the distance.\r\n", ch);
	    else if (fighting_vict)
		send_to_char("You hear the sounds of battle.\r\n", ch);
	    else if (noisy_obj)
		act("You hear a low humming coming from $p.",
		    FALSE, ch, noisy_obj, 0, TO_CHAR);
	    else
		send_to_char("You hear nothing special.\r\n", ch);
	}
	else if (fighting_vict)
	    send_to_char("You hear the sounds of battle.\r\n", ch);
	else if (noisy_obj)
	    act("You hear a low humming coming from $p.",
		FALSE, ch, noisy_obj, 0, TO_CHAR);
	else {
	    for (i = 0; i < NUM_DIRS; i++) {
		if (ch->in_room->dir_option[i] &&
		    ch->in_room->dir_option[i]->to_room &&
		    ch->in_room->dir_option[i]->to_room != ch->in_room &&
		    !IS_SET(ch->in_room->dir_option[i]->exit_info, EX_CLOSED)) {
		    for (fighting_vict = ch->in_room->dir_option[i]->to_room->people;
			 fighting_vict; fighting_vict = fighting_vict->next_in_room) {
			if (FIGHTING(fighting_vict))
			    break;
		    }
		    if (fighting_vict && !number(0, 1)) {
			sprintf(buf, "You hear sounds of battle from %s.\r\n",
				from_dirs[rev_dir[i]]);
			send_to_char(buf, ch);
			return;
		    }
		}
	    }
	    send_to_char("You hear nothing special.\r\n", ch);
	}
    } 
}


ACMD(do_look)
{
    static char arg2[MAX_INPUT_LENGTH];
    int look_type;

    if (!ch->desc)
	return;

    if (ch->getPosition() < POS_SLEEPING)
	send_to_char("You can't see anything but stars!\r\n", ch);
    else if (IS_AFFECTED(ch, AFF_BLIND) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
	send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
	send_to_char("It is pitch black...\r\n", ch);
	list_char_to_char(ch->in_room->people, ch);	/* glowing red eyes */
    } else {
	half_chop(argument, arg, arg2);

	if (subcmd == SCMD_READ) {
	    if (!*arg)
		send_to_char("Read what?\r\n", ch);
	    else
		look_at_target(ch, arg, cmd);
	    return;
	}
	if (!*arg)			/* "look" alone, without an argument at all */
	    look_at_room(ch, ch->in_room, 1);
	else if (is_abbrev(arg, "into"))
	    look_in_obj(ch, arg2);
	/* did the char type 'look <direction>?' */
	else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
	    look_in_direction(ch, look_type);
	else if (is_abbrev(arg, "at"))
	    look_at_target(ch, arg2, cmd);
	else
	    look_at_target(ch, arg, cmd);
    }
}

ACMD(do_glance)
{
    static char arg2[MAX_INPUT_LENGTH];

    if (!ch->desc)
	return;

    if (ch->getPosition() < POS_SLEEPING)
	send_to_char("You can't see anything but stars!\r\n", ch);
    else if (IS_AFFECTED(ch, AFF_BLIND) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
	send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
	send_to_char("It is pitch black...\r\n", ch);
	list_char_to_char(ch->in_room->people, ch);	/* glowing red eyes */
    } else {
	half_chop(argument, arg, arg2);

	if (!*arg)			/* "look" alone, without an argument at all */
	    send_to_char("Glance at who?\r\n", ch);
	else if (is_abbrev(arg, "at"))
	    glance_at_target(ch, arg2, cmd);
	else
	    glance_at_target(ch, arg, cmd);
    }
}
ACMD(do_examine)
{
    int bits;
    struct char_data *tmp_char;
    struct obj_data *tmp_object;

    one_argument(argument, arg);

    if (!*arg) {
	send_to_char("Examine what?\r\n", ch);
	return;
    }
    look_at_target(ch, arg, cmd);

    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
			FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

    if (tmp_object) {
	if (OBJ_REINFORCED(tmp_object))
	    act("$p appears to be structurally reinforced.",
		FALSE,ch,tmp_object,0,TO_CHAR);
	if (OBJ_ENHANCED(tmp_object))
	    act("$p looks like it has been enhanced.",
		FALSE,ch,tmp_object,0,TO_CHAR);
    
	obj_cond_color(tmp_object, ch);
	sprintf(buf, "$p seems to be in %s condition.", buf2);
	act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);

	if (IS_OBJ_TYPE(tmp_object, ITEM_CIGARETTE)) {
	    sprintf(buf, "It seems to have about %d drags left on it.\r\n",
		    GET_OBJ_VAL(tmp_object, 0));
	    send_to_char(buf, ch);
	}
	if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	    (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	    (GET_OBJ_TYPE(tmp_object) == ITEM_PIPE) ||
	    (GET_OBJ_TYPE(tmp_object) == ITEM_VEHICLE) ||
	    (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
	    look_in_obj(ch, arg);
	}
    }
}


ACMD(do_qpoints)
{
    sprintf(buf, "You have %d quest point%s.\r\n", GET_QUEST_POINTS(ch),
	    GET_QUEST_POINTS(ch) == 1 ? "" : "s");
    send_to_char(buf, ch);
}

ACMD(do_gold)
{
    if (GET_GOLD(ch) == 0)
	send_to_char("You're broke!\r\n", ch);
    else if (GET_GOLD(ch) == 1)
	send_to_char("You have one miserable little gold coin.\r\n", ch);
    else {
	sprintf(buf, "You have %d gold coins.\r\n", GET_GOLD(ch));
	send_to_char(buf, ch);
    }
}
ACMD(do_cash)
{
    if (GET_CASH(ch) == 0)
	send_to_char("You're broke!\r\n", ch);
    else if (GET_CASH(ch) == 1)
	send_to_char("You have one almighty credit.\r\n", ch);
    else {
	sprintf(buf, "You have %d credits.\r\n", GET_CASH(ch));
	send_to_char(buf, ch);
    }
}

ACMD(do_encumbrance)
{
    int encumbr;

    encumbr = ((IS_CARRYING_W(ch) + IS_WEARING_W(ch)) * 4) / CAN_CARRY_W(ch);

    if (IS_CARRYING_W(ch) + IS_WEARING_W(ch) <= 0) {
	send_to_char("You aren't carrying a damn thing.\r\n", ch);
	return;
    } else {
	if (IS_CARRYING_W(ch) + IS_WEARING_W(ch) == 1) {
	    send_to_char("Your stuff weighs a total of one pound.\r\n", ch);
	    return;
	} else {
	    sprintf(buf, "You are carrying a total %d pounds of stuff.\r\n",
		    IS_WEARING_W(ch) + IS_CARRYING_W(ch));
	    send_to_char(buf, ch);
	    sprintf(buf, "%d of which you are wearing or equipped with.\r\n", 
		    IS_WEARING_W(ch));
	    send_to_char(buf, ch);
	}
    }
    if (encumbr > 3)
	send_to_char("You are heavily encumbered.\r\n", ch);
    else if (encumbr == 3)
	send_to_char("You are moderately encumbered.\r\n", ch);
    else if (encumbr == 2)
	send_to_char("You are lightly encumbered.\r\n", ch);
		 
}


void 
print_affs_to_string(struct char_data *ch, char *str, byte mode)
{

    struct affected_type *af = NULL;
    struct char_data *mob = NULL;
    char *name = NULL;

    if (IS_AFFECTED(ch, AFF_BLIND))
	strcat(str, "You have been blinded!\r\n");
    if (IS_AFFECTED(ch, AFF_POISON))
	strcat(str, "You are poisoned!\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
	strcat(str, "You have been turned to stone.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_RADIOACTIVE))
	strcat(str, "You are radioactive.\r\n");
    if ( affected_by_spell( ch, SPELL_GAMMA_RAY ) )
	strcat( str, "You have been irradiated.\r\n" );
    if (IS_AFFECTED_2(ch, AFF2_ABLAZE)) {
	sprintf(buf2,"You are %s%sON FIRE!!%s\r\n", CCRED_BLD(ch, C_SPR), 
		CCBLK(ch, C_CMP), CCNRM(ch, C_SPR)); 
	strcat(str, buf2); 
    }
    if (affected_by_spell(ch, SPELL_QUAD_DAMAGE)) {
	sprintf(buf2, "You are dealing out %squad damage%s.\r\n",
		CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	strcat(buf, buf2);
    }
    if (affected_by_spell(ch, SPELL_BLACKMANTLE))
	strcat(str, "You are covered by the blackmantle.\r\n");
      
    if (affected_by_spell(ch, SPELL_ENTANGLE))
	strcat(str, "You are entangled in the undergrowth!\r\n");

    if ((af = affected_by_spell(ch, SKILL_DISGUISE))) {
	if ((mob = real_mobile_proto(af->modifier)))
	    sprintf(str, "%sYou are disguised as %s at a level of %d.\r\n",
		    str, GET_NAME(mob), af->duration);
    }

    // radiation sickness

    if ( ( af = affected_by_spell(ch, TYPE_RAD_SICKNESS)) ) {
	if (!number(0, 2))
	    strcat(str, "You feel nauseous.\r\n");
	else if (!number(0, 1))
	    strcat(str, "You feel sick and your skin is dry.\r\n");
	else
	    strcat(str, "You feel sick and your hair is falling out.\r\n");
    }
	if (IS_AFFECTED_3(ch,AFF3_TAINTED))
	strcat(str, "The very essence of your being has been tainted.\r\n");

    // vampiric regeneration

    if ( ( af = affected_by_spell(ch, SPELL_VAMPIRIC_REGENERATION) ) ) {
	if ( ( name = get_name_by_id(af->modifier) ) )
	    sprintf(str, "%sYou are under the effects of %s's vampiric regeneration.\r\n", str, name);
	else
	    sprintf(str, "%sYou are under the effects of vampiric regeneration from an unknown source.\r\n", str);
    }

    if (mode)    /* Only asked for bad affs? */
	return;
	if (IS_SOULLESS(ch))
	strcat(str, "A deep despair clouds your soulless mind.\r\n");
    if (IS_AFFECTED(ch, AFF_SNEAK))
	strcat(str, "You are sneaking.\r\n");
    if (IS_AFFECTED(ch, AFF_INVISIBLE))
	strcat(str, "You are invisible.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_TRANSPARENT))
	strcat(str, "You are transparent.\r\n");
    if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
	strcat(str, "You are sensitive to the presence of invisible things.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)) 
	strcat(str, "You are seeing truly.\r\n");
    if (AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
	strcat(str, "Your sonic imagers are active.\r\n");
    if (IS_AFFECTED(ch, AFF_SANCTUARY))
	strcat(str, "You are protected by Sanctuary.\r\n");
    if (IS_AFFECTED(ch, AFF_CHARM))
	strcat(str, "You have been charmed!\r\n");
    if (affected_by_spell(ch, SPELL_ARMOR))
	strcat(str, "You feel protected.\r\n");
    if (affected_by_spell(ch, SPELL_BARKSKIN))
	strcat(str, "Your skin is thick and tough like tree bark.\r\n");
    if (affected_by_spell(ch, SPELL_STONESKIN))
	strcat(str, "Your skin is as hard as granite.\r\n");
    if (IS_AFFECTED(ch, AFF_INFRAVISION))
	strcat(str, "Your eyes are glowing red.\r\n");
    if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
	strcat(str, "You are summonable by other players.\r\n");
    if (IS_AFFECTED(ch, AFF_REJUV))
	strcat(str, "You feel like your body will heal with a good rest.\r\n");
    if (IS_AFFECTED(ch, AFF_REGEN))
	strcat(str, "Your body is regenerating itself rapidly.\r\n");
    if (IS_AFFECTED(ch, AFF_GLOWLIGHT))
	strcat(str, "You are followed by a ghostly illumination.\r\n");
    if (IS_AFFECTED(ch, AFF_BLUR))
	strcat(str, "Your image is blurred and shifting.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_DISPLACEMENT)) {
	if ( affected_by_spell( ch, SPELL_REFRACTION ) )
	    strcat( str, "Your body is irregularly refractive.\r\n" );
	else
	    strcat(str, "Your image is displaced.\r\n");
    }
    if ( affected_by_spell( ch, SPELL_ELECTROSTATIC_FIELD ) )
	strcat( str, "You are surrounded by an electrostatic field.\r\n" );
    if (IS_AFFECTED_2(ch, AFF2_FIRE_SHIELD))
	strcat(str, "You are protected by a shield of fire.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_BLADE_BARRIER))
	strcat(str, "You are protected by whirling blades.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_ENERGY_FIELD))
	strcat(str, "You are surrounded by a field of energy.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_PRISMATIC_SPHERE))
	strcat(str, "You are surrounded by a prismatic sphere of light.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_FLUORESCENT))
	strcat(str, "The atoms in your vicinity are fluorescing.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_DIVINE_ILLUMINATION)) {
	if (IS_EVIL(ch))
	    strcat(str, "An unholy light is following you.\r\n");
	else if (IS_GOOD(ch))
	    strcat(str, "A holy light is following you.\r\n");
	else
	    strcat(str, "A sickly light is following you.\r\n");
    }
    if (IS_AFFECTED_2(ch, AFF2_BESERK))
	strcat(str, "You are BESERK!\r\n");
    if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
	strcat(str, "You are protected from good.\r\n");
    if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
	strcat(str, "You are protected from evil.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PROT_DEVILS))
	strcat(str, "You are protected from devils.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PROT_DEMONS))
	strcat(str, "You are protected from demons.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PROTECT_UNDEAD))
	strcat(str, "You are protected from the undead.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PROT_LIGHTNING))
	strcat(str, "You are protected from lightning.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_PROT_FIRE))
	strcat(str, "You are protected from fire.\r\n");
    if (affected_by_spell(ch, SPELL_MAGICAL_PROT))
	strcat(str, "You are protected against magic.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_ENDURE_COLD))
	strcat(str,"You can endure extreme cold.\r\n");
    if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
	strcat(str,"You are sensitive to the presence of living creatures\r\n");
    if (affected_by_spell(ch, SKILL_EMPOWER))
	strcat(str, "You are empowered.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_TELEKINESIS))
	strcat(str,"You are feeling telekinetic.\r\n");
	if ( affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC) )
	strcat(buf, "Melee Combat Tactics are in effect.\r\n");                 
    if (affected_by_spell(ch, SKILL_REFLEX_BOOST))
	strcat(str, "Your Reflex Boosters are active.\r\n");
    else if (IS_AFFECTED_2(ch, AFF2_HASTE))
	strcat(str, "You are moving very fast.\r\n");
    if (AFF2_FLAGGED(ch, AFF2_SLOW))
	strcat(str, "You feel unnaturally slowed.\r\n");
    if (affected_by_spell(ch, SKILL_KATA))
	strcat(str, "You feel focused from your kata.\r\n");
    if (IS_AFFECTED_2(ch, AFF2_OBLIVITY))
	strcat(str, "You are oblivious to pain.\r\n");
    if (affected_by_spell(ch, ZEN_MOTION))
	strcat(str, "The zen of motion is one with your body.\r\n");
    if (affected_by_spell(ch, ZEN_TRANSLOCATION))
	strcat(str, "You are as one with the zen of translocation.\r\n");
    if ( affected_by_spell( ch, ZEN_CELERITY ) )
	strcat( str, "You are under the effects of the zen of celerity.\r\n" );
    if (IS_AFFECTED_3(ch, AFF3_MANA_TAP))
	strcat(str, "You have a direct tap to the spiritual energies of the universe.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_ENERGY_TAP))
	strcat(str, "Your body is absorbing physical energy from the universe.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_SONIC_IMAGERY))
	strcat(str, "You are perceiving sonic images.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_PROT_HEAT))
	strcat(str, "You are protected from heat.\r\n");
    if (affected_by_spell(ch, SPELL_PRAY))
	strcat(str, "You feel extremely righteous.\r\n");
    else if (affected_by_spell(ch, SPELL_BLESS))
	strcat(str, "You feel righteous.\r\n");
    if (affected_by_spell(ch, SPELL_MANA_SHIELD))
	strcat(str, "You are protected by a mana shield.\r\n");
    if (affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS))
	strcat(str, "You are surrounded by a shield of righteousness.\r\n");
    if (affected_by_spell(ch, SPELL_ANTI_MAGIC_SHELL))
	strcat(str, "You are enveloped in an anti-magic shell.\r\n");
    if (affected_by_spell(ch, SPELL_SANCTIFICATION))
	strcat(str, "You have been sanctified!\r\n");
    if (affected_by_spell(ch, SPELL_DIVINE_INTERVENTION))
	strcat(str, "You are shielded by divine intervention.\r\n");
    if (affected_by_spell(ch, SPELL_SPHERE_OF_DESECRATION))
	strcat(str, "You are surrounded by a black sphere of desecration.\r\n");

    /* psionic affections */
    if (affected_by_spell(ch, SPELL_POWER))
	strcat(str, "Your physical strength is augmented.\r\n");
    if (affected_by_spell(ch, SPELL_INTELLECT))
	strcat(str, "Your mental faculties are augmented.\r\n");
    if (IS_AFFECTED(ch, AFF_NOPAIN))
	strcat(str, "Your mind is ignoring pain.\r\n");
    if (IS_AFFECTED(ch, AFF_RETINA))
	strcat(str, "Your retina is especially sensitive.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_SYMBOL_OF_PAIN))
    strcat(str, "Your mind burns with the symbol of pain!\r\n");
    if (IS_AFFECTED(ch, AFF_CONFUSION))
	strcat(str, "You are very confused.\r\n");
	if ( affected_by_spell(ch, SKILL_ADRENAL_MAXIMIZER) )
	strcat(buf, "Shukutei Adrenal Maximizations are active.\r\n");
    else if (IS_AFFECTED(ch, AFF_ADRENALINE))
	strcat(str, "Your adrenaline is pumping.\r\n");
    if (IS_AFFECTED(ch, AFF_CONFIDENCE))
	strcat(str, "You feel very confident.\r\n");
    if (affected_by_spell(ch, SPELL_DERMAL_HARDENING))
	strcat(str, "Your dermal surfaces are hardened.\r\n");
    if (affected_by_spell(ch, SPELL_LATTICE_HARDENING))
	strcat(str, "Your molecular lattice has been strengthened.\r\n");
    if (AFF3_FLAGGED(ch, AFF3_NOBREATHE))
	strcat(str, "You are not breathing.\r\n");
    if (AFF3_FLAGGED(ch, AFF3_PSISHIELD))
	strcat(str, "You are protected by a psionic shield.\r\n");
    if (affected_by_spell(ch, SPELL_METABOLISM))
	strcat(str, "Your metabolism is racing.\r\n");
    else if (affected_by_spell(ch, SPELL_RELAXATION))
	strcat(str, "You feel very relaxed.\r\n");
    if (affected_by_spell(ch, SPELL_WEAKNESS))
	strcat(str, "You feel unusually weakened.\r\n");
    if (affected_by_spell(ch, SPELL_MOTOR_SPASM))
	strcat(str, "Your muscles are spasming uncontrollably!\r\n");
    if (affected_by_spell(ch, SPELL_ENDURANCE))
	strcat(str, "Your endurance is increased.\r\n");
    if (AFF2_FLAGGED(ch, AFF2_VERTIGO))
	strcat(str, "You are lost in a sea of vertigo.\r\n");
    if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH))
	strcat(str, "You feel a psychic force crushing your mind!\r\n");

    /* physic affects */ 
    if (AFF3_FLAGGED(ch, AFF3_ACIDITY))
	strcat(str, "Your body is producing self-corroding acids!\r\n");
    if (AFF3_FLAGGED(ch, AFF3_ATTRACTION_FIELD))
	strcat(str, "You are emitting an an attraction field.\r\n");
    if ( affected_by_spell( ch, SPELL_CHEMICAL_STABILITY ) )
	strcat( str, "You feel chemically inert.\r\n" );


    if (IS_AFFECTED_3(ch, AFF3_DAMAGE_CONTROL))
	strcat(str, "Your Damage Control process is running.\r\n");
    if (IS_AFFECTED_3(ch, AFF3_SHROUD_OBSCUREMENT))
	strcat(str, 
	       "You are surrounded by an magical obscurement shroud.\r\n");
    if (IS_SICK(ch))
	strcat(str, "You are afflicted with a terrible sickness!\r\n");
    if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL))
    strcat (str, "Spacetime is bent around you in a powerful gravity well!\r\n");
	if (IS_AFFECTED_3(ch, AFF3_HAMSTRUNG)){
		char tempstr[128];
		tempstr[0] = '\0';
        sprintf( tempstr, "%sThe gash on your leg is %sBLEEDING%s%s all over!!%s\r\n",
             CCRED( ch, C_SPR ), CCBLD( ch, C_SPR ), CCNRM( ch, C_SPR ),
             CCRED( ch, C_SPR ), CCNRM( ch, C_SPR ) );

		strcat(str,tempstr);
	}
}

ACMD(do_affects)
{
    strcpy(buf, "Current affects:\r\n");
    print_affs_to_string(ch, buf, 0);
    page_string(ch->desc, buf, 1);
}

ACMD(do_experience)
{
  
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
	send_to_char("You are very experienced.\r\n", ch);
	return;
    }
    sprintf(buf, "You need %s%d%s exp to level.\r\n",
	    CCCYN(ch, C_NRM), ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)),
	    CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
}
ACMD(do_score)
{

    struct time_info_data playing_time;
    struct time_info_data real_time_passed(time_t t2, time_t t1);

    sprintf(buf, "%s*****************************************************************\r\n",
	    CCRED(ch, C_NRM));
    sprintf(buf, "%s%s***************************%sS C O R E%s%s*****************************\r\n",
	    buf, CCYEL(ch, C_NRM), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCYEL(ch, C_NRM));
    sprintf(buf, "%s%s*****************************************************************%s\r\n",
	    buf, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  
    sprintf(buf, "%s%s, %d year old %s %s %s.  Your level is %d.\r\n", buf, 
	    GET_NAME(ch),
	    GET_AGE(ch), genders[(int)GET_SEX(ch)], 
	    (GET_RACE(ch) >= 0 && GET_RACE(ch) < NUM_RACES) ? 
	    player_race[(int)GET_RACE(ch)] : "BAD RACE",
	    (GET_CLASS(ch) >= 0 && GET_CLASS(ch) < TOP_CLASS) ?
	    pc_char_class_types[(int)GET_CLASS(ch)] : "BAD CLASS", 
	    GET_LEVEL(ch));
    if (!IS_NPC(ch) && IS_REMORT(ch))
	sprintf(buf, "%sYou have remortalized as a %s (generation %d).\r\n", 
		buf, pc_char_class_types[(int)GET_REMORT_CLASS(ch)], 
		GET_REMORT_GEN(ch));
    if ((age(ch).month == 0) && (age(ch).day == 0))
	strcat(buf, "  It's your birthday today!\r\n\r\n");
    else
	strcat(buf, "\r\n");

    sprintf(buf2, "(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_HIT(ch), 
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_HIT(ch), CCNRM(ch, C_NRM));
    sprintf(buf, "%sHit Points:  %11s           Armor Class: %s%d/10%s\r\n", buf, buf2, 
	    CCGRN(ch, C_NRM), GET_AC(ch), CCNRM(ch, C_NRM));
    sprintf(buf2, "(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MANA(ch), 
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(ch), CCNRM(ch, C_NRM));
    sprintf(buf, "%sMana Points: %11s           Alignment: %s%d%s\r\n", buf, buf2, 
	    CCGRN(ch, C_NRM), GET_ALIGNMENT(ch), CCNRM(ch, C_NRM));
    sprintf(buf2, "(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MOVE(ch), 
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MOVE(ch), CCNRM(ch, C_NRM));
    sprintf(buf, "%sMove Points: %11s           Experience: %s%d%s\r\n", buf, buf2, 
	    CCGRN(ch, C_NRM), GET_EXP(ch), CCNRM(ch, C_NRM));
    sprintf(buf, "%s                                   %sKills%s: %d, %sPKills%s: %d\r\n", buf,
	    CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), (GET_MOBKILLS(ch) + GET_PKILLS(ch)),
	    CCRED(ch, C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(ch));

    sprintf(buf, "%s%s*****************************************************************%s\r\n",
	    buf, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
    sprintf(buf, "%sYou have %s%d%s %s and %s%d%s life points.\r\n", buf,
	    CCCYN(ch, C_NRM), GET_PRACTICES(ch), CCNRM(ch, C_NRM), 
	    GET_CLASS(ch) == CLASS_CYBORG ? "dsu free" : "practice points", 
	    CCCYN(ch, C_NRM), GET_LIFE_POINTS(ch), CCNRM(ch, C_NRM) );
    sprintf(buf, "%sYou are %s%d%s cm tall, and weigh %s%d%s pounds.\r\n", buf,
	    CCCYN(ch, C_NRM), GET_HEIGHT(ch), CCNRM(ch, C_NRM), 
	    CCCYN(ch, C_NRM), GET_WEIGHT(ch), CCNRM(ch, C_NRM));
    if (!IS_NPC(ch)) {
	if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
	    sprintf(buf, "%sYou need %s%d%s exp to reach your next level.\r\n", buf,
		    CCCYN(ch, C_NRM), ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)),
		    CCNRM(ch, C_NRM));
	} else {
	    if (ch->player_specials->poofout) {
		sprintf(buf, "%s%sPoofout:%s  %s\r\n",
			buf,CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), ch->player_specials->poofout);
	    }
	    if (ch->player_specials->poofin) {
		sprintf(buf, "%s%sPoofin :%s  %s\r\n",
			buf, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), ch->player_specials->poofin);
	    }
	}
	playing_time = real_time_passed((time(0) - ch->player.time.logon) +
					ch->player.time.played, 0);
	sprintf(buf, "%sYou have existed here for %d days and %d hours.\r\n",
		buf, playing_time.day, playing_time.hours);
    
	sprintf(buf, "%sYou are known as %s%s %s.%s\r\n", buf,
		CCYEL(ch, C_NRM), GET_NAME(ch), GET_TITLE(ch), CCNRM(ch, C_NRM));
    }
    sprintf(buf, "%sYou carry %s%d%s gold coins.  You have %s%d%s cash credits.\r\n",
	    buf, CCCYN(ch, C_NRM), GET_GOLD(ch), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_CASH(ch), CCNRM(ch, C_NRM));
  
    switch (ch->getPosition()) {
    case POS_DEAD:
	strcat(buf, CCRED(ch, C_NRM));
	strcat(buf, "You are DEAD!\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_MORTALLYW:
	strcat(buf, CCRED(ch, C_NRM));
	strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_INCAP:
	strcat(buf, CCRED(ch, C_NRM));
	strcat(buf, "You are incapacitated, slowly fading away...\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_STUNNED:
	strcat(buf, CCRED(ch, C_NRM));
	strcat(buf, "You are stunned!  You can't move!\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_SLEEPING:
	strcat(buf, CCGRN(ch, C_NRM));
	if (AFF3_FLAGGED(ch, AFF3_STASIS))
	    strcat(buf, "You are inactive in a static state.\r\n");
	else
	    strcat(buf, "You are sleeping.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_RESTING:
	strcat(buf, CCGRN(ch, C_NRM));
	strcat(buf, "You are resting.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_SITTING:
	strcat(buf, CCGRN(ch, C_NRM));
	if (IS_AFFECTED_2(ch, AFF2_MEDITATE))
	    strcat(buf, "You are meditating.\r\n");
	else
	    strcat(buf, "You are sitting.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_FIGHTING:
	strcat(buf, CCYEL(ch, C_NRM));
	if (FIGHTING(ch))
	    sprintf(buf, "%sYou are fighting %s.\r\n", buf, PERS(FIGHTING(ch), ch));
	else
	    strcat(buf, "You are fighting thin air.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_MOUNTED:
	strcat(buf, CCGRN(ch, C_NRM));
	if (MOUNTED(ch))
	    sprintf(buf, "%sYou are mounted on %s.\r\n", buf, PERS(MOUNTED(ch), ch));
	else
	    strcat(buf, "You are mounted on the thin air!?\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_STANDING:
	strcat(buf, CCGRN(ch, C_NRM));
	strcat(buf, "You are standing.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    case POS_FLYING:
	strcat(buf, CCGRN(ch, C_NRM));
	strcat(buf, "You are hovering in midair.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    default:
	strcat(buf, CCCYN(ch, C_NRM));
	strcat(buf, "You are floating.\r\n");
	strcat(buf, CCNRM(ch, C_NRM));
	break;
    }

    if (GET_COND(ch, DRUNK) > 10)
	strcat(buf, "You are intoxicated.\r\n");
    if (GET_COND(ch, FULL) == 0)
	strcat(buf, "You are hungry.\r\n");
    if (GET_COND(ch, THIRST) == 0) {
	if (IS_VAMPIRE(ch))
	    strcat(buf, "You have an intense thirst for blood.\r\n");
	else
	    strcat(buf, "You are thirsty.\r\n");
    } else if (IS_VAMPIRE(ch) && GET_COND(ch, THIRST) < 4)
	strcat(buf, "You feel the onset of your bloodlust.\r\n");

    if (GET_LEVEL(ch) >= LVL_AMBASSADOR && PLR_FLAGGED(ch, PLR_MORTALIZED))
	strcat(buf, "You are mortalized.\r\n");

    print_affs_to_string(ch, buf, PRF2_FLAGGED(ch, PRF2_NOAFFECTS));

    page_string(ch->desc, buf, 1);
}


ACMD(do_inventory)
{
    send_to_char("You are carrying:\r\n", ch);
    list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
    int i, found = 0, pos = -1;
    struct obj_data *obj = NULL;
    char outbuf[MAX_STRING_LENGTH];
    char active_buf[2][28];

    strcpy(active_buf[0], "(inactive)");
    strcpy(active_buf[1], "(active)");

    skip_spaces(&argument);

    if (subcmd == SCMD_EQ) {
	if (*argument && is_abbrev(argument, "status")) {
	    strcpy(outbuf, "Equipment status:\r\n");
	    for (i = 0; i < NUM_WEARS; i++) {
		if (!(obj = GET_EQ(ch, i)))
		    continue;
		sprintf(outbuf, "%s-%s- is in %s condition.\r\n", outbuf,
			obj->short_description, obj_cond_color(obj, ch));
	    }
	    page_string(ch->desc, outbuf, 1);
	    return;
	}

	send_to_char("You are using:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++) {
	    if (GET_EQ(ch, (int)eq_pos_order[i])) {
		if (INVIS_OK_OBJ(ch, GET_EQ(ch, (int)eq_pos_order[i]))) {
		    send_to_char(strcat(strcat(strcpy(buf, 
						      CCGRN(ch, C_NRM)), 
					       where[(int)eq_pos_order[i]]),
					CCNRM(ch, C_NRM)), 
				 ch);
		    /*
		      strcat(buf, where[(int)eq_pos_order[i]]);
		      send_to_char(CCGRN(ch, C_NRM), ch);
		      send_to_char(where[(int)eq_pos_order[i]], ch);
		      send_to_char(CCNRM(ch, C_NRM), ch);
		    */

		    show_obj_to_char(GET_EQ(ch, (int)eq_pos_order[i]), ch, 1, 0);
		    found = TRUE;
		} else {
		    send_to_char(where[(int)eq_pos_order[i]], ch);
		    send_to_char("Something.\r\n", ch);
		    found = TRUE;
		}
	    }
	}
	return;
    } 

    if (subcmd == SCMD_IMPLANTS) {
	if (*argument) {
	    argument = one_argument(argument, buf);
	    if (*buf && is_abbrev(buf, "status")) {
		two_arguments(argument, buf, buf2);
		if (!*buf) {
		    strcpy(outbuf, "Implant status:\r\n");
		    for (i = 0; i < NUM_WEARS; i++) {
			if (!(obj = GET_IMPLANT(ch, i)))
			    continue;
			sprintf(outbuf, "%s-%s- is in %s condition.\r\n", outbuf,
				obj->short_description, obj_cond_color(obj, ch));
		    }
		    page_string(ch->desc, outbuf, 1);
		    return;
		}
		if (*buf2) {
		    if ((pos = search_block(buf, wear_implantpos, 0)) < 0) {
			sprintf(buf, "'%s' is an invalid implant position.\r\n", buf2);
			send_to_char(buf, ch);
			return;
		    }
		    if (!(obj = GET_IMPLANT(ch, pos))) {
			sprintf(buf, "You are not implanted at position '%s'.\r\n",
				wear_implantpos[pos]);
			send_to_char(buf, ch);
			return;
		    }
		    if (!isname(buf, obj->name)) {
			sprintf(buf2, 
				"You are implanted with %s at %s.\r\n"
				"Not '%s'.\r\n", obj->short_description,
				wear_implantpos[pos], buf);
			send_to_char(buf, ch);
			return;
		    }
		} else if (!(obj = 
			     get_object_in_equip_vis(ch, buf, ch->implants, &i))) {
		    send_to_char("You are equipped with no such implant.\r\n", ch);
		} else {
		    if (IS_DEVICE(obj)) {
			sprintf(buf, "-%s- is %sactive with energy (%d/%d).\r\n",
				obj->short_description, ENGINE_STATE(obj) ? "" : "in",
				CUR_ENERGY(obj), MAX_ENERGY(obj));
			send_to_char(buf, ch);
		    }
		    sprintf(buf, "%s is in %s condition.\r\n", obj->short_description,
			    obj_cond_color(obj, ch));
		    send_to_char(buf, ch);
		}
		return;
	    }
	}

	send_to_char("You are implanted with:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++) {
	    if (GET_IMPLANT(ch, (int)eq_pos_order[i])) {

		if (INVIS_OK_OBJ(ch, GET_IMPLANT(ch, (int)eq_pos_order[i]))) {

		    if (IS_DEVICE(GET_IMPLANT(ch, (int)eq_pos_order[i])))
			sprintf(buf2, " %10s %s(%s%d%s/%s%d%s)%s",
				active_buf[ENGINE_STATE(GET_IMPLANT(ch, 
								    (int)
								    eq_pos_order[i]))],
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				CUR_ENERGY(GET_IMPLANT(ch, (int)eq_pos_order[i])),
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				MAX_ENERGY(GET_IMPLANT(ch, (int)eq_pos_order[i])),
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
		    else
			strcpy(buf2, "");

		    sprintf(buf, "%s[%12s]%s - %25s%s\r\n", CCCYN(ch, C_NRM),
			    wear_implantpos[(int)eq_pos_order[i]], CCNRM(ch, C_NRM),
			    GET_IMPLANT(ch, (int)eq_pos_order[i])->short_description,
			    buf2);
		    send_to_char(buf, ch);
		    found = TRUE;
		} else {
		    sprintf(buf, "%s[%12s]%s - (UNKNOWN)\r\n", CCCYN(ch, C_NRM),
			    wear_implantpos[(int)eq_pos_order[i]], CCNRM(ch, C_NRM));
		    found = TRUE;
		}
	    }
	}
    }
    if (!found) {
	if (subcmd == SCMD_EQ)
	    send_to_char(" Nothing.\r\n", ch);
	else
	    send_to_char("No implants detected.\r\n", ch);
    }
}

void set_local_time(struct zone_data *zone,struct time_info_data *local_time);

ACMD(do_time)
{
    char *suf;
    int weekday, day;
    struct time_info_data local_time;
    extern const char *weekdays[7];
    extern const char *month_name[16];

    if (ch->in_room->zone->time_frame == TIME_TIMELESS) {
	send_to_char("Time has no meaning here.\r\n", ch);
	return;
    }

    set_local_time(ch->in_room->zone, &local_time);
  
    sprintf(buf, "It is %d o'clock %s, on ",
	    ((local_time.hours % 12 == 0) ? 12 : ((local_time.hours) % 12)),
	    ((local_time.hours >= 12) ? "pm" : "am"));

    /* 35 days in a month */
    weekday = ((35 * local_time.month) +local_time.day + 1) % 7;

    strcat(buf, weekdays[weekday]);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    day = local_time.day + 1;	/* day in [1..35] */

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

    sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	    day, suf, month_name[(int) local_time.month], 
	    ch->in_room->zone->time_frame == TIME_MODRIAN ? local_time.year :
	    ch->in_room->zone->time_frame == TIME_ELECTRO ? local_time.year +
	    3567 : local_time.year);

    send_to_char(buf, ch);
    sprintf(buf, "The moon is currently %s.\r\n", lunar_phases[lunar_phase]);
    send_to_char(buf, ch);
}

void 
show_mud_date_to_char(struct char_data *ch)
{
    char *suf;
    struct time_info_data local_time;
    extern const char *month_name[16];
    int day;

    if (!ch->in_room) {
	slog("SYSERR: !ch->in_room in show_mud_date_to_char");
	return;
    }

    set_local_time(ch->in_room->zone, &local_time);

    day = local_time.day + 1;	/* day in [1..35] */

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

    sprintf(buf, "It is the %d%s Day of the %s, Year %d.\r\n",
	    day, suf, month_name[(int) local_time.month], local_time.year);

    send_to_char(buf, ch);
}

ACMD(do_weather)
{
    static char *sky_look[] = {
	"cloudless",
	"cloudy",
	"rainy",
	"lit by flashes of lightning"};

    if (OUTSIDE(ch) && !ZONE_FLAGGED(ch->in_room->zone,ZONE_NOWEATHER)) {
        sprintf(buf, "The sky is %s and %s.\r\n", 
            sky_look[(int)ch->in_room->zone->weather->sky],
            (ch->in_room->zone->weather->change >= 0 ? 
             "you feel a warm wind from the south" :
             "your foot tells you bad weather is due"));
        send_to_char(buf, ch);
        if (ch->in_room->zone->weather->sky < SKY_RAINING) {
            sprintf(buf, "The %s moon is %s%s.\r\n",
                lunar_phases[lunar_phase],
                (ch->in_room->zone->weather->moonlight && 
                 ch->in_room->zone->weather->moonlight < MOON_SKY_SET) ? 
                "visible " : "",
                moon_sky_types[(int)ch->in_room->zone->weather->moonlight]);
            send_to_char(buf, ch);
        }
    } else {
        send_to_char("You have no feeling about the weather at all.\r\n", ch);
    }
}

void 
perform_net_who(struct char_data *ch, char *arg)
{
    struct descriptor_data *d = NULL;
    int count = 0;

    strcpy(buf, "Visible users of the global net:\r\n");
    for (d = descriptor_list; d; d = d->next) {
	if (STATE(d) < CON_NET_MENU1)
	    continue;
	if (!CAN_SEE(ch, d->character))
	    continue;
    
	count++;
	sprintf(buf, "%s   (%3d)     %s\r\n", buf, count, GET_NAME(d->character));
	continue;
    }
    sprintf(buf, "%s\r\n%d users detected.\r\n\r\n", buf, count);
    page_string(ch->desc, buf, 1);
}

void perform_net_finger(struct char_data *ch, char *arg)
{
    struct char_data *vict = NULL;

    if (!*arg) {
	send_to_char("ERROR.  NULL arg passed to perform_net_finger.\r\n", ch);
	return;
    }
    if (!(vict = get_char_vis(ch, arg)) || !vict->desc ||
	STATE(vict->desc) < CON_NET_MENU1) {
	send_to_char("No such user detected.\r\n", ch);
	return;
    }
    sprintf(buf, "Finger results:\r\n"
	    "Name:  %s, Level %d %s %s.\r\n"
	    "Logged in at: %s.\r\n"
	    "State:  %s.\r\n\r\n",
	    GET_NAME(vict), GET_LEVEL(vict), 
	    player_race[(int)GET_RACE(vict)], 
	    pc_char_class_types[(int)GET_CLASS(vict)], 
	    vict->in_room != NULL ?ch->in_room->name: "NOWHERE", 
	    connected_types[STATE(vict->desc)]);
    send_to_char(buf, ch);

}

#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c char_classlist] [-a clan] [-<soqrzmftpx>]\r\n"

ACMD(do_who)
{
    struct descriptor_data *d;
    struct char_data *tch;
    struct clan_data *clan = NULL;
    char name_search[MAX_INPUT_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char c_buf[MAX_STRING_LENGTH];
    char tester_buf[64], nowho_buf[64];
    char mode;
    int i, low = 0, high = LVL_GRIMP;
    int showchar_class = 0, short_list = 0, num_can_see = 0, tot_num = 0;
    bool
	localwho    = false,
	questwho    = false,
	outlaws     = false,
	who_room    = false,
	who_plane   = false,
	who_time    = false,
	who_female  = false,
	who_male    = false,
	who_remort  = false,
	who_tester  = false,
	who_pkills  = false,
	who_i       = false;
    int effective_char_class;

    skip_spaces(&argument);
    strcpy(buf, argument);
    name_search[0] = '\0';

    sprintf(tester_buf, "%s#TESTER#%s ", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    sprintf(nowho_buf, "%s(nowho)%s ", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));

    while (*buf) {
	half_chop(buf, arg, buf1);
	if (isdigit(*arg)) {
	    sscanf(arg, "%d-%d", &low, &high);
	    strcpy(buf, buf1);
	} else if (*arg == '-') {
	    mode = *(arg + 1);      /* just in case;we destroy arg in the switch */
	    switch (mode) {
	    case 'o':
		outlaws = true;
		strcpy(buf, buf1);
		break;
	    case 'i':
		who_i = true;
		strcpy(buf, buf1);
		break;
	    case 'k':
		who_pkills = true;
		strcpy(buf, buf1);
		break;
	    case 'z':
		localwho = true;
		strcpy(buf, buf1);
		break;
	    case 's':
		short_list = true;
		strcpy(buf, buf1);
		break;
	    case 'q':
		questwho = true;
		strcpy(buf, buf1);
		break;
	    case 'l':
		half_chop(buf1, arg, buf);
		sscanf(arg, "%d-%d", &low, &high);
		break;
	    case 'n':
		half_chop(buf1, name_search, buf);
		break;
	    case 'a':
		half_chop(buf1, name_search, buf);
		if (!(clan = clan_by_name(name_search)))
		    send_to_char("No such clan!\r\n", ch);
		*name_search = '\0';
		break;
	    case 'x':
		who_remort = true;
		strcpy(buf, buf1);
		break;
	    case 'r':
		who_room = true;
		strcpy(buf, buf1);
		break;
	    case 'c':
		half_chop(buf1, arg, buf);
		for (i = 0; i < (int)strlen(arg); i++)
		    showchar_class |= find_char_class_bitvector(arg[i]);
		break;
	    case 'p':
		who_plane = true;
		strcpy(buf, buf1);
		break;
	    case 't':
		who_time = true;
		strcpy(buf, buf1);
		break;
	    case 'f':
		who_female = true;
		strcpy(buf, buf1);
		break;
	    case 'm':
		who_male = true;
		strcpy(buf, buf1);
		break;
	    case 'e':
		who_tester = true;
		strcpy(buf, buf1);
		break;
	    default:
		send_to_char(WHO_FORMAT, ch);
		return;
		break;
	    }				/* end of switch */

	} else {			/* endif */
	    send_to_char(WHO_FORMAT, ch);
	    return;
	}
    }				/* end while (parser) */

    if (ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED | ZONE_SOUNDPROOF) &&
	GET_LEVEL(ch) < LVL_AMBASSADOR)
	localwho = 1;


    sprintf(buf2, "%s**************       %sVisible Players of TEMPUS%s%s       **************%s\r\n%s",
	    CCBLD(ch, C_CMP), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), 
	    CCBLD(ch, C_CMP), CCNRM(ch, C_SPR), PRF_FLAGGED(ch, PRF_COMPACT) ?
	    "" : "\r\n");

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected && STATE(d) < CON_NET_MENU1)
	    continue;

	if (d->original)
	    tch = d->original;
	else if (!(tch = d->character))
	    continue;

	// skip imms now, before the tot_num gets incremented
	if ( !CAN_SEE( ch, tch ) && GET_LEVEL( tch ) >= LVL_AMBASSADOR )
	    continue;

	tot_num++;

	if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	    !strstr(GET_TITLE(tch), name_search))
	    continue;
	if (clan && (clan->number != GET_CLAN(tch) ||
		     (PRF2_FLAGGED(tch, PRF2_CLAN_HIDE) &&
		      !PRF_FLAGGED(ch, PRF_HOLYLIGHT))))
	    continue;

	if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	    continue;
	if ((low > 0 || high < LVL_GRIMP) && GET_LEVEL(ch) < LVL_AMBASSADOR &&
	    PRF2_FLAGGED(tch, PRF2_ANONYMOUS))
	    continue;

	if (COMM_NOTOK_ZONES(ch, tch))
	    continue;
	if (ch != tch && !PRF_FLAGGED(ch, PRF_HOLYLIGHT) && 
	    PRF2_FLAGGED(tch, PRF2_NOWHO) && 
	    !PLR_FLAGGED(tch, PLR_KILLER | PLR_THIEF))
	    continue;
	if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	    !PLR_FLAGGED(tch, PLR_THIEF))
	    continue;
	if (who_pkills && !GET_PKILLS(tch))
	    continue;
	if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
	    continue;
	if (localwho && ch->in_room->zone != tch->in_room->zone)
	    continue;
	if (who_room && (tch->in_room != ch->in_room))
	    continue;
	if (showchar_class && 
	    ((!(showchar_class & (1 << GET_CLASS(tch))) &&
	      (GET_LEVEL(ch) < LVL_AMBASSADOR ||
	       !(showchar_class & (1 << GET_REMORT_CLASS(tch))))) ||
	     (!PRF_FLAGGED(ch, PRF_HOLYLIGHT) && ch != tch &&
	      PRF2_FLAGGED(tch, PRF2_ANONYMOUS))))
	    continue;
	if (who_female && !IS_FEMALE(tch))
	    continue;
	if (who_male && !IS_MALE(tch))
	    continue;
	if (who_time && 
	    (GET_TIME_FRAME(tch->in_room) != GET_TIME_FRAME(ch->in_room)))
	    continue;
	if (who_plane && 
	    (GET_PLANE(tch->in_room) != GET_PLANE(ch->in_room)))
	    continue;
	if (who_remort && (!IS_REMORT(tch) || 
			   (!IS_REMORT(ch) && GET_LEVEL(ch) < LVL_AMBASSADOR)))
	    continue;
	if (who_tester && !PLR_FLAGGED(tch, PLR_TESTER))
	    continue;
	if (short_list) {
	    if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && 
		!PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		sprintf(buf2, "%s%s[%s-- ----%s]%s %-12.12s%s%s", buf2,
			CCGRN(ch, C_NRM), CCCYN(ch, C_NRM), CCGRN(ch, C_NRM),
			PRF2_FLAGGED(tch, PRF2_NOWHO) ? CCRED(ch, C_NRM) : 
			CCNRM(ch, C_NRM), GET_NAME(tch), CCNRM(ch, C_NRM),
			((!(++num_can_see % 3)) ? "\r\n" : ""));
	    else 
		sprintf(buf2, "%s%s%s[%s%2d%s%s%s]%s %-12.12s%s%s", buf2,
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCGRN(ch, C_SPR) : 
			 PRF2_FLAGGED(tch, PRF2_NOWHO) ? CCRED(ch, C_NRM) : ""),
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCYEL_BLD(ch, C_NRM) : ""),
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCNRM_GRN(ch, C_NRM) : ""),
			GET_LEVEL(tch), 
			(PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && 
			 PRF_FLAGGED(ch, PRF_HOLYLIGHT)) ? "-" : " ",
			CLASS_ABBR(tch), 
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCYEL_BLD(ch, C_NRM) : ""),
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCNRM_GRN(ch, C_NRM) : ""),
			GET_NAME(tch),
			(GET_LEVEL(tch) >= LVL_AMBASSADOR ? CCNRM(ch, C_SPR) : 
			 PRF2_FLAGGED(tch, PRF2_NOWHO) ? CCNRM(ch, C_NRM) : ""),
			((!(++num_can_see % 3)) ? "\r\n" : ""));
	} else {
	    num_can_see++;
	    if (GET_LEVEL(tch) >= LVL_AMBASSADOR) {
		sprintf(buf2, "%s%s%s[%s%s%s]%s %s%s%s %s%s", buf2,
			CCGRN(ch, C_SPR), CCYEL_BLD(ch, C_NRM), CCNRM_GRN(ch, C_NRM),
			(PLR_FLAGGED(tch, PLR_QUESTOR)          ? "QUESTOR" :
			 !strncmp(GET_NAME(tch), "Fishbone", 8) ? "BONEMAN" : 
			 !strncmp(GET_NAME(tch), "Generic", 7)  ? "GENERIC" : 
			 !strncmp(GET_NAME(tch), "Ska", 3)      ? "  LADY " : 
			 !strncmp(GET_NAME(tch), "Shiva", 5)    ? " DEATH " : 
			 !strncmp(GET_NAME(tch), "Sarflin", 7)  ? "MOONMAN" : 
			 !strncmp(GET_NAME(tch), "PLAYBOY", 7)  ? "STUDMAN" : 
			 !strncmp(GET_NAME(tch), "Joran", 5)    ? "OVRLORD" :
			 !strncmp(GET_NAME(tch), "Smitty", 6)   ? "FOREMAN" :
			 !strncmp(GET_NAME(tch), "Dogma", 5)    ? "FOREMAN" :
			 !strncmp(GET_NAME(tch), "Stryker", 7)  ? "CODEMAN" :
			 !strncmp(GET_NAME(tch), "Forget", 6)   ? " CODER " :
			 LEV_ABBR(tch)),  CCYEL_BLD(ch, C_NRM), CCNRM(ch, C_NRM),
			PRF2_FLAGGED(tch, PRF2_NOWHO) ? nowho_buf : "",
			CCGRN(ch, C_NRM), 
			GET_NAME(tch), GET_TITLE(tch), CCNRM(ch, C_SPR));
	    } else {
		if (IS_VAMPIRE(tch))
		    effective_char_class = GET_OLD_CLASS(tch);
		else
		    effective_char_class = GET_CLASS(tch);
	
		switch (effective_char_class) {
		case CLASS_MAGIC_USER:
		    strcpy(c_buf, CCMAG(ch, C_NRM));
		    break;
		case CLASS_CLERIC:
		    if (IS_GOOD(tch)) {
			strcpy(c_buf, CCYEL_BLD(ch, C_NRM));
		    } else if (IS_EVIL(tch)) {
			strcpy(c_buf, CCRED_BLD(ch, C_NRM));
		    } else
			strcpy(c_buf, CCYEL(ch, C_NRM));
		    break;
		case CLASS_KNIGHT:
		    if (IS_GOOD(tch)) {
			strcpy(c_buf, CCBLU_BLD(ch, C_NRM));
		    } else if (IS_EVIL(tch)) {
			strcpy(c_buf, CCRED(ch, C_NRM));
		    } else
			strcpy(c_buf, CCYEL(ch, C_NRM));
		    break;
		case CLASS_RANGER:
		    strcpy(c_buf, CCGRN(ch, C_NRM));
		    break;
		case CLASS_BARB:
		    strcpy(c_buf, CCCYN(ch, C_NRM));
		    break;
		case CLASS_THIEF:
		    strcpy(c_buf, CCNRM_BLD(ch, C_NRM));
		    break;
		case CLASS_CYBORG:
		    strcpy(c_buf, CCCYN(ch, C_NRM));
		    break;
		case CLASS_PSIONIC:
		    strcpy(c_buf, CCMAG(ch, C_NRM));
		    break;
		case CLASS_PHYSIC:
		    strcpy(c_buf, CCNRM_BLD(ch, C_NRM));
		    break;
		case CLASS_HOOD:
		    strcpy(c_buf, CCRED(ch, C_NRM));
		    break;
		case CLASS_MONK:
		    strcpy(c_buf, CCGRN(ch, C_NRM));
		    break;
		case CLASS_MERCENARY:
		    strcpy(c_buf, CCYEL(ch, C_NRM));
		    break;
		default:
		    strcpy(c_buf, CCNRM(ch, C_NRM));
		    break;
		}
		if (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && 
		    !PRF_FLAGGED(ch, PRF_HOLYLIGHT))
		    sprintf(buf2, "%s%s[%s-- ----%s]%s %s%s %s%s", buf2,
			    CCGRN(ch, C_NRM), CCCYN(ch, C_NRM),
			    CCGRN(ch, C_NRM), 
			    CCNRM(ch, C_NRM), PLR_FLAGGED(tch, PLR_TESTER) ?
			    tester_buf : PRF2_FLAGGED(tch, PRF2_NOWHO) ? nowho_buf :"",  
			    GET_NAME(tch), GET_TITLE(tch), CCNRM(ch, C_NRM));
		else
		    sprintf(buf2, "%s%s[%s%2d%s%s%s%s%s]%s %s%s %s%s", buf2,
			    CCGRN(ch, C_NRM), 
			    (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && 
			     PRF_FLAGGED(ch, PRF_HOLYLIGHT)) ? CCRED(ch, C_NRM) :
			    CCNRM(ch, C_NRM), GET_LEVEL(tch), 
			    (PRF2_FLAGGED(tch, PRF2_ANONYMOUS) && 
			     PRF_FLAGGED(ch, PRF_HOLYLIGHT)) ? "-" : " ",
			    c_buf, char_class_abbrevs[(int)effective_char_class],
			    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), 
			    CCNRM(ch, C_NRM), PLR_FLAGGED(tch, PLR_TESTER) ?
			    tester_buf : PRF2_FLAGGED(tch, PRF2_NOWHO) ? nowho_buf :"",  
			    GET_NAME(tch), GET_TITLE(tch), CCNRM(ch, C_NRM));
	    }
	    if (!who_i && 
		real_clan(GET_CLAN(tch))) {
		if (!PRF2_FLAGGED(tch, PRF2_CLAN_HIDE))
		    sprintf(buf2, "%s %s%s%s", buf2,
			    CCCYN(ch, C_NRM), real_clan(GET_CLAN(tch))->badge,
			    CCNRM(ch, C_NRM));
		else if (GET_LEVEL(ch) > LVL_AMBASSADOR)
		    sprintf(buf2, "%s %s)%s(%s", buf2,
			    CCCYN(ch, C_NRM), real_clan(GET_CLAN(tch))->name,
			    CCNRM(ch, C_NRM));
	    }
	    if (GET_INVIS_LEV(tch))
		sprintf(buf2, "%s %s(%si%d%s)%s", 
			buf2, CCMAG(ch, C_NRM), CCRED(ch, C_NRM),
			GET_INVIS_LEV(tch), CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
	    else if (GET_REMORT_INVIS(tch) && GET_LEVEL(tch) < LVL_AMBASSADOR &&
		     (IS_REMORT(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR))
		sprintf(buf2, "%s %s(%si%d%s)%s", 
			buf2, CCBLU(ch, C_NRM), CCMAG(ch, C_NRM),
			GET_REMORT_INVIS(tch), CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
	    else if (!who_i &&
		     IS_AFFECTED(tch, AFF_INVISIBLE)) {
		sprintf(buf2, "%s %s(invis)%s", 
			buf2, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    } else if (!who_i &&
		       IS_AFFECTED_2(tch, AFF2_TRANSPARENT)) {
		sprintf(buf2, "%s %s(transp)%s", 
			buf2, CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if (PLR_FLAGGED(tch, PLR_MAILING)) {
		sprintf(buf2, "%s %s(mailing)%s", 
			buf2, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    } else if (PLR_FLAGGED(tch, PLR_WRITING)) {
		sprintf(buf2, "%s %s(writing)%s", 
			buf2, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    } else if (PLR_FLAGGED(tch, PLR_OLC)) {
		sprintf(buf2, "%s %s(creating)%s", 
			buf2, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    } else if (tch->desc && tch->desc->showstr_point && 
		       !PRF2_FLAGGED(tch, PRF2_LIGHT_READ)) {
		sprintf(buf2, "%s %s(reading)%s", 
			buf2, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if (!who_i) {
		if (PRF_FLAGGED(tch, PRF_DEAF)) {
		    sprintf(buf2, "%s %s(deaf)%s", 
			    buf2, CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		if (PRF_FLAGGED(tch, PRF_NOTELL)) {
		    sprintf(buf2, "%s %s(notell)%s", 
			    buf2, CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
		}
		if (PRF_FLAGGED(tch, PRF_QUEST)) {
		    sprintf(buf2, "%s %s(quest)%s", 
			    buf2, CCYEL_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
		}
	    }
	    if (PLR_FLAGGED(tch, PLR_AFK)) {
		sprintf(buf2, "%s %s(afk)%s", 
			buf2, CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if (PLR_FLAGGED(tch, PLR_THIEF)) {
		sprintf(buf2, "%s %s(THIEF)%s", 
			buf2, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if (PLR_FLAGGED(tch, PLR_KILLER)) {
		sprintf(buf2, "%s %s(KILLER)%s", 
			buf2, CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if (!who_i &&
		PLR_FLAGGED(tch, PLR_COUNCIL)) {
		//	  (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || PLR_FLAGGED(ch, PLR_COUNCIL))) {
		sprintf(buf2, "%s %s<COUNCIL>%s", 
			buf2, CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
	    }
	    if ((outlaws || who_pkills) && GET_PKILLS(tch)) {
		sprintf(buf2, "%s %s*%d KILLS*%s", 
			buf2, CCRED_BLD(ch, C_NRM), GET_PKILLS(tch), CCNRM(ch, C_NRM));
	    }
	    if (GET_LEVEL(tch) >= LVL_AMBASSADOR)
		strcat(buf2, CCNRM(ch, C_SPR));
	    strcat(buf2, "\r\n");
	}				/* endif shortlist */
    }				/* end of for */
    if (short_list && (num_can_see % 4))
	strcat(buf2, "\r\n");
    strcat(buf2, CCBLD(ch, C_CMP));
    if (num_can_see == 0)
	strcat(buf2, "\r\nNo-one at all!  ");
    else if (num_can_see == 1)
	strcat(buf2, "\r\nOne lonely character displayed.  ");
    else
	sprintf(buf2, "%s\r\n%d characters displayed.  ", buf2, num_can_see);
  
    if (tot_num == 1)
	strcat(buf2, "Only one in the game.\r\n");
    else 
	sprintf(buf2, "%s%d total detected in game.\r\n", buf2, tot_num);

    strcat(buf2, CCNRM(ch, C_CMP));  
    page_string(ch->desc, buf2, 1);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c char_classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
    char line[200], line2[220], idletime[10], char_classname[20];
    char state[30], *timeptr, *format, mode;
    char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
    char out_buf[MAX_STRING_LENGTH];
    struct char_data *tch;
    struct descriptor_data *d;
    int low = 0, high = LVL_GRIMP, i, num_can_see = 0;
    int showchar_class = 0, outlaws = 0, playing = 0, deadweight = 0;

    host_search[0] = name_search[0] = '\0';

    strcpy(buf, argument);
    while (*buf) {
	half_chop(buf, arg, buf1);
	if (*arg == '-') {
	    mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
	    switch (mode) {
	    case 'o':
	    case 'k':
		outlaws = 1;
		playing = 1;
		strcpy(buf, buf1);
		break;
	    case 'p':
		playing = 1;
		strcpy(buf, buf1);
		break;
	    case 'd':
		deadweight = 1;
		strcpy(buf, buf1);
		break;
	    case 'l':
		playing = 1;
		half_chop(buf1, arg, buf);
		sscanf(arg, "%d-%d", &low, &high);
		break;
	    case 'n':
		playing = 1;
		half_chop(buf1, name_search, buf);
		break;
	    case 'h':
		playing = 1;
		half_chop(buf1, host_search, buf);
		break;
	    case 'c':
		playing = 1;
		half_chop(buf1, arg, buf);
		for (i = 0; i < (int)strlen(arg); i++)
		    showchar_class |= find_char_class_bitvector(arg[i]);
		break;
	    default:
		send_to_char(USERS_FORMAT, ch);
		return;
		break;
	    }				/* end of switch */

	} else {			/* endif */
	    send_to_char(USERS_FORMAT, ch);
	    return;
	}
    }				/* end while (parser) */
    strcpy(line,
	   " Num Class     Name         State       Idl Login@   Site\r\n");
    strcat(line,
	   " --- --------- ------------ ------------ -- -------- ---------------------\r\n");
    strcpy(out_buf, line);

    one_argument(argument, arg);

    for (d = descriptor_list; d; d = d->next) {
	if (d->connected && playing)
	    continue;
	if (!d->connected && deadweight)
	    continue;
	if (!d->connected) {
	    if (d->original)
		tch = d->original;
	    else if (!(tch = d->character))
		continue;

	    if (*host_search && !strstr(d->host, host_search))
		continue;
	    if (*name_search && str_cmp(GET_NAME(tch), name_search))
		continue;
	    if (GET_LEVEL(ch) < LVL_LUCIFER)
		if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
		    continue;
	    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
		!PLR_FLAGGED(tch, PLR_THIEF))
		continue;
	    if (showchar_class && !(showchar_class & (1 << GET_CLASS(tch))))
		continue;
	    if (GET_LEVEL(ch) < LVL_LUCIFER)
		if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
		    continue;

	    if (d->original)
		sprintf(char_classname, "[%2d %s]", GET_LEVEL(d->original),
			CLASS_ABBR(d->original));
	    else
		sprintf(char_classname, "[%2d %s]", GET_LEVEL(d->character),
			CLASS_ABBR(d->character));
	} else
	    strcpy(char_classname, "    -    ");

	timeptr = asctime(localtime(&d->login_time));
	timeptr += 11;
	*(timeptr + 8) = '\0';

	if (!d->connected && d->original)
	    strcpy(state, "Switched");
	else
	    strcpy(state, connected_types[d->connected]);

	if (d->character && !d->connected && 
	    (GET_LEVEL(d->character) < GET_LEVEL(ch) || GET_LEVEL(ch) >= LVL_LUCIFER))
	    sprintf(idletime, "%2d", d->character->char_specials.timer *
		    SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
	else
	    sprintf(idletime, "%2d", d->idle);


	format = "%4d %-7s %s%-12s%s %-12s %-2s %-8s ";

	if (d->character && d->character->player.name) {
	    if (d->original)
		sprintf(line, format, d->desc_num, char_classname, CCCYN(ch, C_NRM),
			d->original->player.name, CCNRM(ch, C_NRM), state, idletime, timeptr);
	    else
		sprintf(line, format, d->desc_num, char_classname,
			(GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCGRN(ch, C_NRM) : ""),
			d->character->player.name, 
			(GET_LEVEL(d->character) >= LVL_AMBASSADOR ? CCNRM(ch, C_NRM) : ""),
			state, idletime, timeptr);
	} else
	    sprintf(line, format, d->desc_num, "    -    ", CCRED(ch, C_NRM), 
		    "UNDEFINED", CCNRM(ch, C_NRM), state, idletime, timeptr);

	if (d->host && *d->host)
	    sprintf(line + strlen(line), "%s[%s]%s\r\n", isbanned(d->host, buf) ? 
		    (d->character && PLR_FLAGGED(d->character, PLR_SITEOK) ?
		     CCMAG(ch, C_NRM) : CCRED(ch, C_SPR)) : CCGRN(ch, C_NRM),
		    d->host, CCNRM(ch, C_SPR));
	else {
	    strcat(line, CCRED_BLD(ch, C_SPR));
	    strcat(line, "[Hostname unknown]\r\n");
	    strcat(line, CCNRM(ch, C_SPR));
	}

	if (d->connected) {
	    sprintf(line2, "%s%s%s", CCCYN(ch, C_SPR), line, CCNRM(ch, C_SPR));
	    strcpy(line, line2);
	}
	if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
	    strcat(out_buf, line);
	    num_can_see++;
	}
    }

    sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
    strcat(out_buf, line);
    page_string(ch->desc, out_buf, 1);
}



/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
    extern char circlemud_version[];

    switch (subcmd) {
    case SCMD_CREDITS:
	page_string(ch->desc, credits, 0);
	break;
    case SCMD_NEWS:
	page_string(ch->desc, news, 0);
	break;
    case SCMD_INFO:
	page_string(ch->desc, info, 0);
	break;
    case SCMD_WIZLIST:
	if (clr(ch, C_NRM))
	    page_string(ch->desc, ansi_wizlist, 0);
	else
	    page_string(ch->desc, wizlist, 0);
	break;
    case SCMD_IMMLIST:
	if (clr(ch, C_NRM))
	    page_string(ch->desc, ansi_immlist, 0);
	else
	    page_string(ch->desc, immlist, 0);
	break;
    case SCMD_HANDBOOK:
	page_string(ch->desc, handbook, 0);
	break;
    case SCMD_POLICIES:
	page_string(ch->desc, policies, 0);
	break;
    case SCMD_MOTD:
	if (clr(ch, C_NRM))
	    page_string(ch->desc, ansi_motd, 0);
	else
	    page_string(ch->desc, motd, 0);
	break;
    case SCMD_IMOTD:
	if (clr(ch, C_NRM))
	    page_string(ch->desc, ansi_imotd, 0);
	else
	    page_string(ch->desc, imotd, 0);
	break;
    case SCMD_CLEAR:
	send_to_char("\033[H\033[J", ch);
	break;
    case SCMD_VERSION:
	send_to_char(circlemud_version, ch);
	break;
    case SCMD_WHOAMI:
	send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
	break;
    case SCMD_AREAS:
	skip_spaces(&argument);
	if (!*argument)
	    send_to_char("Do you want a list of low, mid, high, or remort level areas?\r\n"
			 "Usage: areas < low | mid | high | remort >\r\n", ch);
	else if (is_abbrev(argument, "low"))
	    page_string(ch->desc, areas_low, 0);
	else if (is_abbrev(argument, "mid"))
	    page_string(ch->desc, areas_mid, 0);
	else if (is_abbrev(argument, "high"))
	    page_string(ch->desc, areas_high, 0);
	else if (is_abbrev(argument, "remort"))
	    page_string(ch->desc, areas_remort, 0);
	else
	    send_to_char("Usage: areas < low | mid | high | remort >\r\n", ch);
	break;
    default:
	return;
	break;
    }
}


void 
perform_mortal_where(struct char_data * ch, char *arg)
{
    register struct char_data *i;
    register struct descriptor_data *d;

    if (!*arg) {
	send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
	for (d = descriptor_list; d; d = d->next)
	    if (!d->connected) {
		i = (d->original ? d->original : d->character);
		if (i && CAN_SEE(ch, i) && (i->in_room != NULL) &&
		    (ch->in_room->zone == i->in_room->zone)) {
		    sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), i->in_room->name);
		    send_to_char(buf, ch);
		}
	    }
    } else {			/* print only FIRST char, not all. */
	for (i = character_list; i; i = i->next)
	    if (i->in_room->zone == ch->in_room->zone && CAN_SEE(ch, i) &&
		(i->in_room != NULL) && isname(arg, i->player.name)) {
		sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), i->in_room->name);
		send_to_char(buf, ch);
		return;
	    }
	send_to_char("No-one around by that name.\r\n", ch);
    }
}

void 
print_object_location(int num, struct obj_data * obj, 
		      struct char_data * ch, int recur)
{
    if ((obj->carried_by && GET_INVIS_LEV(obj->carried_by) > GET_LEVEL(ch)) ||
	(obj->in_obj && obj->in_obj->carried_by && 
	 GET_INVIS_LEV(obj->in_obj->carried_by) > GET_LEVEL(ch)) ||
	(obj->worn_by && GET_INVIS_LEV(obj->worn_by) > GET_LEVEL(ch)) ||
	(obj->in_obj && obj->in_obj->worn_by && 
	 GET_INVIS_LEV(obj->in_obj->worn_by) > GET_LEVEL(ch)))
	return;
  
    if (num > 0)
	sprintf(buf, "%sO%s%3d. %s%-25s%s - ",CCGRN_BLD(ch, C_NRM),CCNRM(ch, C_NRM),
		num, CCGRN(ch, C_NRM), obj->short_description, CCNRM(ch, C_NRM));
    else
	sprintf(buf, "%33s", " - ");
  
    if (obj->in_room != NULL) {
	sprintf(buf + strlen(buf), "%s[%s%5d%s] %s%s%s\n\r",
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), 
		obj->in_room->number, CCGRN(ch, C_NRM), CCCYN(ch, C_NRM),
		obj->in_room->name, CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    } else if (obj->carried_by) {
	sprintf(buf + strlen(buf), "carried by %s%s%s [%s%5d%s]%s\n\r",
		CCYEL(ch, C_NRM), PERS(obj->carried_by, ch), 
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->carried_by->in_room->number,
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    } else if (obj->worn_by) {
	sprintf(buf + strlen(buf), "worn by %s%s%s [%s%5d%s]%s\n\r",
		CCYEL(ch, C_NRM), PERS(obj->worn_by, ch), 
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->worn_by->in_room->number,
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    } else if (obj->in_obj) {
	sprintf(buf + strlen(buf), "inside %s%s%s%s\n\r",
		CCGRN(ch, C_NRM), obj->in_obj->short_description, 
		CCNRM(ch, C_NRM), (recur ? ", which is" : " "));
	send_to_char(buf, ch);
	if (recur)
	    print_object_location(0, obj->in_obj, ch, recur);
    } else {
	sprintf(buf + strlen(buf), "%sin an unknown location%s\n\r", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    }
}



void 
perform_immort_where(struct char_data * ch, char *arg)
{
    register struct char_data *i;
    register struct obj_data *k;
    struct descriptor_data *d;
    int num = 0, found = 0;
    char main_buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (!*arg) {
	strcpy(main_buf, "Players\r\n-------\r\n");
	for (d = descriptor_list; d; d = d->next)
	    if (!d->connected) {
		i = (d->original ? d->original : d->character);
		if (i && CAN_SEE(ch, i) && (i->in_room != NULL)) {
		    if (d->original)
			sprintf(buf, "%s%-20s%s - %s[%s%5d%s]%s %s%s%s %s(in %s)%s\r\n",
				(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch, C_NRM) : ""),
				GET_NAME(i), (GET_LEVEL(i) >= LVL_AMBASSADOR ? CCNRM(ch, C_NRM) : ""),
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), 
				d->character->in_room->number,
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
				d->character->in_room->name, CCNRM(ch, C_NRM), 
				CCRED(ch, C_CMP), GET_NAME(d->character), CCNRM(ch, C_CMP));
		    else
			sprintf(buf, "%s%-20s%s - %s[%s%5d%s]%s %s%s%s\r\n",
				(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch, C_NRM) : ""),
				GET_NAME(i), (GET_LEVEL(i) >= LVL_AMBASSADOR ? CCNRM(ch, C_NRM) : ""),
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), i->in_room->number, 
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), 
				i->in_room->name, CCNRM(ch, C_NRM));
		    strcat(main_buf, buf);
		}
	    }
	page_string(ch->desc, main_buf, 1);
    } else {
	two_arguments(arg, arg1, arg2);
	for (i = character_list; i; i = i->next)
	    if (CAN_SEE(ch, i) && i->in_room && isname(arg1, i->player.name) &&
		(++num) &&
		(!*arg2 || isname(arg2, i->player.name)) &&
		(GET_MOB_SPEC(i) != fate || GET_LEVEL(ch) >= LVL_SPIRIT)) {
		found = 1;
		sprintf(buf, "%sM%s%3d. %s%-25s%s - %s[%s%5d%s]%s %s%s%s\r\n", 
			CCRED_BLD(ch, NRM), CCNRM(ch, NRM), num, 
			CCYEL(ch, C_NRM), GET_NAME(i), CCNRM(ch, C_NRM), 
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), i->in_room->number, 
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), 
			i->in_room->name, CCNRM(ch, C_NRM));
		send_to_char(buf, ch);
	    }
	for (num = 0, k = object_list; k; k = k->next)
	    if (CAN_SEE_OBJ(ch, k) && isname(arg1, k->name) && ++num && (!*arg2 || isname(arg2, k->name))) {
		found = 1;
		print_object_location(num, k, ch, TRUE);
	    }
	if (!found)
	    send_to_char("Couldn't find any such thing.\r\n", ch);
    }
}




ACMD(do_where)
{
    skip_spaces(&argument);
 
    if (GET_LEVEL(ch) >= LVL_AMBASSADOR || 
	(IS_MOB(ch) && ch->desc && ch->desc->original &&
	 GET_LEVEL(ch->desc->original) >= LVL_AMBASSADOR)
	|| PLR_FLAGGED(ch, PLR_TESTER))
	perform_immort_where(ch, argument);
    else {

	sprintf(buf, "You are located: %s%s%s\r\nIn: %s%s%s.\r\n", 
		CCGRN(ch, C_NRM), (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))?
		"In a very dark place." : 
		ch->in_room->name, CCNRM(ch, C_NRM),
		CCGRN(ch, C_NRM), (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))?
		"You can't tell for sure." : 
		ch->in_room->zone->name,  CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    if(ZONE_FLAGGED(ch->in_room->zone,ZONE_NOLAW))
        send_to_char("This place is beyond the reach of law.\r\n",ch);
    if(!IS_APPR(ch->in_room->zone)) {
        sprintf(buf,"This zone is %sUNAPPROVED%s!\r\n",CCRED(ch,C_NRM),CCNRM(ch,C_NRM));
        send_to_char(buf,ch);
    }
	act("$n ponders the implications of $s location.", TRUE, ch, 0, 0, TO_ROOM);
    }
}


void 
print_attributes_to_buf(struct char_data *ch, char *buff)
{

    sbyte str, stradd, intel, wis, dex, con, cha;
    str = GET_STR(ch);
    stradd = GET_ADD(ch);
    intel = GET_INT(ch);
    wis = GET_WIS(ch);
    dex = GET_DEX(ch);
    con = GET_CON(ch);
    cha = GET_CHA(ch);
    sprintf(buf2, " %s%s(augmented)%s\r\n", 
	    CCBLD(ch, C_SPR), CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));

    sprintf(buff, "%s%sSTR:%s ", 
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
  
    if (str <= 5)
	strcat(buff, "You are a weakling.");
    else if (str <= 8)
	strcat(buff, "You are not very strong.");
    else if (str <= 10)
	strcat(buff, "Your strength is average.");
    else if (str <= 12)
	strcat(buff, "You are fairly strong.");
    else if (str <= 15)
	strcat(buff, "You are a nice specimen.");
    else if (str <= 16)
	strcat(buff, "You are a damn nice specimen.");
    else if (str < 18)
	strcat(buff, "You are very strong.");
    else if (str == 18 && stradd <= 20)
	strcat(buff, "You are extremely strong.");
    else if (str == 18 && stradd <= 50)
	strcat(buff, "You are exceptionally strong.");
    else if (str == 18 && stradd <= 70)
	strcat(buff, "You strength is awe inspiring.");
    else if (str == 18 && stradd <= 90)
	strcat(buff, "Your strength is super-human.");
    else if (str == 18)
	strcat(buff, "Your strength is at the human peak!");
    else if (str == 19)
	strcat(buff, "You have the strength of a hill giant!");
    else if (str == 20)
	strcat(buff, "You have the strength of a stone giant!");
    else if (str == 21)
	strcat(buff, "You have the strength of a frost giant!");
    else if (str == 22)
	strcat(buff, "You have the strength of a fire giant!");
    else if (str == 23)
	strcat(buff, "You have the strength of a cloud giant!");
    else if (str == 24)
	strcat(buff, "You have the strength of a storm giant!");
    else if (str == 25)
	strcat(buff, "You have the strength of a titan!");
    else
	strcat(buff, "Your strength is SKREWD.");

    if (str != ch->real_abils.str || stradd != ch->real_abils.str_add)
	strcat(buff, buf2);
    else 
	strcat(buff, "\r\n");

    sprintf(buff, "%s%s%sINT:%s ", buff,
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (intel <= 5)
	strcat(buff, "You are mentally challenged.");
    else if (intel <= 8)
	strcat(buff, "You're about as smart as a rock.");
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
	strcat(buff, "You have the intelligence of a god!");
    else
	strcat(buff, "You solve nonlinear higher dimensional systems in your sleep.");
    if (intel != ch->real_abils.intel)
	strcat(buff, buf2);
    else 
	strcat(buff, "\r\n");

    sprintf(buff, "%s%s%sWIS:%s ", buff, 
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (wis <= 5)
	strcat(buff, "Yoda you are not.");
    else if (wis <= 8)
	strcat(buff, "You enjoy intelligent conversations with grass.");
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
    else 
	strcat(buff, "\r\n");

    sprintf(buff, "%s%s%sDEX:%s ", buff,
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (dex <= 5)
	strcat(buff, "I wouldnt walk too fast if I were you.");
    else if (dex <= 8)
	strcat(buff, "Your pretty clumsy.");
    else if (dex <= 10)
	strcat(buff, "Your agility is average.");
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
    else 
	strcat(buff, "\r\n");

    sprintf(buff, "%s%s%sCON:%s ", buff,
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
  
    if (con <= 5)
	strcat(buff, "Your as healthy as a rabid dog.");
    else if (con <= 8)
	strcat(buff, "Your pretty skinny and sick looking.");
    else if (con <= 10)
	strcat(buff, "Your health is average.");
    else if (con <= 12)
	strcat(buff, "You are fairly healthy.");
    else if (con <= 15)
	strcat(buff, "You are very healthy.");
    else if (con <= 18)
	strcat(buff, "You are exceptionally healthy.");
    else  if (con <= 19)
	strcat(buff, "You are practically immortal!");
    else 
	strcat(buff, "You can eat pathogens for breakfast.");

    if (con != ch->real_abils.con)
	strcat(buff, buf2);
    else 
	strcat(buff, "\r\n");

    sprintf(buff, "%s%s%sCHA:%s ", buff,
	    CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

    if (cha <= 5)
	strcat(buff, "Your face could turn a family of elephants to stone.");
    else if (cha <= 8)
	strcat(buff, "U-G-L-Y");
    else if (cha <= 10)
	strcat(buff, "You are not too unpleasant to deal with.");
    else if (cha <= 12)
	strcat(buff, "You are a pleasant person.");
    else if (cha <= 15)
	strcat(buff, "Others eat from the palm of your hand.");
    else if (cha <= 18)
	strcat(buff, "Your image should be chiseled in marble!");
    else
	strcat(buff, "If the gods made better they'd have kept it for themselves.");
    if (cha != ch->real_abils.cha)
	strcat(buff, buf2);
    else 
	strcat(buff, "\r\n");

}


ACMD(do_attributes)
{
    sprintf(buf, "\r\n%s**********************%sAttributes%s***********************%s\r\n",
	    CCBLU(ch, C_SPR), CCYEL(ch, C_SPR), CCBLU(ch, C_SPR), CCWHT(ch, C_SPR));
    send_to_char(buf, ch);
    sprintf(buf, "Name:  %s%20s%s  Race: %s%10s%s\r\n", CCRED(ch, C_SPR), GET_NAME(ch),
            CCWHT(ch, C_SPR), CCRED(ch, C_SPR), player_race[(int)GET_RACE(ch)], 
	    CCWHT(ch, C_SPR));
    send_to_char(buf, ch);
    sprintf(buf, "Class: %s%20s%s  Level: %s%9d%s\r\n\r\n", CCRED(ch, C_SPR), 
	    pc_char_class_types[(int)GET_CLASS(ch)], CCWHT(ch, C_SPR), 
	    CCRED(ch, C_SPR), GET_LEVEL(ch), CCWHT(ch, C_SPR));
    send_to_char(buf, ch);
 
    print_attributes_to_buf(ch, buf);

    send_to_char(buf, ch);
    sprintf(buf, "\r\n%s*********************************************************%s\r\n",
	    CCBLU(ch, C_SPR), CCWHT(ch, C_SPR));
    send_to_char(buf, ch);
}

ACMD(do_levels)
{
    int i;

    if (IS_NPC(ch)) {
	send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
	return;
    }
    *buf = '\0';

    for (i = 1; i < LVL_AMBASSADOR; i++) {
	sprintf(buf + strlen(buf), "[%s%2d%s] %10d%s-%s%-10d : %s", 
		CCCYN(ch, C_NRM), i, CCNRM(ch, C_NRM),
		exp_scale[i], CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
		exp_scale[i + 1], CCYEL(ch, C_NRM));
	if (GET_CLASS(ch) == CLASS_KNIGHT && IS_EVIL(ch))
	    strcat(buf, evil_knight_titles[i]);
	else {
	    switch (GET_SEX(ch)) {
	    case SEX_MALE:
	    case SEX_NEUTRAL:
		strcat(buf, 
		       titles[(int) MIN(GET_CLASS(ch), NUM_CLASSES-1)][i].title_m);
		break;
	    case SEX_FEMALE:
		strcat(buf, 
		       titles[(int) MIN(GET_CLASS(ch), NUM_CLASSES-1)][i].title_f);
		break;
	    default:
		send_to_char("Oh dear.  You seem to be sexless.\r\n", ch);
		break;
	    }
	}
	strcat(buf, CCNRM(ch, C_NRM));
	strcat(buf, "\r\n");
    }
    page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
    struct char_data *victim;
    int diff, ac;

    one_argument(argument, buf);

    if (!(victim = get_char_room_vis(ch, buf))) {
	send_to_char("Consider killing who?\r\n", ch);
	return;
    }
    if (victim == ch) {
	send_to_char("Easy!  Very easy indeed!\r\n", ch);
	return;
    }
    if (!IS_NPC(victim)) {
	send_to_char("Well, if you really want to kill another player...\r\n", ch);
    }

    diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

    if (diff <= -30)
	send_to_char("It's not even worth the effort...\r\n", ch);
    else if (diff <= -22)
	send_to_char("You could do it without any hands!\r\n", ch);
    else if (diff <= -19)
	send_to_char("You may not even break a sweat.\r\n", ch);
    else if (diff <= -16)
	send_to_char("You've thrashed far worse.\r\n", ch);
    else if (diff <= -13)
	send_to_char("It may be quite easy...\r\n", ch);
    else if (diff <= -10)
	send_to_char("Should be fairly easy.\r\n", ch);
    else if (diff <= -6)
	send_to_char("You should be able to handle it.\r\n", ch);
    else if (diff <= -4)
	send_to_char("It looks like you have a definite advantage.\r\n", ch);
    else if (diff <= -2)
	send_to_char("You seem to have a slight advantage.\r\n", ch);
    else if (diff <= -1)
	send_to_char("The odds here are just barely in your favor.\r\n", ch);
    else if (diff == 0)
	send_to_char("It's a pretty even match.\r\n", ch);
    else if (diff <= 1)
	send_to_char("Keep your wits about you...\r\n", ch);
    else if (diff <= 2)
	send_to_char("With a bit of luck, you could win.\r\n", ch);
    else if (diff <= 4)
	send_to_char("You should probably stretch out first.\r\n", ch);
    else if (diff <= 6)
	send_to_char("You probably couldn't do it bare handed...\r\n", ch);
    else if (diff <= 8)
	send_to_char("You won't catch me betting on you.\r\n", ch);
    else if (diff <= 10)
	send_to_char("You may get your ass whipped!\r\n", ch);
    else if (diff <= 14)
	send_to_char("You better have some nice equipment!\r\n", ch);
    else if (diff <= 18)
	send_to_char("You better spend some more time in the gym!\r\n", ch);
    else if (diff <= 22)
	send_to_char("Well, you only live once...\r\n", ch);
    else if (diff <= 26)
	send_to_char("You must be out of your freakin mind.\r\n", ch);
    else if (diff <= 30)
	send_to_char("What?? Are you STUPID or something?!!\r\n", ch);
    else 
	send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);

    if (GET_SKILL(ch, SKILL_CONSIDER) > 70) {
	diff = (GET_MAX_HIT(victim) - GET_MAX_HIT(ch)); 
	if (diff <= -300)
	    act("$E looks puny, and weak.", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= -200)
	    act("$E would die ten times before you would be killed.", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= -100)
	    act("You could beat $M to death with your forehead.",FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= -50)
	    act("$E can take almost as much as you.",FALSE, ch, 0, victim, TO_CHAR); 
	else if (diff <= 50)
	    send_to_char("You can both take pretty much the same abuse.\r\n", ch);
	else if (diff <= 200)
	    act("$E looks like $E could take a lickin.", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= 600)
	    act("Haven't you seen $M breaking bricks on $S head?", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= 900)
	    act("You would bet $E eats high voltage cable for breakfast.", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= 1200)
	    act("$E probably isn't very scared of bulldozers.", FALSE, ch, 0, victim, TO_CHAR);
	else if (diff <= 1800)
	    act("A blow from a house-sized meteor MIGHT do $M in.", FALSE, ch, 0, victim, TO_CHAR);
	else
	    act("Maybe if you threw $N into the sun...", FALSE, ch, 0, victim, TO_CHAR);

	ac = GET_AC(victim);
	if (ac <= -100)
	    act("$E makes battleships look silly.", FALSE, ch, 0, victim, TO_CHAR);
	else if (ac <= -50)
	    act("$E is about as defensible as a boulder.", FALSE, ch, 0, victim, TO_CHAR);
	else if (ac <= 0)
	    act("$E has better defenses than most small cars.", FALSE, ch, 0, victim, TO_CHAR);
	else if (ac <= 50)
	    act("$S defenses are pretty damn good.", FALSE, ch, 0, victim, TO_CHAR);
	else if (ac <= 70)
	    act("$S body appears to be well protected.", FALSE, ch, 0, victim, TO_CHAR);
	else if (ac <= 90)
	    act("Well, $E's better off than a naked person.", FALSE, ch, 0, victim, TO_CHAR);
	else
	    act("$S armor SUCKS!", FALSE, ch, 0, victim, TO_CHAR);
    }
}



ACMD(do_diagnose)
{
    struct char_data *vict;

    one_argument(argument, buf);

    if (*buf) {
	if (!(vict = get_char_room_vis(ch, buf))) {
	    send_to_char(NOPERSON, ch);
	    return;
	} else
	    diag_char_to_char(vict, ch);
    } else {
	if (FIGHTING(ch))
	    diag_char_to_char(FIGHTING(ch), ch);
	else
	    send_to_char("Diagnose who?\r\n", ch);
    }
}


static const char *ctypes[] = {
    "off", "sparse", "normal", "complete", "\n"};

ACMD(do_color)
{
    int tp;

    if (IS_NPC(ch))
	return;

    one_argument(argument, arg);

    if (!*arg) {
	sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
	send_to_char(buf, ch);
	return;
    }
    if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
	send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
	return;
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
    SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

    sprintf(buf, "Your %scolor%s is now %s%s%s%s.\r\n", CCRED(ch, C_SPR),
	    CCNRM(ch, C_OFF), CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), ctypes[tp], CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
}


ACMD(do_toggle)
{
    if (IS_NPC(ch))
	return;
    if (GET_WIMP_LEV(ch) == 0)
	strcpy(buf2, "OFF");
    else
	sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

    sprintf(buf,
	    "Hit Pnt Display: %-3s    "
	    "     Brief Mode: %-3s    "
	    "Identify Resist: %-3s\r\n"

	    "   Move Display: %-3s    "
	    "   Compact Mode: %-3s    "
	    "       On Quest: %-3s\r\n"

	    "   Mana Display: %-3s    "
	    "         NoTell: %-3s    "
	    "   Repeat Comm.: %-3s\r\n"

	    " Auto Show Exit: %-3s    "
	    "           Deaf: %-3s    "
	    "     Wimp Level: %-3s\r\n"

	    "     See misses: %-3s    "
	    "       Autopage: %-3s    "
	    "    Color Level: %s\r\n"

	    "  Summon Resist: %-3s    "
	    "  Screen Length: %-3d    "
	    " Newbie Helper?: %-3s\r\n"

	    "   Autodiagnose: %-3s    "
	    "    Projections: %-3s    "
	    "   Show Affects: %-3s\r\n"
	  
	    "     Clan title: %-3s    "
	    "      Clan hide: %-3s    "
	    "   Light Reader: %-3s\r\n"

	    "     Autoprompt: %-3s    "
	    "          Nowho: %-3s    "
	    "       TOUGHGUY: %-3s\r\n"

	    "      Anonymous: %-3s    "
	    "     Notrailers: %-3s    "
	    "        PKILLER: %-3s\r\n"
	  
	    "      Autosplit: %-3s    "
	    "       Autoloot: %-3s    "
	    "   REMORT TOUGH: %-3s\r\n"

	    " Gossip Channel: %-3s    "
	    "Auction Channel: %-3s    "
	    "  Grats Channel: %-3s\r\n"
  
	    "   Spew Channel: %-3s    "
	    "  Music Channel: %-3s    "
	    "  Dream Channel: %-3s\r\n",


	    ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	    ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	    ONOFF(PRF_FLAGGED(ch, PRF_NOIDENTIFY)),

	    ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	    ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	    YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	    ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	    ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	    YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	    ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)), 
	    YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	    buf2,

	    YESNO(!PRF_FLAGGED(ch, PRF_GAGMISS)),
	    ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOPAGE)), 
	    ctypes[COLOR_LEV(ch)],

	    ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),
	    GET_PAGE_LENGTH(ch),
	    YESNO(PRF2_FLAGGED(ch, PRF2_NEWBIE_HELPER)),

	    ONOFF(PRF2_FLAGGED(ch, PRF2_AUTO_DIAGNOSE)),
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOPROJECT)),
	    YESNO(!PRF2_FLAGGED(ch, PRF2_NOAFFECTS)),

	    YESNO(PRF2_FLAGGED(ch, PRF2_CLAN_TITLE)),
	    YESNO(PRF2_FLAGGED(ch, PRF2_CLAN_HIDE)),
	    YESNO(PRF2_FLAGGED(ch, PRF2_LIGHT_READ)),

	    YESNO(PRF2_FLAGGED(ch, PRF2_AUTOPROMPT)),
	    YESNO(PRF2_FLAGGED(ch, PRF2_NOWHO)),
	    YESNO(PLR_FLAGGED(ch, PLR_TOUGHGUY)),

	    YESNO(PRF2_FLAGGED(ch, PRF2_ANONYMOUS)),
	    ONOFF(PRF2_FLAGGED(ch, PRF2_NOTRAILERS)),
	    YESNO( PRF2_FLAGGED( ch, PRF2_PKILLER ) ),

	    ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)),
	    ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOLOOT)),
	    YESNO(PLR_FLAGGED(ch, PLR_REMORT_TOUGHGUY)),
	  
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
	  
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOSPEW)),
	    ONOFF(!PRF_FLAGGED(ch, PRF_NOMUSIC)),
	    ONOFF(!PRF_FLAGGED(ch, PRF_NODREAM)));

    send_to_char(buf, ch);

    if (IS_MAGE(ch)) {
	sprintf(buf, "((Mana shield)) Low:[%ld], Percent:[%ld]\n",
		GET_MSHIELD_LOW(ch), GET_MSHIELD_PCT(ch));
	send_to_char(buf, ch);
    }
}


struct sort_struct {
    int sort_pos;
    byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
    int a, b, tmp;

    ACMD(do_action);

    num_of_cmds = 0;

    /*
     * first, count commands (num_of_commands is actually one greater than the
     * number of commands; it inclues the '\n'.
     */
    while (*cmd_info[num_of_cmds].command != '\n')
	num_of_cmds++;

    /* create data array */
    CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

    /* initialize it */
    for (a = 1; a < num_of_cmds; a++) {
	cmd_sort_info[a].sort_pos = a;
	cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
    }

    /* the infernal special case */
    cmd_sort_info[find_command("insult")].is_social = TRUE;

    /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
    for (a = 1; a < num_of_cmds - 1; a++)
	for (b = a + 1; b < num_of_cmds; b++)
	    if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		       cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
		tmp = cmd_sort_info[a].sort_pos;
		cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
		cmd_sort_info[b].sort_pos = tmp;
	    }
}



ACMD(do_commands)
{
    int no, i, cmd_num;
    int wizhelp = 0, socials = 0, level = 0;
    struct char_data *vict = NULL;

    one_argument(argument, arg);

    if (*arg) {
	if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
	    if (is_number(arg)) {
		level = atoi(arg);
	    } else {
		send_to_char("Who is that?\r\n", ch);
		return;
	    }
	} else
	    level = GET_LEVEL(vict);

	if (level < 0) {
	    send_to_char("What a comedian.\r\n", ch);
	    return;
	}
	if (GET_LEVEL(ch) < level) {
	    send_to_char("You can't see the commands of people above your level.\r\n", ch);
	    return;
	}
    } else {
	vict = ch;
	level = GET_LEVEL(ch);
    }

    if (subcmd == SCMD_SOCIALS)
	socials = 1;
    else if (subcmd == SCMD_WIZHELP)
	wizhelp = 1;

    sprintf(buf, "The following %s%s are available to %s:\r\n",
	    wizhelp ? "privileged " : "",
	    socials ? "socials" : "commands",
	    (vict && vict == ch) ? "you" : vict ? GET_NAME(vict) :
	    "that level");

    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
	i = cmd_sort_info[cmd_num].sort_pos;
	if (cmd_info[i].minimum_level >= 0 &&
	    level >= cmd_info[i].minimum_level &&
	    (cmd_info[i].minimum_level >= LVL_AMBASSADOR) == wizhelp &&
	    (wizhelp || socials == cmd_sort_info[i].is_social)) {
	    sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
	    if (!(no % 7))
		strcat(buf, "\r\n");
	    no++;
	}
    }

    strcat(buf, "\r\n");
    page_string(ch->desc, buf, 1);
}

ACMD(do_soilage)
{
    int i, j, k, pos, found = 0;

    strcpy(buf, "Soilage status:\r\n");

    for (i = 0; i < NUM_WEARS; i++) {
	pos = (int) eq_pos_order[i];

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
	    found = FALSE;
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
    page_string(ch->desc, buf, 1);
 
}
ACMD(do_skills)
{
    int parse_char_class(char *arg);
    void show_char_class_skills(struct char_data *ch, int con, int immort, int bits);
    void list_skills(struct char_data * ch, int mode, int type);
    char arg[MAX_INPUT_LENGTH];
    int char_class = 0;

    argument = one_argument(argument, arg);
  
    if (*arg && is_abbrev(arg, "list")) {
	if (*argument) {
	    skip_spaces(&argument);
	    if ((char_class = parse_char_class(argument)) < 0) {
		sprintf(buf, "No such char_class, '%s'.\r\n", argument);
		send_to_char(buf, ch);
	    } else if (GET_LEVEL(ch) < LVL_AMBASSADOR && 
		       char_class != GET_CLASS(ch) && char_class != GET_REMORT_CLASS(ch)) {
		send_to_char("You do not belong to that char_class.\r\n", ch);
	    } else
		show_char_class_skills(ch, char_class, 0, 
				  (subcmd ? 
				   (char_class == CLASS_PSIONIC ? TRIG_BIT :
				    char_class == CLASS_PHYSIC ? ALTER_BIT :
				    char_class == CLASS_MONK ? ZEN_BIT :
				    SPELL_BIT) : 0));
	} else 
	    show_char_class_skills(ch, (char_class = GET_CLASS(ch)), 0, 
			      (subcmd ? 
			       (char_class == CLASS_PSIONIC ? TRIG_BIT :
				char_class == CLASS_PHYSIC ? ALTER_BIT :
				char_class == CLASS_MONK ? ZEN_BIT :
				SPELL_BIT) : 0));
			
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
		if (j == MAX_WEAPON_SPEC-1) {
		    GET_WEAP_SPEC(ch, j).vnum =  0;
		    GET_WEAP_SPEC(ch, j).level = 0;
		    break;
		}
		GET_WEAP_SPEC(ch, j).vnum = GET_WEAP_SPEC(ch, j+1).vnum;
		GET_WEAP_SPEC(ch, j).level = GET_WEAP_SPEC(ch, j+1).level;
	    }
	}
    }
  
    if ((char_class = GET_CLASS(ch)) >= NUM_CLASSES)
	char_class = CLASS_WARRIOR;
    if (IS_REMORT(ch) && GET_REMORT_CLASS(ch) < NUM_CLASSES) {
	if (weap_spec_char_class[char_class].max < weap_spec_char_class[GET_REMORT_CLASS(ch)].max)
	    char_class = GET_REMORT_CLASS(ch);
    }

    sprintf(buf, "As a %s you can specialize in %d weapons.\r\n",
	    pc_char_class_types[char_class], weap_spec_char_class[char_class].max);
    for (obj = NULL, i = 0; i < MAX_WEAPON_SPEC;obj = NULL) {
	if (GET_WEAP_SPEC(ch, i).level &&
	    GET_WEAP_SPEC(ch, i).vnum > 0 &&
	    (obj = real_object_proto(GET_WEAP_SPEC(ch, i).vnum)))
	    sprintf(buf, "%s %2d. %-30s [%d]\r\n", buf, ++i, obj->short_description,
		    GET_WEAP_SPEC(ch, i).level);
	else
	    break;
    }
    send_to_char(buf, ch);
}
      

ACMD(do_alignment)     
{

    char cbuf[ MAX_INPUT_LENGTH ];



    if ( GET_ALIGNMENT( ch ) < -300 ) {
	sprintf( cbuf, "%s", CCRED( ch, C_NRM ) );
    }

    else if (GET_ALIGNMENT( ch ) > 300 ) {
	sprintf( cbuf, "%s", CCCYN( ch, C_NRM ) );
    }

    else {
	sprintf( cbuf, "%s", CCYEL( ch, C_NRM ) );
    }

    sprintf( buf, "%sYour alignment is%s %s%d%s.\r\n",CCWHT( ch, C_NRM), CCNRM( ch, C_NRM ),  cbuf, GET_ALIGNMENT( ch ),
	     CCNRM( ch, C_NRM ) ); 

    send_to_char( buf, ch );

}




