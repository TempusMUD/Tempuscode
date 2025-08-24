#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
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
#include "account.h"
#include "screen.h"
#include "house.h"
#include "clan.h"
#include "char_class.h"
#include "sector.h"
#include "players.h"
#include "tmpstr.h"
#include "str_builder.h"
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
#include "account.h"

static char *
parse_offon(const char *val, bool *result)
{
    if (streq(val, "on") || streq(val, "yes") || streq(val, "1") || streq(val, "true")) {
        *result = true;
        return NULL;
    }
    if (streq(val, "off") || streq(val, "no") || streq(val, "0") || streq(val, "false")) {
        *result = false;
        return NULL;
    }

    return "You must select 'on' or 'off' for this setting.";
}

static void
set_pflag_num(struct creature *ch, int pflag_num, uint32_t pflag_bit, bool setbit)
{
    if (pflag_num == 1) {
        if (setbit) {
            SET_BIT(PLR_FLAGS(ch), pflag_bit);
        } else {
            REMOVE_BIT(PLR_FLAGS(ch), pflag_bit);
        }
    } else {
        if (setbit) {
            SET_BIT(PLR2_FLAGS(ch), pflag_bit);
        } else {
            REMOVE_BIT(PLR2_FLAGS(ch), pflag_bit);
        }
    }
}

static const char *
set_pflag_bit(struct creature *ch, const char *val, int pflag_num, uint32_t pref_bit)
{
    if (ch->player_specials == NULL) {
        return "Mobiles don't have preferences.";
    }

    bool setbit;
    const char *err = parse_offon(val, &setbit);
    if (err) {
        return err;
    }
    set_pflag_num(ch, pflag_num, pref_bit, setbit);
    return NULL;
}

static const char *
set_pflag_invbit(struct creature *ch, const char *val, int pflag_num, uint32_t pref_bit)
{
    if (ch->player_specials == NULL) {
        return "Mobiles don't have preferences.";
    }

    bool setbit;
    const char *err = parse_offon(val, &setbit);
    if (err) {
        return err;
    }
    set_pflag_num(ch, pflag_num, pref_bit, !setbit);
    return NULL;
}

static void
set_pref_num(struct creature *ch, int pref_num, uint32_t pref_bit, bool setbit)
{
    if (pref_num == 1) {
        if (setbit) {
            SET_BIT(PRF_FLAGS(ch), pref_bit);
        } else {
            REMOVE_BIT(PRF_FLAGS(ch), pref_bit);
        }
    } else {
        if (setbit) {
            SET_BIT(PRF2_FLAGS(ch), pref_bit);
        } else {
            REMOVE_BIT(PRF2_FLAGS(ch), pref_bit);
        }
    }
}

static const char *
set_pref_bit(struct creature *ch, const char *val, int pref_num, uint32_t pref_bit)
{
    if (ch->player_specials == NULL) {
        return "Mobiles don't have preferences.";
    }

    bool setbit;
    const char *err = parse_offon(val, &setbit);
    if (err) {
        return err;
    }
    set_pref_num(ch, pref_num, pref_bit, setbit);
    return NULL;
}

static const char *
set_pref_invbit(struct creature *ch, const char *val, int pref_num, uint32_t pref_bit)
{
    if (ch->player_specials == NULL) {
        return "Mobiles don't have preferences.";
    }

    bool setbit;
    const char *err = parse_offon(val, &setbit);
    if (err) {
        return err;
    }
    set_pref_num(ch, pref_num, pref_bit, !setbit);
    return NULL;
}

static const char *
get_pref_str(const char *val)
{
    if (val == NULL) {
        return "NULL";
    }
    return tmp_sprintf("\"%s\"", val);
}

static const char *
set_pref_str(struct creature *ch, const char *val, char **strp)
{
    free(*strp);
    *strp = strdup(val);
    return NULL;
}

static const char *
get_pref_enum(int val, const char *descs[])
{
    return strlist_aref(val, descs);
}

static const char *
set_pref_enum(uint8_t *result, const char *val, const char *descs[])
{
    int i = search_block(val, descs, false);
    if (i == -1) {
        return tmp_sprintf("'%s' is not one of: %s", val, tmp_join(", ", descs));
    }
    *result = i;
    return NULL;
}

static const char *
get_pref_bool(int val, const char *on, const char *off)
{
    return (val) ? on:off;
}

static const char *
set_pref_bool(bool *result, const char *val, const char *on, const char *off)
{
    if (is_abbrev(val, on)) {
        *result = true;
        return NULL;
    }
    if (is_abbrev(val, off)) {
        *result = false;
        return NULL;
    }
    return tmp_sprintf("Value must be either '%s' or '%s'", on, off);
}

static const char *
get_pref_int(int val)
{
    return tmp_sprintf("%d", val);
}

static const char *
set_pref_int(int *result, const char *val, int min, int max)
{
    char *endptr;
    long i = strtol(val, &endptr, 10);
    if (endptr == val) {
        return tmp_sprintf("The value '%s' is not a number.", val);
    }
    if (i < min || max < i) {
        return tmp_sprintf("The number must be between %d and %d", min, max);
    }
    *result = i;
    return NULL;
}

static const char *
get_pref_uint(uint val)
{
    return tmp_sprintf("%d", val);
}

static const char *
set_pref_uint(unsigned int *result, const char *val, uint min, uint max)
{
    char *endptr;
    long i = strtol(val, &endptr, 10);
    if (endptr == val) {
        return tmp_sprintf("The value '%s' is not a number.", val);
    }
    if (i < min || max < i) {
        return tmp_sprintf("The number must be between %d and %d", min, max);
    }
    *result = i;
    return NULL;
}

static const char *
get_pref_syslog(struct creature *ch)
{
    int tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
              (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    return logtypes[tp];
}

static const char *
set_pref_syslog(struct creature *ch, const char *val)
{
    int tp = search_block(val, logtypes, false);
    if (tp == -1) {
        return tmp_sprintf("'%s' is not one of: %s", val, tmp_join(", ", logtypes));
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
    SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));
    return NULL;
}

// Getter/setter definition
//   Getters return a const string describing the option.
//   Setters take a const string describing the option, set the parsed
//     value, and optionally return an error message or NULL on success.
#define SETTING(keysym, perm, getcode, setcode, helptxt) static const char * get_##keysym(struct creature *ch){return getcode;} static const char * set_##keysym(struct creature *ch, const char *val){return setcode;}

#include "settingdefs.c"

// Array definition
#undef SETTING
#define SETTING(keysym, perm, getcode, setcode, helptxt) { .key=#keysym, .sgroup=perm, .getterfunc=get_##keysym, .setterfunc=set_##keysym, .help=helptxt},


// Setting struct
// key: period separated hierarchical key
// sgroup: Needed security group to see/set or NULL
struct setting {
    const char *key;
    const char *sgroup;
    const char *(*getterfunc)(struct creature *);
    const char *(*setterfunc)(struct creature *, const char *);
    const char *help;
};

struct setting settings[] = {
    #include "settingdefs.c"
    {NULL, NULL, NULL, NULL, NULL}
};

// setting command for handling all settings (maybe settings command?)
// setting -> shows top level categories
// setting allow -> shows settings starting with allow.
// setting allow.pkilling -> shows just the one setting
// setting allow.pkilling yes -> sets the setting
// maybe resolve just with substring search, so that setting pkill shows all the settings matching pkill.  Setting the setting would require exactly one resolution.

static bool settings_keys_broken = true;

// fix_settings_keys replaces the underscores in the settings keys
// with periods.  This is necessary because we want periods as
// separators, and we can't have periods or do the replacement within
// the capabilities of the CONFOPT macro.
void
fix_settings_keys(void)
{
    for (int i = 0;settings[i].key != NULL;i++) {
        settings[i].key = strdup(tmp_gsub(settings[i].key, "_", "."));
    }
    settings_keys_broken = false;
}

ACMD(do_settings)
{
    bool help_mode = false;
    const char *search_arg = tmp_getword(&argument);
    if (streq(search_arg, "help")) {
        search_arg = tmp_getword(&argument);
        help_mode = true;
    }
    char *set_arg = argument;

    if (settings_keys_broken) {
        fix_settings_keys();
    }

    if (!*search_arg) {
        send_to_char(ch,
                     "Top level settings categories\r\n"
                     " settings <term> to search, settings help <term> for meaning.\r\n");
        // TODO: Probably should generate this, but it's unnecessarily complicated atm.
        send_to_char(ch,
                     "  allow.     - allowed behavior\r\n"
                     "  channel.   - communication channels\r\n"
                     "  display.   - what things are shown\r\n"
                     "  pref.      - miscellaneous preferences\r\n"
                     "  prompt.    - what your prompt looks like, if any\r\n"
                     "  screen.    - how things are displayed\r\n"
                     "  wholist.   - affects how you look on the who list.\r\n");
        return;
    }

    int search_idx = -1;
    for (int i = 0;settings[i].key != NULL;i++) {
        // Skip unauthorized configs
        if (settings[i].sgroup != NULL && !is_named_role_member(ch, settings[i].sgroup)) {
            continue;
        }
        if (*search_arg && !strstr(settings[i].key, search_arg)) {
            continue;
        }
        if (!help_mode && *set_arg) {
            if (search_idx != -1) {
                send_to_char(ch,
                             "More than one setting matched.  Matching settings:\r\n"
                             " %-20s %-10s %s\r\n",
                             settings[search_idx].key,
                             settings[search_idx].getterfunc(ch),
                             settings[search_idx].help);
                // Continue loop as though it wasn't a change request.
                *set_arg = '\0';
            } else {
                search_idx = i;
                continue;
            }
        }
        search_idx = i;
        if (help_mode) {
            send_to_char(ch, " %-20s %s\r\n",
                     settings[i].key,
                     settings[i].help);
        } else {
            send_to_char(ch, " %-20s %s\r\n",
                         settings[i].key,
                         settings[i].getterfunc(ch));
        }

    }
    if (search_idx == -1) {
        send_to_char(ch, "No setting matches '%s'.\r\n", search_arg);
    }
    if (!*set_arg) {
        return;
    }
    const char *err = settings[search_idx].setterfunc(ch, set_arg);
    if (err) {
        send_to_char(ch, "%s\r\n", err);
        return;
    }
    send_to_char(ch, "Setting %s to '%s'\r\n",
                 settings[search_idx].key,
                 settings[search_idx].getterfunc(ch));
}
