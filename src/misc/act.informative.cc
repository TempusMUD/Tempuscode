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

#include <list>
#include <string>
#include <sstream>
using namespace std;

#include "structs.h"
#include "utils.h"
#include "comm.h"
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
#include "events.h"
#include "security.h"
#include "char_class.h"
#include "tmpstr.h"
#include "accstr.h"
#include "tokenizer.h"
#include "player_table.h"
#include "language.h"

/* extern variables */
extern int mini_mud;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern CreatureList characterList;

extern struct obj_data *object_list;
//extern const struct command_info cmd_info[];
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
extern const char *char_class_abbrevs[];
extern const char *level_abbrevs[];
extern const char *player_race[];
extern const char *pc_char_class_types[];
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
extern const struct {
	double multiplier;
	int max;
} weap_spec_char_class[];
long find_char_class_bitvector(char arg);

int isbanned(char *hostname, char *blocking_hostname);
char *obj_cond(struct obj_data *obj);  /** writes to buf2 **/
char *obj_cond_color(struct obj_data *obj, struct Creature *ch);  /**writes to buf2 **/
int same_obj(struct obj_data *obj1, struct obj_data *obj2);

ACMD(do_stand);
ACMD(do_say);

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

#define SHOW_OBJ_ROOM    0
#define SHOW_OBJ_INV     1
#define SHOW_OBJ_CONTENT 2
#define SHOW_OBJ_NOBITS  3
#define SHOW_OBJ_EXTRA   5
#define SHOW_OBJ_BITS    6

void
show_obj_to_char(struct obj_data *object, struct Creature *ch,
	int mode, int count)
{
	bool found = false;

	acc_string_clear();
	if (mode == SHOW_OBJ_ROOM) {
		if (object->line_desc)
			acc_strcat(object->line_desc, NULL);
		else if (IS_IMMORT(ch))
			acc_sprintf("%s exists here.\r\n", object->name);
	} else if (mode == 1 || mode == 2 || mode == 3 || mode == 4) {
		if (object->name)
			acc_strcat(object->name, NULL);
	} else if (mode == 5) {
		if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
			if (object->action_desc) {
				page_string(ch->desc, object->action_desc);
			} else {
				act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
			}
			return;
		} else if (GET_OBJ_TYPE(object) == ITEM_DRINKCON)
			acc_strcat("It looks like a drink container.", NULL);
		else if (GET_OBJ_TYPE(object) == ITEM_FOUNTAIN)
			acc_strcat("It looks like a source of drink.", NULL);
		else if (GET_OBJ_TYPE(object) == ITEM_FOOD)
			acc_strcat("It looks edible.", NULL);
		else if (GET_OBJ_TYPE(object) == ITEM_HOLY_SYMB)
			acc_strcat("It looks like the symbol of some deity.", NULL);
		else if (GET_OBJ_TYPE(object) == ITEM_CIGARETTE ||
			GET_OBJ_TYPE(object) == ITEM_PIPE) {
			if (GET_OBJ_VAL(object, 3))
				acc_strcat("It appears to be lit and smoking.", NULL);
			else
				acc_strcat("It appears to be unlit.", NULL);
		} else if (GET_OBJ_TYPE(object) == ITEM_CONTAINER) {
			if (GET_OBJ_VAL(object, 3)) {
				acc_strcat("It looks like a corpse.\r\n", NULL);
			} else {
				acc_strcat("It looks like a container.\r\n", NULL);
				if (CAR_CLOSED(object) && CAR_OPENABLE(object))	/*macro maps to containers too */
					acc_strcat("It appears to be closed.\r\n", NULL);
				else if (CAR_OPENABLE(object)) {
					acc_strcat("It appears to be open.\r\n", NULL);
					if (object->contains)
						acc_strcat(
							"There appears to be something inside.\r\n", NULL);
					else
						acc_strcat("It appears to be empty.\r\n", NULL);
				}
			}
		} else if (GET_OBJ_TYPE(object) == ITEM_SYRINGE) {
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
	if (mode != 3) {
		found = false;

		if (IS_OBJ_STAT2(object, ITEM2_BROKEN)) {
			acc_sprintf(" %s<broken>", CCNRM(ch, C_NRM));
			found = true;
		}
		if (object->in_obj && IS_CORPSE(object->in_obj) && IS_IMPLANT(object)
			&& !CAN_WEAR(object, ITEM_WEAR_TAKE)) {
			acc_strcat(" (implanted)", NULL);
			found = true;
		}

		if (ch == object->carried_by || ch == object->worn_by) {
			if (OBJ_REINFORCED(object))
				acc_sprintf(" %s[%sreinforced%s]%s",
					CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM),
					CCNRM(ch, C_NRM));
			if (OBJ_ENHANCED(object))
				acc_sprintf(" %s|enhanced|%s",
					CCMAG(ch, C_NRM), CCNRM(ch, C_NRM));
		}

		if (IS_OBJ_TYPE(object, ITEM_DEVICE) && ENGINE_STATE(object))
			acc_strcat(" (active)", NULL);

		if (((GET_OBJ_TYPE(object) == ITEM_CIGARETTE ||
					GET_OBJ_TYPE(object) == ITEM_PIPE) &&
				GET_OBJ_VAL(object, 3)) ||
			(IS_BOMB(object) && object->contains && IS_FUSE(object->contains)
				&& FUSE_STATE(object->contains))) {
			acc_sprintf(" %s(lit)%s",
				CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
			acc_sprintf(" %s(invisible)%s",
				CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT(object, ITEM_TRANSPARENT)) {
			acc_sprintf(" %s(transparent)%s",
				CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT2(object, ITEM2_HIDDEN)) {
			acc_sprintf(" %s(hidden)%s",
				CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}

		if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) ||
			(IS_CLERIC(ch) && IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))) {
			if (IS_OBJ_STAT(object, ITEM_BLESS)) {
				acc_sprintf(" %s(holy aura)%s",
					CCBLU_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
				found = true;
			} else if (IS_OBJ_STAT(object, ITEM_DAMNED)) {
				acc_sprintf(" %s(unholy aura)%s",
					CCRED_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
				found = true;
			}
		}
		if ((IS_AFFECTED(ch, AFF_DETECT_MAGIC)
				|| IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))
			&& IS_OBJ_STAT(object, ITEM_MAGIC)) {
			acc_sprintf(" %s(yellow aura)%s",
				CCYEL_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
			found = true;
		}

		if ((IS_AFFECTED(ch, AFF_DETECT_MAGIC)
				|| IS_AFFECTED_2(ch, AFF2_TRUE_SEEING)
				|| PRF_FLAGGED(ch, PRF_HOLYLIGHT))
			&& GET_OBJ_SIGIL_IDNUM(object)) {
			acc_sprintf(" %s(%ssigil%s)%s",
				CCYEL(ch, C_NRM), CCMAG(ch, C_NRM), CCYEL(ch, C_NRM),
				CCNRM(ch, C_NRM));
		}

		if (IS_OBJ_STAT(object, ITEM_GLOW)) {
			acc_sprintf(" %s(glowing)%s",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT(object, ITEM_HUM)) {
			acc_sprintf(" %s(humming)%s",
				CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT2(object, ITEM2_ABLAZE)) {
			acc_sprintf(" %s*burning*%s",
				CCRED_BLD(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (IS_OBJ_STAT3(object, ITEM3_HUNTED)) {
			acc_sprintf(" %s(hunted)%s",
				CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (OBJ_SOILED(object, SOIL_BLOOD)) {
			acc_sprintf(" %s(bloody)%s",
				CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (OBJ_SOILED(object, SOIL_WATER)) {
			acc_sprintf(" %s(wet)%s",
				CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (OBJ_SOILED(object, SOIL_MUD)) {
			acc_sprintf(" %s(muddy)%s",
				CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if (OBJ_SOILED(object, SOIL_ACID)) {
			acc_sprintf(" %s(acid covered)%s",
				CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
			found = true;
		}
		if ( object->shared->owner_id != 0 ) {
			acc_sprintf(" %s(protected)%s",
				CCYEL_BLD(ch, C_SPR), CCNRM(ch, C_SPR));
			found = true;
		}
        if( object->tmp_affects != NULL ) {
            if( object->affectedBySpell(SPELL_ITEM_REPULSION_FIELD) != NULL ) {
                acc_sprintf(" %s(repulsive)%s",
                                  CCCYN(ch, C_SPR), CCNRM(ch, C_SPR));
            } else if( object->affectedBySpell(SPELL_ITEM_ATTRACTION_FIELD) != NULL ) {
                acc_sprintf(" %s(attractive)%s",
                                  CCCYN(ch, C_SPR), CCNRM(ch, C_SPR));
            }
            if( object->affectedBySpell(SPELL_ENVENOM) != NULL ) {
                acc_sprintf(" %s(venom)%s",
                                  CCGRN(ch, C_SPR), CCNRM(ch, C_SPR));
            }
            if( object->affectedBySpell(SPELL_ELEMENTAL_BRAND) != NULL ) {
                acc_sprintf(" %s(branded)%s",
                                  CCRED(ch, C_SPR), CCNRM(ch, C_SPR));
            }
        }
		if (mode == 0)
			acc_strcat(CCGRN(ch, C_NRM), NULL);
	}

	if (!((mode == 0) && !object->line_desc)) {
		if (count > 1)
			acc_sprintf(" [%d]", count);
		acc_strcat("\r\n", NULL);
	}

	page_string(ch->desc, acc_get_string());

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
list_obj_to_char(struct obj_data *list, struct Creature *ch, int mode,
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
				if (same_obj(o, i) &&
					(!IS_BOMB(o) || !IS_BOMB(i)
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
		send_to_char(ch, " Nothing.\r\n");

}

void
list_obj_to_char_GLANCE(struct obj_data *list, struct Creature *ch,
	struct Creature *vict, int mode, bool show, int glance)
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
						SKILL_GLANCE) >> ((glance) ? 2 : 0)) < (number(0,
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
				if (same_obj(o, i) &&
					(!IS_BOMB(o) || !IS_BOMB(i)
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
		send_to_char(ch, "You can't see anything.\r\n");
}

void
diag_char_to_char(struct Creature *i, struct Creature *ch)
{
	int percent;

	if (GET_MAX_HIT(i) > 0)
		percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
	else
		percent = -1;			/* How could MAX_HIT be < 1?? */

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
diag_conditions(struct Creature *ch)
{

    if (!ch) {
        strcpy(buf, "(NULL)");
        return buf;
    }
	int percent;
	if (GET_MAX_HIT(ch) > 0)
		percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
	else
		percent = -1;			/* How could MAX_HIT be < 1?? */

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

char *
desc_char_trailers(Creature *ch, Creature *i)
{
	acc_string_clear();

	if (affected_by_spell(i, SPELL_QUAD_DAMAGE))
		acc_strcat("...", HSSH(i),
			" is glowing with a bright blue light!\r\n", NULL);

	if (IS_AFFECTED_2(i, AFF2_ABLAZE))
		acc_strcat("...", HSHR(i),
			" body is blazing like a bonfire!\r\n", NULL);
	if (IS_AFFECTED(i, AFF_BLIND))
		acc_strcat("...", HSSH(i),
			" is groping around blindly!\r\n", NULL);

	if (IS_AFFECTED(i, AFF_SANCTUARY)) {
		if (IS_EVIL(i))
			acc_strcat("...", HSSH(i),
				" is surrounded by darkness!\r\n", NULL);
		else
			acc_strcat("...", HSSH(i),
				" glows with a bright light!\r\n", NULL);
	}
	
	if (IS_AFFECTED(i, AFF_CONFUSION))
		acc_strcat("...", HSSH(i),
			" is looking around in confusion!\r\n", NULL);
	if (IS_AFFECTED_3(i, AFF3_SYMBOL_OF_PAIN))
		acc_strcat("...a symbol of pain burns bright on ",
			HSHR(i), " forehead!\r\n", NULL);
	if (IS_AFFECTED(i, AFF_BLUR))
		acc_strcat("...", HSHR(i),
			" form appears to be blurry and shifting.\r\n", NULL);
	if (IS_AFFECTED_2(i, AFF2_FIRE_SHIELD))
		acc_strcat("...a blazing sheet of fire floats before ",
			HSHR(i), " body!\r\n", NULL);
	if (IS_AFFECTED_2(i, AFF2_BLADE_BARRIER))
		acc_strcat("...", HSSH(i),
			" is surrounded by whirling blades!\r\n", NULL);
	if (IS_AFFECTED_2(i, AFF2_ENERGY_FIELD))
		acc_strcat("...", HSSH(i),
			" is covered by a crackling field of energy!\r\n", NULL);

	if (IS_SOULLESS(i))
		acc_strcat("...a deep red pentagram has been burned into ",
			HSHR(i), " forehead!\r\n", NULL);
	else if (IS_AFFECTED_3(i, AFF3_TAINTED))
		acc_strcat("...the mark of the tainted has been burned into ",
			HSHR(i), " forehead!\r\n", NULL);

	if (IS_AFFECTED_3(i, AFF3_PRISMATIC_SPHERE))
		acc_strcat("...", HSSH(i),
			" is surrounded by a prismatic sphere of light!\r\n", NULL);

	if (affected_by_spell(i, SPELL_SKUNK_STENCH) ||
			affected_by_spell(i, SPELL_TROG_STENCH))
		acc_strcat("...", HSSH(i),
			" is followed by a malodorous stench...\r\n", NULL);

	if (IS_AFFECTED_2(i, AFF2_PETRIFIED))
		acc_strcat("...", HSSH(i),
			" is petrified into solid stone.\r\n", NULL);

	if (affected_by_spell(i, SPELL_ENTANGLE)) {
		if (i->in_room->sector_type == SECT_CITY
			|| i->in_room->sector_type == SECT_CRACKED_ROAD)
			acc_strcat("...", HSSH(i),
				" is hopelessly tangled in the weeds and sparse vegetation.\r\n", NULL);
		else
			acc_strcat("...", HSSH(i),
				" is hopelessly tangled in the undergrowth.\r\n", NULL);
	}

	if (IS_AFFECTED_2(i, AFF2_DISPLACEMENT) &&
			IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))
		acc_strcat("...the image of ", HSHR(i),
			" body is strangely displaced.\r\n", NULL);

	if (IS_AFFECTED(i, AFF_INVISIBLE))
		acc_strcat("...", HSSH(i),
			" is invisible to the unaided eye.\r\n", NULL);

	if (IS_AFFECTED_2(i, AFF2_TRANSPARENT))
		acc_strcat("...", HSHR(i),
			" body is completely transparent.\r\n", NULL);

	if (affected_by_spell(i, SKILL_KATA) &&
			i->getLevelBonus(SKILL_KATA) >= 50)
		acc_strcat("...", HSHR(i),
			" hands are glowing eerily.\r\n", NULL);

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
        acc_strcat("...", HSSH(i), " is surrounded by mirror images.\r\n", NULL);

	return acc_get_string();
}

void
look_at_char(struct Creature *i, struct Creature *ch, int cmd)
{
	int j, found = 0, app_height, app_weight, h, k, pos;
	char *description = NULL;
	struct affected_type *af = NULL;
	struct Creature *mob = NULL;

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
			act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

		app_height = GET_HEIGHT(i) - number(1, 6) + number(1, 6);
		app_weight = GET_WEIGHT(i) - number(1, 6) + number(1, 6);
		if (!IS_NPC(i) && !mob) {
			send_to_char(ch, "%s appears to be a %d cm tall, %d pound %s %s.\r\n",
				i->player.name, app_height, app_weight,
				genders[(int)GET_SEX(i)], player_race[(int)GET_RACE(i)]);
		}
	}

	diag_char_to_char(i, ch);

	if (CMD_IS("look"))
		send_to_char(ch, "%s", desc_char_trailers(ch, i));

	if (!CMD_IS("glance") && !IS_NPC(i)) {

		buf[0] = '\0';

		for (h = 0; h < NUM_WEARS; h++) {
			pos = (int)eq_pos_order[h];

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
			send_to_char(ch, "%s", buf);
	}

	if (CMD_IS("examine") || CMD_IS("glance")) {
		found = FALSE;
		for (j = 0; !found && j < NUM_WEARS; j++)
			if (GET_EQ(i, j) && can_see_object(ch, GET_EQ(i, j)))
				found = TRUE;

		if (found) {
			act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
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
					send_to_char(ch, CCGRN(ch, C_NRM));
					send_to_char(ch, where[(int)eq_pos_order[j]]);
					send_to_char(ch, CCNRM(ch, C_NRM));
					show_obj_to_char(GET_EQ(i, (int)eq_pos_order[j]), ch, 1,
						0);
				}
		}
		if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_AMBASSADOR)) {
			found = FALSE;
			act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch,
				TO_VICT);
			list_obj_to_char_GLANCE(i->carrying, ch, i, 1, TRUE,
				(GET_LEVEL(ch) >= LVL_AMBASSADOR));
		}
	}
}


char *
desc_one_char(Creature *ch, Creature *i, bool is_group)
{
	char *positions[] = {
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
	char *align = "";
	char *desc = "";
	char *appr = "";

	if (AFF2_FLAGGED(i, AFF2_MOUNTED))
		return "";

	if (!IS_NPC(ch) && MOB2_FLAGGED(i, MOB2_UNAPPROVED) &&
			!(PRF_FLAGGED(ch, PRF_HOLYLIGHT) || ch->isTester()))
		return "";

	if (IS_NPC(i)) {
		desc = tmp_capitalize(i->player.short_descr);
	} else if (affected_by_spell(i, SKILL_DISGUISE)) {
		desc = tmp_capitalize(GET_DISGUISED_NAME(ch, i));
	} else
		desc = tmp_strcat(i->player.name, GET_TITLE(i));

	if (!IS_NPC(i)) {
		if (!i->desc)
			desc = tmp_sprintf("%s %s(linkless)%s", desc,
				CCMAG(ch, C_NRM), CCYEL(ch, C_NRM));
		if (PLR_FLAGGED(i, PLR_WRITING))
			desc = tmp_sprintf("%s %s(writing)%s", desc,
				CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
		if (PLR_FLAGGED(i, PLR_OLC))
			desc = tmp_sprintf("%s %s(creating)%s", desc,
				CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
		if (PLR_FLAGGED(i, PLR_AFK))
			desc = tmp_sprintf("%s %s(afk)%s", desc,
				CCGRN(ch, C_NRM), CCYEL(ch, C_NRM));
	}


	if (IS_NPC(i) && i->getPosition() == GET_DEFAULT_POS(i)) {
		if (i->player.long_descr)
			desc = i->player.long_descr;
		else
			desc = tmp_strcat(tmp_capitalize(desc), " exists here.");
	} else if (i->getPosition() == POS_FIGHTING) {
		if (!i->numCombatants())
			desc = tmp_sprintf("%s is here, fighting thin air!", desc);
		else if (i->findRandomCombat() == ch)
			desc = tmp_sprintf("%s is here, fighting YOU!", desc);
		else if (i->findRandomCombat()->in_room == i->in_room)
			desc = tmp_sprintf("%s is here, fighting %s!", desc,
				PERS(i->findRandomCombat(), ch));
		else
			desc = tmp_sprintf("%s is here, fighting someone who already left!",
				desc);
	} else if (i->getPosition() == POS_MOUNTED) {
		if (!i->isMounted())
			desc = tmp_sprintf("%s is here, mounted on thin air!", desc);
		else if (i->isMounted() == ch)
			desc = tmp_sprintf("%s is here, mounted on YOU.  Heh heh...", desc);
		else if (i->isMounted()->in_room == i->in_room)
			desc = tmp_sprintf("%s is here, mounted on %s.", desc,
				PERS(i->isMounted(), ch));
		else
			desc = tmp_sprintf("%s is here, mounted on someone who already left!",
				desc);
	} else if (IS_AFFECTED_2(i, AFF2_MEDITATE) && i->getPosition() == POS_SITTING)
		desc = tmp_strcat(desc, " is meditating here.");
	else if (AFF_FLAGGED(i, AFF_HIDE))
		desc = tmp_strcat(desc, " is hiding here.");
	else if (AFF3_FLAGGED(i, AFF3_STASIS)
		&& i->getPosition() == POS_SLEEPING)
		desc = tmp_strcat(desc, " is lying here in a static state.");
	else if ((SECT_TYPE(i->in_room) == SECT_WATER_NOSWIM ||
			SECT_TYPE(i->in_room) == SECT_WATER_SWIM ||
			SECT_TYPE(i->in_room) == SECT_FIRE_RIVER) &&
		(!IS_AFFECTED(i, AFF_WATERWALK)
			|| ch->getPosition() < POS_STANDING))
		desc = tmp_strcat(desc, " is swimming here.");
	else if ((SECT_TYPE(i->in_room) == SECT_UNDERWATER
				|| SECT_TYPE(i->in_room) == SECT_DEEP_OCEAN)
			&& i->getPosition() > POS_RESTING)
		desc = tmp_strcat(desc, " is swimming here.");
	else if (SECT_TYPE(i->in_room) == SECT_PITCH_PIT
		&& i->getPosition() < POS_FLYING)
		desc = tmp_strcat(desc, " is struggling in the pitch.");
	else if (SECT_TYPE(i->in_room) == SECT_PITCH_SUB)
		desc = tmp_strcat(desc, " is struggling blindly in the pitch.");
	else
		desc = tmp_strcat(desc, positions[(int)MAX(0, MIN(i->getPosition(),
						POS_SWIMMING))]);

	if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) ||
		(IS_CLERIC(ch) && IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))) {
		if (IS_EVIL(i))
			align = tmp_sprintf(" %s%s(Red Aura)%s",
				CCRED(ch, C_NRM), CCBLD(ch, C_CMP), CCYEL(ch, C_NRM));
		else if (IS_GOOD(i))
			align = tmp_sprintf(" %s%s(Blue Aura)%s",
				CCBLU(ch, C_NRM), CCBLD(ch, C_CMP), CCYEL(ch, C_NRM));
	}

	if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
		if (IS_EVIL(i))
			align = tmp_sprintf(" %s%s(%da)%s", CCRED(ch, C_NRM),
				CCBLD(ch, C_CMP), GET_ALIGNMENT(i), CCYEL(ch, C_NRM));
		else if (IS_GOOD(i))
			align = tmp_sprintf(" %s%s(%da)%s", CCBLU(ch, C_NRM),
				CCBLD(ch, C_CMP), GET_ALIGNMENT(i), CCYEL(ch, C_NRM));
		else
			align = tmp_sprintf(" %s%s(%da)%s", CCNRM(ch, C_NRM),
				CCBLD(ch, C_CMP), GET_ALIGNMENT(i), CCYEL(ch, C_NRM));
	}
	// If they can see it, they probably need to know it's unapproved
	if (MOB2_FLAGGED(i, MOB2_UNAPPROVED))
		appr = tmp_sprintf(" %s(!appr)%s",
			CCRED(ch, C_NRM), CCYEL(ch, C_NRM));
		
	desc = tmp_strcat(CCYEL(ch, C_NRM), (is_group) ? CCBLD(ch, C_CMP):"",
		desc, align, appr, CCNRM(ch, C_NRM), "\r\n", NULL);

	return desc;
}

void
list_char_to_char(struct Creature *list, struct Creature *ch)
{
	struct Creature *i;
	bool is_group = false;
	char *msg = "", *desc;
	int unseen = 0;
	int hide_prob, hide_roll;

	if (list == NULL)
		return;

	CreatureList::iterator it = list->in_room->people.begin();
	for (; it != list->in_room->people.end(); ++it) {
		i = *it;
		is_group = false;
		if (ch == i)
			continue;
		
		if (ch->in_room != i->in_room && AFF_FLAGGED(i, AFF_HIDE | AFF_SNEAK))
			continue;

		if (room_is_dark(ch->in_room) && !has_dark_sight(ch) &&
			IS_AFFECTED(i, AFF_INFRAVISION)) {
			switch (number(0, 2)) {
			case 0:
				msg = tmp_strcat(msg,
					"You see a pair of glowing red eyes looking your way.\r\n");
				break;
			case 1:
				msg = tmp_strcat(msg, 
					"A pair of eyes glow red in the darkness.\r\n");
				break;
			case 2:
				unseen++; break;
			}

			continue;
		}

		// skip those with no ldesc
		if (!IS_IMMORT(ch) && IS_NPC(i) && !i->player.long_descr)
			continue;

		// skip those you can't see for in-game reasons
		if (!can_see_creature(ch, i)) {
			if (!IS_IMMORT(i))
				unseen++;
			continue;
		}

		if (AFF_FLAGGED(i, AFF_HIDE) && !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
			hide_prob = number(0, i->getLevelBonus(SKILL_HIDE));
			hide_roll = number(0, ch->getLevelBonus(true));
			if (affected_by_spell(ch, ZEN_AWARENESS))
				hide_roll += ch->getLevelBonus(ZEN_AWARENESS) / 4;

			if (hide_prob > hide_roll) {
				unseen++;
				continue;
			}

			if (can_see_creature(i, ch))
				send_to_char(i, "%s seems to have seen you.\r\n", GET_NAME(ch));
		}

		if (IS_AFFECTED(ch, AFF_GROUP) && IS_AFFECTED(i, AFF_GROUP)) {
			is_group =
				ch->master == i ||
				i->master == ch ||
				(ch->master && i->master == ch->master);
		}

		desc = desc_one_char(ch, i, is_group);
		if (*desc) {
			msg = tmp_strcat(msg, desc);
			if (!PRF2_FLAGGED(ch, PRF2_NOTRAILERS) && ch->in_room == i->in_room)
				msg = tmp_strcat(msg, desc_char_trailers(ch, i));
		}
	}

	if( unseen && 
		(AFF_FLAGGED(ch, AFF_SENSE_LIFE) || affected_by_spell(ch, SKILL_HYPERSCAN)) ) 
	{
		send_to_char(ch, "%s", CCMAG(ch, C_NRM));
		if (unseen == 1)
			send_to_char(ch, "You sense an unseen presence.\r\n");
		else if (unseen < 4)
			send_to_char(ch, "You sense a few unseen presences.\r\n");
		else if (unseen < 7)
			send_to_char(ch, "You sense many unseen presences.\r\n");
		else
			send_to_char(ch, "You sense a crowd of unseen presences.\r\n");
		send_to_char(ch, "%s", CCNRM(ch, C_NRM));
	}

	send_to_char(ch, "%s", msg);
}


void
do_auto_exits(struct Creature *ch, struct room_data *room)
{
	int door;

	buf[0] = '\0';

	if (room == NULL)
		room = ch->in_room;

	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!room->dir_option[door] || !room->dir_option[door]->to_room)
			continue;
		
		if (IS_SET(room->dir_option[door]->exit_info, EX_HIDDEN | EX_SECRET))
			continue;
		
		if (IS_SET(room->dir_option[door]->exit_info, EX_CLOSED))
			sprintf(buf, "%s|%c| ", buf, tolower(*dirs[door]));
		else
			sprintf(buf, "%s%c ", buf, tolower(*dirs[door]));
	}

	send_to_char(ch, "%s[ Exits: %s]%s   ", CCCYN(ch, C_NRM),
		*buf ? buf : "None obvious ", CCNRM(ch, C_NRM));


	if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		*buf = '\0';

		for (door = 0; door < NUM_OF_DIRS; door++)
			if (ABS_EXIT(room, door) && ABS_EXIT(room, door)->to_room != NULL
				&& IS_SET(ABS_EXIT(room, door)->exit_info,
					EX_SECRET | EX_HIDDEN))
				sprintf(buf, "%s%c ", buf, tolower(*dirs[door]));

		send_to_char(ch, "%s[ Hidden Doors: %s]%s\r\n", CCCYN(ch, C_NRM),
			*buf ? buf : "None ", CCNRM(ch, C_NRM));

	} else
		send_to_char(ch, "\r\n");
}

/* functions and macros for 'scan' command */
void
list_scanned_chars(struct Creature *list, struct Creature *ch,
	int distance, int door)
{
	const char *how_far[] = {
		"close by",
		"a ways off",
		"far off to the"
	};

	int count = 0;
	*buf = '\0';
	if (list == NULL)
		return;
	/* this loop is a quick, easy way to help make a grammatical sentence
	   (i.e., "You see x, x, y, and z." with commas, "and", etc.) */
	CreatureList::iterator it = list->in_room->people.begin();
	for (; it != list->in_room->people.end(); ++it) {

		/* put any other conditions for scanning someone in this if statement -
		   i.e., if (can_see_creature(ch, i) && condition2 && condition3) or whatever */

		if (can_see_creature(ch, (*it)) && ch != (*it) &&
			(!AFF_FLAGGED((*it), AFF_SNEAK | AFF_HIDE) ||
				PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
			count++;
	}

	if (!count)
		return;

	it = list->in_room->people.begin();
	for (; it != list->in_room->people.end(); ++it) {
		/* make sure to add changes to the if statement above to this one also, using
		   or's to join them.. i.e., 
		   if (!can_see_creature(ch, i) || !condition2 || !condition3) */

		if (!can_see_creature(ch, (*it)) || ch == (*it) ||
			((AFF_FLAGGED((*it), AFF_SNEAK | AFF_HIDE)) &&
				!PRF_FLAGGED(ch, PRF_HOLYLIGHT)))
			continue;
		if (!*buf)
			sprintf(buf, "You see %s%s%s",
				CCYEL(ch, C_NRM), GET_DISGUISED_NAME(ch, (*it)), CCNRM(ch,
					C_NRM));
		else
			sprintf(buf, "%s%s%s%s",
				buf, CCYEL(ch, C_NRM), GET_DISGUISED_NAME(ch, (*it)),
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
	send_to_char(ch, "%s", buf);
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
	act("$n quickly scans $s surroundings.", TRUE, ch, 0, 0, TO_ROOM);

	for (door = 0; door < NUM_OF_DIRS - 4; door++)	/* don't scan up/down */
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
			send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
			return;
		}

		if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
				!AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
			send_to_char(ch,
				"The thick smoke is too disorienting to tell.\r\n");
			return;
		}
	}

	for (door = 0; door < NUM_OF_DIRS; door++) {
		if (!EXIT(ch, door) || !EXIT(ch, door)->to_room)
			continue;

		if (!IS_IMMORT(ch) &&
				IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN | EX_SECRET))
			continue;

		if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
			send_to_char(ch, "%s%s%-8s%s - %s[%s%5d%s] %s%s%s%s%s%s%s\r\n",
				CCBLD(ch, C_SPR), CCBLU(ch, C_NRM), tmp_capitalize(dirs[door]),
				CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
				EXIT(ch, door)->to_room->number, CCGRN(ch, C_NRM),
				CCCYN(ch, C_NRM), EXIT(ch, door)->to_room->name,
				CCNRM(ch, C_SPR),
				IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ?
				" (closed)" : "",
				IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN) ?
				" (hidden)" : "",
				IS_SET(EXIT(ch, door)->exit_info, EX_SECRET) ?
				" (secret)" : "",
				IS_SET(EXIT(ch, door)->exit_info, EX_NOPASS) ?
				" (nopass)" : "");
			continue;
		}

		if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
			send_to_char(ch, "%s%s%-8s%s - Closed %s\r\n",
				CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
				tmp_capitalize(dirs[door]),
				(EXIT(ch, door)->keyword) ? fname(EXIT(ch, door)->keyword):"TYPO ME>",
				CCNRM(ch, C_SPR));
		} else if ((IS_AFFECTED(ch, AFF_BLIND) ||
				ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED)) &&
				AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
			send_to_char(ch, "%s%s%-8s%s - Out of range\r\n",
				CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
				tmp_capitalize(dirs[door]), CCNRM(ch, C_SPR));
		} else if (!check_sight_room(ch, EXIT(ch, door)->to_room) &&
				!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH)) {
			send_to_char(ch, "%s%s%-8s%s - Too dark to tell\r\n",
				CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
				tmp_capitalize(dirs[door]), CCNRM(ch, C_SPR));
		} else {
			send_to_char(ch, "%s%s%-8s%s - %s%s%s\r\n",
				CCBLD(ch, C_SPR), CCBLU(ch, C_NRM),
				tmp_capitalize(dirs[door]), CCNRM(ch, C_SPR),
				CCCYN(ch, C_NRM),
				(EXIT(ch, door)->to_room->name) ?
					(EXIT(ch, door)->to_room->name):"<TYPO ME>",
				CCNRM(ch, C_SPR));
		}
	}
}



void
look_at_room(struct Creature *ch, struct room_data *room, int ignore_brief)
{

	struct room_affect_data *aff = NULL;
	struct obj_data *o = NULL;

	int ice_shown = 0;			// 1 if ice has already been shown to room...same for blood
	int blood_shown = 0;

	if (room == NULL)
		room = ch->in_room;

	if (!ch->desc)
		return;

	if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
		send_to_char(ch, "It is pitch black...\r\n");
		return;
	}

	send_to_char(ch, CCCYN(ch, C_NRM));
	if (PRF_FLAGGED(ch, PRF_ROOMFLAGS) ||
		(ch->desc->original
			&& PRF_FLAGGED(ch->desc->original, PRF_ROOMFLAGS))) {
		sprintbit((long)ROOM_FLAGS(room), room_bits, buf);
		send_to_char(ch, "[%5d] %s [ %s] [ %s ]", room->number,
			room->name, buf, sector_types[room->sector_type]);
	} else {
		send_to_char(ch, room->name);
	}

	send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));

	if ((!PRF_FLAGGED(ch, PRF_BRIEF) &&
		(!ch->desc->original || !PRF_FLAGGED(ch->desc->original, PRF_BRIEF)))
		|| ignore_brief || ROOM_FLAGGED(room, ROOM_DEATH)) {
		// We need to show them something...
		if (ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) &&
				!(PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
				ROOM_FLAGGED(room, ROOM_DEATH)) && 
                !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
			send_to_char(ch, "The smoke swirls around you...\r\n");
		else if (room->description)
			send_to_char(ch, "%s", room->description);
	}


	for (aff = room->affects; aff; aff = aff->next)
		if (aff->description)
			send_to_char(ch, aff->description);

	if ((GET_LEVEL(ch) >= LVL_AMBASSADOR ||
			!ROOM_FLAGGED(room, ROOM_SMOKE_FILLED) ||
			AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))) {

		/* autoexits */
		if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
			do_auto_exits(ch, room);
		/* now list characters & objects */

		for (o = room->contents; o; o = o->next_content) {
			if (GET_OBJ_VNUM(o) == BLOOD_VNUM) {
				if (!blood_shown) {
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
						"Blood is pooled and splattered all over everything here"
						: "Dark red blood covers everything in sight here",
						CCNRM(ch, C_NRM));
					blood_shown = 1;
					send_to_char(ch, "%s", buf);
				}
			}

			if (GET_OBJ_VNUM(o) == ICE_VNUM) {
				if (!ice_shown) {
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
					send_to_char(ch, "%s", buf);
					break;
				}
			}
		}

		send_to_char(ch, CCGRN(ch, C_NRM));
		list_obj_to_char(room->contents, ch, 0, FALSE);
		send_to_char(ch, CCYEL(ch, C_NRM));
		list_char_to_char(room->people, ch);
		send_to_char(ch, CCNRM(ch, C_NRM));
	}
}

void
look_in_direction(struct Creature *ch, int dir)
{
#define EXNUMB EXIT(ch, dir)->to_room
	struct room_affect_data *aff;

	if (ROOM_FLAGGED(ch->in_room, ROOM_SMOKE_FILLED) &&
		GET_LEVEL(ch) < LVL_AMBASSADOR) {
		send_to_char(ch, "The thick smoke limits your vision.\r\n");
		return;
	}
	if (EXIT(ch, dir)) {
		if (EXIT(ch, dir)->general_description) {
			send_to_char(ch, EXIT(ch, dir)->general_description);
		} else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR | EX_CLOSED) &&
			EXIT(ch, dir)->keyword) {
			send_to_char(ch, "You see %s %s.\r\n", AN(fname(EXIT(ch,
							dir)->keyword)), fname(EXIT(ch, dir)->keyword));
		} else if (EXNUMB) {
			if ((IS_SET(EXIT(ch, dir)->exit_info, EX_NOPASS) &&
					GET_LEVEL(ch) < LVL_AMBASSADOR) ||
				IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN))
				send_to_char(ch, "You see nothing special.\r\n");

			else if (EXNUMB->name) {
				send_to_char(ch, CCCYN(ch, C_NRM));
				if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
					sprintbit((long)ROOM_FLAGS(EXNUMB), room_bits, buf);
					send_to_char(ch, "[%5d] %s [ %s] [ %s ]", EXNUMB->number,
						EXNUMB->name, buf, sector_types[EXNUMB->sector_type]);
				} else
					send_to_char(ch, EXNUMB->name);
				send_to_char(ch, CCNRM(ch, C_NRM));
				send_to_char(ch, "\r\n");
				for (aff = ch->in_room->affects; aff; aff = aff->next)
					if (aff->type == dir && aff->description)
						send_to_char(ch, aff->description);
			} else
				send_to_char(ch, "You see nothing special.\r\n");
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
				send_to_char(ch, CCGRN(ch, C_NRM));
				list_obj_to_char(EXNUMB->contents, ch, 0, FALSE);
				send_to_char(ch, CCYEL(ch, C_NRM));
				list_char_to_char(EXNUMB->people, ch);
				send_to_char(ch, CCNRM(ch, C_NRM));
			}
		}
		//      send_to_char("You see nothing special.\r\n", ch);


		if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN | EX_NOPASS)) {
			if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)
				&& EXIT(ch, dir)->keyword) {
				send_to_char(ch, "The %s %s closed.\r\n", fname(EXIT(ch,
							dir)->keyword), ISARE(fname(EXIT(ch,
								dir)->keyword)));
			} else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) &&
				EXIT(ch, dir)->keyword) {
				send_to_char(ch, "The %s %s open.\r\n", fname(EXIT(ch,
							dir)->keyword), ISARE(fname(EXIT(ch,
								dir)->keyword)));
				if (EXNUMB != NULL && room_is_dark(EXNUMB) &&
					!EXIT(ch, dir)->general_description) {
					sprintf(buf,
						"It's too dark through the %s to see anything.\r\n",
						fname(EXIT(ch, dir)->keyword));
					send_to_char(ch, "%s", buf);
				}
			}
		}

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
			} else if ((ch->in_room->sector_type == SECT_UNDERWATER))
				send_to_char(ch, "You see water above you.\r\n");
			else if ((ch->in_room->sector_type == SECT_DEEP_OCEAN))
				send_to_char(ch, "You see dark waters above you.\r\n");
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
			else if ((ch->in_room->sector_type == SECT_WATER_SWIM) ||
				(ch->in_room->sector_type == SECT_WATER_NOSWIM))
				send_to_char(ch, "You see the water below you.\r\n");
			else if (ch->in_room->isOpenAir()) {
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
			else if (ch->in_room->sector_type == SECT_SWAMP)
				send_to_char(ch, "You see the swampy ground below your feet.\r\n");
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


void
look_in_obj(struct Creature *ch, char *arg)
{
	struct obj_data *obj = NULL;
	struct Creature *dummy = NULL;
	int amt, bits;
	struct room_data *room_was_in = NULL;

	if (!*arg)
		send_to_char(ch, "Look in what?\r\n");
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
		send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
	} else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
		(GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
		(GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
		(GET_OBJ_TYPE(obj) != ITEM_PIPE) &&
		(GET_OBJ_TYPE(obj) != ITEM_VEHICLE))
		send_to_char(ch, "There's nothing inside that!\r\n");
	else {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
			if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED) &&
				!GET_OBJ_VAL(obj, 3) && GET_LEVEL(ch) < LVL_GOD)
				send_to_char(ch, "It is closed.\r\n");
			else {
				send_to_char(ch, obj->name);
				switch (bits) {
				case FIND_OBJ_INV:
					send_to_char(ch, " (carried): \r\n");
					break;
				case FIND_OBJ_ROOM:
					send_to_char(ch, " (here): \r\n");
					break;
				case FIND_OBJ_EQUIP:
				case FIND_OBJ_EQUIP_CONT:
					send_to_char(ch, " (used): \r\n");
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
				char_from_room(ch,false);
				char_to_room(ch, real_room(ROOM_NUMBER(obj)),false);
				list_char_to_char(ch->in_room->people, ch);
				act("$n looks in from the outside.", FALSE, ch, 0, 0, TO_ROOM);
				char_from_room(ch,false);
				char_to_room(ch, room_was_in,false);
			}
		} else if (GET_OBJ_TYPE(obj) == ITEM_PIPE) {
			if (GET_OBJ_VAL(obj, 0))
				send_to_char(ch, "There appears to be some tobacco in it.\r\n");
			else
				send_to_char(ch, "There is nothing in it.\r\n");

		} else {				/* item must be a fountain or drink container */
			if (GET_OBJ_VAL(obj, 1) == 0)
				send_to_char(ch, "It is empty.\r\n");
			else {
				if (GET_OBJ_VAL(obj, 1) < 0)
					amt = 3;
				else
					amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
				send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt],
					color_liquid[GET_OBJ_VAL(obj, 2)]);
			}
		}
	}
}

char *
find_exdesc(char *word, struct extra_descr_data *list, int find_exact = 0)
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
look_at_target(struct Creature *ch, char *arg, int cmd)
{
	int bits, found = 0, j;
	struct Creature *found_char = NULL;
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
					find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) {
				page_string(ch->desc, desc);
				found = 1;
				bits = 1;
				found_obj = GET_EQ(ch, j);
			}
	/* Does the argument match an extra desc in the char's inventory? */
	for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
		if (can_see_object(ch, obj) &&
			(GET_OBJ_VAL(obj, 3) != -999 || isname(arg, obj->aliases)))
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
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
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
				page_string(ch->desc, desc);
				found = bits = 1;
				found_obj = obj;
				break;
			}

	if (bits) {					/* If an object was found back in
								   * generic_find */
		if (found_obj) {
			if (IS_OBJ_TYPE(found_obj, ITEM_WINDOW)) {
				if (!CAR_CLOSED(found_obj)) {
					if (real_room(GET_OBJ_VAL(found_obj, 0)) != NULL)
						look_at_room(ch, real_room(GET_OBJ_VAL(found_obj, 0)),
							1);
				} else
					act("$p is closed right now.", FALSE, ch, found_obj, 0,
						TO_CHAR);
				found = 1;
			} else if (IS_V_WINDOW(found_obj)) {

				for (car = object_list; car; car = car->next)
					if (GET_OBJ_VNUM(car) == V_CAR_VNUM(found_obj) &&
						ROOM_NUMBER(car) == ROOM_NUMBER(found_obj) &&
						car->in_room)
						break;

				if (car) {
					act("You look through $p.", FALSE, ch, found_obj, 0,
						TO_CHAR);
					look_at_room(ch, car->in_room, 1);
					found = 1;
				}
			} else if (!found)
				show_obj_to_char(found_obj, ch, 5, 0);	/* Show no-description */
			else
				show_obj_to_char(found_obj, ch, 6, 0);	/* Find hum, glow etc */
		}

	} else if (!found)
		send_to_char(ch, "You do not see that here.\r\n");
}

void
glance_at_target(struct Creature *ch, char *arg, int cmd)
{
	int bits;
	struct Creature *found_char = NULL;
	struct obj_data *found_obj = NULL;
	char glancebuf[512];

	if (!*arg) {
		send_to_char(ch, "Look at what?\r\n");
		return;
	}
	bits = generic_find(arg, FIND_CHAR_ROOM, ch, &found_char, &found_obj);

	/* Is the target a character? */
	if (found_char != NULL) {
		look_at_char(found_char, ch, cmd);	/** CMD_IS("glance") !! **/
		if (ch != found_char) {
			if (can_see_creature(found_char, ch) &&
				((GET_SKILL(ch, SKILL_GLANCE) + GET_LEVEL(ch)) <
					(number(0, 101) + GET_LEVEL(found_char)))) {
				act("$n glances sidelong at you.", TRUE, ch, 0, found_char,
					TO_VICT);
				act("$n glances sidelong at $N.", TRUE, ch, 0, found_char,
					TO_NOTVICT);

				if (IS_NPC(found_char) && !(found_char->numCombatants())
					&& AWAKE(found_char) && (!found_char->master
						|| found_char->master != ch)) {
					if (IS_ANIMAL(found_char) || IS_BUGBEAR(found_char)
						|| GET_INT(found_char) < number(3, 5)) {
						act("$N growls at you.", FALSE, ch, 0, found_char,
							TO_CHAR);
						act("$N growls at $n.", FALSE, ch, 0, found_char,
							TO_NOTVICT);
					} else if (IS_UNDEAD(found_char)) {
						act("$N regards you with an icy glare.",
							FALSE, ch, 0, found_char, TO_CHAR);
						act("$N regards $n with an icy glare.",
							FALSE, ch, 0, found_char, TO_NOTVICT);
					} else if (IS_MINOTAUR(found_char) || IS_DEVIL(found_char)
						|| IS_DEMON(found_char) || IS_MANTICORE(found_char)) {
						act("$N roars at you.", FALSE, ch, 0, found_char,
							TO_CHAR);
						act("$N roars at $n.", FALSE, ch, 0, found_char,
							TO_NOTVICT);
					} else if (!number(0, 4)) {
						sprintf(glancebuf, "Piss off, %s.",
							GET_DISGUISED_NAME(found_char, ch));
						do_say(found_char, glancebuf, 0, 0, 0);
					} else if (!number(0, 3)) {
						sprintf(glancebuf, "Take a hike, %s.",
							GET_DISGUISED_NAME(found_char, ch));
						do_say(found_char, glancebuf, 0, 0, 0);
					} else if (!number(0, 2)) {
						sprintf(glancebuf, "%s You lookin' at me?",
							GET_DISGUISED_NAME(found_char, ch));
						do_say(found_char, glancebuf, 0, SCMD_SAY_TO, 0);
					} else if (!number(0, 1)) {
						sprintf(glancebuf, "Hit the road, %s.",
							GET_DISGUISED_NAME(found_char, ch));
						do_say(found_char, glancebuf, 0, 0, 0);
					} else {
						sprintf(glancebuf, "Get lost, %s.",
							GET_DISGUISED_NAME(found_char, ch));
						do_say(found_char, glancebuf, 0, 0, 0);
					}

					if (MOB_FLAGGED(found_char, MOB_AGGRESSIVE) &&
						(GET_MORALE(found_char) >
							number(GET_LEVEL(ch) >> 1, GET_LEVEL(ch))) &&
						!PRF_FLAGGED(ch, PRF_NOHASSLE)) {
						if (found_char->getPosition() >= POS_SITTING) {
							if (peaceful_room_ok(found_char, ch, false))
								hit(found_char, ch, TYPE_UNDEFINED);
						} else
							do_stand(found_char, "", 0, 0, 0);
					}
				}
			} else
				gain_skill_prof(ch, SKILL_GLANCE);
		}
	} else
		send_to_char(ch, "No-one around by that name.\r\n");
	return;
}

ACMD(do_listen)
{
	struct Creature *fighting_vict = NULL;
	struct obj_data *noisy_obj = NULL;
	int i;

	if (ch->in_room->sounds) {
		send_to_char(ch, ch->in_room->sounds);
		return;
	}
	CreatureList::iterator it = ch->in_room->people.begin();
	for (; it != ch->in_room->people.end(); ++it) {
		if ((*it)->numCombatants()) {
			fighting_vict = *it;
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
				send_to_char(ch, "You hear the sounds of a bustling city.\r\n");
		}
		break;
	case SECT_FOREST:
		if (ch->in_room->zone->weather->sky == SKY_RAINING)
			send_to_char(ch, "You hear the rain on the leaves of the trees.\r\n");
		else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
			send_to_char(ch, "You hear peals of thunder in the distance.\r\n");
		else
			send_to_char(ch, "You hear the breeze rustling through the trees.\r\n");
		break;
	default:
		if (OUTSIDE(ch)) {
			if (ch->in_room->zone->weather->sky == SKY_RAINING)
				send_to_char(ch, "You hear the falling rain all around you.\r\n");
			else if (ch->in_room->zone->weather->sky == SKY_LIGHTNING)
				send_to_char(ch, "You hear peals of thunder in the distance.\r\n");
			else if (fighting_vict)
				send_to_char(ch, "You hear the sounds of battle.\r\n");
			else if (noisy_obj)
				act("You hear a low humming coming from $p.",
					FALSE, ch, noisy_obj, 0, TO_CHAR);
			else
				send_to_char(ch, "You hear nothing special.\r\n");
		} else if (fighting_vict)
			send_to_char(ch, "You hear the sounds of battle.\r\n");
		else if (noisy_obj)
			act("You hear a low humming coming from $p.",
				FALSE, ch, noisy_obj, 0, TO_CHAR);
		else {
			for (i = 0; i < NUM_DIRS; i++) {
				if (ch->in_room->dir_option[i] &&
					ch->in_room->dir_option[i]->to_room &&
					ch->in_room->dir_option[i]->to_room != ch->in_room &&
					!IS_SET(ch->in_room->dir_option[i]->exit_info,
						EX_CLOSED)) {

					CreatureList::iterator end =
						ch->in_room->dir_option[i]->to_room->people.end();
					CreatureList::iterator it =
						ch->in_room->dir_option[i]->to_room->people.begin();
					for (; it != end; ++it) {
						fighting_vict = *it;
						if ((fighting_vict->numCombatants()))
							break;
					}
					if (fighting_vict && !number(0, 1)) {
						send_to_char(ch, "You hear sounds of battle from %s.\r\n",
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
	static char arg2[MAX_INPUT_LENGTH];
	int look_type;

	if (!ch->desc)
		return;

	if (ch->getPosition() < POS_SLEEPING)
		send_to_char(ch, "You can't see anything but stars!\r\n");
	else if (!check_sight_self(ch))
		send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
	else if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
		send_to_char(ch, "It is pitch black...\r\n");
		list_char_to_char(ch->in_room->people, ch);	// glowing red eyes
	} else {
		half_chop(argument, arg, arg2);

		if (subcmd == SCMD_READ) {
			if (!*arg)
				send_to_char(ch, "Read what?\r\n");
			else
				look_at_target(ch, arg, cmd);
			return;
		}
		if (!*arg)				/* "look" alone, without an argument at all */
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
		send_to_char(ch, "You can't see anything but stars!\r\n");
	else if (IS_AFFECTED(ch, AFF_BLIND)
		&& !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY))
		send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
	else if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
		send_to_char(ch, "It is pitch black...\r\n");
		list_char_to_char(ch->in_room->people, ch);	/* glowing red eyes */
	} else {
		half_chop(argument, arg, arg2);

		if (!*arg)				/* "look" alone, without an argument at all */
			send_to_char(ch, "Glance at who?\r\n");
		else if (is_abbrev(arg, "at"))
			glance_at_target(ch, arg2, cmd);
		else
			glance_at_target(ch, arg, cmd);
	}
}

ACMD(do_examine)
{
	int bits;
	struct Creature *tmp_char;
	struct obj_data *tmp_object;

	if (ch->getPosition() < POS_SLEEPING) {
		send_to_char(ch, "You can't see anything but stars!\r\n");
		return;
	}
	if (IS_AFFECTED(ch, AFF_BLIND)
		&& !AFF3_FLAGGED(ch, AFF3_SONIC_IMAGERY)) {
		send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
		return;
	}

	if (room_is_dark(ch->in_room) && !has_dark_sight(ch)) {
		send_to_char(ch, "It is pitch black, and you can't see anything.\r\n");
		return;
	}

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Examine what?\r\n");
		return;
	}
	look_at_target(ch, arg, cmd);

	bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

	if (tmp_object) {
		if (OBJ_REINFORCED(tmp_object))
			act("$p appears to be structurally reinforced.",
				FALSE, ch, tmp_object, 0, TO_CHAR);
		if (OBJ_ENHANCED(tmp_object))
			act("$p looks like it has been enhanced.",
				FALSE, ch, tmp_object, 0, TO_CHAR);

		
		sprintf(buf, "$p seems to be in %s condition.", 
                obj_cond_color(tmp_object, ch) );
		act(buf, FALSE, ch, tmp_object, 0, TO_CHAR);

		if (IS_OBJ_TYPE(tmp_object, ITEM_CIGARETTE)) {
			send_to_char(ch, "It seems to have about %d drags left on it.\r\n",
				GET_OBJ_VAL(tmp_object, 0));
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
	int qp;

	if (IS_IMMORT(ch)) {
		qp = GET_IMMORT_QP(ch);
		send_to_char(ch, "You have %d quest point%s to award.\r\n", qp,
			(qp == 1) ? "" : "s");
	} else {
		qp = ch->account->get_quest_points();
		send_to_char(ch, "You have %d quest point%s.\r\n", qp,
			(qp == 1) ? "" : "s");
	}
}

ACMD(do_gold)
{
	if (GET_GOLD(ch) == 0)
		send_to_char(ch, "You're broke!\r\n");
	else if (GET_GOLD(ch) == 1)
		send_to_char(ch, "You have one miserable little gold coin.\r\n");
	else {
		send_to_char(ch, "You have %d gold coins.\r\n", GET_GOLD(ch));
	}
}

ACMD(do_cash)
{
	if (GET_CASH(ch) == 0)
		send_to_char(ch, "You're broke!\r\n");
	else if (GET_CASH(ch) == 1)
		send_to_char(ch, "You have one almighty credit.\r\n");
	else {
		send_to_char(ch, "You have %d credits.\r\n", GET_CASH(ch));
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
		if (IS_CARRYING_W(ch) + IS_WEARING_W(ch) == 1) {
			send_to_char(ch, "Your stuff weighs a total of one pound.\r\n");
			return;
		} else {
			send_to_char(ch, "You are carrying a total %d pounds of stuff.\r\n",
				IS_WEARING_W(ch) + IS_CARRYING_W(ch));
			send_to_char(ch, "%d of which you are wearing or equipped with.\r\n",
				IS_WEARING_W(ch));
		}
	}
	if (encumbr > 3)
		send_to_char(ch, "You are heavily encumbered.\r\n");
	else if (encumbr == 3)
		send_to_char(ch, "You are moderately encumbered.\r\n");
	else if (encumbr == 2)
		send_to_char(ch, "You are lightly encumbered.\r\n");

}


void
acc_append_affects(struct Creature *ch, byte mode)
{

	struct affected_type *af = NULL;
	struct Creature *mob = NULL;
	const char *name = NULL;
	char *str = "";

    if (affected_by_spell(ch, SPELL_FIRE_BREATHING)) {
		acc_strcat("You are empowered with breath of FIRE!\r\n", NULL);
    } else if (affected_by_spell(ch, SPELL_FROST_BREATHING)) {
		acc_strcat("You are empowered with breath of FROST!\r\n", NULL);
    }
	if (IS_AFFECTED(ch, AFF_BLIND))
		acc_strcat("You have been blinded!\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_POISON))
		acc_strcat("You are poisoned!\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PETRIFIED))
		acc_strcat("You have been turned to stone.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_RADIOACTIVE))
		acc_strcat("You are radioactive.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_GAMMA_RAY))
		acc_strcat("You have been irradiated.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_ABLAZE))
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
			acc_strcat("You feel sick and your hair is falling out.\r\n", NULL);
	}

	if (AFF2_FLAGGED(ch, AFF2_SLOW))
		acc_strcat("You feel unnaturally slowed.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_CHARM))
		acc_strcat("You have been charmed!\r\n", NULL);
    if (IS_AFFECTED_3(ch, AFF3_MANA_LEAK) && !IS_AFFECTED_3(ch, AFF3_MANA_TAP))
        acc_strcat(str, "You are slowly being drained of your spiritual energy.\r\n", NULL);
    if (IS_AFFECTED_3(ch, AFF3_ENERGY_LEAK) && !IS_AFFECTED_3(ch, AFF3_ENERGY_TAP))
        acc_strcat(str, "Your body is slowly being drained of physical energy.\r\n", NULL);

	if (IS_AFFECTED_3(ch, AFF3_SYMBOL_OF_PAIN))
		acc_strcat("Your mind burns with the symbol of pain!\r\n", NULL);
	if (affected_by_spell(ch, SPELL_WEAKNESS))
		acc_strcat("You feel unusually weakened.\r\n", NULL);
	if (AFF3_FLAGGED(ch, AFF3_PSYCHIC_CRUSH))
		acc_strcat("You feel a psychic force crushing your mind!\r\n", NULL);
	if (affected_by_spell(ch, SPELL_FEAR))
		acc_strcat("The world is a terribly frightening place!\r\n", NULL);
	if (AFF3_FLAGGED(ch, AFF3_ACIDITY))
		acc_strcat("Your body is producing self-corroding acids!\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL))
		acc_strcat(
			"Spacetime is bent around you in a powerful gravity well!\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_HAMSTRUNG))
		acc_sprintf(
			"%sThe gash on your leg is %sBLEEDING%s%s all over!!%s\r\n",
			CCRED(ch, C_SPR), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCRED(ch,
				C_SPR), CCNRM(ch, C_SPR));
	if (IS_SICK(ch))
		acc_strcat("You are afflicted with a terrible sickness!\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_GRAVITY_WELL))
		acc_strcat(
			"Spacetime is bent around you in a powerful gravity well!\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_HAMSTRUNG))
		acc_sprintf(
			"%sThe gash on your leg is %sBLEEDING%s%s all over!!%s\r\n",
			CCRED(ch, C_SPR), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR), CCRED(ch,
				C_SPR), CCNRM(ch, C_SPR));
	if (IS_AFFECTED(ch, AFF_CONFUSION))
		acc_strcat("You are very confused.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_MOTOR_SPASM))
		acc_strcat("Your muscles are spasming uncontrollably!\r\n", NULL);
	if (AFF2_FLAGGED(ch, AFF2_VERTIGO))
		acc_strcat("You are lost in a sea of vertigo.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_TAINTED))
		acc_strcat("The very essence of your being has been tainted.\r\n", NULL);
	if (mode)					/* Only asked for bad affs? */
		return;
	if (IS_SOULLESS(ch))
		acc_strcat("A deep despair clouds your soulless mind.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED_3(ch, AFF3_INFILTRATE))
		acc_strcat("You are sneaking.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_INFILTRATE))
		acc_strcat("You are infiltrating.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_INVISIBLE))
		acc_strcat("You are invisible.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_TRANSPARENT))
		acc_strcat("You are transparent.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
		acc_strcat(str,
			"You are sensitive to the presence of invisible things.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_TRUE_SEEING))
		acc_strcat("You are seeing truly.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_SANCTUARY))
		acc_strcat("You are protected by Sanctuary.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_ARMOR))
		acc_strcat("You feel protected.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_BARKSKIN))
		acc_strcat("Your skin is thick and tough like tree bark.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_STONESKIN))
		acc_strcat("Your skin is as hard as granite.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_INFRAVISION))
		acc_strcat("Your eyes are glowing red.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_REJUV))
		acc_strcat("You feel like your body will heal with a good rest.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_REGEN))
		acc_strcat("Your body is regenerating itself rapidly.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_GLOWLIGHT))
		acc_strcat("You are followed by a ghostly illumination.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_BLUR))
		acc_strcat("Your image is blurred and shifting.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_DISPLACEMENT)) {
		if (affected_by_spell(ch, SPELL_REFRACTION))
			acc_strcat("Your body is irregularly refractive.\r\n", NULL);
		else
			acc_strcat("Your image is displaced.\r\n", NULL);
	}
	if (affected_by_spell(ch, SPELL_ELECTROSTATIC_FIELD))
		acc_strcat("You are surrounded by an electrostatic field.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_FIRE_SHIELD))
		acc_strcat("You are protected by a shield of fire.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_BLADE_BARRIER))
		acc_strcat("You are protected by whirling blades.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_ENERGY_FIELD))
		acc_strcat("You are surrounded by a field of energy.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_PRISMATIC_SPHERE))
		acc_strcat("You are surrounded by a prismatic sphere of light.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_FLUORESCENT))
		acc_strcat("The atoms in your vicinity are fluorescent.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_DIVINE_ILLUMINATION)) {
		if (IS_EVIL(ch))
			acc_strcat("An unholy light is following you.\r\n", NULL);
		else if (IS_GOOD(ch))
			acc_strcat("A holy light is following you.\r\n", NULL);
		else
			acc_strcat("A sickly light is following you.\r\n", NULL);
	}
	if (IS_AFFECTED_2(ch, AFF2_BERSERK))
		acc_strcat("You are BERSERK!\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_PROTECT_GOOD))
		acc_strcat("You are protected from good.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_PROTECT_EVIL))
		acc_strcat("You are protected from evil.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROT_DEVILS))
		acc_strcat("You are protected from devils.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROT_DEMONS))
		acc_strcat("You are protected from demons.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROTECT_UNDEAD))
		acc_strcat("You are protected from the undead.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROT_LIGHTNING))
		acc_strcat("You are protected from lightning.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROT_FIRE))
		acc_strcat("You are protected from fire.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_MAGICAL_PROT))
		acc_strcat("You are protected against magic.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_ENDURE_COLD))
		acc_strcat("You can endure extreme cold.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
		acc_strcat(str,
			"You are sensitive to the presence of living creatures\r\n", NULL);
	if (affected_by_spell(ch, SKILL_EMPOWER))
		acc_strcat("You are empowered.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_TELEKINESIS))
		acc_strcat("You are feeling telekinetic.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_MELEE_COMBAT_TAC))
		acc_strcat("Melee Combat Tactics are in effect.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_REFLEX_BOOST))
		acc_strcat("Your Reflex Boosters are active.\r\n", NULL);
	else if (IS_AFFECTED_2(ch, AFF2_HASTE))
		acc_strcat("You are moving very fast.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_KATA))
		acc_strcat("You feel focused from your kata.\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_OBLIVITY))
		acc_strcat("You are oblivious to pain.\r\n", NULL);
	if (affected_by_spell(ch, ZEN_MOTION))
		acc_strcat("The zen of motion is one with your body.\r\n", NULL);
	if (affected_by_spell(ch, ZEN_TRANSLOCATION))
		acc_strcat("You are as one with the zen of translocation.\r\n", NULL);
	if (affected_by_spell(ch, ZEN_CELERITY))
		acc_strcat("You are under the effects of the zen of celerity.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_MANA_TAP) && !IS_AFFECTED_3(ch, AFF3_MANA_LEAK))
		acc_strcat(str,
			"You have a direct tap to the spiritual energies of the universe.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_ENERGY_TAP) && !IS_AFFECTED_3(ch, AFF3_ENERGY_LEAK))
		acc_strcat(str,
			"Your body is absorbing physical energy from the universe.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_SONIC_IMAGERY))
		acc_strcat("You are perceiving sonic images.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_PROT_HEAT))
		acc_strcat("You are protected from heat.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_RIGHTEOUS_PENETRATION))
		acc_strcat("You feel overwhelmingly righteous!\r\n", NULL);
	if (affected_by_spell(ch, SPELL_PRAY))
		acc_strcat("You feel extremely righteous.\r\n", NULL);
	else if (affected_by_spell(ch, SPELL_BLESS))
		acc_strcat("You feel righteous.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_DEATH_KNELL))
		acc_strcat("You feel giddy from the absorption of a life.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_MALEFIC_VIOLATION))
		acc_strcat("You feel overwhelmingly wicked!\r\n", NULL);
	if (affected_by_spell(ch, SPELL_MANA_SHIELD))
		acc_strcat("You are protected by a mana shield.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_SHIELD_OF_RIGHTEOUSNESS))
		acc_strcat("You are surrounded by a shield of righteousness.\r\n", NULL);
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
	if (IS_AFFECTED(ch, AFF_NOPAIN))
		acc_strcat("Your mind is ignoring pain.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_RETINA))
		acc_strcat("Your retina is especially sensitive.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_ADRENAL_MAXIMIZER))
		acc_strcat("Shukutei Adrenal Maximizations are active.\r\n", NULL);
	else if (IS_AFFECTED(ch, AFF_ADRENALINE))
		acc_strcat("Your adrenaline is pumping.\r\n", NULL);
	if (IS_AFFECTED(ch, AFF_CONFIDENCE))
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
        acc_strcat("You energy capacitance is boosted.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_PSYCHIC_RESISTANCE))
		acc_strcat("Your mind is resistant to external energies.\r\n", NULL);

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
        acc_strcat("You are protected from electromagnetic attacks.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_GAUSS_SHIELD))
        acc_strcat("You feel somewhat protected from metal.\r\n", NULL);

	if (IS_AFFECTED_3(ch, AFF3_DAMAGE_CONTROL))
		acc_strcat("Your Damage Control process is running.\r\n", NULL);
	if (IS_AFFECTED_3(ch, AFF3_SHROUD_OBSCUREMENT))
		acc_strcat(str,
			"You are surrounded by an magical obscurement shroud.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_DETECT_SCRYING))
		acc_strcat("You are sensitive to attempts to magical scrying.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_ELUSION))
		acc_strcat("You are attempting to hide your tracks.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_TELEPATHY))
		acc_strcat("Your telepathic abilities are greatly enhanced.\r\n", NULL);
	if (affected_by_spell(ch, SPELL_ANIMAL_KIN))
		acc_strcat("You are feeling a strong bond with animals.\r\n", NULL);
    if (affected_by_spell(ch, SPELL_THORN_SKIN))
        acc_strcat("There are thorns protruding from your skin.\r\n", NULL);
	if (affected_by_spell(ch, SKILL_NANITE_RECONSTRUCTION))
		acc_strcat("Your implants are undergoing nanite reconstruction\r\n", NULL);
	if (IS_AFFECTED_2(ch, AFF2_PROT_RAD))
		acc_strcat("You are immune to the effects of radiation.\r\n", NULL);
	if (affected_by_spell(ch, SONG_WOUNDING_WHISPERS))
		acc_strcat("You are surrounded by whirling slivers of sound.\r\n", NULL);
    if (affected_by_spell(ch, SONG_MIRROR_IMAGE_MELODY))
        acc_strcat("You are surrounded by mirror images.\r\n", NULL);
	// vampiric regeneration

	if ((af = affected_by_spell(ch, SPELL_VAMPIRIC_REGENERATION))) {
		if ((name = playerIndex.getName(af->modifier)))
			acc_sprintf(
				"You are under the effects of %s's vampiric regeneration.\r\n",
				name);
		else
			acc_strcat("You are under the effects of vampiric regeneration from an unknown source.\r\n", NULL);
	}

	if ((af = affected_by_spell(ch, SPELL_LOCUST_REGENERATION))) {
		if ((name = playerIndex.getName(af->modifier)))
			acc_strcat("You are under the effects of ", name,
				"'s locust regeneration.\r\n", NULL);
		else
			acc_strcat(str,
				"You are under the effects of locust regeneration from an unknown source.\r\n", NULL);
	}
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

ACMD(do_experience)
{

	if (GET_LEVEL(ch) >= LVL_AMBASSADOR) {
		send_to_char(ch, "You are very experienced.\r\n");
		return;
	}
	send_to_char(ch, "You need %s%d%s exp to level.\r\n",
		CCCYN(ch, C_NRM), ((exp_scale[GET_LEVEL(ch) + 1]) - GET_EXP(ch)),
		CCNRM(ch, C_NRM));
}

ACMD(do_score)
{

	struct time_info_data playing_time;
	struct time_info_data real_time_passed(time_t t2, time_t t1);
	int get_hunted_id(int hunter_id);

	acc_string_clear();
	acc_sprintf(
		"%s*****************************************************************\r\n",
		CCRED(ch, C_NRM));
	acc_sprintf(
		"%s***************************%sS C O R E%s%s*****************************\r\n",
		CCYEL(ch, C_NRM), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR),
		CCYEL(ch, C_NRM));
	acc_sprintf(
		"%s*****************************************************************%s\r\n",
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));

	acc_sprintf("%s, %d year old %s %s %s.  Your level is %d.\r\n",
		GET_NAME(ch),
		GET_AGE(ch), genders[(int)GET_SEX(ch)],
		(GET_RACE(ch) >= 0 && GET_RACE(ch) < NUM_RACES) ?
		player_race[(int)GET_RACE(ch)] : "BAD RACE",
		(GET_CLASS(ch) >= 0 && GET_CLASS(ch) < TOP_CLASS) ?
		pc_char_class_types[(int)GET_CLASS(ch)] : "BAD CLASS", GET_LEVEL(ch));
	if (!IS_NPC(ch) && IS_REMORT(ch))
		acc_sprintf("You have remortalized as a %s (generation %d).\r\n",
			pc_char_class_types[(int)GET_REMORT_CLASS(ch)],
			GET_REMORT_GEN(ch));
	if ((age(ch).month == 0) && (age(ch).day == 0))
		acc_strcat("  It's your birthday today!\r\n\r\n", NULL);
	else
		acc_strcat("\r\n", NULL);

	acc_sprintf("Hit Points:  %11s           Armor Class: %s%d/10%s\r\n",
		tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_HIT(ch),
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_HIT(ch),
			CCNRM(ch, C_NRM)),
		CCGRN(ch, C_NRM), GET_AC(ch), CCNRM(ch, C_NRM));
	acc_sprintf("Mana Points: %11s           Alignment: %s%d%s\r\n",
		tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MANA(ch),
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MANA(ch),
			CCNRM(ch, C_NRM)),
		CCGRN(ch, C_NRM), GET_ALIGNMENT(ch), CCNRM(ch, C_NRM));
	acc_sprintf("Move Points: %11s           Experience: %s%d%s\r\n",
		tmp_sprintf("(%s%4d%s/%s%4d%s)", CCYEL(ch, C_NRM), GET_MOVE(ch),
			CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MAX_MOVE(ch),
			CCNRM(ch, C_NRM)),
		CCGRN(ch, C_NRM), GET_EXP(ch), CCNRM(ch, C_NRM));
	acc_sprintf(
		"                                   %sKills%s: %d, %sPKills%s: %d, %sArena%s: %d\r\n",
		CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
		(GET_MOBKILLS(ch) + GET_PKILLS(ch) + GET_ARENAKILLS(ch)),
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM), GET_PKILLS(ch),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_ARENAKILLS(ch));

	acc_sprintf(
		"%s*****************************************************************%s\r\n",
		CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	acc_sprintf("You have %s%d%s life points.\r\n",
		CCCYN(ch, C_NRM), GET_LIFE_POINTS(ch), CCNRM(ch, C_NRM));
	acc_sprintf("You are %s%d%s cm tall, and weigh %s%d%s pounds.\r\n",
		CCCYN(ch, C_NRM), GET_HEIGHT(ch), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
		GET_WEIGHT(ch), CCNRM(ch, C_NRM));
	if (!IS_NPC(ch)) {
		if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
			acc_sprintf("You need %s%d%s exp to reach your next level.\r\n",
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
		playing_time = real_time_passed((time(0) - ch->player.time.logon) +
			ch->player.time.played, 0);
		acc_sprintf("You have existed here for %d days and %d hours.\r\n",
			playing_time.day, playing_time.hours);

		acc_sprintf("You are known as %s%s%s.%s\r\n",
			CCYEL(ch, C_NRM), GET_NAME(ch), GET_TITLE(ch), CCNRM(ch, C_NRM));
		acc_strcat("You have a reputation of being -",
			reputation_msg[GET_REPUTATION_RANK(ch)], "-\r\n", NULL);
		if (get_hunted_id(GET_IDNUM(ch)))
			acc_sprintf("You are registered to bounty hunt %s.\r\n",
				playerIndex.getName(get_hunted_id(GET_IDNUM(ch))));

	}
	acc_sprintf("You are currently speaking %s.\r\n",
				 ((GET_LANGUAGE(ch) > LANGUAGE_COMMON) ? 
				  language_names[(int)GET_LANGUAGE(ch)] :
				  "common"));
	acc_sprintf(
		"You carry %s%d%s gold coins.  You have %s%d%s cash credits.\r\n",
		CCCYN(ch, C_NRM), GET_GOLD(ch), CCNRM(ch, C_NRM), CCCYN(ch,
			C_NRM), GET_CASH(ch), CCNRM(ch, C_NRM));

	switch (ch->getPosition()) {
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
		acc_strcat(CCRED(ch, C_NRM), "You are incapacitated, slowly fading away...",
			CCNRM(ch, C_NRM), "\r\n", NULL);
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
		if (IS_AFFECTED_2(ch, AFF2_MEDITATE))
			acc_strcat(CCGRN(ch, C_NRM), "You are meditating.",
				CCNRM(ch, C_NRM), "\r\n", NULL);
		else
			acc_strcat(CCGRN(ch, C_NRM), "You are sitting.",
				CCNRM(ch, C_NRM), "\r\n", NULL);
		break;
	case POS_FIGHTING:
		if ((ch->numCombatants()))
			acc_strcat(CCYEL(ch, C_NRM),
				"You are fighting ", PERS(ch->findRandomCombat(), ch), ".",
				CCNRM(ch, C_NRM), "\r\n", NULL);
		else
			acc_strcat(CCYEL(ch, C_NRM),
				"You are fighting thin air.", CCNRM(ch, C_NRM), "\r\n", NULL);
		break;
	case POS_MOUNTED:
		if (ch->isMounted())
			acc_strcat(CCGRN(ch, C_NRM), "You are mounted on ",
				PERS(ch->isMounted(), ch), ".", CCNRM(ch, C_NRM), "\r\n", NULL);
		else
			acc_strcat(CCGRN(ch, C_NRM), "You are mounted on the thin air!?",
				CCNRM(ch, C_NRM), "\r\n", NULL);
		break;
	case POS_STANDING:
		acc_strcat(CCGRN(ch, C_NRM), "You are standing",
			CCNRM(ch, C_NRM), "\r\n", NULL);
		break;
	case POS_FLYING:
		acc_strcat(CCGRN(ch, C_NRM), "You are hovering in midair",
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
	send_to_char(ch, "You are carrying:\r\n");
	list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
	int idx, pos;
	bool found = false;
	struct obj_data *obj = NULL;
	char outbuf[MAX_STRING_LENGTH];
	char *str;
	const char *active_buf[2] = { "(inactive)", "(active)" };
	bool show_all = false;
	skip_spaces(&argument);

	if (subcmd == SCMD_EQ) {
		if (*argument && is_abbrev(argument, "status")) {
			strcpy(outbuf, "Equipment status:\r\n");
			for (idx = 0; idx < NUM_WEARS; idx++) {
				pos = eq_pos_order[idx];

				if (!(obj = GET_EQ(ch, pos)))
					continue;
				found = 1;
				sprintf(outbuf, "%s-%s- is in %s condition.\r\n", outbuf,
					obj->name, obj_cond_color(obj, ch));
			}
			if (found)
				page_string(ch->desc, outbuf);
			else
				send_to_char(ch, "You're totally naked!\r\n");
			return;
		} else if( *argument && is_abbrev(argument, "all")) {
			show_all = true;
		}
		
		if(show_all) {
			send_to_char(ch, "You are using:\r\n");
		}
		for (idx = 0; idx < NUM_WEARS; idx++) {
			pos = eq_pos_order[idx];
			if (GET_EQ(ch, pos)) {
				if (!found && !show_all) {
					send_to_char(ch, "You are using:\r\n");
					found = true;
				}
				if (can_see_object(ch, GET_EQ(ch, pos))) {
					send_to_char(ch, "%s%s%s", CCGRN(ch, C_NRM),
						where[pos], CCNRM(ch, C_NRM));
					show_obj_to_char(GET_EQ(ch, pos), ch, 1, 0);
				} else {
					send_to_char(ch, "%sSomething.\r\n", where[pos]);
				}
			} else if (show_all && pos != WEAR_ASS) {
				send_to_char(ch, "%sNothing!\r\n", where[pos]);
			}
		}

		if (!found && !show_all) {
			send_to_char(ch, "You're totally naked!\r\n");
		}
		return;
	}

	if (subcmd == SCMD_IMPLANTS) {
		if (*argument && is_abbrev(argument, "status")) {
			strcpy(outbuf, "Implant status:\r\n");
			for (idx = 0; idx < NUM_WEARS; idx++) {
				pos = implant_pos_order[idx];

				if (!(obj = GET_IMPLANT(ch, pos)))
					continue;
				found = true;
				sprintf(outbuf, "%s-%s- is in %s condition.\r\n",
					outbuf, obj->name, obj_cond_color(obj,
						ch));
			}
			if (found)
				page_string(ch->desc, outbuf);
			else
				send_to_char(ch, "You don't have any implants.\r\n");
			return;
		} else if( *argument && is_abbrev(argument, "all")) {
			show_all = true;
		}

		if (show_all) {
			send_to_char(ch, "You are implanted with:\r\n");
		}
		for (idx = 0; idx < NUM_WEARS; idx++) {
			pos = implant_pos_order[idx];
			if (ILLEGAL_IMPLANTPOS(pos))
				continue;

			if (GET_IMPLANT(ch, pos)) {
				if (!found && !show_all) {
					send_to_char(ch, "You are implanted with:\r\n");
					found = 1;
				}
				if (can_see_object(ch, GET_IMPLANT(ch, pos))) {

					if (IS_DEVICE(GET_IMPLANT(ch, pos)))
						str = tmp_sprintf(" %10s %s(%s%d%s/%s%d%s)%s",
							active_buf[(ENGINE_STATE(GET_IMPLANT(ch, pos))) ? 1:0],
							CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
							CUR_ENERGY(GET_IMPLANT(ch, pos)),
							CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
							MAX_ENERGY(GET_IMPLANT(ch, pos)),
							CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
					else
						str = "";

					send_to_char(ch, "%s[%12s]%s - %-30s%s\r\n",
						CCCYN(ch, C_NRM), wear_implantpos[pos],
						CCNRM(ch, C_NRM), GET_IMPLANT(ch, pos)->name, str);
				} else {
					send_to_char(ch, "%s[%12s]%s - (UNKNOWN)\r\n", CCCYN(ch,
							C_NRM), wear_implantpos[pos],
						CCNRM(ch, C_NRM));
				}
			} else if (show_all && pos != WEAR_ASS && pos != WEAR_HOLD) {
				send_to_char(ch, "[%12s] - Nothing!\r\n", 
					wear_implantpos[pos]);
			}
		}
	}
	if (!found && !show_all) {
		send_to_char(ch, "You don't have any implants.\r\n");
	}
}

void set_local_time(struct zone_data *zone, struct time_info_data *local_time);

ACMD(do_time)
{
	char *suf;
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

	send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
		day, suf, month_name[(int)local_time.month],
		ch->in_room->zone->time_frame == TIME_MODRIAN ? local_time.year :
		ch->in_room->zone->time_frame == TIME_ELECTRO ? local_time.year +
		3567 : local_time.year);

	send_to_char(ch, "The moon is currently %s.\r\n",
		lunar_phases[get_lunar_phase(lunar_day)]);
}

void
show_mud_date_to_char(struct Creature *ch)
{
	char *suf;
	struct time_info_data local_time;
	extern const char *month_name[16];
	int day;

	if (!ch->in_room) {
		errlog("!ch->in_room in show_mud_date_to_char");
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

	send_to_char(ch, "It is the %d%s Day of the %s, Year %d.\r\n",
		day, suf, month_name[(int)local_time.month], local_time.year);

}

ACMD(do_weather)
{
	static char *sky_look[] = {
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

#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-a clan] [-<soqmfx>]\r\n"


//generates a formatted string representation of a player for the who list
string
whoString(Creature *ch, Creature *target) {
	ostringstream out;
	int len = strlen(BADGE(target));
	
	//show badge
	if (GET_LEVEL(target) >= LVL_AMBASSADOR) {
		out << CCBLD(ch, C_NRM) << CCYEL(ch, C_NRM) << '[' << CCGRN(ch, C_NRM);
        out << tmp_pad(' ', (MAX_BADGE_LENGTH - len) / 2);
		out << BADGE(target);
		out << tmp_pad(' ', (MAX_BADGE_LENGTH - len + 1) / 2);
        out << CCNRM(ch, C_NRM) << CCBLD(ch, C_NRM) << CCYEL(ch, C_NRM) << ']';
	} else { //show level/class
		out << CCGRN(ch, C_NRM) << '[';
        if (PRF2_FLAGGED(target, PRF2_ANONYMOUS) && !Security::isMember(ch, Security::ADMINBASIC)) {
			out << CCCYN(ch, C_NRM) << "--";
		} else if (Security::isMember(ch, Security::ADMINBASIC)) {
			if (PRF2_FLAGGED(target, PRF2_ANONYMOUS)) {
				out << CCRED(ch, C_NRM);
			} else {
				out << CCNRM(ch, C_NRM);
			}
			if (GET_LEVEL(target) < 10) {
				out << ' ';
			}
			out << (int)GET_LEVEL(target) << CCCYN(ch, C_NRM) << '(' << CCNRM(ch, C_NRM);
			if (GET_REMORT_GEN(target) < 10) {
				out << ' ';
			}
			out << (int)GET_REMORT_GEN(target) << CCCYN(ch, C_NRM) << ')' << CCNRM(ch, C_NRM);
		} else {
			out << CCNRM(ch, C_NRM); 
			if (GET_LEVEL(target) < 10) {
				out << ' ';
			}
			out << (int)GET_LEVEL(target);
		}
		out << ' ' << get_char_class_color(ch, target, GET_CLASS(target));
		out << char_class_abbrevs[(int)GET_CLASS(target)];
        out << CCNRM(ch, C_NRM) << CCGRN(ch, C_NRM) << ']';
	}
	
	
	//name
    if (GET_LEVEL(target) >= LVL_AMBASSADOR) {
        out << CCNRM(ch, C_NRM) << CCGRN(ch, C_NRM);
    } else {
        out << CCNRM(ch, C_NRM);
    }
	if (can_see_creature(ch, target)) {
        out << ' ' << GET_NAME(target);
    } else {
        out << CCNRM(ch, C_NRM) << ' ' << "Someone";
    }
	
	//title
	out << GET_TITLE(target);

	return out.str();
}
	
//generates a formatted string representation of a player for the who list
string
whoFlagsString(Creature *ch, Creature *target) {
	ostringstream out;

	//nowho
	if (PRF2_FLAGGED(target, PRF2_NOWHO)) {
		out << CCRED(ch, C_NRM) << " (nowho)"  << CCNRM(ch, C_NRM);
	}
	
	//clan badge
	if (real_clan(GET_CLAN(target))) {
		if (PRF2_FLAGGED(target, PRF2_CLAN_HIDE)) {
			if (Security::isMember(ch, Security::ADMINBASIC)) {
				out << CCCYN(ch, C_NRM) << " )" << real_clan(GET_CLAN(target))->name;
				out << '(' << CCNRM(ch, C_NRM);
			}
		} else {
			out << CCCYN(ch, C_NRM) << ' ' << real_clan(GET_CLAN(target))->badge << CCNRM(ch, C_NRM);
		}
	}
	
	//imm invis
	if (GET_INVIS_LVL(target) && IS_IMMORT(ch)) {
		out << ' ' << CCBLU(ch, C_NRM) << '(' << CCMAG(ch, C_NRM) << 'i' << GET_INVIS_LVL(target);
		out << CCBLU(ch, C_NRM) << ')' << CCNRM(ch, C_NRM);
	}
	
	//invis
	if (IS_AFFECTED(target, AFF_INVISIBLE)) {
		out << CCCYN(ch, C_NRM) << " (invis)" << CCNRM(ch, C_NRM);
	}
	
	//trans
	if (IS_AFFECTED_2(target, AFF2_TRANSPARENT)) {
		out << CCCYN(ch, C_NRM) << " (transp)" << CCNRM(ch, C_NRM);
	}
	
	//mailing
	if (PLR_FLAGGED(target, PLR_MAILING)) {
		out << CCGRN(ch, C_NRM) << " (mailing)" << CCNRM(ch, C_NRM);
	} else if (PLR_FLAGGED(target, PLR_WRITING)) { //writing
		out << CCGRN(ch, C_NRM) << " (writing)" << CCNRM(ch, C_NRM);
	}
	
	//creating
	if (PLR_FLAGGED(target, PLR_OLC)) {
		out << CCGRN(ch, C_NRM) << " (creating)" << CCNRM(ch, C_NRM);
	}
	
	//deaf
	if (PRF_FLAGGED(target, PRF_DEAF)) {
		out << CCBLU(ch, C_NRM) << " (deaf)" << CCNRM(ch, C_NRM);
	}
	
	//notell
	if (PRF_FLAGGED(target, PRF_NOTELL)) {
		out << CCBLU(ch, C_NRM) << " (notell)" << CCNRM(ch, C_NRM);
	}
	
	//questing
	if (GET_QUEST(target)) {
		out << CCYEL_BLD(ch, C_NRM) << " (quest)" << CCNRM(ch, C_NRM);
	}
	
	//afk
	if (PLR_FLAGGED(target, PLR_AFK)) {
		out << CCGRN(ch, C_NRM) << " (afk)" << CCNRM(ch, C_NRM);
	}
	
	//thief
	if (PLR_FLAGGED(target, PLR_THIEF)) {
		out << CCRED(ch, C_NRM) << " (THIEF)" << CCNRM(ch, C_NRM);
	}
	
	//killer
	if (PLR_FLAGGED(target, PLR_KILLER)) {
		out << CCRED(ch, C_NRM) << " (KILLER)" << CCNRM(ch, C_NRM);
	}
	
	//council
	if (PLR_FLAGGED(target, PLR_COUNCIL)) {
		out << CCBLU_BLD(ch, C_NRM) << " <COUNCIL>" << CCNRM(ch, C_NRM);
	}
	
	return out.str();
}

string
whoKillsString(Creature *ch, Creature *target) {
	ostringstream out;

	out << CCRED_BLD(ch, C_NRM) << " *" << GET_PKILLS(target) << " KILLS* -";
	out << reputation_msg[GET_REPUTATION_RANK(target)] << "-" << CCNRM(ch, C_NRM);
	
	return out.str();
}

ACMD(do_who)
{
	
	struct descriptor_data *d;
	ostringstream out, imms, testers, players;
	int immCount=0, testerCount=0, playerCount=0, playerTotal=0, immTotal=0;
	char *tester_s="s";
	bool zone=false, plane=false, time=false, kills=false, noflags=false;
	bool classes=false, clan=false;
	bool mage=false, thief=false, ranger=false, knight=false, cleric=false, barbarian=false;
	bool bard=false, monk=false, physic=false, cyborg=false, psionic=false, mercenary=false;
	int low=-1, high=-1;
	clan_data *realClan;
	
	string args = argument;
	
	unsigned int dashPos = string::npos;
	if ((dashPos = args.find("-")) != string::npos) {
		string levelRange = args.substr(args.rfind(' ', dashPos), args.find(' ', dashPos));
		sscanf(levelRange.c_str(), "%d-%d", &low, &high);
	}
	if (args.find("zone") != string::npos) {
		zone = true;
	}
	if (args.find("plane") != string::npos) {
		plane = true;
	}
	if (args.find("time") != string::npos) {
		time = true;
	}
	if (args.find("kills") != string::npos) {
		kills = true;
	}
	if (args.find("noflags") != string::npos) {
		noflags = true;
	}
	if (args.find("class") != string::npos) {
		classes = true;
		if (args.find("mag") != string::npos) {
			mage = true;
		}
		if (args.find("thi") != string::npos) {
			thief = true;
		}
		if (args.find("ran") != string::npos) {
			ranger = true;
		}
		if (args.find("kni") != string::npos) {
			knight = true;
		}
		if (args.find("cle") != string::npos) {
			cleric = true;
		}
		if (args.find("barb") != string::npos) {
			barbarian = true;
		}
		if (args.find("bard") != string::npos) {
			bard = true;
		}
		if (args.find("mon") != string::npos) {
			monk = true;
		}
		if (args.find("phy") != string::npos) {
			physic = true;
		}
		if (args.find("cyb") != string::npos || args.find("borg") != string::npos) {
			cyborg = true;
		}
		if (args.find("psi") != string::npos) {
			psionic = true;
		}
		if (args.find("mer") != string::npos) {
			mercenary = true;
		}
	}
	if (args.find("clan") != string::npos) {
		clan = true;
		unsigned int start = args.find(' ', args.find("clan"));
		unsigned int end = args.find(' ', start+1);
        if (start == string::npos || start == end) {
            realClan = NULL;
        } else {
            char *clanName = tmp_strdup(args.substr(start, end).c_str());
            realClan = clan_by_name(clanName);
        }
	}
	
	
	for (d = descriptor_list; d; d = d->next) {
		Creature *curr;
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
		if (GET_LEVEL(curr) < LVL_AMBASSADOR && !curr->isTester()) {
			playerTotal++;
		} else if (GET_LEVEL(curr) >= LVL_AMBASSADOR) {
            immTotal++;
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
		if (time && ch->in_room->zone->time_frame != curr->in_room->zone->time_frame) {
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
		//levels
		if (low >=0 && high >=0) { //perform a level check
			//character is outside of level range
			if (GET_LEVEL(curr) > high || GET_LEVEL(curr) < low ||
			//character's level is hidden
			(PRF2_FLAGGED(curr, PRF2_ANONYMOUS) && !Security::isMember(ch, Security::ADMINBASIC))) {
				continue;
			}
		}
		//clans
		if (clan && (realClan != NULL)) {
			//not in clan
			if (realClan->number != GET_CLAN(curr) ||
			//not able to see the clan
			(PRF2_FLAGGED(curr, PRF2_CLAN_HIDE) && !Security::isMember(ch, Security::ADMINBASIC))) {
				continue;
			}
		}
        //nowho
        if (PRF2_FLAGGED(curr, PRF2_NOWHO) && GET_LEVEL(curr) >= LVL_AMBASSADOR &&
            GET_LEVEL(ch) < LVL_AMBASSADOR) {
            continue;
        }

        //imm invis
        if (GET_INVIS_LVL(curr) > GET_LEVEL(ch)) {
            continue;
        }

		/////////////////END CONDITIONS/////////////////////////
		
		
		if (GET_LEVEL(curr) >= LVL_AMBASSADOR) {
			immCount++;
			imms << whoString(ch, curr);
			if (!noflags) {
				imms << whoFlagsString(ch, curr);
			}
			if (kills) {
				imms << whoKillsString(ch, curr);
			}
			imms << "\r\n";
		} else if (curr->isTester()) {
			testerCount++;
			testers << whoString(ch, curr);
			if (!noflags) {
				testers << whoFlagsString(ch, curr);
			}
			if (kills) {
				testers << whoKillsString(ch, curr);
			}
			testers << "\r\n";
		} else {
			playerCount++;
			players << whoString(ch, curr);
			if (!noflags) {
				players << whoFlagsString(ch, curr);
			}
			if (kills) {
				players << whoKillsString(ch, curr);
			}
			players << "\r\n";
		}
	}
	
	
	//List the immortals
	if (immCount > 0) {
		out << CCBLD(ch, C_CMP) << "**************     " << CCGRN(ch, C_NRM);
		out << "Visible Immortals of TEMPUS" << CCNRM(ch, C_NRM) << CCBLD(ch, C_CMP);
		out << "     **************" << CCNRM(ch, C_SPR) << "\r\n";
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
		out << imms.str();
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
        out << "\r\n";
	}
	//testers
	if ((GET_LEVEL(ch) >= LVL_AMBASSADOR || ch->isTester()) && testerCount > 0) {
		out << CCNRM(ch, C_SPR) << CCBLD(ch, C_CMP) << "**************      " << CCGRN(ch, C_NRM);
		out << "Visible Testers of TEMPUS" << CCNRM(ch, C_NRM) << CCBLD(ch, C_CMP);
		out << "      **************" << CCNRM(ch, C_SPR) << "\r\n";
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
		out << testers.str();
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
        out << "\r\n";
	}
	//players
	if (playerCount > 0) {
		out << CCNRM(ch, C_SPR) << CCBLD(ch, C_CMP) << "**************      " << CCGRN(ch, C_NRM);
		out << "Visible Players of TEMPUS" << CCNRM(ch, C_NRM) << CCBLD(ch, C_CMP);
		out << "      **************" << CCNRM(ch, C_SPR) << "\r\n";
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
		out << players.str();
		out << (IS_NPC(ch) ? "" : (ch->account->get_compact_level() > 1) ? "" : "\r\n");
        out << "\r\n";
	}
	
	//determine plurality of nouns
	//if (immCount == 1)
	//	imm_s="";
	if (testerCount == 1)
		tester_s="";
	
	out << immCount << " of " << immTotal << " immortals";
	if (GET_LEVEL(ch) >= LVL_AMBASSADOR || ch->isTester()) {
		out << ", " << testerCount << " tester" << tester_s << ",";
	}
	out << " and " << playerCount << " of " << playerTotal << " players displayed.\r\n";
	page_string(ch->desc, out.str().c_str());
}



#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c char_claslist] [-o] [-p]\r\n"

ACMD(do_users)
{
	char line[200], line2[220], idletime[10], char_classname[20];
	char state[30], *timeptr, *format, mode;
	char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
	char out_buf[MAX_STRING_LENGTH];
	struct Creature *tch;
	struct descriptor_data *d;
	int low = 0, high = LVL_GRIMP, i, num_can_see = 0;
	int showchar_class = 0, outlaws = 0, playing = 0, deadweight = 0;

	host_search[0] = name_search[0] = '\0';

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf1);
		if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
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
				send_to_char(ch, USERS_FORMAT);
				return;
				break;
			}					/* end of switch */

		} else {				/* endif */
			send_to_char(ch, USERS_FORMAT);
			return;
		}
	}							/* end while (parser) */
	strcpy(line,
		" Num Class     Name         State       Idl Login@   Site\r\n");
	strcat(line,
		" --- --------- ------------ ------------ -- -------- ---------------------\r\n");
	strcpy(out_buf, line);

	one_argument(argument, arg);

	for (d = descriptor_list; d; d = d->next) {
		if (IS_PLAYING(d) && playing)
			continue;
		if (STATE(d) == CXN_PLAYING && deadweight)
			continue;
		if (STATE(d) == CXN_PLAYING) {
			if (d->original)
				tch = d->original;
			else if (!(tch = d->creature))
				continue;

			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && str_cmp(GET_NAME(tch), name_search))
				continue;
			if (GET_LEVEL(ch) < LVL_LUCIFER)
				if (!can_see_creature(ch, tch) || GET_LEVEL(tch) < low
					|| GET_LEVEL(tch) > high)
					continue;
			if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
				!PLR_FLAGGED(tch, PLR_THIEF))
				continue;
			if (showchar_class && !(showchar_class & (1 << GET_CLASS(tch))))
				continue;
			if (GET_LEVEL(ch) < LVL_LUCIFER)
				if (GET_INVIS_LVL(ch) > GET_LEVEL(ch))
					continue;

			if (d->original)
				sprintf(char_classname, "[%2d %s]", GET_LEVEL(d->original),
					CLASS_ABBR(d->original));
			else
				sprintf(char_classname, "[%2d %s]", GET_LEVEL(d->creature),
					CLASS_ABBR(d->creature));
		} else
			strcpy(char_classname, "    -    ");

		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';

		if (STATE(d) == CXN_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			strcpy(state, desc_modes[STATE(d)]);

		if (d->creature && STATE(d) == CXN_PLAYING &&
			(GET_LEVEL(d->creature) < GET_LEVEL(ch)
				|| GET_LEVEL(ch) >= LVL_LUCIFER))
			sprintf(idletime, "%2d",
				d->creature->char_specials.timer * SECS_PER_MUD_HOUR /
				SECS_PER_REAL_MIN);
		else
			sprintf(idletime, "%2d", d->idle);


		format = "%4d %-7s %s%-12s%s %-12s %-2s %-8s ";

		if (d->creature && d->creature->player.name) {
			if (d->original)
				sprintf(line, format, d->desc_num, char_classname, CCCYN(ch,
						C_NRM), d->original->player.name, CCNRM(ch, C_NRM),
					state, idletime, timeptr);
			else
				sprintf(line, format, d->desc_num, char_classname,
					(GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCGRN(ch,
							C_NRM) : ""), d->creature->player.name,
					(GET_LEVEL(d->creature) >= LVL_AMBASSADOR ? CCNRM(ch,
							C_NRM) : ""), state, idletime, timeptr);
		} else
			sprintf(line, format, d->desc_num, "    -    ", CCRED(ch, C_NRM),
				"UNDEFINED", CCNRM(ch, C_NRM), state, idletime, timeptr);

		if (d->host && *d->host)
			sprintf(line + strlen(line), "%s[%s]%s\r\n", isbanned(d->host,
					buf) ? (d->creature
					&& PLR_FLAGGED(d->creature, PLR_SITEOK) ? CCMAG(ch,
						C_NRM) : CCRED(ch, C_SPR)) : CCGRN(ch, C_NRM), d->host,
				CCNRM(ch, C_SPR));
		else {
			strcat(line, CCRED_BLD(ch, C_SPR));
			strcat(line, "[Hostname unknown]\r\n");
			strcat(line, CCNRM(ch, C_SPR));
		}

		if (STATE(d) != CXN_PLAYING) {
			sprintf(line2, "%s%s%s", CCCYN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}
		if (STATE(d) != CXN_PLAYING || (STATE(d) == CXN_PLAYING
				&& can_see_creature(ch, d->creature))) {
			strcat(out_buf, line);
			num_can_see++;
		}
	}

	sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
	strcat(out_buf, line);
	page_string(ch->desc, out_buf);
}



/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
	extern char circlemud_version[];

	switch (subcmd) {
	case SCMD_CREDITS:
		page_string(ch->desc, credits);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info);
		break;
    /*
	case SCMD_WIZLIST:
		if (clr(ch, C_NRM))
			page_string(ch->desc, ansi_wizlist);
		else
			page_string(ch->desc, wizlist);
	case SCMD_IMMLIST:
		if (clr(ch, C_NRM))
			page_string(ch->desc, ansi_immlist);
		else
			page_string(ch->desc, immlist);
		break;
    */
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
		send_to_char(ch, circlemud_version);
		break;
	case SCMD_WHOAMI:
		send_to_char(ch, strcat(strcpy(buf, GET_NAME(ch)), "\r\n"));
		break;
	case SCMD_AREAS:
		skip_spaces(&argument);
		if (!*argument)
			send_to_char(ch, 
				"Do you want a list of low, mid, high, or remort level areas?\r\n"
				"Usage: areas < low | mid | high | remort >\r\n");
		else if (is_abbrev(argument, "low"))
			page_string(ch->desc, areas_low);
		else if (is_abbrev(argument, "mid"))
			page_string(ch->desc, areas_mid);
		else if (is_abbrev(argument, "high"))
			page_string(ch->desc, areas_high);
		else if (is_abbrev(argument, "remort"))
			page_string(ch->desc, areas_remort);
		else
			send_to_char(ch, "Usage: areas < low | mid | high | remort >\r\n");
		break;
	default:
		return;
		break;
	}
}


void
perform_mortal_where(struct Creature *ch, char *arg)
{
	register struct Creature *i = NULL;
	register struct descriptor_data *d;

	if (!*arg) {
		send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) == CXN_PLAYING) {
				i = (d->original ? d->original : d->creature);
				if (i && can_see_creature(ch, i) && (i->in_room != NULL) &&
					(ch->in_room->zone == i->in_room->zone)) {
					send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i),
						i->in_room->name);
				}
			}
		}
	} else {					/* print only FIRST char, not all. */
		CreatureList::iterator cit = characterList.begin();
		for (; cit != characterList.end(); ++cit) {
			i = *cit;
			if (i->in_room->zone == ch->in_room->zone && can_see_creature(ch, i) &&
				(i->in_room != NULL) && isname(arg, i->player.name)) {
				send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i), i->in_room->name);
				return;
			}
		}
		send_to_char(ch, "No-one around by that name.\r\n");
	}
}

void
print_object_location(int num, struct obj_data *obj,
	struct Creature *ch, int recur, char *to_buf)
{
	if ((obj->carried_by && GET_INVIS_LVL(obj->carried_by) > GET_LEVEL(ch)) ||
		(obj->in_obj && obj->in_obj->carried_by &&
			GET_INVIS_LVL(obj->in_obj->carried_by) > GET_LEVEL(ch)) ||
		(obj->worn_by && GET_INVIS_LVL(obj->worn_by) > GET_LEVEL(ch)) ||
		(obj->in_obj && obj->in_obj->worn_by &&
			GET_INVIS_LVL(obj->in_obj->worn_by) > GET_LEVEL(ch)))
		return;

	if (num > 0)
		sprintf(buf, "%sO%s%3d. %s%-25s%s - ", CCGRN_BLD(ch, C_NRM), CCNRM(ch,
				C_NRM), num, CCGRN(ch, C_NRM), obj->name,
			CCNRM(ch, C_NRM));
	else
		sprintf(buf, "%33s", " - ");

	strncat(to_buf, buf, MAX_STRING_LENGTH - 1);

	if (obj->in_room != NULL) {
		sprintf(buf, "%s[%s%5d%s] %s%s%s\r\n",
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			obj->in_room->number, CCGRN(ch, C_NRM), CCCYN(ch, C_NRM),
			obj->in_room->name, CCNRM(ch, C_NRM));
	} else if (obj->carried_by) {
		sprintf(buf, "carried by %s%s%s [%s%5d%s]%s\r\n",
			CCYEL(ch, C_NRM), PERS(obj->carried_by, ch),
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
			obj->carried_by->in_room->number, CCGRN(ch, C_NRM), CCNRM(ch,
				C_NRM));
	} else if (obj->worn_by) {
		sprintf(buf, "worn by %s%s%s [%s%5d%s]%s\r\n",
			CCYEL(ch, C_NRM), PERS(obj->worn_by, ch),
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), obj->worn_by->in_room->number,
			CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
	} else if (obj->in_obj) {
		sprintf(buf, "inside %s%s%s%s\r\n",
			CCGRN(ch, C_NRM), obj->in_obj->name,
			CCNRM(ch, C_NRM), (recur ? ", which is" : " "));
		strncat(to_buf, buf, MAX_STRING_LENGTH - 1);

		if (recur)
			print_object_location(0, obj->in_obj, ch, recur, to_buf);
		return;
	} else {
		sprintf(buf, "%sin an unknown location%s\r\n",
			CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
	}

	strncat(to_buf, buf, MAX_STRING_LENGTH - 1);
	to_buf[MAX_STRING_LENGTH - 1] = '\0';
}


/**
 * Checks to see that the given object matches the search criteria
 * @param req The list of required search parameters
 * @param exc The list of excluded search parameters
 * @param thing The obj_data of the object to be reviewed
 *
 * @return a boolean value representing if the object passed the search criteria
 *
**/ 

bool isWhereMatch( const list<char *> &req, const list<char *> &exc, obj_data *thing) {
    list<char *>::const_iterator reqit, excit;
    
	if (!thing->aliases)
		return false;

    for(reqit = req.begin(); reqit != req.end(); reqit++) {
        if(!isname(*reqit, thing->aliases)) 
            return false;
    }
    for(excit = exc.begin(); excit != exc.end(); excit++) {
        if(isname(*excit, thing->aliases)) 
            return false;
    }
    return true;
}

/**
 * Checks to see that the given object matches the search criteria
 * @param req The list of required search parameters
 * @param exc The list of excluded search parameters
 * @param mob The Creature struct of the mobile to be reviewed
 *
 * @return a boolean value representing if the object passed the search criteria
 *
**/ 
bool 
isWhereMatch( const list<char *> &req, const list<char *> &exc, Creature *mob) {
    list<char *>::const_iterator reqit, excit;

    for(reqit = req.begin(); reqit != req.end(); reqit++) {
        if(!isname(*reqit, mob->player.name)) {
            return false;
        }
    }
    for(excit = exc.begin(); excit != exc.end(); excit++) {
        if(isname(*excit, mob->player.name)) {
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
isInHouse( obj_data *obj) {
    //if in_obj isn't null, it's still inside a container.
    if(obj->in_obj == NULL) {
        //container is being carried, not in house
        if(obj->in_room == NULL) {
            return false;
        }
        if(ROOM_FLAGGED(obj->in_room, ROOM_HOUSE)) {
            return true;
        }
        //container lying on the ground outside a house
        else { 
            return false;
        }
    }
    //still inside a container, call again
    else { 
        return isInHouse(obj->in_obj);
    }
}

void
perform_immort_where(struct Creature *ch, char *arg, bool show_morts)
{
	register struct Creature *i = NULL;
	register struct obj_data *k;
	struct descriptor_data *d;
	int num = 0, found = 0;
	char main_buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
    Tokenizer arguments(arg);
    list<char *> required, excluded;
    bool house_only, no_house, no_mob, no_object;
    
    house_only = no_house = no_mob = no_object = false;

	arg1[0] = '\0';
	if (!arguments.hasNext() || !show_morts) {
		strcpy(main_buf, "Players\r\n-------\r\n");
		for (d = descriptor_list; d; d = d->next) {
			if (STATE(d) == CXN_PLAYING) {
				i = (d->original ? d->original : d->creature);
				if (i && can_see_creature(ch, i) && (i->in_room != NULL) &&	(show_morts || IS_IMMORT(i))) {
					if (d->original)
						sprintf(buf,
							"%s%-20s%s - %s[%s%5d%s]%s %s%s%s %s(in %s)%s\r\n",
							(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch,
									C_NRM) : ""), GET_NAME(i),
							(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCNRM(ch,
									C_NRM) : ""), CCGRN(ch, C_NRM), CCNRM(ch,
								C_NRM), d->creature->in_room->number,
							CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch,
								C_NRM), d->creature->in_room->name, CCNRM(ch,
								C_NRM), CCRED(ch, C_CMP),
							GET_NAME(d->creature), CCNRM(ch, C_CMP));
					else
						sprintf(buf, "%s%-20s%s - %s[%s%5d%s]%s %s%s%s\r\n",
							(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCGRN(ch,
									C_NRM) : ""), GET_NAME(i),
							(GET_LEVEL(i) >= LVL_AMBASSADOR ? CCNRM(ch,
									C_NRM) : ""), CCGRN(ch, C_NRM), CCNRM(ch,
								C_NRM), i->in_room->number, CCGRN(ch, C_NRM),
							CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
							i->in_room->name, CCNRM(ch, C_NRM));
					strcat(main_buf, buf);
				}
			}
		}
		page_string(ch->desc, main_buf);
	} else {
        while(arguments.next(arg1)) {
            if(arg1[0] == '!') {
                excluded.push_back(tmp_strdup(arg1 + 1));
            }
            else if(arg1[0] == '-') {
                if(is_abbrev(arg1, "-nomob")) no_mob = true;
                if(is_abbrev(arg1, "-noobject")) no_object = true;
                if(is_abbrev(arg1, "-nohouse")) no_house = true;
                if(is_abbrev(arg1, "-house")) house_only = true;
            }
            else {
                required.push_back(tmp_strdup(arg1));
            }
        }
        
	    if(required.empty()) { //if there are no required fields don't search
            send_to_char(ch, "You're going to have to be a bit more specific than that.\r\n");
            return;
        }

        if( house_only && no_house ) {
            send_to_char(ch, "Nothing exists both inside and outside a house.\r\n");
            return;
        }
        
		list <string> outList;

		if(!no_mob) {
            CreatureList::iterator cit = characterList.begin();
            for (; cit != characterList.end(); ++cit) {
                i = *cit;
                if (can_see_creature(ch, i) && i->in_room && isWhereMatch(required, excluded, i) &&
                    (GET_MOB_SPEC(i) != fate || GET_LEVEL(ch) >= LVL_SPIRIT)) {
                    found = 1;
                    sprintf(buf, "%sM%s%3d. %s%-25s%s - %s[%s%5d%s]%s %s%s%s\r\n",
                        CCRED_BLD(ch, NRM), CCNRM(ch, NRM), ++num,
                        CCYEL(ch, C_NRM), GET_NAME(i), CCNRM(ch, C_NRM),
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), i->in_room->number,
                        CCGRN(ch, C_NRM), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
                        i->in_room->name, CCNRM(ch, C_NRM));
                    outList.push_back(buf);
                }
            }
        }

        if(!no_object) {
            for (num = 0, k = object_list; k; k = k->next) {
                if(! can_see_object(ch, k) )
                    continue;
                if( house_only && !isInHouse(k) )
                    continue;
                if( no_house && isInHouse(k) )
                    continue;
                if( isWhereMatch(required, excluded, k) ) {
                    found = 1;
                    main_buf[0] = '\0';
                    print_object_location(++num, k, ch, TRUE, main_buf);
                    outList.push_back(main_buf);
                }
            }
        }

		if (found) {
			main_buf[0] = '\0';
			list <string>::iterator it = outList.begin();
			int space = MAX_STRING_LENGTH - 128;
			for (; it != outList.end(); it++) {
				space -= (*it).length();
				if (space <= 0) {
					strcat(main_buf,
						"\r\n****** Buffer size exceeded. Use more specific search terms. ******\r\n");
					break;
				}
				strcat(main_buf, (*it).c_str());
			}
			page_string(ch->desc, main_buf);
		} else {
			send_to_char(ch, "Couldn't find any such thing.\r\n");
		}
	}
}

ACMD(do_where)
{
	skip_spaces(&argument);

	if (Security::isMember(ch, "Questor,AdminBasic,WizardBasic") ||
		(IS_MOB(ch) && ch->desc && ch->desc->original &&
			Security::isMember(ch->desc->original, "Questor,AdminBasic,WizardBasic")))
	  perform_immort_where(ch, argument, true);
	else if (IS_IMMORT(ch)) {
	  perform_immort_where(ch, argument, false);
	} else {

		send_to_char(ch, "You are located: %s%s%s\r\nIn: %s%s%s.\r\n",
			CCGRN(ch, C_NRM), (room_is_dark(ch->in_room) && !has_dark_sight(ch)) ?
			"In a very dark place." :
			ch->in_room->name, CCNRM(ch, C_NRM),
			CCGRN(ch, C_NRM), (room_is_dark(ch->in_room) && !has_dark_sight(ch)) ?
			"You can't tell for sure." :
			ch->in_room->zone->name, CCNRM(ch, C_NRM));
		if (ZONE_FLAGGED(ch->in_room->zone, ZONE_NOLAW))
			send_to_char(ch, "This place is beyond the reach of law.\r\n");
		if (!IS_APPR(ch->in_room->zone)) {
			send_to_char(ch, "This zone is %sUNAPPROVED%s!\r\n", CCRED(ch, C_NRM),
				CCNRM(ch, C_NRM));
		}
		if (ch->in_room->zone->public_desc)
			send_to_char(ch, "%s", ch->in_room->zone->public_desc);
		act("$n ponders the implications of $s location.", TRUE, ch, 0, 0,
			TO_ROOM);
	}
}


void
print_attributes_to_buf(struct Creature *ch, char *buff)
{

	sbyte str, stradd, intel, wis, dex, con, cha;
	str = GET_STR(ch);
	stradd = GET_ADD(ch);
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
		strcat(buff, tmp_sprintf(" [%d/%d]", str, stradd));
	if (str <= 3)
		strcat(buff, "You can barely stand up under your own weight.");
	else if (str <= 4)
		strcat(buff, "You couldn't beat your way out of a paper bag.");
	else if (str <= 5)
		strcat(buff, "You are laughed at by ten year olds.");
	else if (str <= 6)
		strcat(buff, "You are a weakling.");
	else if (str <= 7)
		strcat(buff, "You can pick up large rocks without breathing too hard.");
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
		strcat(buff, "You can toss boulders with ease!");
	else if (str == 23)
		strcat(buff, "You have the strength of a cloud giant!");
	else if (str == 24)
		strcat(buff, "You possess a herculean might!");
	else if (str == 25)
		strcat(buff, "You have the strength of a god!");
	else
		strcat(buff, "Your strength is SKREWD.");

	if (str != ch->real_abils.str || stradd != ch->real_abils.str_add)
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
	strcat(buff, "\r\n");

	sprintf(buff, "%s     %s%sDexterity:%s ", buff,
		CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));

	if (mini_mud)
		strcat(buff, tmp_sprintf(" [%d]", dex));
	if (dex <= 5)
		strcat(buff, "I wouldnt walk too fast if I were you.");
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
		strcat(buff, "A child poked you once, and you have the scars to prove it.");
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
	send_to_char(ch, "        Name:  %s%20s%s        Race: %s%10s%s\r\n", CCRED(ch,
			C_SPR), GET_NAME(ch), CCWHT(ch, C_SPR), CCRED(ch, C_SPR),
		player_race[(int)GET_RACE(ch)], CCWHT(ch, C_SPR));
	send_to_char(ch, "        Class: %s%20s%s        Level: %s%9d%s\r\n\r\n",
		CCRED(ch, C_SPR), pc_char_class_types[(int)GET_CLASS(ch)], CCWHT(ch,
			C_SPR), CCRED(ch, C_SPR), GET_LEVEL(ch), CCWHT(ch, C_SPR));

	print_attributes_to_buf(ch, buf);

	send_to_char(ch, "%s", buf);
	sprintf(buf,
		"\r\n%s*******************************************************************************%s\r\n",
		CCBLU(ch, C_SPR), CCWHT(ch, C_SPR));
	send_to_char(ch, "%s", buf);
}

ACMD(do_consider)
{
	struct Creature *victim;
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
		send_to_char(ch, "Well, if you really want to kill another player...\r\n");
	}
	diff = victim->getLevelBonus(true) - ch->getLevelBonus(true);
	//diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

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
		send_to_char(ch, "You must be out of your freakin mind.\r\n");
	else if (diff <= 30)
		send_to_char(ch, "What?? Are you STUPID or something?!!\r\n");
	else
		send_to_char(ch, "Would you like to borrow a cross and a shovel for your grave?\r\n");

	if (GET_SKILL(ch, SKILL_CONSIDER) > 70) {
		diff = (GET_MAX_HIT(victim) - GET_MAX_HIT(ch));
		if (diff <= -300)
			act("$E looks puny, and weak.", FALSE, ch, 0, victim, TO_CHAR);
		else if (diff <= -200)
			act("$E would die ten times before you would be killed.", FALSE,
				ch, 0, victim, TO_CHAR);
		else if (diff <= -100)
			act("You could beat $M to death with your forehead.", FALSE, ch, 0,
				victim, TO_CHAR);
		else if (diff <= -50)
			act("$E can take almost as much as you.", FALSE, ch, 0, victim,
				TO_CHAR);
		else if (diff <= 50)
			send_to_char(ch, "You can both take pretty much the same abuse.\r\n");
		else if (diff <= 200)
			act("$E looks like $E could take a lickin.", FALSE, ch, 0, victim,
				TO_CHAR);
		else if (diff <= 600)
			act("Haven't you seen $M breaking bricks on $S head?", FALSE, ch,
				0, victim, TO_CHAR);
		else if (diff <= 900)
			act("You would bet $E eats high voltage cable for breakfast.",
				FALSE, ch, 0, victim, TO_CHAR);
		else if (diff <= 1200)
			act("$E probably isn't very scared of bulldozers.", FALSE, ch, 0,
				victim, TO_CHAR);
		else if (diff <= 1800)
			act("A blow from a house-sized meteor MIGHT do $M in.", FALSE, ch,
				0, victim, TO_CHAR);
		else
			act("Maybe if you threw $N into the sun...", FALSE, ch, 0, victim,
				TO_CHAR);

		ac = GET_AC(victim);
		if (ac <= -100)
			act("$E makes battleships look silly.", FALSE, ch, 0, victim,
				TO_CHAR);
		else if (ac <= -50)
			act("$E is about as defensible as a boulder.", FALSE, ch, 0,
				victim, TO_CHAR);
		else if (ac <= 0)
			act("$E has better defenses than most small cars.", FALSE, ch, 0,
				victim, TO_CHAR);
		else if (ac <= 50)
			act("$S defenses are pretty damn good.", FALSE, ch, 0, victim,
				TO_CHAR);
		else if (ac <= 70)
			act("$S body appears to be well protected.", FALSE, ch, 0, victim,
				TO_CHAR);
		else if (ac <= 90)
			act("Well, $E's better off than a naked person.", FALSE, ch, 0,
				victim, TO_CHAR);
		else
			act("$S armor SUCKS!", FALSE, ch, 0, victim, TO_CHAR);
	}
}



ACMD(do_diagnose)
{
	struct Creature *vict;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_room_vis(ch, buf))) {
			send_to_char(ch, NOPERSON);
			return;
		} else
			diag_char_to_char(vict, ch);
	} else {
		if (ch->numCombatants())
			diag_char_to_char(ch->findRandomCombat(), ch);
		else
			send_to_char(ch, "Diagnose who?\r\n");
	}
}

ACMD(do_pkiller)
{
	if (IS_NPC(ch))
		return;

	char *arg = tmp_getword(&argument);

	if( *arg ) {
		if( strcasecmp(arg,"on") == 0 ) {
            if (GET_REPUTATION(ch) <= 0) {
                arg = tmp_getword(&argument);
                if (strcasecmp(arg, "yes")) {
                    send_to_char(ch, "Your reputation is 0.  You must type pk on yes "
                                     "to enter the world of PK.\r\n");
                    return;
                }

                ch->gain_reputation(5);
            }
			SET_BIT(PRF2_FLAGS(ch), PRF2_PKILLER);
		} else if( strcasecmp(arg,"off") == 0 ) {
			REMOVE_BIT(PRF2_FLAGS(ch), PRF2_PKILLER);
		} else {
			send_to_char(ch, "Usage: pkiller { Off | On [yes]}\r\n");
			return;
		}
	}

	const char *color = CCCYN(ch, C_NRM);
	if( PRF2_FLAGGED(ch, PRF2_PKILLER) )
		color = CCRED(ch, C_SPR);

	send_to_char(ch, "Your current pkiller status is: %s%s%s\r\n",
		color, ONOFF(PRF2_FLAGGED(ch, PRF2_PKILLER)), CCNRM(ch, C_NRM));
}


ACMD(do_compact)
{
	int tp;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Your current compact level is %s.\r\n",
			compact_levels[ch->account->get_compact_level()]);
		return;
	}
	if (((tp = search_block(arg, compact_levels, FALSE)) == -1)) {
		send_to_char(ch, "Usage: compact { off | minimal | partial | full }\r\n");
		return;
	}
	ch->account->set_compact_level(tp);

	send_to_char(ch, "Your %scompact setting%s is now %s%s%s%s.\r\n", CCRED(ch, C_SPR),
		CCNRM(ch, C_OFF), CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), compact_levels[tp],
		CCNRM(ch, C_NRM));
}

ACMD(do_color)
{
	int tp;

	if (IS_NPC(ch))
		return;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Your current color level is %s.\r\n",
			strlist_aref(COLOR_LEV(ch), ansi_levels));
		return;
	}
	if (((tp = search_block(tmp_tolower(arg), ansi_levels, false)) == -1)) {
		send_to_char(ch, "Usage: color { none | sparse | normal | complete }\r\n");
		return;
	}
	ch->account->set_ansi_level(tp);

	send_to_char(ch, "Your color is now %s%s%s%s.\r\n",
		CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), ansi_levels[tp], CCNRM(ch, C_NRM));
}


void
show_all_toggles(Creature *ch)
{
	if (IS_NPC(ch))
		return;
	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "OFF");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

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
		"  Screen Length: %-3d    "
		"     Notrailers: %-3s    "
		"    Color Level: %s\r\n\r\n"
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
		"   Clan Channel: %-3s\r\n"
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
		"      Anonymous: %-3s\r\n"
		"        PKILLER: %-3s\r\n"
		,
		ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
		ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
		ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
		ONOFF(PRF2_FLAGGED(ch, PRF2_AUTO_DIAGNOSE)),
		YESNO(PRF2_FLAGGED(ch, PRF2_AUTOPROMPT)),
		ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
        (IS_NPC(ch) ? "mob" :
		compact_levels[ch->account->get_compact_level()]),
		YESNO(!PRF_FLAGGED(ch, PRF_GAGMISS)),
		GET_PAGE_LENGTH(ch),
		ONOFF(PRF2_FLAGGED(ch, PRF2_NOTRAILERS)),
		ansi_levels[COLOR_LEV(ch)],

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

		ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOSPLIT)),
		ONOFF(PRF2_FLAGGED(ch, PRF2_AUTOLOOT)),
		buf2,
		YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
		ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
		YESNO(GET_QUEST(ch)),
		YESNO(!PRF2_FLAGGED(ch, PRF2_NOAFFECTS)),
		YESNO(PRF2_FLAGGED(ch, PRF2_CLAN_HIDE)),
		YESNO(PRF2_FLAGGED(ch, PRF2_ANONYMOUS)),
		YESNO(PRF2_FLAGGED(ch, PRF2_PKILLER)));

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
		do_gen_tog(ch, NULL, cmd, SCMD_NOGOSSIP, 0);
	else if (is_abbrev(arg, "spew"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOSPEW, 0);
	else if (is_abbrev(arg, "guild"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOGUILDSAY, 0);
	else if (is_abbrev(arg, "clan"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOCLANSAY, 0);
	else if (is_abbrev(arg, "sing"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOMUSIC, 0);
	else if (is_abbrev(arg, "auction"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOAUCTION, 0);
	else if (is_abbrev(arg, "grats"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOGRATZ, 0);
	else if (is_abbrev(arg, "newbie"))
		do_gen_tog(ch, NULL, cmd, SCMD_NEWBIE_HELP, 0);
	else if (is_abbrev(arg, "dream"))
		do_gen_tog(ch, NULL, cmd, SCMD_NODREAM, 0);
	else if (is_abbrev(arg, "project"))
		do_gen_tog(ch, NULL, cmd, SCMD_NOPROJECT, 0);
	else if (is_abbrev(arg, "brief"))
		do_gen_tog(ch, NULL, cmd, SCMD_BRIEF, 0);
	else
		send_to_char(ch, "What is it that you want to toggle?\r\n");
	return;
}




ACMD(do_commands)
{
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0, moods = 0, level = 0;
	struct Creature *vict = NULL;

	one_argument(argument, arg);

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
		socials ? "socials" : moods ? "moods":"commands",
		(vict && vict == ch) ? "you" : vict ? GET_NAME(vict) : "that level");

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
		i = cmd_sort_info[cmd_num].sort_pos;
		if (cmd_info[i].minimum_level < 0)
			continue;
		if (level < cmd_info[i].minimum_level)
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
	page_string(ch->desc, buf);

}

ACMD(do_skills)
{
	int parse_player_class(char *arg);
	int parse_char_class(char *arg);
	void show_char_class_skills(struct Creature *ch, int con, int immort,
		int bits);
	void list_skills(struct Creature *ch, int mode, int type);
	int char_class = 0;
	char *arg;

	arg = tmp_getword(&argument);

	if (*arg && is_abbrev(arg , "list")) {
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
		pc_char_class_types[char_class], weap_spec_char_class[char_class].max);
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


ACMD(do_alignment)
{

	char cbuf[MAX_INPUT_LENGTH];



	if (GET_ALIGNMENT(ch) < -300) {
		sprintf(cbuf, "%s", CCRED(ch, C_NRM));
	}

	else if (GET_ALIGNMENT(ch) > 300) {
		sprintf(cbuf, "%s", CCCYN(ch, C_NRM));
	}

	else {
		sprintf(cbuf, "%s", CCYEL(ch, C_NRM));
	}

	send_to_char(ch, "%sYour alignment is%s %s%d%s.\r\n", CCWHT(ch, C_NRM),
		CCNRM(ch, C_NRM), cbuf, GET_ALIGNMENT(ch), CCNRM(ch, C_NRM));


}


void send_wizlist_section_splitter( Creature *ch )
{
    sprintf(buf, 
            "%s    %so~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~%s\r\n",
            buf, CCCYN(ch,C_NRM), CCNRM(ch,C_NRM) );
}

void send_wizlist_section_title( char* name, Creature *ch )
{
    sprintf(buf,
            "%s\r\n\r\n        %s%s%s\r\n", 
            buf,CCYEL(ch,C_NRM), name, CCNRM(ch,C_NRM) );
    send_wizlist_section_splitter(ch);
}

ACMD(do_wizlist)
{
    using namespace Security;
    sprintf(buf,
            "\r\n                %sThe Immortals of TempusMUD\r\n"
			"                %s--------------------------%s\r\n",
             CCBLU(ch,C_NRM),CCBLU_BLD(ch,C_NRM),
			 CCNRM(ch,C_NRM) );

    send_wizlist_section_title("Greater Implementors",ch);
    getGroup("Wizlist_Grimps").sendPublicMemberList(ch, buf);

    send_wizlist_section_title("Administrators",ch);
    getGroup("Wizlist_Admins").sendPublicMemberList(ch, buf,"WizardAdmin");

    send_wizlist_section_title("Directors",ch);
	strcat(buf, "        ");
    getGroup("Wizlist_Foreman").sendPublicMember(ch, buf, "Foreman: ");
	strcat(buf,"   ");
    getGroup("Wizlist_Advisors").sendPublicMember(ch, buf, "Advisor: ");
	strcat(buf,"   ");
    getGroup("Wizlist_Architects").sendPublicMember(ch, buf, "Architect: ");

    send_wizlist_section_title("Builders",ch);
    getGroup("Wizlist_Blders").sendPublicMemberList(ch, buf);

    send_wizlist_section_title("Implementors",ch);
    getGroup("Wizlist_Coders").sendPublicMemberList(ch, buf, "CoderAdmin");

    send_wizlist_section_title("Questors",ch);
    getGroup("Wizlist_Quests").sendPublicMemberList(ch, buf, "QuestorAdmin" );

    send_wizlist_section_title("Elder Gods",ch);
    getGroup("Wizlist_Elders").sendPublicMemberList(ch, buf, "GroupsAdmin" );

    send_wizlist_section_title("Founders",ch);
    getGroup("Wizlist_Founders").sendPublicMemberList(ch, buf);

	strcat(buf, "\r\n\r\n");
	page_string(ch->desc, buf);
}

ACMD(do_areas)
{
    zone_data *zone;
    bool found_one = false;

	acc_string_clear();
    acc_sprintf("%s%s                    --- Areas appropriate for your level ---%s\r\n",
        CCYEL(ch, C_NRM), CCBLD(ch, C_CMP), CCNRM(ch, C_NRM));
    
    for (zone = zone_table;zone;zone = zone->next) {
        if (!zone->public_desc || !zone->min_lvl)
            continue;
        if (IS_IMMORT(ch) ||
                (zone->min_lvl <= GET_LEVEL(ch)
                    && zone->max_lvl >= GET_LEVEL(ch)
                    && zone->min_gen <= GET_REMORT_GEN(ch)
                    && zone->max_gen >= GET_REMORT_GEN(ch))) {
            // name
            acc_strcat((found_one) ? "\r\n":"", CCCYN(ch, C_NRM), 
                        zone->name, CCNRM(ch, C_NRM), "\r\n", NULL );

            // min/max level
            if( zone->min_lvl == 1 && zone->max_lvl == 49 &&
                zone->min_gen == 0 && zone->max_gen == 10 ) {
                acc_strcat("[ All Levels ]\r\n", NULL );
            } else if( zone->min_gen > 0 || zone->max_gen > 0 ) {
                // [ Level 12 Generation 2 to Level 20 Generation 10 ]
                acc_sprintf("[ Level %d Gen %d to Level %d Gen %d ]\r\n", 
                            zone->min_lvl, zone->min_gen,
                            zone->max_lvl, zone->max_gen );
            } else {
                // [ Level 12 to Level 20 ]
                acc_sprintf("[ Level %d to Level %d ]\r\n",
                            zone->min_lvl, zone->max_lvl );
            }
            // desc
            acc_strcat( zone->public_desc, NULL);
            found_one = true;
        }
    }

    if (found_one)
        page_string(ch->desc, acc_get_string());
    else {
        send_to_char(ch, "Bug the immortals about adding zones appropriate for your level!\r\n");
		mudlog(GET_INVIS_LVL(ch), NRM, true,
			"%s (%d:%d) didn't have any appropriate areas listed.",
			GET_NAME(ch), GET_LEVEL(ch), GET_REMORT_GEN(ch));
	}
}
