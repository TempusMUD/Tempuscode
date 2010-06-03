#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <time.h>
#include <tmpstr.h>
#include <ctype.h>
#include <stdlib.h>
#include <map>
#include "interpreter.h"
#include "constants.h"
#include "utils.h"
#include "creature.h"
#include "comm.h"
#include "screen.h"
#include "handler.h"
#include "voice.h"
#include "accstr.h"

Voice_Voice(void)
    :_idnum(0), _name(""), _emits()
{
}

Voice_Voice(const Voice &o)
{
    *this = o;
}

Voice_~Voice(void)
{
}

Voice &
Voice_operator=(const Voice &o)
{
    _idnum = o._idnum;
    _name = o._name;
    _emits = o._emits;

    return *this;
}

void
Voice_clear(void)
{
    _idnum = 0;
    _name = "";
    _emits.erase(_emits.begin(), _emits.end());
}

// Note: These must be in the same order as the voice_situation enum
// in voice.h
static const char *voice_emit_categories[] = {
    "taunt",
    "attack",
    "panic",
    "hunt_found",
    "hunt_lost",
    "hunt_gone",
    "hunt_taunt",
    "hunt_unseen",
    "hunt_openair",
    "hunt_water",
    "fight_winning",
    "fight_losing",
    "fight_helping",
    "obeying",
    "\n"
};

bool
Voice_load(xmlNodePtr node)
{
    xmlNodePtr child;

    clear();

    _idnum = xmlGetIntProp(node, "idnum");
    _name = xmlGetProp(node, "name");

    for (child = node->children;child;child = child->next) {
        if (xmlMatches(child->name, "text"))
            continue;
        int emit_idx = search_block((char *)child->name, voice_emit_categories, true);
        if (emit_idx < 0) {
            errlog("Invalid node %s in voices.xml", child->name);
            return false;
        }
        char *str = (char *)xmlNodeGetContent(child);
        _emits[emit_idx].push_back(std_string(tmp_trim(str)));
        free(str);
    }
    return true;
}

void
Voice_perform(Creature *ch, void *vict, voice_situation situation)
{
    if (_emits[situation].empty())
        return;

    int cmd_idx = number(0, _emits[situation].size() - 1);
    char buf[MAX_STRING_LENGTH];

    make_act_str(_emits[situation][cmd_idx].c_str(),
                 buf,
                 ch, NULL, vict, ch);
    command_interpreter(ch, tmp_trim(buf));
}

void
emit_voice(Creature *ch, void *vict, voice_situation situation)
{
    if (!IS_NPC(ch))
        return;

    int voice_idx = GET_VOICE(ch);

    if (!voice_idx)
        voice_idx = IS_ANIMAL(ch) ? VOICE_ANIMAL:VOICE_MOBILE;

    if (voices.find(voice_idx) != voices.end()) {
        voices[voice_idx].perform(ch, vict, situation);
    }
}

std_map<int,Voice> voices;

void
boot_voices(void)
{
	xmlDocPtr doc;
	xmlNodePtr node;

    doc = xmlParseFile("etc/voices.xml");
    if (!doc) {
        errlog("Couldn't load etc/voices.xml");
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("etc/voices.xml is empty");
        return;
    }

    if (!xmlMatches(node->name, "voices")) {
        xmlFreeDoc(doc);
        errlog("/etc/voices.xml root node is not voices!");
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "voice")) {
            int idnum = xmlGetIntProp(node, "idnum");
            Voice voice;

            if (voice.load(node))
                voices[idnum] = voice;
            else
                safe_exit(1);
        }
        node = node->next;
    }

    xmlFreeDoc(doc);

    slog("%zd voices loaded", voices.size());
}

int
find_voice_idx_by_name(const char *voice_name)
{
    if (is_number(voice_name)) {
        int result = atoi(voice_name);
        if (voices.find(result) != voices.end())
            return result;
        return VOICE_NONE;
    }
    map<int, Voice>_iterator it = voices.begin();
    for (;it != voices.end();++it)
        if (is_abbrev(voice_name, it->second._name.c_str()))
            return it->first;

	return VOICE_NONE;
}

const char *
voice_name(int voice_idx)
{
    if (voice_idx == VOICE_NONE)
        return "<none>";
    if (voices.find(voice_idx) == voices.end())
        return tmp_sprintf("<#%d>", voice_idx);
    return voices[voice_idx]._name.c_str();
}

void
show_voices(Creature *ch)
{
    acc_string_clear();

    acc_sprintf("VOICES:\r\n");

    map<int, Voice>_iterator it = voices.begin();
    for (;it != voices.end();++it) {
        Voice &voice = it->second;

        acc_sprintf("%2d         %s%-10s%s\r\n",
                    voice._idnum,
                    CCCYN(ch, C_NRM),
                    voice._name.c_str(),
                    CCNRM(ch, C_NRM));
    }
    page_string(ch->desc, acc_get_string());
}
