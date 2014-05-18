#ifdef HAS_CONFIG_H
#endif

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <glib.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "tmpstr.h"
#include "spells.h"
#include "obj_data.h"
#include "strutil.h"

/*
 * Templates:
 *   #c - alias of creature in room or self
 *   #o - alias of object in room, inventory, worn, or implanted
 *   #a - alteration name
 *   #t - trigger name
 *   #s - spell name
 *   #g - song name
 *   #d - direction
 *   #p - body position
 */
const char *cmd_templates[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "future",
    "past",
    "areas",
    "attributes",
    "attack",
    "attack #c",
    "attach #o #o",
    "alias",
    "alignment",
    "alter '#a'",
    "alter '#a' #o",
    "alter '#a' #o",
    "alterations",
    "activate #o",
    "activate internal #o",
    "analyze",
    "anonymous",
    "appraise #o",
    "assist #c",
    "assemble #o",
    "assimilate #o",
    "autodiagnose",
    "autoexits",
    "autoloot",
    "autopage",
    "autoprompt",
    "autopsy",
    "autopsy #o",
    "autosplit",
    "affects",
    "afk",

    "backstab #c",
    "bandage #c",
    "balance",
    "bash #d",
    "battlecry",
    "beguile #c",
    "berserk",
    "bearhug #c",
    "behead #c",
    "beeper",
    "bell",
    "bioscan #c",
    "bid",
    "bidlist",
    "bidstat",
    "bite #c",
    "board",
    "bodyslam #c",
    "break #o",
    "breathe #c",
    "brief",
    "buy",

    "cast '#s'",
    "cast '#s' #o",
    "cast '#s' #c",
    "cash",
    "charge #c",
    "check",
    "choke #c",
    "crossface #c",
    "cleave #c",
    "ceasefire",
    "cities",
    "claw #c",
    "clothesline #c",
    "clean #o",
    "clean #c #p",
    "clear",
    "close #d",
    "combine #o #o",
    "combo #c",
    "commands",
    "compact",
    "compare #o #o",
    "consider #c",
    "conceal #o",
    "convert #o",
    "corner #c",
    "crawl #d",
    "cranekick #c",
    "cry_from_beyond",
    "cyberscan #c",

    "deactivate #o",
    "deactivate internal #o",
    "deassimilate",
    "deathtouch #c",
    "defend #c",
    "defuse #o",
    "detach #o #o",
    "de-energize",
    "diagnose",
    "disguise #c",
    "discharge #c",
    "dismount",
    "disarm #c",
    "disembark",
    "dodge",
    "donate #o",
    "drag #o",
    "drive",
    "drink #o",
    "drop #o",

    "eat #o",
    "econvert #o",
    "elbow #c",
    "elude",
    "empower",
    "empty #o",
    "enter #o",
    "encumbrance",
    "equipment",
    "equipment all",
    "equipment stat",
    "evade",
    "evaluate",
    "exit",
    "exits",
    "examine #o",
    "examine #c",
    "exchange",
    "experience",
    "extinguish #o",
    "extinguish #c",
    "extract",
    "extract self #o #i",
    "extract #c #o #i",

    "fill #o #o",
    "firstaid #c",
    "fight #c",
    "feed #c",
    "feign",
    "flee",
    "fly",
    "flip",
    "flip coin",
    "follow #c",

    "get #o",
    "get #o #o",
    "get all #o",
    "gagmiss",
    "gain",
    "garrote #c",
    "gasify",
    "give #o #c",
    "glance #c",
    "gold",
    "gore #c",
    "gouge #c",
    "group",
    "grab #o",
    "groinkick #c",
    "group all"
    "guilddonate #o",

    "help",
    "hack #d",
    "headbutt #c",
    "headlights",
    "hide",
    "hit #c",
    "hiptoss #c",
    "hamstring #c",
    "hold #o",
    "honk",
    "hook #c",
    "house",
    "holytouch #c",
    "hotwire",

    "inventory",
    "increase hit",
    "increase mana",
    "increase move",
    "infiltrate",
    "inject #o",
    "ignite #o",
    "intimidate #c",
    "imbibe #o",
    "impale #c",
    "implants",
    "implant self #o #i",
    "implant #c #o #i",
    "improve str",
    "improve con",
    "improve wis",
    "improve cha",
    "improve dex",
    "improve int",
    "info",
    "insert #o #c",
    "insult #c",

    "jab #c",
    "join #o",
    "jump #d",
    "junk #o",

    "kill #c",
    "kia",
    "kata",
    "kick #c",
    "kneethrust #c",
    "knock #d",

    "look",
    "look #c",
    "look #o",
    "look in #o",
    "load #o",
    "leave",
    "lecture #c",
    "levitate",
    "light #o",
    "list",
    "listen",
    "lock #d",
    "lungepunch #c",

    "medic #c",
    "meditate",
    "mount #c",
    "murder #c",

    "nasty",
    "nervepinch alpha #c",
    "nervepinch beta #c",
    "nervepinch gamma #c",
    "nervepinch delta #c",
    "nervepinch epsilon #c",
    "nervepinch omega #c",
    "nervepinch zeta #c",

    "open #d",
    "overhaul #c",
    "overdrain #c",

    "pack #o #o",
    "palmstrike #c",
    "pelekick #c",
    "perform '#g'",
    "perform '#g' #c",
    "perform '#g' #o",
    "place #o #o",
    "pry",
    "put #o #o",
    "pull",
    "push",
    "pick #o",
    "pick #o #o",
    "piledrive #c",
    "pistolwhip #c",
    "plant #o #c",
    "point",
    "point #c",
    "point #o",
    "point #d",
    "pour #o",
    "psiblast #c",
    "psidrain #c",
    "psilocate #c",
    "punch #c",

    "quaff #o",
    "qpoints",

    "repair #o",
    "rest",
    "read #o",
    "reboot",
    "rabbitpunch #c",
    "refill #o #o",
    "recite #o",
    "recite #o #c",
    "recite #o #o",
    "recharge #o #o",
    "reconfigure hit",
    "reconfigure mana",
    "reconfigure move",
    "remove #o",
    "report",
    "rescue",
    "retreat #d",
    "return",
    "recorporealize",
    "retrieve",
    "rev",
    "reset",
    "reward",
    "ridgehand #c",
    "ring #o",
    "roll #o",
    "roundhouse #c",
    "rules",

    "sacrifice #o",
    "scan",
    "scissorkick #c",
    "score",
    "scream #c",
    "sell #o",
    "selfdestruct",
    "shoulderthrow #c",
    "shower",
    "shoot #o",
    "shutoff #o",
    "shutoff internal #o",
    "sidekick #c",
    "sip #o",
    "sit",
    "skills",
    "sleep",
    "sleeper #c",
    "slist",
    "sling #o #c",
    "sling #o #d",
    "slay #c",
    "smoke #o",
    "snatch #o #c",
    "sneak",
    "snipe #c",
    "socials",
    "soilage",
    "specializations",
    "spells",
    "spinfist #c",
    "spinkick #c",
    "stand",
    "status",
    "stalk #c",
    "steal #o #c",
    "strike #c",
    "stomp #c",
    "store",
    "stun #c",
    "swallow",
    "sweepkick #c",

    "tempus",
    "tester",
    "take #o",
    "taste #o",
    "tattoos",
    "tag",
    "throatstrike #c",
    "throw #o #c",
    "throw #o #d",
    "title",
    "time",
    "toggle",
    "toke",
    "tornado #c",
    "track",
    "train",
    "trample",
    "translocate 1 #d",
    "taunt #c",
    "trigger '#t'",
    "trigger '#t' #c",
    "triggers",
    "trip #c",
    "typo",
    "turn #c",
    "tune",

    "unlock #d",
    "unlock #o",
    "unload #o",
    "ungroup",
    "undisguise",
    "unalias",
    "uppercut #c",
    "unwield #o",
    "use #o",

    "value",
    "version",
    "view",
    "visible",
    "ventriloquize",

    "wake",
    "wear #o",
    "wear #o #p",
    "weather",
    "weigh #o",
    "where",
    "who",
    "whoami",
    "where",
    "whisper",
    "whirlwind #c",
    "wield #o",
    "wimpy",
    "wind",
    "withdraw",
    "wormhole",
    "wrench #c",
    "\n"
};

void
random_active_creature(struct creature *ch)
{
    static int template_count = 0;
    static int alter_count = 0;
    static int spell_count = 0;
    static int trigger_count = 0;
    static int song_count = 0;
    static int alterations[TOP_SPELL_DEFINE];
    static int magic_spells[TOP_SPELL_DEFINE];
    static int triggers[TOP_SPELL_DEFINE];
    static int songs[TOP_SPELL_DEFINE];
    char buf[1024];
    char *d = buf;
    int i = 0;
    int pos;
    struct creature *tch;
    struct obj_data *tob, *ob;

    if (is_dead(ch) || IS_PC(ch))
        return;

    if (get_char_in_world_by_idnum(-NPC_IDNUM(ch)) != ch)
        raise(SIGSEGV);

    if (template_count == 0) {
        while (cmd_templates[template_count][0] != '\n')
            template_count++;

        // Preprocess available spells
        for (i = 1;i < TOP_SPELL_DEFINE;i++) {
            if (spell_info[i].routines & MAG_MAGIC)
                magic_spells[spell_count++] = i;
            else if (spell_info[i].routines & MAG_PHYSICS)
                alterations[alter_count++] = i;
            else if (spell_info[i].routines & MAG_PSIONIC)
                triggers[trigger_count++] = i;
            else if (spell_info[i].routines & MAG_BARD)
                songs[song_count++] = i;
        }
    }

    // select a template to use
    const char *tmpl = cmd_templates[number(0,template_count - 1)];

    // expand template
    for (const char *s = tmpl;*s;s++) {
        if (*s == '#') {
            s++;
            switch (*s) {
            case 'c':           /* creature */
                i = 0;
                tch = NULL;
                for (GList *cit = first_living(ch->in_room->people);cit;cit = next_living(cit))
                    if (can_see_creature(ch, cit->data) && !number(0, i++))
                        tch = cit->data;
                strcpy_s(d, sizeof(buf) - (d - buf), fname(tch->player.name));
                d += strlen(fname(tch->player.name));
                break;
            case 'o':           /* object */
                i = 0;
                tob = NULL;
                for (ob = ch->in_room->contents;ob;ob = ob->next_content)
                    if (can_see_object(ch, ob) && !number(0, i++))
                        tob = ob;
                for (ob = ch->carrying;ob;ob = ob->next_content)
                    if (can_see_object(ch, ob) && !number(0, i++))
                        tob = ob;
                for (pos = 0;pos < NUM_WEARS;pos++)
                    if (GET_EQ(ch, pos)
                        && can_see_object(ch, GET_EQ(ch, pos))
                        && !number(0, i++))
                        tob = GET_EQ(ch, pos);
                for (pos = 0;pos < NUM_WEARS;pos++)
                    if (GET_IMPLANT(ch, pos)
                        && can_see_object(ch, GET_IMPLANT(ch, pos))
                        && !number(0, i++))
                        tob = GET_IMPLANT(ch, pos);
                if (tob) {
                    strcpy_s(d, sizeof(buf) - (d - buf), fname(tob->aliases));
                    d += strlen(fname(tob->aliases));
                }
                break;
            case 'a':           /* alteration */
                i = alterations[number(0, alter_count)];
                strcpy_s(d, sizeof(buf) - (d - buf), spells[i]);
                d += strlen(spells[i]);
                break;
            case 't':           /* trigger */
                i = triggers[number(0, trigger_count)];
                strcpy_s(d, sizeof(buf) - (d - buf), spells[i]);
                d += strlen(spells[i]);
                break;
            case 's':           /* spell */
                i = magic_spells[number(0, spell_count)];
                strcpy_s(d, sizeof(buf) - (d - buf), spells[i]);
                d += strlen(spells[i]);
                break;
            case 'g':           /* song */
                i = songs[number(0, song_count)];
                strcpy_s(d, sizeof(buf) - (d - buf), spells[i]);
                d += strlen(spells[i]);
                break;
            case 'd':           /* direction */
                i = number(0, NUM_DIRS);
                strcpy_s(d, sizeof(buf) - (d - buf), dirs[i]);
                d += strlen(dirs[i]);
                break;
            case 'p':           /* position */
                i = number(0, 28);
                strcpy_s(d, sizeof(buf) - (d - buf), wear_eqpos[i]);
                d += strlen(wear_eqpos[i]);
                break;
            case 'i':
                i = number(0, 28);
                strcpy_s(d, sizeof(buf) - (d - buf), wear_implantpos[i]);
                d += strlen(wear_implantpos[i]);
                break;
            default:
                *d++ = *s;
            }
        } else {
            *d++ = *s;
        }
    }

    *d = '\0';
    command_interpreter(ch, buf);
}


void
random_mob_activity(void)
{
    g_list_foreach(creatures, (GFunc) random_active_creature, NULL);
}
