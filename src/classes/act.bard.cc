//
// File: act.hood.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "char_class.h"
#include "vehicle.h"
#include "materials.h"
#include "fight.h"
#include "guns.h"
#include "bomb.h"
#include "utils.h"
#include "house.h"
#include "vendor.h"
#include "security.h"
#include "clan.h"
#include "specs.h"
#include "flow_room.h"

extern const char *instrument_types[];

char *pad_song(char *lyrics);
void sing_song(Creature *ch, Creature *vict, int songnum);
bool check_instrument(Creature *ch, int songnum);
char *get_instrument_type(int songnum);

// incomplete
/*
    static const int SONG_LULLABY = 352; // puts a room to sleep
    static const int SONG_SONG_SHIELD = 361; // self only, like psi shield
    static const int SONG_HYMN_OF_PEACE = 363; // stops fighting in room, counters req of rage
    static const int SONG_SONG_OF_SILENCE = 364; // Area, disallow speaking, casting. singing
    static const int SONG_SIRENS_SONG = 374; // single target, charm
    static const int SONG_SHATTER = 382; // target; damage persons/objects, penetrate WALL O SOUND
    static const int SONG_PURPLE_HAZE = 385; // area, pauses fighting for short time
    static const int SONG_GHOST_INSTRUMENT = 389; // causes instrument to replay next some played
    static const int SONG_REQUIEM = 395; // allows bard song to affect undead at half potency
*/

// complete
/*
    static const int SONG_INSTANT_AUDIENCE = 346; // conjures an audience, like summon elem
    static const int SONG_WALL_OF_SOUND = 347; // seals an exit, broken by shatter
    static const int SONG_LAMENT_OF_LONGING = 349; // creates a portal to another character
    static const int SONG_MISDIRECTION_MELISMA = 350; // misleads a tracker
    static const int SONG_ARIA_OF_ARMAMENT = 351; // Similar to armor, group spell
    static const int SONG_VERSE_OF_VULNERABILITY = 353; // lowers AC of target
    static const int SONG_EXPOSURE_OVERTURE = 354; // Area affect, causes targets to vis
    static const int SONG_REGALERS_RHAPSODY = 356; // caster and groupies get satiated
    static const int SONG_MELODY_OF_METTLE = 357; // caster and group get con and maxhit
    static const int SONG_LUSTRATION_MELISMA = 358; // caster and group cleansed of blindness, poison, sickness
    static const int SONG_DEFENSE_DITTY = 359; // save spell, psi, psy based on gen
    static const int SONG_ALRONS_ARIA = 360; // singer/group confidence
    static const int SONG_VERSE_OF_VALOR = 362; // self/group increase hitroll
    static const int SONG_DRIFTERS_DITTY = 365; // self/group increases move
    static const int SONG_UNRAVELLING_DIAPASON = 366; //dispel magic
    static const int SONG_CHANT_OF_LIGHT = 368; // group, light and prot_cold
    static const int SONG_ARIA_OF_ASYLUM = 369; // self/group/target up to 25 percent dam reduction
    static const int SONG_WHITE_NOISE = 370; // single target, confusion
    static const int SONG_RHYTHM_OF_RAGE = 371; // self only, berserk, counter = hymn of peace
    static const int SONG_POWER_OVERTURE = 372; // self only, increase strength and hitroll
    static const int SONG_GUIHARIAS_GLORY = 373; // self/group, + damroll
    static const int SONG_SONIC_DISRUPTION = 375; // area, medium damage
    static const int SONG_MIRROR_IMAGE_MELODY = 376; // causes multiple images of the singer
    static const int SONG_CLARIFYING_HARMONIES = 377; // identify
    static const int SONG_UNLADEN_SWALLOW_SONG = 378; // group flight
    static const int SONG_IRRESISTABLE_DANCE = 379; // Target, -hitroll
    static const int SONG_RHYTHM_OF_ALARM = 380; // room affect, notifies the bard of person entering room
    static const int SONG_RHAPSODY_OF_REMEDY = 381; // self/target, heal
    static const int SONG_HOME_SWEET_HOME = 383; // recall
    static const int SONG_WEIGHT_OF_THE_WORLD = 384; // self/group/target, like telekinesis
    static const int SONG_WOUNDING_WHISPERS = 386; // self, like blade barrier
    static const int SONG_DIRGE = 387; // area, high damage, undead only
    static const int SONG_EAGLES_OVERTURE = 388; // self, group, target, increases cha
    static const int SONG_LICHS_LYRICS = 390; // area, only affects living creatures
    static const int SONG_FORTISSIMO = 391; // increases damage of sonic attacks
    static const int SONG_INSIDIOUS_RHYTHM = 392; // target, - int

    static const int SKILL_SCREAM = 672; // damage like psiblast, chance to stun
    static const int SKILL_VENTRILOQUISM = 673; // makes objects talk
    static const int SKILL_TUMBLING = 674; // like uncanny dodge
    static const int SKILL_LINGERING_SONG = 676; // increases duration of song affects
*/
void
sing_song(struct Creature *ch, Creature *vict, struct obj_data *ovict, int songnum)
{
  char *buf, *vbuf;
  struct bard_song *song = &songs[songnum];

  if (!SPELL_FLAGGED(songnum, MAG_BARD)) {
	mlog(Security::ADMINBASIC, LVL_AMBASSADOR, NRM, true,
			"(%d) Not a bard song in sing_song()", songnum);
	return;
  }

  CreatureList::iterator it = ch->in_room->people.begin();
  for (; it != ch->in_room->people.end(); ++it) {
	if (!(*it)->desc || !AWAKE((*it)) ||
		PLR_FLAGGED((*it), PLR_WRITING | PLR_OLC))
	  continue;

	Creature *tch = *it;
       
	if (ovict)
	  vbuf = tmp_sprintf(" to %s", OBJS(ovict, tch));
	else if (!vict)
	  vbuf = "";
	else if (vict == tch && tch != ch)
	  vbuf = " to you";
	else if (vict != ch)
	  vbuf = tmp_sprintf(" to %s", PERS(vict, tch));
    else if (ch == vict && tch != ch) {
        if (ch->player.sex == 1)
            vbuf = " to himself";
        else if (ch->player.sex == 2)
            vbuf = " to herself";
        else
            vbuf = " to itself";
    }
	else
	  vbuf = " to yourself";

	if (song->instrumental) {
	  if (tch == ch)
		buf = tmp_sprintf("You begin to play %s%s.", song->lyrics, vbuf);
	  else
		buf = tmp_sprintf("%s begins playing %s%s.", 
						  PERS(ch, tch),
						  song->lyrics,
						  vbuf);
	} else {
	  if (tch == ch)
		buf = tmp_sprintf("You sing%s, %s\"%s\"%s",
						  vbuf,
						  CCCYN(tch, C_NRM),
						  song->lyrics,
						  CCNRM(tch, C_NRM));
	  else
		buf = tmp_sprintf("%s sings%s, %s\"%s\"%s",
						  PERS(ch, tch),
						  vbuf, 
						  CCCYN(tch, C_NRM), 
						  song->lyrics,
						  CCNRM(tch, C_NRM));
	  buf = pad_song(buf);
	}

	perform_act(buf, ch, NULL, NULL, tch, 0);
  }
}

char *pad_song(char *lyrics)
{
    int counter = 0, pad;
    char *ptr = lyrics;

    while (*ptr != '"') {
        counter++;
        ptr++;
    }
    pad = counter;

    // Subtract some for color codes
    char *sub = tmp_sprintf("\n%s", tmp_pad(' ', counter - 4));
    return tmp_gsub(lyrics, "\n", sub);
};

bool bard_can_charm_more(Creature *ch)
{
    follow_type *cur;
    int count = 0;

    if (!ch->followers)
        return true;

    for (cur = ch->followers;cur;cur = cur->next)
        if (IS_AFFECTED(cur->follower, AFF_CHARM))
            count++;

    return ((GET_CHA(ch) >> 2 - count) > 0);


}

bool check_instrument(Creature *ch, int songnum)
{
    struct obj_data *objs[4];
    short req_type = songs[songnum].type;
    bool found = false;
    int x = 0;
    
    objs[0] = GET_EQ(ch, WEAR_HOLD);
    objs[1] = GET_EQ(ch, WEAR_WIELD);
    objs[2] = GET_EQ(ch, WEAR_WIELD_2);
    objs[3] = NULL;

    if (ch->getLevel() > LVL_AMBASSADOR)
        return true;

    while (objs[x] != NULL) {
        if (GET_OBJ_TYPE(objs[x]) == ITEM_INSTRUMENT &&
            GET_OBJ_VAL(objs[x], 0) == req_type)
            found = true;
        x++;
    }

    return found;
}

char *get_instrument_type(int songnum)
{
    char *type = tmp_strdup(instrument_types[songs[songnum].type]);

    return tmp_tolower(type);
}

ASPELL(song_instant_audience)
{
    // 1205 - 1209
    int limit = MAX(1, (ch->getLevelBonus(SONG_INSTANT_AUDIENCE) / 33));
    struct affected_type af;
    bool success = false;

    extern void add_follower(struct Creature *ch, struct Creature *leader);

    if (ch->in_room->isOpenAir()) {
        send_to_char(ch, "Oh come now!  There's no one up here to hear you!\r\n");
        return;
    }

    for (int i = 0; i <= limit; i++) {
        Creature *member = NULL;

        if (!bard_can_charm_more(ch))
            break;

        success = true;
        int vnum = number(1205, 1209);

        member = read_mobile(vnum);

        if (!member) {
            send_to_char(ch, "Something just went terribly wrong.  Please report.\r\n");
            return;
        }

        float mult = MAX(0.5 , 
                         (float)((ch->getLevelBonus(SONG_INSTANT_AUDIENCE)) * 1.5) / 100);
        
        // tweak them out
        GET_HITROLL(member) = MIN((int)(GET_HITROLL(member) * mult), 60);
        GET_DAMROLL(member) = MIN((int)(GET_DAMROLL(member) * mult), 75);
        GET_MAX_HIT(member) = MIN((int)(GET_MAX_HIT(member) * mult), 30000);
        GET_HIT(member) = GET_MAX_HIT(member);

        char_to_room(member, ch->in_room);
        SET_BIT(MOB_FLAGS(member), MOB_PET);

        if (member->master)
            stop_follower(member);
        
        add_follower(member, ch);

        af.type = SPELL_CHARM;
        af.is_instant = 0;
        af.duration = 5 + (ch->getLevelBonus(SONG_INSTANT_AUDIENCE) >> 2);

        if (CHECK_SKILL(ch, SKILL_LINGERING_SONG) > number(0, 120))
            af.duration = (int)(af.duration * 1.5);

        af.modifier = 0;
        af.location = 0;
        af.bitvector = AFF_CHARM;
        af.level = level;
        af.owner = ch->getIdNum();
        affect_to_char(member, &af);
    }

    if (success) {
        act("You have gathered an audience!", false, ch, NULL, NULL, TO_CHAR);
        act("$n's playing has gathered an audience!", false, ch, NULL, NULL, TO_ROOM);
        gain_skill_prof(ch, SONG_INSTANT_AUDIENCE);
    }
    else {
        act("No one came to hear your music!", false, ch, NULL, NULL, TO_CHAR);
        act("No one came to hear $n's music!!", false, ch, NULL, NULL, TO_ROOM);
        gain_skill_prof(ch, SONG_INSTANT_AUDIENCE);
    }
}

ASPELL(song_exposure_overture)
{
    int prob;
    int percent;

    if (!ch)
        return;

    CreatureList::iterator ci = ch->in_room->people.begin();

    for (; ci != ch->in_room->people.end(); ++ci) {
        const char *to_char = NULL;
        const char *to_vict = NULL;
        const char *to_room = NULL;
        Creature *tch = *ci;

        if (GET_LEVEL(tch) >= LVL_AMBASSADOR)
            continue;

        prob = ((ch->getLevelBonus(SONG_EXPOSURE_OVERTURE) * 3 / 4) + GET_CHA(ch));
        percent = GET_LEVEL(tch) + (GET_CHA(tch) / 2) + number(1, 60);

        if (prob < percent)
            continue;
        
        if (affected_by_spell(tch, SPELL_INVISIBLE)) {
            affect_from_char(tch, SPELL_INVISIBLE);
            to_char = "Your music causes $N to fade into view!";
            to_vict = "$n's music causes you to fade into view!";
            to_room = "$n's music causes $N to fade into view!";
        }

        if (affected_by_spell(tch, SPELL_GREATER_INVIS)) {
            affect_from_char(tch, SPELL_GREATER_INVIS);
            to_char = "Your music causes $N to fade into view!";
            to_vict = "$n's music causes you to fade into view!";
            to_room = "$n's music causes $N to fade into view!";
        }

        if (affected_by_spell(tch, SPELL_TRANSMITTANCE)) {
            affect_from_char(tch, SPELL_TRANSMITTANCE);
            to_char = "$N is no longer transparent!";
            to_vict = "You are no longer transparent!!";
            to_room = "$N is no longer transparent!";
        }

        if (IS_AFFECTED(tch, AFF_HIDE)) {
            REMOVE_BIT(AFF_FLAGS(tch), AFF_HIDE);
            to_char = "$N is no longer hidden!";
            to_vict = "You are no longer hidden!!";
            to_room = "$N is no longer hidden!";
        }

		if (to_char)
            act(to_char, false, ch, NULL, tch, TO_CHAR);
		if (to_room)
            act(to_room, false, ch, NULL, tch, TO_NOTVICT);
		if (to_vict)
            act(to_vict, false, ch, NULL, tch, TO_VICT);
    }

    gain_skill_prof(ch, SONG_EXPOSURE_OVERTURE);
}

ASPELL(song_lament_of_longing)
{
    struct room_data *targ_room = victim->in_room;

    if (ch == NULL || victim == NULL || ch == victim)
        return;

    if (ch->in_room == victim->in_room) {
        act("$N appears to be touched by your longing for $M.",
            false, ch, NULL, victim, TO_CHAR);
        return;
    }

    if (ROOM_FLAGGED(ch->in_room, ROOM_NORECALL)) {
        send_to_char(ch, "A portal tries to form but snaps suddenly out of existence.\r\n");
        act("A shimmering portal appears for a moment but "
            "snaps suddenly out of existence.\r\n", false, ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (IS_NPC(victim)) {
        send_to_char(ch, "Better not sneak up on a monster like that.  "
                     "It might be upset!\r\n");
        return;
    }

    if (victim->in_room->people.size() >= (unsigned)ch->in_room->max_occupancy) {
        send_to_char(ch, "But there is no room for you there!\r\n");
        return;
    }

    if (GET_LEVEL(victim) >= LVL_AMBASSADOR) {
        send_to_char(ch, "No, I don't think so.\r\n");
        return;
    }

    if (ROOM_FLAGGED(victim->in_room, ROOM_CLAN_HOUSE) &&
        !clan_house_can_enter(ch, victim->in_room)) {
        send_to_char(ch, "You are not allowed in that clan house!\r\n");
    }

    if (victim->in_room->zone != ch->in_room->zone &&
        (ZONE_FLAGGED(victim->in_room->zone, ZONE_ISOLATED) ||
         ZONE_FLAGGED(ch->in_room->zone, ZONE_ISOLATED))) {
        act("The place $E exists is completely isolated from this.",
            false, ch, 0, victim, TO_CHAR);
        return;
    }

    if (GET_PLANE(ch->in_room) != GET_PLANE(victim->in_room)) {
        if (GET_PLANE(victim->in_room) != PLANE_ASTRAL) {
            if (number(0, 120) > (CHECK_SKILL(ch, SONG_LAMENT_OF_LONGING) + 
                (GET_CHA(ch) / 2))) {
                if ((targ_room = real_room(number(41100, 41863))) == NULL)
                    targ_room = victim->in_room;
            }
        }
    }

    if (number(0, 225) > (ch->getLevelBonus(SONG_LAMENT_OF_LONGING) +
        CHECK_SKILL(ch, SONG_LAMENT_OF_LONGING))) {
        send_to_char(ch, "A shimmering portal begins to appear but it "
                         "fades along with your music..\r\n");
        act("A shimmering portal begins to appear but it "
            "fades along with $n's music.\r\n", false, ch, NULL, NULL, TO_ROOM);
        return;
    }

    struct obj_data *rift1 = read_object(QUANTUM_RIFT_VNUM);
    struct obj_data *rift2 = read_object(QUANTUM_RIFT_VNUM);

    if (!rift1 || !rift2) {
        errlog("QUANTUM_RIFT_VNUM does not exist?  WTF!");
        send_to_char(ch, "We seem to be having some technical difficulties. "
                     "please report this to the nearest imm.  Thanks.\r\n");
        return;
    }

    GET_OBJ_TIMER(rift1) = 1 + ch->getLevelBonus(SONG_LAMENT_OF_LONGING) >> 5;
    GET_OBJ_TIMER(rift2) = GET_OBJ_TIMER(rift1);

    rift1->line_desc = str_dup("A beautiful, shimmering rift has been opened here.");
    rift2->line_desc = str_dup("A beautiful, shimmering rift has been opened here.");

    GET_OBJ_VAL(rift1, 0) = targ_room->number;
    GET_OBJ_VAL(rift2, 0) = ch->in_room->number;

    obj_to_room(rift1, ch->in_room);
    obj_to_room(rift2, targ_room);

    act("A beautiful rift in space appears, pulsing to the sound of your music!",
         true, ch, NULL, NULL, TO_CHAR);
    act("A beautiful rift in space appears, pulsing to the sound of $n's music!",
         true, ch, NULL, NULL, TO_ROOM);

    if (targ_room == victim->in_room) {
        act("A lonely song suddenly drifts through the air and "
            "a beautiful rift appears before you!",
             true, victim, NULL, NULL, TO_CHAR);
    }

    CreatureList::iterator ci = ch->in_room->people.begin();
    for (; ci != ch->in_room->people.end(); ++ci) {
        if (!IS_NPC((*ci))) {
            WAIT_STATE(ch, 5 RL_SEC);
        }
    }

    gain_skill_prof(ch, SONG_LAMENT_OF_LONGING);
}

ASPELL(song_rhythm_of_alarm)
{
    struct room_affect_data rm_aff;
    struct room_affect_data *rm_aff_ptr;

    memset(&rm_aff, 0x0, sizeof(struct room_affect_data));
    rm_aff.type = RM_AFF_OTHER;

    if ((rm_aff_ptr = room_affected_by(ch->in_room, SONG_RHYTHM_OF_ALARM)) &&
         rm_aff_ptr->owner == ch->getIdNum()) {
        send_to_char(ch, "Your song fades leaving no affect.\r\n");
        return;
    }

    send_to_room("The music summons a large symbol which "
                 "disappears into the ground.\r\n", ch->in_room);

    rm_aff.level = level;
    rm_aff.spell_type = SONG_RHYTHM_OF_ALARM;
    rm_aff.duration = number(1, 100) + (ch->getLevelBonus(SONG_RHYTHM_OF_ALARM) * 2);
    rm_aff.owner = ch->getIdNum();
    affect_to_room(ch->in_room, &rm_aff);

    gain_skill_prof(ch, SONG_RHYTHM_OF_ALARM);
}

ASPELL(song_wall_of_sound)
{
    struct room_affect_data rm_aff;
    struct room_affect_data *rm_aff_ptr, *next_aff;
    const char *dir_str;

    if (!EXIT(ch, *dir)) {
        send_to_char(ch, "There's no exit in that direction!\r\n");
        return;
    }

    if ((rm_aff_ptr = room_affected_by(ch->in_room, SONG_WALL_OF_SOUND)) &&
        rm_aff_ptr->type == *dir) {
        send_to_char(ch, "There is already a wall of sound in that direction!\r\n");
        return;
    }

	for (rm_aff_ptr = ch->in_room->affects; rm_aff_ptr; rm_aff_ptr = next_aff) {
	  next_aff = rm_aff_ptr->next;
	  if (rm_aff_ptr->spell_type == SONG_WALL_OF_SOUND) {
		send_to_room(tmp_sprintf("The wall of sound to the %s falters and fades.\r\n",
								 dirs[(int)rm_aff_ptr->type]),
					 ch->in_room);
		affect_from_room(ch->in_room, rm_aff_ptr);
	  }
	}

    dir_str = dirs[*dir]; 
	if (*dir == FUTURE || *dir == PAST)
	  dir_str = tmp_sprintf("to the %s", dir_str);

    send_to_room(tmp_sprintf("A wall of sound appears, sealing the exit %s!\r\n",
							 dir_str),
				 ch->in_room);

    rm_aff.level = level;
    rm_aff.spell_type = SONG_WALL_OF_SOUND;
    rm_aff.type = *dir;
    rm_aff.duration = number(1, 50) + (ch->getLevelBonus(SONG_WALL_OF_SOUND));
    rm_aff.flags = EX_NOPASS;
    rm_aff.owner = ch->getIdNum();
	rm_aff.description = str_dup(tmp_sprintf("A wall of sound appears, sealing the exit %s.\r\n",
											 dir_str));
    affect_to_room(ch->in_room, &rm_aff);

    gain_skill_prof(ch, SONG_WALL_OF_SOUND);
}

ASPELL(song_hymn_of_peace)
{
    
    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        (*it)->removeAllCombat();
        mag_unaffects(level, ch, *it, SONG_HYMN_OF_PEACE, 0);
    }

    send_to_char(ch, "Your song brings a feeling of peacefulness.\r\n");
    act("A feeling of peacefulness is heralded by $n's song.", FALSE, ch, 0, 0, TO_ROOM);

    gain_skill_prof(ch, SONG_HYMN_OF_PEACE);
}

ACMD(do_ventriloquize)
{
    char *obj_str = NULL;
    char *target_str = NULL;
    struct obj_data *obj = NULL;
    Creature *target = NULL;
    struct obj_data *otarget = NULL;

    if (CHECK_SKILL(ch, SKILL_VENTRILOQUISM) < 60) {
        send_to_char(ch, "You really have no idea how.\r\n");
        return;
    }

    obj_str = tmp_getword(&argument);
    skip_spaces(&argument);

    obj = get_obj_in_list_vis(ch, obj_str, ch->carrying);

    if (!obj) {
        obj = get_obj_in_list_vis(ch, obj_str, ch->in_room->contents);
    }

    if (!obj) {
        send_to_char(ch, "Sorry friend, I can't find that object.\r\n");
        return;
    }

    if (*argument == '>') {
        target_str = tmp_getword(&argument);
        skip_spaces(&argument);
        target_str++;
    }

    if (target_str) {
        target = get_char_room_vis(ch, target_str);
        
        if (!target) {
            otarget = get_obj_in_list_vis(ch, target_str, ch->carrying);

            if (!otarget) {
                otarget = get_obj_in_list_vis(ch, target_str, ch->in_room->contents);
            }
        }

        if (!target && !otarget) {
            send_to_char(ch, "I can't find the target of your ventriloquization!\r\n");
            return;
        }
    }

    if (!*argument) {
        send_to_char(ch, "Yes!  But what you do want it to say?!\r\n");
        return;
    }

    CreatureList::iterator ci = ch->in_room->people.begin();
    for (; ci != ch->in_room->people.end(); ++ci) {
        if (target) {
            if (target == *ci)
                target_str = tmp_strdup(" to you");
            else
                target_str = tmp_sprintf(" to %s", GET_NAME(target));
        }
        else if (otarget) {
            if (otarget == obj)
                target_str = tmp_strdup(" to itself");
            else
                target_str = tmp_sprintf(" to %s", otarget->name);
        }
        else
            target_str = NULL;
        
        send_to_char(*ci, "%s%s says%s,%s%s \'%s\'%s\r\n",
                     CCBLU_BLD(ch, C_NRM), 
                     tmp_capitalize(obj->name),
                     (target_str ? target_str : ""), 
                     CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), argument, 
                     CCNRM(ch, C_NRM)); 
    }

    gain_skill_prof(ch, SKILL_VENTRILOQUISM);
}
