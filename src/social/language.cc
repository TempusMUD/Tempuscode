/*************************************************************************
 *  File: language.cc                                                    *
 *  Usage: Source file for multi-language support                        *
 *                                                                       *
 *  All rights reserved.  See license.doc for complete information.      *
 *                                                                       *
 *********************************************************************** */

//
// File: tongue.cc                           -- Part of TempusMUD
//

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
#include "language.h"
#include "accstr.h"

Tongue::Tongue(void)
    :_idnum(0), _name(NULL), _syllables(NULL), _syllable_count(0), _nospeak_msg(NULL)
{
    for (int c = 0;c < 256;c++)
        _letters[c] = c;
}

Tongue::Tongue(const Tongue &o)
{
    *this = o;
}

Tongue::~Tongue(void)
{
    free(_name);
    free(_nospeak_msg);
    delete [] _syllables;
}

Tongue &
Tongue::operator=(const Tongue &o)
{
    _idnum = o._idnum;
    _syllable_count = o._syllable_count;
    _name = (o._name) ? strdup(o._name):NULL;
    _nospeak_msg = (o._nospeak_msg) ? strdup(o._nospeak_msg):NULL;
    if (_syllable_count) {
        _syllables = new trans_pair[_syllable_count];
        memcpy(_syllables, o._syllables, sizeof(trans_pair) * _syllable_count);
    } else {
        _syllables = NULL;
    }
    memcpy(_letters, o._letters, sizeof(_letters));

    return *this;
}

void
Tongue::clear(void)
{
    _idnum = 0;
    free(_name);
    _name = NULL;
    free(_nospeak_msg);
    _nospeak_msg = NULL;
    delete [] _syllables;
    _syllables = NULL;
    _syllable_count = 0;
    for (int c = 0;c < 256;c++)
        _letters[c] = c;
}

bool
Tongue::load(xmlNodePtr node)
{
    xmlNodePtr child;

    clear();

    _idnum = xmlGetIntProp(node, "idnum");
    _name = xmlGetProp(node, "name");
    
    for (child = node->children;child;child = child->next)
        if (xmlMatches(child->name, "syllable"))
            _syllable_count++;

    _syllables = new trans_pair[_syllable_count];

    int syllable_idx = 0;
    for (child = node->children;child;child = child->next) {
        if (xmlMatches(child->name, "syllable")) {
            char *s;
            s = xmlGetProp(child, "pattern");
            strcpy(_syllables[syllable_idx]._pattern, s);
            free(s);
            s = xmlGetProp(child, "replacement");
            strcpy(_syllables[syllable_idx]._replacement, s);
            free(s);
            syllable_idx++;
        } else if (xmlMatches(child->name, "letter")) {
            char *pattern = xmlGetProp(child, "pattern");
            char *replace = xmlGetProp(child, "replacement");

            _letters[tolower(*pattern)] = tolower(*replace);
            _letters[toupper(*pattern)] = toupper(*replace);
            free(pattern);
            free(replace);
        } else if (xmlMatches(child->name, "nospeak")) {
			_nospeak_msg = (char *)xmlNodeGetContent(child);
        }
    }
    return true;
}

char *
Tongue::translate(const char *phrase, int amount)
{
	char *arg = NULL, *outbuf = NULL;

    if (!strcmp(phrase, "") || !_syllables || amount == 100)
        return tmp_strdup(phrase);

	while (*phrase) {
		arg = tmp_gettoken(&phrase);

        if (number(1, 100) > amount)
            arg = translate_word(arg);

		if (outbuf)
			outbuf = tmp_strcat(outbuf, " ", arg, NULL);
		else
			outbuf = tmp_strcat(arg, NULL);
	}

	return outbuf;
}

char *
Tongue::translate_word(char *word)
{
	char *arg;
	bool found = false;

	arg = tmp_strdup(word);

	for (int x = 0; x < _syllable_count; x++) {
		found = false;
		if (!strcmp(arg, _syllables[x]._pattern)) {
			found = true;
			arg = tmp_strdup(_syllables[x]._replacement);
			break;
		}
	}

	if (!found) {
        int length = strlen(word);
		for (int x = 0; x < length; x++)
            arg[x] = _letters[(int)arg[x]];
		arg[length] = 0;
	}

	return arg;
}

std::map<int,Tongue> tongues;

extern const char *player_race[];

const char *race_tongue[][2] = {
	{"Human", "modriatic"},
	{"Elf", "elvish"},
	{"Dwarf", "dwarven"},
	{"Half Orc", "orcish"},
	{"Klingon", "klingon"},
	{"Halfling", "hobbish"},
	{"Tabaxi", "sylvan"},
	{"Drow", "underdark"},
	{"ILL", ""},
	{"ILL", ""},
	{"Mobile", ""},
	{"Undead", "deadite"},
	{"Humanoid", ""},
	{"Animal", ""},
	{"Dragon", "draconian"},
	{"Giant", "ogrish"},
	{"Orc", "orcish"},
	{"Goblin", "mordorian"},
	{"Hafling", "hobbish"},
	{"Minotaur", "daedalus"},
	{"Troll", "trollish"},
	{"Golem", ""},
	{"Elemental", "elemental"},
	{"Ogre", "ogrish"},
	{"Devil", "infernum"},
	{"Trog", "trogish"},
	{"Manticore", "manticora"},
	{"Bugbear", "ghennish"},
	{"Draconian", "draconian"},
	{"Duergar", "underdark"},
	{"Slaad", "astral"},
	{"Robot", "centralian"},
	{"Demon", "abyssal"},
	{"Deva", "abyssal"},
	{"Plant", ""},
	{"Archon", "celestial"},
	{"Pudding", ""},
	{"Alien 1", "inconnu"},
	{"Predator Alien", "inconnu"},
	{"Slime", ""},
	{"Illithid", "sibilant"},
	{"Fish", ""},
	{"Beholder", "underdark"},
	{"Gaseous", "elemental"},
	{"Githyanki", "gish"},
	{"Insect", ""},
	{"Daemon", "abyssal"},
	{"Mephit", "elemental"},
	{"Kobold", "kobolas"},
	{"Umber Hulk", "underdark"},
	{"Wemic", "kalerrian"},
	{"Rakshasa", "rakshasian"},
	{"Spider", ""},
	{"Griffin", "gryphus"},
	{"Rotarian", "rotarial"},
	{"Half Elf", "elvish"},
	{"Celestial", "celestial"},
	{"Guardinal", "elysian"},
	{"Olympian", "greek"},
	{"Yugoloth", "all"},
	{"Rowlahr", ""},
	{"Githzerai", "gish"},
	{"\n", "\n"},
};

void
boot_tongues(void)
{
	xmlDocPtr doc;
	xmlNodePtr node;

    doc = xmlParseFile("etc/tongues.xml");
    if (!doc) {
        errlog("Couldn't load etc/tongues.xml");
        return;
    }

    node = xmlDocGetRootElement(doc);
    if (!node) {
        xmlFreeDoc(doc);
        errlog("etc/tongues.xml is empty");
        return;
    }

    if (!xmlMatches(node->name, "tongues")) {
        xmlFreeDoc(doc);
        errlog("/etc/tongues.xml root node is not tongues!");
    }

    node = node->children;
    while (node) {
        // Parse different nodes here.
        if (xmlMatches(node->name, "tongue")) {
            int idnum = xmlGetIntProp(node, "idnum");
            Tongue lang;

            if (lang.load(node))
                tongues[idnum] = lang;
        }
        node = node->next;
    }
    
    xmlFreeDoc(doc);
    
    slog("%zd tongues loaded", tongues.size());
}

int
find_tongue_idx_by_name(const char *tongue_name)
{
    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        if (is_abbrev(tongue_name, it->second._name)) {
            return it->first;
            break;
        }
    }

	return TONGUE_NONE;
}

int
racial_tongue(int race_idx)
{
    const char *race_name = player_race[race_idx];
	char *tongue_name = NULL;

	for (int x = 0; *race_tongue[x][0] != '\n'; x++) {
		if (!strcmp(race_tongue[x][0], race_name)) {
			tongue_name = tmp_strdup(race_tongue[x][1]);
			break;
		}
	}

    if (tongue_name)
        return find_tongue_idx_by_name(tongue_name);

    return TONGUE_NONE;
}

ACMD(do_speak_tongue)
{
	char *tongue;
	int tongue_idx;

	if (!*argument) {
		send_to_char(ch, "You are currently speaking %s.\r\n",
                     tongues[GET_TONGUE(ch)]._name);
		return;
	}

	tongue = tmp_getword(&argument);
	if (isname(tongue, "common")) {
		GET_TONGUE(ch) = TONGUE_COMMON;
		send_to_char(ch, "Ok, you're now speaking common.\r\n");
		return;
	}

    // Find the tongue they want
    tongue_idx = -1;
    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        if (is_abbrev(tongue, it->second._name)) {
            tongue_idx = it->first;
            break;
        }
    }

	if (tongue_idx < 0) {
		send_to_char(ch, "That's not a tongue!\r\n");
		return;
	}

    if (tongues[tongue_idx]._nospeak_msg) {
        send_to_char(ch, "%s\r\n", tongues[tongue_idx]._nospeak_msg);
        return;
    }

	if (CHECK_TONGUE(ch, tongue_idx) > 75) {
		GET_TONGUE(ch) = tongue_idx;
		send_to_char(ch, "Ok, you're now speaking %s.\r\n",
                     tongues[tongue_idx]._name);
    } else if (CHECK_TONGUE(ch, tongue_idx) > 50) {
		GET_TONGUE(ch) = tongue_idx;
		send_to_char(ch, "Ok, you're now trying to speak %s.\r\n",
                     tongues[tongue_idx]._name);
    } else if (CHECK_TONGUE(ch, tongue_idx) > 25) {
		GET_TONGUE(ch) = tongue_idx;
		send_to_char(ch, "Ok, you're now trying to speak %s (badly).\r\n",
                     tongues[tongue_idx]._name);
	} else
		send_to_char(ch, "You don't know that tongue well enough to speak it.\r\n");
}

const char *
fluency_desc(Creature *ch, int tongue_idx)
{
    int fluency = CHECK_TONGUE(ch, tongue_idx);

    if (fluency < 0)
        return "(terrible)";
    if (fluency == 0)
        return "(not learned)";
    if (fluency <= 10)
        return "(awful)";
    if (fluency <= 20)
        return "(bad)";
    if (fluency <= 40)
        return "(poor)";
    if (fluency <= 55)
        return "(average)";
    if (fluency <= 70)
        return "(average)";
    if (fluency <= 80)
        return "(good)";
    if (fluency <= 90)
        return "(very good)";

    return "(fluent)";
}

ACMD(do_show_languages)
{
    acc_string_clear();
	acc_sprintf("%sYou are currently speaking:  %s%s\r\n\r\n",
                CCCYN(ch, C_NRM), CCNRM(ch, C_NRM),
                tongues[GET_TONGUE(ch)]._name);

    acc_sprintf("%s%sYou know of the following languages:%s\r\n",
                CCYEL(ch, C_CMP), CCBLD(ch, C_SPR), CCNRM(ch, C_SPR));

    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        if (CHECK_TONGUE(ch, it->first)) {
			if (IS_IMMORT(ch)) {
				acc_sprintf("%s%3d. %-30s %s%-17s%s%s\r\n",
                            CCCYN(ch, C_NRM),
                            it->first,
                            it->second._name,
                            CCBLD(ch, C_SPR),
                            fluency_desc(ch, it->first),
                            tmp_sprintf("%s[%3d]%s", CCYEL(ch, C_NRM),
                                        CHECK_TONGUE(ch, it->first),
                                        CCNRM(ch, C_NRM)),
                            CCNRM(ch, C_SPR));
			} else {
				acc_sprintf("%s%-30s %s%s%s\r\n",
					CCCYN(ch, C_NRM), it->second._name, CCBLD(ch, C_SPR),
					fluency_desc(ch, it->first), CCNRM(ch, C_SPR));
			}
        }
    }
    
	page_string(ch->desc, acc_get_string());
}

void
set_initial_tongue(Creature * ch)
{
    // Only set initial tongues if this is the first time their
    // language is being set.  If they change race, they shouldn't
    // automatically get the language
    if (CHECK_TONGUE(ch, TONGUE_COMMON) != 100) {
        int tongue_idx = racial_tongue(GET_RACE(ch));

        // Everyone knows common
        SET_TONGUE(ch, TONGUE_COMMON, 100);
        if (tongue_idx != TONGUE_NONE)
            SET_TONGUE(ch, tongue_idx, 100);
        if (tongue_idx == TONGUE_NONE) {
            GET_TONGUE(ch) = TONGUE_COMMON;
        } else {
            // Speakers know their native language
            GET_TONGUE(ch) = tongue_idx;
        }
    }
}

const char *
tongue_name(int tongue_idx)
{
    return tongues[tongue_idx]._name;
}

char *
make_tongue_str(Creature *ch, Creature *to)
{
    int lang = GET_TONGUE(ch);

    if (lang == TONGUE_COMMON || !CHECK_TONGUE(to, lang))
        return tmp_strdup("");
    if (CHECK_TONGUE(ch, lang) < 50 && CHECK_TONGUE(to, lang) > 50)
        return tmp_sprintf(" in broken %s", tongues[lang]._name);
    return tmp_sprintf(" in %s", tongues[lang]._name);
}

char *
translate_tongue(Creature *speaker, Creature *listener, const char *message)
{
    int lang = GET_TONGUE(speaker);

    // Immortals understand all
    if (IS_IMMORT(listener))
        return tmp_strdup(message);

    // Perfect fluency for both speaker and listener
    if (CHECK_TONGUE(speaker, lang) == 100 &&
        CHECK_TONGUE(listener, lang) == 100)
        return tmp_strdup(message);

    // Listener might gain in fluency by listening to a sufficiently
    // skilled speaker
    if (CHECK_TONGUE(speaker, lang) > 75 &&
        CHECK_TONGUE(listener, lang) < 100 &&
        speaker != listener) {
        std::list<int>::iterator result;

        result = std::find(GET_LANG_HEARD(listener).begin(),
                           GET_LANG_HEARD(listener).end(),
                           lang);
        if (result == GET_LANG_HEARD(listener).end())
            GET_LANG_HEARD(listener).push_front(lang);
    }

    return tongues[lang].translate(message,
                                   MIN(CHECK_TONGUE(speaker, lang),
                                       CHECK_TONGUE(listener, lang)));
}

void
write_tongue_xml(Creature *ch, FILE *ouf)
{
    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        if (CHECK_TONGUE(ch, it->first))
            fprintf(ouf, "<tongue name=\"%s\" level=\"%d\"/>\n",
                    it->second._name, CHECK_TONGUE(ch, it->first));
    }
}

void
show_language_help(Creature *ch)
{
    acc_string_clear();

    acc_sprintf("LANGUAGES:\r\n");
        
    map<int, Tongue>::iterator it = tongues.begin();
    for (;it != tongues.end();++it) {
        Tongue &tongue = it->second;
        
        acc_sprintf("%2d         %s%-10s     [ %s",
                    tongue._idnum,
                    CCCYN(ch, C_NRM),
                    tongue._name,
                    CCNRM(ch, C_NRM));
        bool printed = false;
        for (int y = 0; *race_tongue[y][0] != '\n'; y++) {
            if (!strcmp(race_tongue[y][1], tongue._name)) {
                if (printed)
                    acc_strcat(", ", NULL);
                acc_strcat(race_tongue[y][0], NULL);
                printed = true;
            }
        }
        acc_sprintf(" %s]%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    }
    page_string(ch->desc, acc_get_string());
}
