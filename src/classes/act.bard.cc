//
// File: act.hood.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

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

static const int TOP_BARD_SONG = 345;
static const int NUM_BARD_SONGS = 51;

struct bard_song {
    char *name;
    char *lyrics; // The lyrics, if any
    bool instrumental; // is it an instrumental
    int type; // what type of instrument do we need?
};

struct bard_song songs[] = {
    { "Instance Audience",
      "I must have an audience before I can play,\nPlease give me some listeners to hear me today.", // 346
      false,
      ITEM_PERCUSSION 
    },
    { "Wall of Sound", // 347
      "a loud and boisterous tune",
      true,
      ITEM_WIND
    },
    { "Ventriloquism (error)", // 348
      "(error)",
      false,
      -1 
    },
    { "Lament of Longing", // 349
      "a mild and melancholy tune",
      true,
      ITEM_WIND
    },
    { "Misdirection Melisma", // 350
      "",
      false,
      ITEM_STRING
    },
    { "Aria of Amament",
      "To arms!  To arms!  The battle doth call!\nMay the light aid us and protect us all!", // 351
      false,
      ITEM_STRING
    },
    { "Lullaby", // 352
      "",
      false,
      ITEM_STRING
    },
    { "Verse of Vulnerability",
      "Our foe doth approach, his essence is near,\nBlight him and smite him, his weakness make clear!", // 353
      false,
      ITEM_STRING
    },
    { "Exposure Overture", // 354
      "a light and revealing tune",
      true,
      ITEM_WIND
    },
    { "Verse of Vibration", // 355
      "",
      false,
      ITEM_PERCUSSION
    },
    { "Regalers Rhapsody",
      "Think of the feasts our stomache's abated,\nRemember them fondly and soon we are sated.", // 356
      false,
      ITEM_STRING
    },
    { "Melody of Mettle",
      "Our weapons are keen, our shields are strong,\nOur mettle is heavy to bear us along.", // 357
      false,
      ITEM_PERCUSSION
    },
    { "Lustration Melisma",
      "Bring forth the cleansing the body must gain,\nAnd rid us of illness that's causing pain.", // 358
      false,
      ITEM_STRING
    },
    { "Defense Ditty", // 359
      "a spirited march",
      true,
      ITEM_WIND
    },
    { "Alron's Aria", // 360
      "I sing of the Knights of Alron, they're bold, pure and brave!\nTheir might in the heat of battle is what my group does crave.",
      false,
      ITEM_STRING
    },
    { "Song Shield", // 361
      "a light, comforting melody",
      true,
      ITEM_WIND
    },
    { "Verse of Valor",
      "To help us strike hard and help us strike true,\nWe sing of the heros whose valor is true.", // 362
      false,
      ITEM_PERCUSSION
    },
    { "Hymn of Peace", // 363
      "a cool and peaceful hymn",
      false,
      ITEM_WIND
    },
    { "Song of Silence", // 364
      "a quiet and doleful song",
      true,
      ITEM_WIND
    },
    { "Drifters Ditty",
      "We are tired and so full of woes,\nMay we find puissance to defend against foes!", // 365
      false,
      ITEM_PERCUSSION
    },
    { "Unravelling Diapason", // 366
      "Unwind this magic and pull it like string,\nDisperse it from sight as this song I sing.",
      false,
      ITEM_STRING
    },
    { "Rhapsody of Depression", // 367
      "a depressing rhapsody",
      true,
      ITEM_WIND
    },
    { "Chant of Light",
      "Oh cold of night, come into the light,\nFeed me the rapture of warm delight.", // 368
      false,
      ITEM_PERCUSSION
    },
    { "Aria of Asylum", // 369
      "No fortress can hide, no army provide,\nLike the comfort of Asylum!",
      false,
      ITEM_STRING
    },
    { "White Noise", // 370
      "a cacophony of sound",
      true,
      ITEM_WIND
    },
    { "Rhythm of Rage", // 371
      "an intense an violent rhythm",
      true,
      ITEM_PERCUSSION
    },
    { "Power Overture",
      "Guiharia, Zhel, and Velroth, Selene,\nAlron we seek your power to glean.", // 372
      false,
      ITEM_PERCUSSION
    },
    { "Guiharia's Glory",
      "Guiharia's Glory, her essense so pure,\nHer soul, it renews me, my heart can endure.", // 373
      false,
      ITEM_PERCUSSION
    },
    { "Siren's Song", // 374
      "an alluring melody",
      true,
      ITEM_WIND
    },
    { "Sonic Disruption", // 375
      "I'll bread the world with my new song,\nYour body will break, so dance along.",
      false,
      ITEM_PERCUSSION
    },
    { "Mirror Image Melody", // 376
      "an off-beat, confusing tune",
      true,
      ITEM_WIND
    },
    { "Clarifying Harmonies",
      "Curiosity drives me to know what I must,\nLeaving my ignorance dead in the dust.", // 377
      false,
      ITEM_STRING
    },
    { "Unladen Swallow Song", // 378
      "The flight of the swallow unburdened is high,\nThey lighten us with wings that beat out a sigh.",
      false,
      ITEM_PERCUSSION
    },
    { "Irresistable Dance", // 379
      "Come dance to my tune, your senses ensnared,\nYou may never stop and will never care.",
      false,
      ITEM_PERCUSSION
    },
    { "Rhythm of Alarm", // 380
      "Warn me, protect me, by singing this song,\nAlert me should anyone come along.",
      false,
      ITEM_PERCUSSION
    },
    { "Rhapsody of Remedy", // 381
      "Your wounds are deep, your scars are true,\nMy song can restore you through and through.",
      false,
      ITEM_STRING
    },
    { "Shatter", // 382
      "",
      false,
      ITEM_PERCUSSION
    },
    { "Home Sweet Home",
      "I'm on my way, just set me free,\nHome, sweet home.", // 383
      false,
      ITEM_STRING
    },
    { "Weight of the World", // 384
      "The weight on my shoulders is heavy and cumbrous,\nWe'll lighten the load with the light of this chorus.",
      false,
      ITEM_PERCUSSION
    },
    { "Purple Haze", // 385
      "Purple Haze was in my eyes.  Don't know if it's day or night.\nYou've got me blowing, blowing my mind.  Is it tomorrow or just the end of time?",
      false,
      ITEM_STRING
    },
    { "Wounding Whispers", // 386
      "So soft, so close, so sharp and so free,\nThe Whispers of Wounding, they gather 'round me.",
      false,
      ITEM_PERCUSSION
    },
    { "Dirge",
      "a slow and mournful tune", // 387
      true,
      ITEM_WIND
    },
    { "Eagle's Overture",
      "a loud and boisterous tune", // 388
      true,
      ITEM_WIND
    },
    { "Ghost Instrument", // 389
      "an ethereal,haunting song",
      true,
      ITEM_WIND
    },
    { "Lich's Lyrics", // 390
      "",
      false,
      ITEM_PERCUSSION
    },
    { "Fortissimo",
      "an upbeat tune that swells to a deafening pitch", // 391
      true,
      ITEM_WIND
    },
    { "Insidious Rythm", // 392
      "an insidious, bewitching rythm",
      true,
      ITEM_WIND
    },
    { "Tumbling (error)", // 393
      "(error)",
      false,
      -1
    },
    { "Lingering Song (error)", // 394
      "(error)",
      false,
      -1
    },
    { "Requiem", // 395
      "a dark and joyless requiem",
      true,
      ITEM_WIND
    },
    { "Scream (error)", // 396
      "(error)",
      false,
      -1
    }
};

// incomplete
/*
    static const int SKILL_VENTRILOQUISM = 673; // makes objects talk
    static const int SONG_WALL_OF_SOUND = 347; // seals an exit, broken by shatter
    static const int SONG_MISDIRECTION_MELISMA = 350; // misleads a tracker
    static const int SONG_LULLABY = 352; // puts a room to sleep
    static const int SONG_VERSE_OF_VIBRATION = 355; // motor spasm++
    static const int SONG_SONG_SHIELD = 361; // self only, like psi shield
    static const int SONG_HYMN_OF_PEACE = 363; // stops fighting in room, counters req of rage
    static const int SONG_SONG_OF_SILENCE = 364; // Area, disallow speaking, casting. singing
    static const int SONG_RHAPSODY_OF_DEPRESSION = 367; // slow a target
    static const int SONG_SIRENS_SONG = 374; // single target, charm
    static const int SONG_SONIC_DISRUPTION = 375; // area, medium damage
    static const int SONG_MIRROR_IMAGE_MELODY = 376; // causes multiple images of the singer
    static const int SONG_SHATTER = 382; // target; damage persons/objects, penetrate WALL O SOUND
    static const int SONG_PURPLE_HAZE = 385; // area, pauses fighting for short time
    static const int SONG_DIRGE = 387; // area, high damage, undead only
    static const int SONG_GHOST_INSTRUMENT = 389; // causes instrument to replay next some played
    static const int SONG_LICHS_LYRICS = 390; // area, only affects living creatures
    static const int SONG_FORTISSIMO = 391; // increases damage of sonic attacks
    static const int SONG_REQUIEM = 395; // allows bard song to affect undead at half potency
*/

// complete
/*
    static const int SONG_INSTANT_AUDIENCE = 346; // conjures an audience, like summon elem
    static const int SONG_LAMENT_OF_LONGING = 349; // creates a portal to another character
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
    static const int SONG_CLARIFYING_HARMONIES = 377; // identify
    static const int SONG_UNLADEN_SWALLOW_SONG = 378; // group flight
    static const int SONG_IRRESISTABLE_DANCE = 379; // Target, -hitroll
    static const int SONG_RHYTHM_OF_ALARM = 380; // room affect, notifies the bard of person entering room
    static const int SONG_RHAPSODY_OF_REMEDY = 381; // self/target, heal
    static const int SONG_HOME_SWEET_HOME = 383; // recall
    static const int SONG_WEIGHT_OF_THE_WORLD = 384; // self/group/target, like telekinesis
    static const int SONG_WOUNDING_WHISPERS = 386; // self, like blade barrier
    static const int SONG_EAGLES_OVERTURE = 388; // self, group, target, increases cha
    static const int SONG_INSIDIOUS_RHYTHM = 392; // target, - int

    static const int SKILL_SCREAM = 672; // damage like psiblast, chance to stun
    static const int SKILL_TUMBLING = 674; // like uncanny dodge
    static const int SKILL_LINGERING_SONG = 676; // increases duration of song affects
*/
void
sing_song(struct Creature *ch, Creature *vict, struct obj_data *ovict, int songnum)
{
    char *buf, *vbuf;
    struct bard_song *song = &songs[songnum - TOP_BARD_SONG - 1];

    if (songnum < TOP_BARD_SONG || songnum > TOP_BARD_SONG + NUM_BARD_SONGS) {
        char *errbuf = tmp_sprintf("SYSERR: (%d) Not a bard song in sing_song()", songnum);
        mlog(Security::ADMINBASIC, LVL_AMBASSADOR, NRM, true, errbuf);
        return;
    }

    CreatureList::iterator it = ch->in_room->people.begin();
    for (; it != ch->in_room->people.end(); ++it) {
        if (!(*it)->desc || !AWAKE((*it)) ||
            PLR_FLAGGED((*it), PLR_WRITING | PLR_OLC))
            continue;

        Creature *tch = *it;
        
        if (tch == ch) {
            if (vict && vict != ch && (vict->in_room == ch->in_room))
                vbuf = tmp_sprintf( " to %s", GET_NAME(vict));
            else if (ovict)
                vbuf = tmp_sprintf(" to %s", ovict->name);
            else
                vbuf = "";

            if (!song->instrumental) {
                buf = tmp_sprintf("You sing%s, %s\"%s\"%s", vbuf, CCCYN(ch, C_NRM),
                                   song->lyrics, CCNRM(ch, C_NRM));
                buf = pad_song(buf);
            }
            else  {
                buf = tmp_sprintf("You begin to play %s%s.", song->lyrics, vbuf);
            }
        }
        else {
            if (vict && vict == tch)
                vbuf = tmp_strdup(" to you");
            else if (vict && vict != ch)
                vbuf = tmp_sprintf(" to %s", GET_NAME(vict));
            else if (ovict)
                vbuf = tmp_sprintf(" to %s", ovict->name);
            else
                vbuf = "";

            if (!song->instrumental) {
                buf = tmp_sprintf("%s sings%s, %s\"%s\"%s", GET_NAME(ch), vbuf, 
                                  CCCYN(ch, C_NRM), song->lyrics, CCNRM(ch, C_NRM));
                buf = pad_song(buf);
            }
            else {
                buf = tmp_sprintf("%s begins playing %s%s.", GET_NAME(ch), song->lyrics, vbuf);
            }
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
    short req_type = songs[songnum - TOP_BARD_SONG - 1].type;
    bool found = false;
    int x = 0;
    
    objs[0] = GET_EQ(ch, WEAR_HOLD);
    objs[1] = GET_EQ(ch, WEAR_WIELD);
    objs[2] = GET_EQ(ch, WEAR_WIELD_2);
    objs[3] = NULL;

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
    char *type = tmp_strdup(instrument_types[songs[songnum - TOP_BARD_SONG - 1].type]);

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
    char *to_char;
    char *to_vict;
    char *to_room;
    bool found = false;

    if (!ch)
        return;

    CreatureList::iterator ci = ch->in_room->people.begin();

    for (; ci != ch->in_room->people.end(); ++ci) {
        Creature *tch = *ci;

        if (GET_LEVEL(tch) >= LVL_AMBASSADOR)
            continue;

        prob = ((ch->getLevelBonus(SONG_EXPOSURE_OVERTURE) >> 1) + GET_INT(ch));
        percent =  GET_LEVEL(tch) + (GET_INT(tch) / 2) + number(1, 30);

        if (prob < percent)
            continue;
        
        found = false;
        if (affected_by_spell(tch, SPELL_INVISIBLE)) {
            found = true;
            affect_from_char(tch, SPELL_INVISIBLE);
            to_char = tmp_sprintf("Your music causes $N to fade into view!");
            to_vict = tmp_sprintf("$n's music causes you to fade into view!");
            to_room = tmp_sprintf("$n's music causes $N to fade into view!");
        }

        if (affected_by_spell(tch, SPELL_GREATER_INVIS)) {
            found = true;
            affect_from_char(tch, SPELL_GREATER_INVIS);
            to_char = tmp_sprintf("Your music causes $N to fade into view!");
            to_vict = tmp_sprintf("$n's music causes you to fade into view!");
            to_room = tmp_sprintf("$n's music causes $N to fade into view!");
        }

        if (affected_by_spell(tch, SPELL_TRANSMITTANCE)) {
            found = true;
            affect_from_char(tch, SPELL_TRANSMITTANCE);
            to_char = tmp_sprintf("$N is no longer transparent!");
            to_vict = tmp_sprintf("You are no longer transparent!!");
            to_room = tmp_sprintf("$N is no longer transparent!");
        }

        if (IS_AFFECTED(tch, AFF_HIDE)) {
            found = true;
            REMOVE_BIT(AFF_FLAGS(tch), AFF_HIDE);
            to_char = tmp_sprintf("$N is no longer hidden!");
            to_vict = tmp_sprintf("You are no longer hidden!!");
            to_room = tmp_sprintf("$N is no longer hidden!");
        }

        if (found) {
            act(to_char, false, ch, NULL, tch, TO_CHAR);
            act(to_room, false, ch, NULL, tch, TO_NOTVICT);
            act(to_vict, false, ch, NULL, tch, TO_VICT);
        }
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
                (GET_INT(ch) / 2))) {
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
        errlog("SYSERR: QUANTUM_RIFT_VNUM does not exist?  WTF!");
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

    act("A beautiful rift in space appears, pulsing to to sound of your music!",
         true, ch, NULL, NULL, TO_CHAR);
    act("A beautiful rift in space appears, pulsing to to sound of $n's music!",
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
    rm_aff.type = -1;

    if ((rm_aff_ptr = room_affected_by(ch->in_room, SONG_RHYTHM_OF_ALARM)) &&
         rm_aff_ptr->owner == ch) {
        send_to_char(ch, "Your song fades leaving no affect.\r\n");
        return;
    }

    send_to_room("The music summons a large symbol which "
                 "dissapears into the ground.\r\n", ch->in_room);

    rm_aff.level = level;
    rm_aff.spell_type = SONG_RHYTHM_OF_ALARM;
    rm_aff.duration = number(1, 100) + (ch->getLevelBonus(SONG_RHYTHM_OF_ALARM) * 2);
    rm_aff.owner = ch;
    affect_to_room(ch->in_room, &rm_aff);

}
