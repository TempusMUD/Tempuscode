#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
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
#include "tmpstr.h"
#include "spells.h"
#include "vehicle.h"
#include "fight.h"
#include "obj_data.h"
#include "actions.h"
#include "weather.h"
#include "search.h"
#include "prog.h"
#include "strutil.h"

extern char locate_buf[256];

static int loop_fence = 0;
gint prog_tick = 0;
GTrashStack *dead_progs = NULL;
GList *active_progs = NULL;

#define DEFPROGHANDLER(cmd, env, evt, args) \
    void prog_do_ ## cmd(struct prog_env *env __attribute__ ((unused)), \
                         struct prog_evt *evt __attribute__ ((unused)), \
                         char *args __attribute__ ((unused)))

// Prog command prototypes
static void prog_do_before(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_handle(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_after(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_require(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_unless(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_do(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_silently(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_force(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_pause(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_stepto(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_walkto(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_driveto(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_halt(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_target(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_nuke(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_trans(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_set(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_let(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_oload(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_mload(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_opurge(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_giveexp(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_randomly(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_or(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_resume(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_echo(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_mobflag(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_ldesc(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_damage(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_spell(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_doorset(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_doorexit(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_selfpurge(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_compare_cmd(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_cond_next_handler(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_compare_obj_vnum(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_clear_cond(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_trace(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_tag(struct prog_env *env, struct prog_evt *evt, char *args);
static void prog_do_untag(struct prog_env *env, struct prog_evt *evt, char *args);

// external prototypes
struct creature *real_mobile_proto(int vnum);
struct obj_data *real_object_proto(int vnum);

struct prog_command prog_cmds[] = {
    {"!ENDOFPROG!", false, prog_do_halt },
    {"halt", false, prog_do_halt},
    {"resume", false, prog_do_resume},
    {"before", false, prog_do_before},
    {"handle", false, prog_do_handle},
    {"after", false, prog_do_after},
    {"or", false, prog_do_or},
    {"do", true, prog_do_do},
    {"!CLRCOND!", false, prog_do_clear_cond},
    {"!CMPCMD!", false, prog_do_compare_cmd},
    {"!CMPOBJVNUM!", false, prog_do_compare_obj_vnum},
    {"!CONDNEXTHANDLER!", false, prog_do_cond_next_handler},
    {"require", false, prog_do_require},
    {"unless", false, prog_do_unless},
    {"randomly", false, prog_do_randomly},
    {"trace", false, prog_do_trace},
    {"pause", true, prog_do_pause},
    {"stepto", true, prog_do_stepto},
    {"walkto", true, prog_do_walkto},
    {"driveto", true, prog_do_driveto},
    {"silently", true, prog_do_silently},
    {"force", true, prog_do_force},
    {"target", true, prog_do_target},
    {"nuke", true, prog_do_nuke},
    {"trans", true, prog_do_trans},
    {"set", true, prog_do_set},
    {"let", true, prog_do_let},
    {"oload", true, prog_do_oload},
    {"mload", true, prog_do_mload},
    {"opurge", true, prog_do_opurge},
    {"giveexp", true, prog_do_giveexp},
    {"echo", true, prog_do_echo},
    {"mobflag", true, prog_do_mobflag},
    {"ldesc", true, prog_do_ldesc},
    {"damage", true, prog_do_damage},
    {"spell", true, prog_do_spell},
    {"doorset", true, prog_do_doorset},
    {"doorexit", true, prog_do_doorexit},
    {"selfpurge", true, prog_do_selfpurge},
    {"tag", true, prog_do_tag},
    {"untag", true, prog_do_untag},
    {NULL, false, prog_do_halt}
};

unsigned char *
prog_get_obj(void *owner, enum prog_evt_type owner_type)
{
    switch (owner_type) {
    case PROG_TYPE_OBJECT:
        break;
    case PROG_TYPE_MOBILE:
        if (((struct creature *)owner)) {
            return GET_NPC_PROGOBJ((((struct creature *)owner)));
        } else {
            errlog("Mobile Prog with no owner - Can't happen at %s:%d",
                   __FILE__, __LINE__);
            return NULL;
        }
    case PROG_TYPE_ROOM:
        return ((struct room_data *)owner)->progobj;
    default:
        break;
    }
    errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    return NULL;
}

char *
prog_get_desc(struct prog_env *env)
{
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        return tmp_sprintf("object %d", GET_OBJ_VNUM((struct obj_data *)env->owner));
    case PROG_TYPE_MOBILE:
        return tmp_sprintf("mobile %d", GET_NPC_VNUM((struct creature *)env->owner));
    case PROG_TYPE_ROOM:
        return tmp_sprintf("room %d", ((struct room_data *)env->owner)->number);
    default:
        break;
    }
    errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    return tmp_strdup("");
}

struct prog_state_data *
prog_get_prog_state(struct prog_env *env)
{
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        return NULL;
    case PROG_TYPE_MOBILE:
        return ((struct creature *)env->owner)->prog_state;
    case PROG_TYPE_ROOM:
        return ((struct room_data *)env->owner)->prog_state;
    default:
        break;
    }
    errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    return NULL;
}

void
prog_set_prog_state(struct prog_env *env, struct prog_state_data *state)
{
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        return;
    case PROG_TYPE_MOBILE:
        ((struct creature *)env->owner)->prog_state = state;
        break;
    case PROG_TYPE_ROOM:
        ((struct room_data *)env->owner)->prog_state = state;
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        break;
    }
}

int
prog_event_handler(void *owner, enum prog_evt_type owner_type,
                   enum prog_evt_phase phase,
                   enum prog_evt_kind kind)
{
    unsigned char *obj = prog_get_obj(owner, owner_type);

    if (!obj) {
        return 0;
    }
    return *((prog_word_t *)obj + phase * PROG_EVT_COUNT + kind);
}

static void
prog_next_handler(struct prog_env *env, bool use_resume)
{
    unsigned char *prog;

    prog = prog_get_obj(env->owner, env->owner_type);
    // skip over lines until we find another handler (or bust)
    while (*((prog_word_t *)&prog[env->exec_pt])) {
        prog_word_t cmd;

        cmd = *((prog_word_t *)&prog[env->exec_pt]);
        if (cmd == PROG_CMD_BEFORE ||
            cmd == PROG_CMD_HANDLE ||
            cmd == PROG_CMD_AFTER ||
            cmd == PROG_CMD_ENDOFPROG ||
            (use_resume && cmd == PROG_CMD_RESUME)) {
            return;
        }
        env->exec_pt += sizeof(prog_word_t) * 2;
    }

    // We reached the end of the prog, so we leave the exec_pt
    // where it is - the end of prog handler will terminate
    // the prog gracefully
}

DEFPROGHANDLER(before, env, evt, args)
{}

DEFPROGHANDLER(handle, env, evt, args)
{}

DEFPROGHANDLER(after, env, evt, args)
{}

struct room_data *
prog_get_owner_room(struct prog_env *env)
{
    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        return ((struct creature *)env->owner)->in_room;
    case PROG_TYPE_ROOM:
        return ((struct room_data *)env->owner);
    case PROG_TYPE_OBJECT:
        return find_object_room((struct obj_data *)env->owner);
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    }

    return NULL;
}

static void
prog_send_debug(struct prog_env *env, const char *msg)
{
    struct room_data *room = prog_get_owner_room(env);

    for (GList *cit = first_living(room->people); cit; cit = next_living(cit)) {
        struct creature *ch = cit->data;

        if (PRF2_FLAGGED(ch, PRF2_DEBUG)) {
            send_to_char(ch, "%sprog x%p_ %s%s\r\n",
                         CCCYN(ch, C_NRM), env, msg, CCNRM(ch, C_NRM));
        }
    }
}

struct prog_var *
prog_get_var(struct prog_env *env, const char *key, bool exact)
{
    struct prog_var *cur_var;

    if (exact) {
        if (env->state) {
            for (cur_var = env->state->var_list; cur_var; cur_var = cur_var->next) {
                if (streq(cur_var->key, key)) {
                    return cur_var;
                }
            }
        }
        if (prog_get_prog_state(env)) {
            for (cur_var = prog_get_prog_state(env)->var_list; cur_var; cur_var = cur_var->next) {
                if (streq(cur_var->key, key)) {
                    return cur_var;
                }
            }
        }
    } else {
        if (env->state) {
            for (cur_var = env->state->var_list; cur_var; cur_var = cur_var->next) {
                if (!strncmp(cur_var->key, key, strlen(cur_var->key))) {
                    return cur_var;
                }
            }
        }
        if (prog_get_prog_state(env)) {
            for (cur_var = prog_get_prog_state(env)->var_list; cur_var; cur_var = cur_var->next) {
                if (!strncmp(cur_var->key, key, strlen(cur_var->key))) {
                    return cur_var;
                }
            }
        }
    }
    return NULL;
}

static void
prog_set_var(struct prog_env *env, bool local, const char *key, const char *arg)
{
    struct prog_state_data *state;
    struct prog_var *var;

    if (local) {
        state = env->state;
        if (!state) {
            CREATE(state, struct prog_state_data, 1);
            env->state = state;
        }
    } else {
        state = prog_get_prog_state(env);
        if (!state) {
            CREATE(state, struct prog_state_data, 1);
            prog_set_prog_state(env, state);
        }
    }

    var = prog_get_var(env, key, true);

    if (!var) {
        CREATE(var, struct prog_var, 1);
        strcpy_s(var->key, sizeof(var->key), key);

        // Sort by key length descending so inexact matches will work
        // properly
        if (!state->var_list || strlen(key) > strlen(state->var_list->key)) {
            var->next = state->var_list;
            state->var_list = var;
        } else {
            struct prog_var *prev_var = state->var_list;
            while (prev_var->next && strlen(prev_var->next->key) < strlen(key)) {
                prev_var = prev_var->next;
            }
            var->next = prev_var->next;
            prev_var->next = var;
        }
    }

    strcpy_s(var->value, sizeof(var->value), arg);
}

static void
prog_set_target(struct prog_env *env, struct creature *target)
{
    if (env->tracing) {
        if (target) {
            prog_send_debug(env, tmp_sprintf("(setting target to %s)",
                                             GET_NAME(target)));
        } else {
            prog_send_debug(env, "(setting target to <none>)");
        }
    }
    env->target = target;
    prog_set_var(env, true, "N", (target) ? fname(target->player.name) : "");
    prog_set_var(env, true, "E", (target) ? HSSH(target) : "");
    prog_set_var(env, true, "S", (target) ? HSHR(target) : "");
    prog_set_var(env, true, "M", (target) ? HMHR(target) : "");
}

bool
prog_var_equal(struct prog_env *env, char *key, char *arg)
{
    struct prog_var *var;

    var = prog_get_var(env, key, true);
    if (var == NULL) {
        return !(*arg);
    }
    return !strcasecmp(var->value, arg);
}

char *
prog_expand_vars(struct prog_env *env, char *args)
{
    char *search_pt = strchr(args, '$');
    const char *result = "";
    struct prog_var *var;

    if (!search_pt) {
        return args;
    }

    while (search_pt) {
        result = tmp_strcat(result, tmp_substr(args, 0, search_pt - args - 1), NULL);
        args = search_pt + 1;
        if (*args == '$') {
            // Double dollar expands to simple dollar sign
            result = tmp_strcat(result, "$", NULL);
        } else if (*args == '{') {
            // Variable reference
            args++;
            search_pt = strchr(args, '}');
            if (!search_pt) {
                struct room_data *room = prog_get_owner_room(env);
                zerrlog(room->zone, "Invalid variable reference: no } found");
                result = tmp_strcat(result, "<TYPO ME>", NULL);
                break;
            }
            var = prog_get_var(env, tmp_substr(args, 0, search_pt - args - 1), true);
            if (var) {
                result = tmp_strcat(result, var->value, NULL);
            } else {
                struct room_data *room = prog_get_owner_room(env);
                zerrlog(room->zone, "Invalid variable reference: %s not a variable",
                        tmp_substr(args, 0, search_pt - args - 1));
                result = tmp_strcat(result, "<TYPO ME>", NULL);
                continue;
            }
            args = search_pt + 1;
        } else {
            // Potential variable reference
            var = prog_get_var(env, args, false);
            if (var) {
                result = tmp_strcat(result, var->value, NULL);
                args += strlen(var->key);
            } else {
                result = tmp_strcat(result, "$", NULL);
            }
        }
        search_pt = strchr(args, '$');
    }
    return tmp_strcat(result, args, NULL);
}

char *
prog_get_alias_list(char *args)
{
    char *type, *vnum_str;
    int vnum = 0;
    struct creature *mob = NULL;
    struct obj_data *obj = NULL;

    type = tmp_getword(&args);
    vnum_str = tmp_getword(&args);

    if (!is_number(vnum_str)) {
        return NULL;
    }

    vnum = atoi(vnum_str);

    if (tolower(*type) == 'm') {
        mob = real_mobile_proto(vnum);
        if (mob) {
            return tmp_strdup(mob->player.name);
        }
    } else if (tolower(*type) == 'o') {
        obj = real_object_proto(vnum);
        if (obj) {
            return tmp_strdup(obj->aliases);
        }
    }

    return NULL;
}

bool
prog_eval_alias(struct prog_evt *evt, char *args)
{
    char *alias_list = NULL;
    bool result = false;
    char *str, *arg;

    if (!(alias_list = prog_get_alias_list(args))) {
        result = false;
    }
    if (*evt->args != '\0' && alias_list) {
        str = evt->args;
        arg = tmp_getword(&str);
        while (*arg) {
            if (isname(arg, alias_list)) {
                result = true;
                break;
            }
            arg = tmp_getword(&str);
        }
    }

    return result;
}

bool
prog_eval_keyword(struct prog_evt *evt, char *args)
{
    bool result = false;
    char *str, *arg;

    if (*evt->args != '\0') {
        str = evt->args;
        arg = tmp_getword(&str);
        while (*arg) {
            if (isname_exact(arg, args)) {
                result = true;
                break;
            }
            arg = tmp_getword(&str);
        }
    }

    return result;
}

bool
prog_eval_abbrev(struct prog_evt *evt, char *args)
{
    bool result = false;
    char *str, *arg;

    if (*evt->args != '\0') {
        str = evt->args;
        arg = tmp_getword(&str);
        while (*arg) {
            char *len_ptr = NULL, *tmp_args = args, *saved_args = args;
            while (*args && (args = tmp_getword(&tmp_args))) {
                int len = 0;
                if ((len_ptr = strstr(args, "*"))) {
                    len = len_ptr - args;
                    memmove(len_ptr, len_ptr + 1, strlen(args) - len - 1);
                    args[strlen(args) - 1] = 0;
                }
                if (is_abbrevn(arg, args, len)) {
                    result = true;
                    break;
                }
            }
            args = saved_args;
            arg = tmp_getword(&str);
        }
    }

    return result;
}

bool
prog_eval_holding(struct prog_env *env, char *args)
{
    bool result = false;
    int vnum;
    struct obj_data *obj = NULL;

    vnum = atoi(tmp_getword(&args));
    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        obj = ((struct creature *) env->owner)->carrying;
        break;
    case PROG_TYPE_OBJECT:
        obj = ((struct obj_data *) env->owner)->contains;
        break;
    case PROG_TYPE_ROOM:
        obj = ((struct room_data *) env->owner)->contents;
        break;
    default:
        obj = NULL;
    }
    while (obj) {
        if (GET_OBJ_VNUM(obj) == vnum) {
            result = true;
        }
        obj = obj->next_content;
    }

    return result;
}

bool
prog_eval_phase(char *args)
{
    bool result = false;
    char *str;
    int phase;

    str = tmp_getword(&args);
    phase = get_lunar_phase(lunar_day);
    if (strcasecmp(str, "full")) {
        result = phase == MOON_FULL;
    } else if (strcasecmp(str, "waning")) {
        result = phase == MOON_WANE_GIBBOUS
                 || phase == MOON_WANE_CRESCENT || phase == MOON_LAST_QUARTER;
    } else if (strcasecmp(str, "new")) {
        result = phase == MOON_NEW;
    } else if (strcasecmp(str, "waxing")) {
        result = phase == MOON_WAX_GIBBOUS
                 || phase == MOON_WAX_CRESCENT || phase == MOON_FIRST_QUARTER;
    }

    return result;
}

bool
prog_eval_alignment(struct prog_env *env, char *args)
{
    bool result = false;
    char *str;

    str = tmp_getword(&args);
    if (!strcasecmp(str, "evil") && IS_EVIL(env->target)) {
        result = true;
    } else if (!strcasecmp(str, "good") && IS_GOOD(env->target)) {
        result = true;
    } else if (!strcasecmp(str, "neutral") && IS_NEUTRAL(env->target)) {
        result = true;
    }

    return result;
}

bool
prog_eval_class(struct prog_env *env, char *args)
{
    bool result = false;
    // Required class
    char *rclass = tmp_tolower(args);
    // Actual character class
    char *cclass;

    extern const char *class_names[];

    if (!env->target) {
        return false;
    }

    cclass = tmp_tolower(class_names[GET_CLASS(env->target)]);
    if (strstr(rclass, cclass)) {
        result = true;
    }

    if (IS_REMORT(env->target) &&
        GET_REMORT_CLASS(env->target) != CLASS_NONE) {
        cclass = tmp_tolower(class_names[GET_REMORT_CLASS(env->target)]);
        if (strstr(rclass, cclass)) {
            result = true;
        }
    }

    return result;
}

bool
prog_eval_vnum(struct prog_env *env, char *args)
{
    bool result = false;
    char *arg;

    if (env->target && IS_NPC(env->target)) {
        arg = tmp_getword(&args);
        while (*arg) {
            if (is_number(arg)
                && atoi(arg) == GET_NPC_VNUM(env->target)) {
                result = true;
                break;
            }
            arg = tmp_getword(&args);
        }
    }

    return result;
}

bool
prog_eval_level(struct prog_env *env, char *args)
{
    bool result = false;
    char *arg;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "greater")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
                 && GET_LEVEL(env->target) > atoi(arg);
    } else if (!strcasecmp(arg, "less")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
                 && GET_LEVEL(env->target) < atoi(arg);
    }

    return result;
}

bool
prog_eval_gen(struct prog_env *env, char *args)
{
    bool result = false;
    char *arg;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "greater")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
                 && GET_REMORT_GEN(env->target) > atoi(arg);
    } else if (!strcasecmp(arg, "less")) {
        arg = tmp_getword(&args);
        result = is_number(arg) && env->target
                 && GET_REMORT_GEN(env->target) < atoi(arg);
    }

    return result;
}

bool
prog_eval_tar_holding(struct prog_env *env, char *args)
{
    bool result = false;
    struct obj_data *obj;
    int vnum;

    vnum = atoi(tmp_getword(&args));
    if (env->target) {
        obj = env->target->carrying;
    } else {
        obj = NULL;
    }

    while (obj) {
        if (GET_OBJ_VNUM(obj) == vnum) {
            result = true;
        }
        obj = obj->next_content;
    }

    return result;
}

bool
prog_eval_wearing(struct prog_env *env, char *args)
{
    bool result = false;
    int vnum;

    vnum = atoi(tmp_getword(&args));
    if (env->target) {
        for (int i = 0; i < NUM_WEARS; i++) {
            if ((env->target->equipment[i]
                 && GET_OBJ_VNUM(env->target->equipment[i]) == vnum)
                || (env->target->implants[i]
                    && GET_OBJ_VNUM(env->target->implants[i]) == vnum)) {
                result = true;
            }
        }
    }

    return result;
}

// If have a new condition to add and you can do it in
// 3 lines or less, you can add it inline here.  Otherwise
// factor it out into a function
bool
prog_eval_condition(struct prog_env *env, struct prog_evt *evt, char *args)
{
    char *arg;
    bool result = false, not_flag = false;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "not")) {
        not_flag = true;
        arg = tmp_getword(&args);
    }

    if (streq(arg, "argument")) {
        result = (*evt->args != '\0' && !strcasecmp(args, evt->args));
        // Mobs using "alias"
        // 1200 3062 90800
    } else if (streq(arg, "alias")) {
        result = prog_eval_alias(evt, args);
    } else if (streq(arg, "keyword")) {
        result = prog_eval_keyword(evt, args);
    } else if (streq(arg, "abbrev")) {
        result = prog_eval_abbrev(evt, args);
    } else if (streq(arg, "fighting")) {
        result = (env->owner_type == PROG_TYPE_MOBILE
                  && is_fighting(((struct creature *) env->owner)));
    } else if (streq(arg, "randomly")) {
        result = number(0, 100) < atoi(args);
    } else if (streq(arg, "variable")) {
        arg = tmp_gettoken(&args);
        result = prog_var_equal(env, arg, args);
        if (env->tracing) {
            struct prog_var *var = prog_get_var(env, arg, true);
            prog_send_debug(env,
                            tmp_sprintf("('%s' %s '%s')",
                                        (var) ? var->value : "(null)",
                                        (result) ? "==" : "!=",
                                        args));
        }
    } else if (!strcasecmp(arg, "holding")) {
        result = prog_eval_holding(env, args);
    } else if (!strcasecmp(arg, "hour")) {
        result = time_info.hours == atoi(tmp_getword(&args));
    } else if (!strcasecmp(arg, "phase")) {
        result = prog_eval_phase(args);
    } else if (streq(arg, "position")) {
        if (env->owner_type != PROG_TYPE_MOBILE) {
            struct room_data *room = prog_get_owner_room(env);
            zerrlog(room->zone, "Illegal position condition in %s prog",
                    prog_get_desc(env));
            result = false;
        } else {
            int pos = search_block(args, position_types, false);

            if (pos < 0) {
                struct room_data *room = prog_get_owner_room(env);
                zerrlog(room->zone, "Invalid position '%s' in %s prog",
                        args, prog_get_desc(env));
                result = false;
            } else {
                result = (pos == GET_POSITION(((struct creature *)env->owner)));
            }
        }
    } else if (!strcasecmp(arg, "room")) {
        struct room_data *room = prog_get_owner_room(env);

        arg = tmp_getword(&args);
        result = (*arg && room) ? (room->number == atoi(arg)) : false;
    } else if (!strcasecmp(arg, "target")) {
        // These are all subsets of the *require target <attribute> directive
        arg = tmp_getword(&args);
        if (arg[0] == '\0') {
            result = (env->target != NULL);
        } else if (!strcasecmp(arg, "class")) {
            result = prog_eval_class(env, args);
        } else if (!strcasecmp(arg, "alignment")) {
            result = prog_eval_alignment(env, args);
        } else if (streq(arg, "position")) {
            int pos = search_block(args, position_types, false);

            if (pos < 0) {
                struct room_data *room = prog_get_owner_room(env);
                zerrlog(room->zone, "Invalid position '%s' in %s prog",
                        args, prog_get_desc(env));
                result = false;
            } else {
                result = (pos == GET_POSITION(env->target));
            }
        } else if (!strcasecmp(arg, "player")) {
            result = env->target && IS_PC(env->target);
        } else if (!strcasecmp(arg, "vnum")) {
            result = prog_eval_vnum(env, args);
        } else if (!strcasecmp(arg, "level")) {
            result = prog_eval_level(env, args);
        } else if (!strcasecmp(arg, "gen")) {
            result = prog_eval_gen(env, args);
        } else if (!strcasecmp(arg, "holding")) {
            result = prog_eval_tar_holding(env, args);
        } else if (!strcasecmp(arg, "wearing")) {
            result = prog_eval_wearing(env, args);
        } else if (!strcasecmp(arg, "self")) {
            result = (env->owner == env->target);
        } else if (!strcasecmp(arg, "tagged")) {
            char *tag = tmp_getword(&args);
            result = (env->target && player_has_tag(env->target, tag));
        } else if (!strcasecmp(arg, "visible")) {
            if (env->owner_type == PROG_TYPE_MOBILE) {
                result = can_see_creature(((struct creature *)env->owner), env->target);
            } else {
                result = true;
            }
        }
    }

    return (not_flag) ? (!result) : result;
}

DEFPROGHANDLER(require, env, evt, args)
{
    if (!prog_eval_condition(env, evt, args)) {
        prog_next_handler(env, true);
    }
}

DEFPROGHANDLER(unless, env, evt, args)
{
    if (prog_eval_condition(env, evt, args)) {
        prog_next_handler(env, true);
    }
}

DEFPROGHANDLER(randomly, env, evt, args)
{
    unsigned char *exec;
    int cur_pt, last_pt, num_paths;

    // We save the execution point and find the next handler.
    cur_pt = env->exec_pt;
    prog_next_handler(env, true);
    last_pt = env->exec_pt;
    env->exec_pt = cur_pt;

    // now we run through, setting randomly which code path to take
    exec = prog_get_obj(env->owner, env->owner_type);
    num_paths = 0;
    while (cur_pt < last_pt) {
        if (*((prog_word_t *)&exec[cur_pt]) == PROG_CMD_OR) {
            num_paths += 1;
            if (!number(0, num_paths)) {
                env->exec_pt = cur_pt + sizeof(prog_word_t) * 2;
            }
        }
        cur_pt += sizeof(prog_word_t) * 2;
    }

    // At this point, exec_pt should be on a randomly selected code path
    // within the current handler
}

DEFPROGHANDLER(or, env, evt, args)
{
    prog_next_handler(env, true);
}

DEFPROGHANDLER(do, env, evt, args)
{
    if (env->owner_type == PROG_TYPE_MOBILE) {
        command_interpreter((struct creature *) env->owner, args);
    }
}

DEFPROGHANDLER(silently, env, evt, args)
{
    suppress_output = true;
    if (env->owner_type == PROG_TYPE_MOBILE) {
        command_interpreter((struct creature *) env->owner, args);
    }
    suppress_output = false;
}

DEFPROGHANDLER(force, env, evt, args)
{
    if (!env->target) {
        return;
    }

    if (env->owner_type == PROG_TYPE_MOBILE) {
        command_interpreter((struct creature *) env->target, args);
    }
}

DEFPROGHANDLER(target, env, evt, args)
{
    struct room_data *room = prog_get_owner_room(env);
    struct creature *new_target = NULL;
    char *arg;

    arg = tmp_getword(&args);
    if (!strcasecmp(arg, "random")) {
        if (!strcasecmp(tmp_getword(&args), "player")) {
            new_target = (env->owner_type == PROG_TYPE_MOBILE) ?
                         get_player_random_vis(((struct creature *)env->owner), room) :
                         get_player_random(room);
        } else {
            new_target = (env->owner_type == PROG_TYPE_MOBILE) ?
                         get_char_random_vis(((struct creature *)env->owner), room) :
                         get_char_random(room);
        }
    } else if (!strcasecmp(arg, "opponent")) {
        switch (env->owner_type) {
        case PROG_TYPE_MOBILE:
            new_target = random_opponent((struct creature *) env->owner);
            break;
        default:
            new_target = NULL;
        }
    } else {
        zerrlog(room->zone, "Bad *target argument '%s' in prog in %s",
                arg, prog_get_desc(env));
    }

    prog_set_target(env, new_target);
}

DEFPROGHANDLER(trace, env, evt, args)
{
    env->tracing = !strcasecmp(args, "on");
}

DEFPROGHANDLER(pause, env, evt, args)
{
    env->next_tick = prog_tick + MAX(1, atoi(args));
}

DEFPROGHANDLER(stepto, env, evt, args)
{
    struct creature *ch;
    struct room_data *room;
    int dir;

    if (env->owner_type != PROG_TYPE_MOBILE) {
        return;
    }

    ch = (struct creature *) env->owner;

    // Make sure the creature can walk
    if (GET_POSITION(ch) < POS_STANDING) {
        return;
    }

    room = real_room(atoi(tmp_getword(&args)));

    if (room && room != ch->in_room) {
        dir = find_first_step(ch->in_room, room, STD_TRACK);
        if (dir >= 0) {
            smart_mobile_move(ch, dir);
        }
    }
}

DEFPROGHANDLER(walkto, env, evt, args)
{
    struct creature *ch;
    struct room_data *room;
    int dir, pause;

    if (env->owner_type != PROG_TYPE_MOBILE) {
        return;
    }

    ch = (struct creature *) env->owner;

    // Make sure the creature can walk
    if (GET_POSITION(ch) < POS_STANDING) {
        return;
    }

    room = real_room(atoi(tmp_getword(&args)));

    if (room && room != ch->in_room) {
        dir = find_first_step(ch->in_room, room, STD_TRACK);
        if (dir >= 0) {
            smart_mobile_move(ch, dir);
        }

        // we have to wait at least one second
        pause = atoi(tmp_getword(&args));
        env->next_tick = prog_tick + MAX(1, pause);

        // we stay on the same line until we get to the destination
        env->exec_pt -= sizeof(prog_word_t) * 2;
    }
}

DEFPROGHANDLER(driveto, env, evt, args)
{
    // move_car courtesy of vehicle.cc
    int move_car(struct creature *ch, struct obj_data *car, int dir);

    struct obj_data *console, *vehicle, *engine;
    struct creature *ch;
    struct room_data *target_room;
    struct room_direction_data *exit;
    int dir, pause;

    if (env->owner_type != PROG_TYPE_MOBILE) {
        return;
    }

    ch = (struct creature *) env->owner;

    // Make sure the creature can drive
    if (GET_POSITION(ch) < POS_SITTING) {
        return;
    }

    // Get the target room, bailing if it doesn't actually exist
    target_room = real_room(atoi(tmp_getword(&args)));
    if (!target_room) {
        return;
    }

    // Find the console in the room.  Do nothing if there's no console
    for (console = ch->in_room->contents; console;
         console = console->next_content) {
        if (IS_OBJ_TYPE(console, ITEM_V_CONSOLE)) {
            break;
        }
    }
    if (!console) {
        return;
    }

    // Now find the vehicle vnum that the console points to
    for (vehicle = object_list; vehicle; vehicle = vehicle->next) {
        if (GET_OBJ_VNUM(vehicle) == V_CAR_VNUM(console) && vehicle->in_room) {
            break;
        }
    }
    if (!vehicle) {
        return;
    }

    // Check for engine status
    engine = vehicle->contains;
    if (!engine || !IS_ENGINE(engine) || !ENGINE_ON(engine)) {
        return;
    }

    if (target_room != vehicle->in_room) {
        dir = find_first_step(vehicle->in_room, target_room, GOD_TRACK);

        // Validate exit the vehicle is going to take
        if (dir >= 0) {
            exit = vehicle->in_room->dir_option[dir];
            if (!exit ||
                !exit->to_room ||
                ROOM_FLAGGED(exit->to_room, ROOM_DEATH | ROOM_INDOORS) ||
                IS_SET(exit->exit_info, EX_CLOSED | EX_ISDOOR)) {
                return;
            }

            move_car(ch, vehicle, dir);
        }

        // we have to wait at least one second
        pause = atoi(tmp_getword(&args));
        env->next_tick = prog_tick + MAX(1, pause);

        // we stay on the same line until we get to the destination
        env->exec_pt -= sizeof(prog_word_t) * 2;
    }
}

DEFPROGHANDLER(halt, env, evt, args)
{
    env->exec_pt = -1;
}

DEFPROGHANDLER(mobflag, env, evt, args)
{
    bool op;
    char *arg;
    int flag_idx = 0, flags = 0;

    if (env->owner_type != PROG_TYPE_MOBILE) {
        return;
    }

    if (*args == '+') {
        op = true;
    } else if (*args == '-') {
        op = false;
    } else {
        return;
    }

    args += 1;
    skip_spaces(&args);
    while (*(arg = tmp_getword(&args))) {
        flag_idx = search_block(arg, action_bits_desc, false);
        if (flag_idx != -1) {
            flags |= (1U << flag_idx);
        }
    }
    // some flags can't change
    flags &= ~(NPC_SPEC | NPC_ISNPC | NPC_PET);

    if (op) {
        NPC_FLAGS(((struct creature *) env->owner)) |= flags;
    } else {
        NPC_FLAGS(((struct creature *) env->owner)) &= ~flags;
    }
}

DEFPROGHANDLER(ldesc, env, evt, args)
{
    struct creature *mob;
    struct obj_data *obj;

    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        mob = (struct creature *) env->owner;
        if (mob->player.long_descr) {
            if (mob->player.long_descr !=
                mob->mob_specials.shared->proto->player.long_descr) {
                free(mob->player.long_descr);
            }
        }
        mob->player.long_descr = strdup(args);
        break;
    case PROG_TYPE_OBJECT:
        obj = (struct obj_data *) env->owner;
        if (obj->line_desc && obj->line_desc != obj->shared->proto->line_desc) {
            free(obj->line_desc);
        }
        obj->line_desc = strdup(args);
        break;
    default:
        // do nothing
        break;
    }
}

DEFPROGHANDLER(damage, env, evt, args)
{
    struct room_data *room;
    struct creature *mob;
    struct obj_data *obj;
    int damage_amt;
    char *target_arg;
    int damage_type;

    target_arg = tmp_getword(&args);

    damage_amt = atoi(tmp_getword(&args));
    if (!*args) {
        return;
    }
    damage_type = str_to_spell(args);
    if (damage_type == -1) {
        return;
    }

    search_nomessage = true;
    if (streq(target_arg, "self")) {
        // Trans the owner of the prog
        switch (env->owner_type) {
        case PROG_TYPE_OBJECT:
            obj = (struct obj_data *) env->owner;
            damage_eq((obj->worn_by) ? obj->worn_by : obj->carried_by,
                      obj, damage_amt, damage_type);
            break;
        case PROG_TYPE_MOBILE:
            mob = (struct creature *) env->owner;
            if (GET_POSITION(mob) > POS_DEAD) {
                damage(NULL, mob, NULL, damage_amt, damage_type, WEAR_RANDOM);
            }
            break;
        case PROG_TYPE_ROOM:
            break;
        default:
            break;
        }
        search_nomessage = false;
        return;
    } else if (streq(target_arg, "target")) {
        // Transport the target, which is always a creature
        if (!env->target) {
            search_nomessage = false;
            return;
        }
        if (GET_POSITION(env->target) > POS_DEAD) {
            damage(NULL, env->target, NULL, damage_amt, damage_type, WEAR_RANDOM);
        }
        search_nomessage = false;
        return;
    }
    // The rest of the options deal with multiple creatures

    bool players = false, mobs = false;

    room = prog_get_owner_room(env);
    if (!room) {
        return;
    }

    if (streq(target_arg, "mobiles")) {
        mobs = true;
    } else if (streq(target_arg, "players")) {
        players = true;
    } else if (!streq(target_arg, "all")) {
        zerrlog(room->zone, "Bad *damage argument '%s' in prog in %s",
                target_arg, prog_get_desc(env));
    }

    for (GList *it = first_living(room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if ((!players || IS_PC(tch)) &&
            (!mobs || IS_NPC(tch)) &&
            GET_POSITION(tch) > POS_DEAD) {
            damage(NULL, tch, NULL, damage_amt, damage_type, WEAR_RANDOM);
        }
    }
    search_nomessage = false;
}

DEFPROGHANDLER(spell, env, evt, args)
{
    struct room_data *room;
    struct creature *caster = NULL;
    struct obj_data *obj;
    int spell_num, spell_lvl, spell_type;
    char *target_arg;

    // Parse out arguments
    target_arg = tmp_getword(&args);
    if (!*args) {
        return;
    }

    spell_lvl = atoi(tmp_getword(&args));
    if (spell_lvl > 49) {
        return;
    }
    if (!*args) {
        return;
    }
    spell_num = str_to_spell(args);
    if (spell_num == -1) {
        return;
    }

    // Set locate_buf to avoid accidents
    locate_buf[0] = '\0';

    // Determine the spell type
    if (SPELL_IS_MAGIC(spell_num) || SPELL_IS_DIVINE(spell_num)) {
        spell_type = CAST_SPELL;
    } else if (SPELL_IS_PHYSICS(spell_num)) {
        spell_type = CAST_PHYSIC;
    } else if (SPELL_IS_PSIONIC(spell_num)) {
        spell_type = CAST_PSIONIC;
    } else {
        spell_type = CAST_ROD;
    }

    // Determine the caster of the spell, so to speak
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        obj = (struct obj_data *) env->owner;
        caster = (obj->worn_by) ? obj->worn_by : obj->carried_by;
        break;
    case PROG_TYPE_MOBILE:
        caster = ((struct creature *)env->owner);
        break;
    case PROG_TYPE_ROOM:
        caster = NULL;
        break;
    default:
        break;
    }

    search_nomessage = true;
    if (streq(target_arg, "self")) {
        // Cast a spell on the owner of the prog
        switch (env->owner_type) {
        case PROG_TYPE_OBJECT:
            obj = (struct obj_data *) env->owner;
            call_magic(caster, caster, obj, NULL,
                       spell_num, spell_lvl, spell_type);
            break;
        case PROG_TYPE_MOBILE:
            if (GET_POSITION(caster) > POS_DEAD
                && !affected_by_spell(caster, spell_num)) {
                call_magic(caster, caster, NULL, NULL,
                           spell_num, spell_lvl, spell_type);
            }
            break;
        case PROG_TYPE_ROOM:
            break;
        default:
            break;
        }
        search_nomessage = false;
        return;
    } else if (streq(target_arg, "target")) {
        // Cast a spell on the target
        if (!env->target) {
            search_nomessage = false;
            return;
        }
        if (GET_POSITION(env->target) > POS_DEAD
            && !affected_by_spell(env->target, spell_num)) {
            call_magic(caster ? caster : (env->target),
                       env->target,
                       NULL, NULL,
                       spell_num, spell_lvl, spell_type);
        }
        search_nomessage = false;
        return;
    }
    // The rest of the options deal with multiple creatures

    room = prog_get_owner_room(env);
    if (!room) {
        return;
    }

    bool players = false, mobs = false;

    if (streq(target_arg, "mobiles")) {
        mobs = true;
    } else if (streq(target_arg, "players")) {
        players = true;
    } else if (!streq(target_arg, "all")) {
        zerrlog(room->zone, "Bad *spell argument '%s' in prog in %s",
                target_arg, prog_get_desc(env));
    }

    for (GList *it = first_living(room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if ((!players || IS_PC(tch)) &&
            (!mobs || IS_NPC(tch)) &&
            GET_POSITION(tch) > POS_DEAD &&
            !affected_by_spell(tch, spell_num)) {
            call_magic(caster ? caster : (tch), tch, NULL, NULL,
                       spell_num, spell_lvl, spell_type);
        }
    }
    search_nomessage = false;
}

DEFPROGHANDLER(doorset, env, evt, args)
{
    // *doorset <room vnum> <direction> +/- <doorflags>
    bool op;
    char *arg;
    struct room_data *room;
    int dir, flag_idx = 0, flags = 0;

    arg = tmp_getword(&args);
    room = real_room(atoi(arg));
    if (!room) {
        return;
    }

    arg = tmp_getword(&args);
    dir = search_block(arg, dirs, false);
    if (dir < 0) {
        return;
    }

    if (*args == '+') {
        op = true;
    } else if (*args == '-') {
        op = false;
    } else {
        return;
    }

    args += 1;
    skip_spaces(&args);
    while (*(arg = tmp_getword(&args))) {
        flag_idx = search_block(arg, exit_bits, false);
        if (flag_idx != -1) {
            flags |= (1U << flag_idx);
        }
    }

    if (!ABS_EXIT(room, dir)) {
        CREATE(room->dir_option[dir], struct room_direction_data, 1);
        room->dir_option[dir]->to_room = NULL;
    }

    if (op) {
        room->dir_option[dir]->exit_info |= flags;
    } else {
        room->dir_option[dir]->exit_info &= ~flags;
    }
}

DEFPROGHANDLER(doorexit, env, evt, args)
{
    // *doorexit <room vnum> <direction> (<target room>|none)
    char *arg;
    struct room_data *room, *target_room;
    int dir;

    arg = tmp_getword(&args);
    room = real_room(atoi(arg));
    if (!room) {
        return;
    }

    arg = tmp_getword(&args);
    dir = search_block(arg, dirs, false);
    if (dir < 0) {
        return;
    }

    arg = tmp_getword(&args);
    if (is_number(arg)) {
        int target_roomnum = atoi(arg);
        target_room = real_room(target_roomnum);
        if (!target_room) {
            return;
        }

        if (!room->dir_option[dir]) {
            CREATE(room->dir_option[dir], struct room_direction_data, 1);
        }
    } else if (!strcasecmp("none", arg)) {
        target_room = NULL;
    } else {
        return;
    }

    if (room->dir_option[dir]) {
        room->dir_option[dir]->to_room = target_room;
    } else if (target_room) {
        CREATE(room->dir_option[dir], struct room_direction_data, 1);
        room->dir_option[dir]->to_room = target_room;
    }
}

DEFPROGHANDLER(selfpurge, env, evt, args)
{
    if (env->owner_type == PROG_TYPE_MOBILE) {
        prog_do_nuke(env, evt, args);
        env->exec_pt = -1;

        if (evt->kind != PROG_EVT_DYING) {
            creature_purge((struct creature *) env->owner, true);
        }
        env->owner = NULL;
    }
}

DEFPROGHANDLER(hunt, env, evt, args)
{
    if (env->owner_type == PROG_TYPE_MOBILE && env->target) {
        start_hunting((struct creature *) env->owner, env->target);
    }
}

DEFPROGHANDLER(clear_cond, env, evt, args)
{
    env->condition = 0;
}

DEFPROGHANDLER(compare_cmd, env, evt, args)
{
    // FIXME: nasty hack
    if (!env->condition) {
        env->condition = (evt->cmd == *((int *)args));
    }
}

DEFPROGHANDLER(compare_obj_vnum, env, evt, args)
{
    // FIXME: nasty hack
    if (!env->condition) {
        env->condition = (evt->object_type == PROG_TYPE_OBJECT
                          && evt->object
                          && GET_OBJ_VNUM((struct obj_data *)evt->object) == *((int *)args));
    }
}

DEFPROGHANDLER(cond_next_handler, env, evt, args)
{
    if (!env->condition) {
        prog_next_handler(env, false);
    }
}

DEFPROGHANDLER(nuke, env, evt, args)
{

    for (GList *cur = active_progs; cur; cur = cur->next) {
        struct prog_env *cur_prog = cur->data;
        if (cur_prog != env && cur_prog->owner == env->owner) {
            cur_prog->exec_pt = -1;
        }
    }
}

DEFPROGHANDLER(tag, env, evt, args)
{
    if (!env->target) {
        return;
    }
    add_player_tag(env->target, tmp_getword(&args));
}

DEFPROGHANDLER(untag, env, evt, args)
{
    if (!env->target) {
        return;
    }
    remove_player_tag(env->target, tmp_getword(&args));
}

static void
prog_trans_creature(struct creature *ch, struct room_data *targ_room)
{
    if (!is_authorized(ch, ENTER_ROOM, targ_room)) {
        return;
    }

    char_from_room(ch, true);
    char_to_room(ch, targ_room, true);
    targ_room->zone->enter_count++;

    // Immortal following
    if (ch->followers) {
        struct follow_type *k, *next;

        for (k = ch->followers; k; k = next) {
            next = k->next;
            if (targ_room == k->follower->in_room &&
                GET_LEVEL(k->follower) >= LVL_AMBASSADOR &&
                !PLR_FLAGGED(k->follower, PLR_WRITING)
                && can_see_creature(k->follower, ch)) {
                perform_goto(k->follower, targ_room, true);
            }
        }
    }

    if (IS_SET(ROOM_FLAGS(targ_room), ROOM_DEATH)) {
        if (GET_LEVEL(ch) < LVL_AMBASSADOR) {
            log_death_trap(ch);
            raw_kill(ch, NULL, TYPE_SUFFERING);
        } else {
            mudlog(LVL_GOD, NRM, true,
                   "(GC) %s trans-searched into deathtrap %d.",
                   GET_NAME(ch), targ_room->number);
        }
    }
}

DEFPROGHANDLER(trans, env, evt, args)
{
    struct room_data *room, *targ_room;
    struct obj_data *obj;
    int targ_num;
    char *target_arg;

    target_arg = tmp_getword(&args);

    targ_num = atoi(tmp_getword(&args));
    targ_room = real_room(targ_num);
    if (targ_room == NULL) {
        errlog("trans target room %d nonexistent in prog in %s",
               targ_num, prog_get_desc(env));
        return;
    }

    if (streq(target_arg, "self")) {
        // Trans the owner of the prog
        switch (env->owner_type) {
        case PROG_TYPE_OBJECT:
            obj = (struct obj_data *) env->owner;
            if (obj->carried_by) {
                obj_from_char(obj);
            } else if (obj->in_room) {
                obj_from_room(obj);
            } else if (obj->worn_by) {
                unequip_char(obj->worn_by, obj->worn_on,
                             (obj == GET_EQ(obj->worn_by, obj->worn_on) ?
                              EQUIP_WORN : EQUIP_IMPLANT));
            } else if (obj->in_obj) {
                obj_from_obj(obj);
            }

            obj_to_room(obj, targ_room);
            break;
        case PROG_TYPE_MOBILE:
            prog_trans_creature((struct creature *)env->owner, targ_room);
            break;
        case PROG_TYPE_ROOM:
            break;
        default:
            break;
        }
        return;
    } else if (streq(target_arg, "target")) {
        // Transport the target, which is always a creature
        if (!env->target) {
            return;
        }
        prog_trans_creature((struct creature *) env->target, targ_room);
        return;
    }
    // The rest of the options deal with multiple creatures

    room = prog_get_owner_room(env);
    if (!room) {
        return;
    }

    bool players = false, mobs = false;

    if (streq(target_arg, "mobiles")) {
        mobs = true;
    } else if (streq(target_arg, "players")) {
        players = true;
    } else if (!streq(target_arg, "all")) {
        zerrlog(room->zone, "Bad *trans argument '%s' in prog in %s",
                target_arg, prog_get_desc(env));
    }

    GList *people = g_list_copy(room->people);
    for (GList *it = first_living(people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        if ((!players || IS_PC(tch)) && (!mobs || IS_NPC(tch))) {
            prog_trans_creature(tch, targ_room);
        }
    }
    g_list_free(people);
}

// Set the value for an owner-scoped variable
DEFPROGHANDLER(set, env, evt, args)
{
    // Now find the variable record.  If they don't have one
    // with the right key, create one
    char *key = tmp_getword(&args);
    prog_set_var(env, false, key, args);
}

// Set the value for a thread-scoped variable
DEFPROGHANDLER(let, env, evt, args)
{
    char *key = tmp_getword(&args);
    prog_set_var(env, true, key, args);
}

DEFPROGHANDLER(oload, env, evt, args)
{
    struct obj_data *obj;
    struct room_data *room = NULL;
    int vnum, max_load = 200, target_num = -1;
    char *target_str, *arg;

    vnum = atoi(tmp_getword(&args));
    if (vnum <= 0) {
        return;
    }
    target_str = tmp_getword(&args);

    arg = tmp_getword(&args);
    if (is_number(arg)) {
        target_num = atoi(arg);
        arg = tmp_getword(&args);
    }

    if (!strcasecmp(arg, "max")) {
        max_load = atoi(tmp_getword(&args));
    }

    obj = real_object_proto(vnum);
    if (!obj || (max_load != -1 && !obj_maxload_allow_load(obj, max_load))) {
        return;
    }

    obj = read_object(vnum);
    if (!obj) {
        return;
    }
    obj->creation_method = CREATED_PROG;
    switch (env->owner_type) {
    case PROG_TYPE_OBJECT:
        obj->creator = GET_OBJ_VNUM((struct obj_data *)env->owner); break;
    case PROG_TYPE_ROOM:
        obj->creator = ((struct room_data *)env->owner)->number; break;
    case PROG_TYPE_MOBILE:
        obj->creator = GET_NPC_VNUM(((struct creature *)env->owner)); break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    }

    if (!*target_str || !strcasecmp(target_str, "room")) {
        if (target_num == -1) {
            // They mean the current room
            room = prog_get_owner_room(env);
        } else {
            // They're specifying a room number
            room = real_room(target_num);
        }
        if (!room) {
            extract_obj(obj);
            return;
        }
        obj_to_room(obj, room);
    } else if (!strcasecmp(target_str, "target")) {
        if (env->target) {
            obj_to_char(obj, env->target);
        } else {
            extract_obj(obj);
        }
    } else if (!strcasecmp(target_str, "self")) {
        switch (env->owner_type) {
        case PROG_TYPE_MOBILE:
            obj_to_char(obj, (struct creature *) env->owner);
            break;
        case PROG_TYPE_OBJECT:
            obj_to_obj(obj, (struct obj_data *) env->owner);
            break;
        case PROG_TYPE_ROOM:
            obj_to_room(obj, (struct room_data *) env->owner);
            break;
        default:
            errlog("Can't happen at %s:%d", __FILE__, __LINE__);
        }
    } else {
        // Bad target str, so we need to free the object
        extract_obj(obj);
    }
}

DEFPROGHANDLER(mload, env, evt, args)
{
    struct creature *mob;
    struct room_data *room = NULL;
    int vnum, max_load = 200;
    char *arg;

    vnum = atoi(tmp_getword(&args));
    if (vnum <= 0) {
        return;
    }
    mob = real_mobile_proto(vnum);
    if (!mob) {
        return;
    }

    arg = tmp_getword(&args);
    if (*arg && strcasecmp(arg, "max")) {
        room = real_room(atoi(arg));
        if (!room) {
            return;
        }
        arg = tmp_getword(&args);
    } else {
        room = prog_get_owner_room(env);
    }
    if (*arg && !strcasecmp(arg, "max")) {
        max_load = MIN(atoi(tmp_getword(&args)), 200);
    }

    if (mob->mob_specials.shared->number < max_load) {
        mob = read_mobile(vnum);
        if (mob == NULL) {
            return;
        }
        char_to_room(mob, room, true);
        if (GET_NPC_PROG(mob)) {
            trigger_prog_load(mob);
        }
    }
}

DEFPROGHANDLER(opurge, env, evt, args)
{
    struct obj_data *obj, *obj_list, *next_obj;
    int vnum;

    vnum = atoi(tmp_getword(&args));
    if (vnum <= 0) {
        return;
    }

    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        obj_list = ((struct creature *) env->owner)->carrying;
        break;
    case PROG_TYPE_OBJECT:
        obj_list = ((struct obj_data *) env->owner)->contains;
        break;
    case PROG_TYPE_ROOM:
        obj_list = ((struct room_data *) env->owner)->contents;
        break;
    default:
        obj_list = NULL;
        errlog("Can't happen at %s:%d", __FILE__, __LINE__);
    }

    if (!obj_list) {
        return;
    }

    // Eliminate the selected objects at the head of the list
    while (obj_list && GET_OBJ_VNUM(obj_list) == vnum) {
        next_obj = obj_list->next_content;
        extract_obj(obj_list);
        obj_list = next_obj;
    }

    if (obj_list) {
        // There are still objects in the list, so iterate through
        // them, extracting the ones of the proper vnum
        for (obj = obj_list->next_content; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (GET_OBJ_VNUM(obj) == vnum) {
                extract_obj(obj);
            }
        }
    }

    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        ((struct creature *) env->owner)->carrying = obj_list;
        break;
    case PROG_TYPE_OBJECT:
        ((struct obj_data *) env->owner)->contains = obj_list;
        break;
    case PROG_TYPE_ROOM:
        ((struct room_data *) env->owner)->contents = obj_list;
        break;
    default:
        break;
    }
}

DEFPROGHANDLER(giveexp, env, evt, args)
{
    int amount = atoi(args);
    const char *owner_type = "";

    if (!env->target) {
        return;
    }

    if (amount > 1000000) {
        int num;

        switch (env->owner_type) {
        case PROG_TYPE_MOBILE:
            owner_type = "mobile";
            num = GET_NPC_VNUM(((struct creature *)env->owner));
            break;
        case PROG_TYPE_OBJECT:
            owner_type = "object";
            num = GET_OBJ_VNUM((struct obj_data *)env->owner);
            break;
        case PROG_TYPE_ROOM:
            owner_type = "room";
            num = ((struct room_data *)env->owner)->number;
            break;
        default:
            owner_type = "invalid";
            num = -1;
        }
        slog("WARNING: Prog on %s %d giving %d experience", owner_type, num, amount);
    }

    gain_exp(env->target, amount);
}

DEFPROGHANDLER(resume, env, evt, args)
{
    // literally does nothing
}

DEFPROGHANDLER(echo, env, evt, args)
{
    char *arg;
    struct creature *ch = NULL;
    struct obj_data *obj = NULL;
    struct room_data *room = NULL;

    arg = tmp_getword(&args);
    room = prog_get_owner_room(env);

    if (!strcasecmp(arg, "zone")) {
        // We handle zone echos by iterating through connections here,
        // not through every creature.  This differs from the other cases,
        // which are room-bound.
        for (struct descriptor_data *pt = descriptor_list; pt; pt = pt->next) {
            if (pt->input_mode == CXN_PLAYING &&
                pt->creature && pt->creature->in_room
                && pt->creature->in_room->zone == room->zone
                && !PLR_FLAGGED(pt->creature, PLR_WRITING)) {
                act(args, false, pt->creature, NULL, env->target, TO_CHAR);
            }
        }
        return;
    }

    switch (env->owner_type) {
    case PROG_TYPE_MOBILE:
        ch = ((struct creature *)env->owner); break;
    case PROG_TYPE_OBJECT:
        obj = ((struct obj_data *)env->owner); break;
    case PROG_TYPE_ROOM:
        break;
    default:
        errlog("Can't happen at %s:%d", __FILE__, __LINE__); break;
    }

    if (ch == NULL) {
        ch = env->target;
    }
    if (ch == NULL) {
        // if there's no one in the room no point in echoing
        if (!first_living(room->people)) {
            return;
        }
        // we just pick the top guy off the people list for rooms.
        ch = first_living(room->people)->data;
    }

    if (!strcasecmp(arg, "room")) {
        act(args, false, ch, obj, env->target, TO_CHAR);
        act(args, false, ch, obj, env->target, TO_ROOM);
    } else if (!strcasecmp(arg, "target")) {
        act(args, false, ch, obj, env->target, TO_VICT);
    } else if (!strcasecmp(arg, "!target")) {
        act(args, false, ch, obj, env->target, TO_NOTVICT);
        if (ch != env->target) {
            act(args, false, ch, obj, env->target, TO_CHAR);
        }
    }
}

static void
prog_emit_trace(struct prog_env *env, int cmd, const char *arg)
{
    prog_send_debug(env, tmp_sprintf("%s %s", prog_cmds[cmd].str, arg));
}

static void
prog_execute(struct prog_env *env)
{
    unsigned char *exec;
    int cmd, arg_addr;
    char *arg_str;

    // Called with NULL environment
    if (!env) {
        return;
    }

    // Terminated, but not freed
    if (env->exec_pt < 0) {
        return;
    }

    env->next_tick = prog_tick + env->speed;

    exec = prog_get_obj(env->owner, env->owner_type);
    if (!exec) {
        // Damn prog disappeared on us
        env->exec_pt = -1;
        return;
    }

    while (env->exec_pt >= 0 && env->next_tick <= prog_tick) {
        // Get the command and the arg address
        cmd = *((prog_word_t *)(exec + env->exec_pt));
        arg_addr = *((prog_word_t *)(exec + env->exec_pt + sizeof(prog_word_t)));
        // Set the execution point to the next command by default
        env->exec_pt += sizeof(prog_word_t) * 2;
        // Call the handler for the command
        arg_str = (arg_addr) ? prog_expand_vars(env, (char *)exec + arg_addr) : NULL;
        // Emit a trace if the prog is being traced
        if (env->tracing) {
            prog_emit_trace(env, cmd, arg_str);
        }

        // Execute the command
        prog_cmds[cmd].func(env, &env->evt, arg_str);

        // If the command did something, count it
        if (prog_cmds[cmd].count) {
            env->executed +=1;
        }
    }
}

struct prog_env *
prog_start(enum prog_evt_type owner_type, void *owner, struct creature *target, struct prog_evt *evt)
{
    struct prog_env *new_prog;
    int initial_exec_pt;

    initial_exec_pt = prog_event_handler(owner, owner_type, evt->phase, evt->kind);
    if (!initial_exec_pt) {
        return NULL;
    }

    new_prog = g_trash_stack_pop(&dead_progs);
    if (!new_prog) {
        CREATE(new_prog, struct prog_env, 1);
    }

    active_progs = g_list_prepend(active_progs, new_prog);

    new_prog->owner_type = owner_type;
    new_prog->owner = owner;
    new_prog->exec_pt = initial_exec_pt;
    new_prog->executed = 0;
    new_prog->next_tick = prog_tick;
    new_prog->speed = 0;
    new_prog->target = NULL;
    new_prog->evt = *evt;
    new_prog->tracing = false;
    new_prog->state = NULL;

    if (owner_type == PROG_TYPE_MOBILE) {
        ((struct creature *)owner)->prog_marker += 1;
    } else {
        ((struct room_data *)owner)->prog_marker += 1;
    }

    if (target) {
        prog_set_target(new_prog, target);
    }

    return new_prog;
}

void
destroy_attached_progs(void *owner)
{
    for (GList *cur = active_progs; cur; cur = cur->next) {
        struct prog_env *cur_prog = cur->data;
        if (cur_prog->owner == owner ||
            cur_prog->target == owner ||
            cur_prog->evt.subject == owner ||
            cur_prog->evt.object == owner) {
            cur_prog->exec_pt = -1;
        }
        if (cur_prog->owner == owner) {
            cur_prog->owner = NULL;
        }
    }
}

void
prog_unreference_object(struct obj_data *obj)
{
    for (GList *cur = active_progs; cur; cur = cur->next) {
        struct prog_env *cur_prog = cur->data;
        if (cur_prog->evt.object_type == PROG_TYPE_OBJECT
            && cur_prog->evt.object == obj) {
            cur_prog->evt.object_type = PROG_TYPE_NONE;
            cur_prog->evt.object = NULL;
        }
    }
}

static void
report_prog_loop(void *owner, enum prog_evt_type owner_type, struct creature *ch, const char *where)
{
    const char *owner_desc = "<unknown>";
    const char *ch_desc = "<unknown>";

    switch (owner_type) {
    case PROG_TYPE_OBJECT:
        owner_desc = tmp_sprintf("object %d", GET_OBJ_VNUM((struct obj_data *)owner)); break;
    case PROG_TYPE_MOBILE:
        owner_desc = tmp_sprintf("mobile %d", GET_NPC_VNUM(((struct creature *)owner))); break;
    case PROG_TYPE_ROOM:
        owner_desc = tmp_sprintf("room %d", ((struct room_data *)owner)->number); break;
    default:
        break;
    }

    if (ch) {
        ch_desc = tmp_sprintf("%s %s[%ld]",
                              (IS_NPC(ch)) ? "MOB" : "PC",
                              GET_NAME(ch),
                              (IS_NPC(ch)) ? GET_NPC_VNUM(ch) : GET_IDNUM(ch));
    }

    errlog("Infinite prog loop detected in %s of %s (triggered by %s)",
           where,
           owner_desc,
           ch_desc);
}

bool
trigger_prog_cmd(void *owner, enum prog_evt_type owner_type, struct creature *ch, int cmd,
                 char *argument)
{
    struct prog_env *env, *handler_env;
    struct prog_evt evt;
    bool handled = false;

    if (ch == owner) {
        return false;
    }
    if (!prog_get_obj(owner, owner_type)) {
        return false;
    }
    // We don't want an infinite loop with triggered progs that
    // trigger a prog, etc.
    if (loop_fence >= 20) {
        report_prog_loop(owner, owner_type, ch, "command handler");
        return false;
    }

    loop_fence++;

    evt.phase = PROG_EVT_BEGIN;
    evt.kind = PROG_EVT_COMMAND;
    evt.cmd = cmd;
    evt.subject = ch;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    skip_spaces(&argument);
    strcpy_s(evt.args, sizeof(evt.args), argument);
    env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(env);

    // Check for death or destruction of the owner
    if (env && !env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_HANDLE;
    strcpy_s(evt.args, sizeof(evt.args), argument);
    handler_env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(handler_env);

    // Check for death or destruction of the owner
    if (handler_env && !handler_env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_AFTER;
    strcpy_s(evt.args, sizeof(evt.args), argument);
    prog_start(owner_type, owner, ch, &evt);
    // note that we don't start executing yet...

    loop_fence -= 1;

    if (handler_env && handler_env->executed) {
        return true;
    }

    return handled;
}

bool
trigger_prog_spell(void *owner, enum prog_evt_type owner_type, struct creature *ch, int cmd)
{
    struct prog_env *env, *handler_env;
    struct prog_evt evt;
    bool handled = false;

    if (ch == owner) {
        return false;
    }

    if (!prog_get_obj(owner, owner_type)) {
        return false;
    }

    // We don't want an infinite loop with triggered progs that
    // trigger a prog, etc.
    if (loop_fence >= 20) {
        report_prog_loop(owner, owner_type, ch, "spell handler");
        return false;
    }

    loop_fence++;

    evt.phase = PROG_EVT_BEGIN;
    evt.kind = PROG_EVT_SPELL;
    evt.cmd = cmd;
    evt.subject = ch;
    evt.object = NULL;                     // this should perhaps be updated to hold the target of the spell
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(env);

    // Check for death or destruction of the owner
    if (env && !env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_HANDLE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    handler_env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(handler_env);

    // Check for death or destruction of the owner
    if (env && !env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_AFTER;
    strcpy_s(evt.args, sizeof(evt.args), "");
    prog_start(owner_type, owner, ch, &evt);
    // note that we don't start executing yet...

    loop_fence -= 1;

    if (handler_env && handler_env->executed) {
        return true;
    }

    return handled;
}

bool
trigger_prog_move(void *owner, enum prog_evt_type owner_type, struct creature *ch,
                  enum special_mode mode)
{
    struct prog_env *env, *handler_env;
    struct prog_evt evt;
    bool handled = false;

    if (ch == owner) {
        return false;
    }

    if (!prog_get_obj(owner, owner_type)) {
        return false;
    }

    // We don't want an infinite loop with mobs triggering progs that
    // trigger a prog, etc.
    if (loop_fence >= 20) {
        report_prog_loop(owner, owner_type, ch, "move handler");
        return false;
    }

    loop_fence++;

    evt.phase = PROG_EVT_BEGIN;
    evt.kind = (mode == SPECIAL_ENTER) ? PROG_EVT_ENTER : PROG_EVT_LEAVE;
    evt.cmd = 0;
    evt.subject = ch;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(env);

    // Check for death or destruction of the owner
    if (env && !env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_HANDLE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    handler_env = prog_start(owner_type, owner, ch, &evt);
    prog_execute(handler_env);

    // Check for death or destruction of the owner
    if (env && !env->owner) {
        loop_fence -= 1;
        return true;
    }

    evt.phase = PROG_EVT_AFTER;
    strcpy_s(evt.args, sizeof(evt.args), "");
    prog_start(owner_type, owner, ch, &evt);
    // note that we don't start executing yet...  prog_update_pending()
    // gets called by interpret_command(), after all command processing
    // takes place

    loop_fence -= 1;

    if (handler_env && handler_env->executed) {
        return true;
    }

    return handled;
}

void
trigger_prog_fight(struct creature *ch, struct creature *self)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!self || !self->in_room || !GET_NPC_PROG(self)) {
        return;
    }
    if (!GET_NPC_PROGOBJ(self)) {
        return;
    }
    evt.phase = PROG_EVT_AFTER;
    evt.kind = PROG_EVT_FIGHT;
    evt.cmd = -1;
    evt.subject = ch;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
    prog_execute(env);
}

void
trigger_prog_dying(struct creature *owner, struct creature *killer)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!prog_get_obj(owner, PROG_TYPE_MOBILE)) {
        return;
    }

    // We don't want an infinite loop with triggered progs that
    // trigger a prog, etc.
    if (loop_fence >= 20) {
        report_prog_loop(owner, PROG_TYPE_MOBILE, killer, "dying handler");
        return;
    }

    loop_fence++;

    evt.phase = PROG_EVT_BEGIN;
    evt.kind = PROG_EVT_DYING;
    evt.cmd = -1;
    evt.subject = killer;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    env = prog_start(PROG_TYPE_MOBILE, owner, NULL, &evt);
    prog_execute(env);

    loop_fence -= 1;
}

void
trigger_prog_death(void *owner, enum prog_evt_type owner_type, struct creature *doomed)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!prog_get_obj(owner, owner_type)) {
        return;
    }

    // We don't want an infinite loop with triggered progs that
    // trigger a prog, etc.
    if (loop_fence >= 20) {
        report_prog_loop(owner, owner_type, doomed, "death handler");
        return;
    }

    loop_fence++;

    evt.phase = PROG_EVT_BEGIN;
    evt.kind = PROG_EVT_DEATH;
    evt.cmd = -1;
    evt.subject = doomed;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");
    env = prog_start(owner_type, owner, doomed, &evt);
    prog_execute(env);


    // We don't target the doomed creature after death, since the
    // creature will be removed from the game shortly.
    evt.phase = PROG_EVT_AFTER;
    env = prog_start(owner_type, owner, NULL, &evt);
    prog_execute(env);

    loop_fence -= 1;
}

void
trigger_prog_give(struct creature *ch, struct creature *self, struct obj_data *obj)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!self || !self->in_room || !GET_NPC_PROGOBJ(self)) {
        return;
    }

    if (!obj) {
        errlog("trigger_prog_give() called with no object!");
        return;
    }

    evt.phase = PROG_EVT_AFTER;
    evt.kind = PROG_EVT_GIVE;
    evt.cmd = -1;
    evt.subject = ch;
    evt.object = obj;
    evt.object_type = PROG_TYPE_OBJECT;
    strcpy_s(evt.args, sizeof(evt.args), "");

    env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
    prog_execute(env);
}

bool
trigger_prog_chat(struct creature *ch, struct creature *self)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!self || !self->in_room || !GET_NPC_PROGOBJ(self)) {
        return false;
    }

    evt.phase = PROG_EVT_HANDLE;
    evt.kind = PROG_EVT_CHAT;
    evt.cmd = -1;
    evt.subject = ch;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    env = prog_start(PROG_TYPE_MOBILE, self, ch, &evt);
    if (env) {
        prog_execute(env);
    }
    return (env && env->executed);
}

void
trigger_prog_idle(void *owner, enum prog_evt_type owner_type)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!prog_get_obj(owner, owner_type)) {
        return;
    }

    loop_fence++;

    evt.phase = PROG_EVT_HANDLE;
    evt.kind = PROG_EVT_IDLE;
    evt.cmd = -1;
    evt.subject = NULL;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    // We start an idle mobprog here
    env = prog_start(owner_type, owner, NULL, &evt);
    prog_execute(env);

    loop_fence--;
}

// handles idle combat actions
void
trigger_prog_combat(void *owner, enum prog_evt_type owner_type)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!prog_get_obj(owner, owner_type)) {
        return;
    }

    loop_fence++;

    evt.phase = PROG_EVT_HANDLE;
    evt.kind = PROG_EVT_COMBAT;
    evt.cmd = -1;
    evt.subject = NULL;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    // We start a combat prog here
    env = prog_start(owner_type, owner, NULL, &evt);
    prog_execute(env);

    loop_fence--;
}

void
trigger_prog_room_load(struct room_data *owner)
{
    if (!GET_ROOM_PROGOBJ(owner)) {
        return;
    }

    struct prog_evt evt = {
        .phase = PROG_EVT_AFTER,
        .kind = PROG_EVT_LOAD,
        .cmd = -1,
        .args = "",
        .subject = NULL,
        .object = NULL,
        .object_type = PROG_TYPE_NONE,
    };

    struct prog_env *env = prog_start(PROG_TYPE_ROOM, owner, NULL, &evt);
    prog_execute(env);
}

void
trigger_prog_load(struct creature *owner)
{
    struct prog_env *env;
    struct prog_evt evt;

    // Do we have a mobile program?
    if (!GET_NPC_PROGOBJ(owner)) {
        return;
    }

    evt.phase = PROG_EVT_AFTER;
    evt.kind = PROG_EVT_LOAD;
    evt.cmd = -1;
    evt.subject = owner;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    env = prog_start(PROG_TYPE_MOBILE, owner, NULL, &evt);
    prog_execute(env);
}

void
trigger_prog_tick(void *owner, enum prog_evt_type owner_type)
{
    struct prog_env *env;
    struct prog_evt evt;

    if (!prog_get_obj(owner, owner_type)) {
        return;
    }

    loop_fence++;

    evt.phase = PROG_EVT_HANDLE;
    evt.kind = PROG_EVT_TICK;
    evt.cmd = -1;
    evt.subject = NULL;
    evt.object = NULL;
    evt.object_type = PROG_TYPE_NONE;
    strcpy_s(evt.args, sizeof(evt.args), "");

    env = prog_start(owner_type, owner, NULL, &evt);
    prog_execute(env);

    loop_fence--;
}

void
prog_state_free(struct prog_state_data *state)
{
    struct prog_var *cur_var, *next_var;

    if (state) {
        for (cur_var = state->var_list; cur_var; cur_var = next_var) {
            next_var = cur_var->next;
            free(cur_var);
        }
        free(state);
    }
}

static void
prog_free(struct prog_env *prog)
{
    prog_state_free(prog->state);
    if (prog->owner) {
        if (prog->owner_type == PROG_TYPE_MOBILE) {
            ((struct creature *)prog->owner)->prog_marker -= 1;
        } else {
            ((struct room_data *)prog->owner)->prog_marker -= 1;
        }
    }
    active_progs = g_list_remove(active_progs, prog);
    g_trash_stack_push(&dead_progs, prog);
}

static void
prog_execute_and_mark(void)
{
    // Execute progs and mark them as non-idle
    GList *next;
    for (GList *cur = active_progs; cur; cur = next) {
        struct prog_env *cur_prog = cur->data;
        next = cur->next;

        if (cur_prog->next_tick > prog_tick) {
            continue;
        }
        if (cur_prog->exec_pt < 0) {
            continue;
        }

        prog_execute(cur_prog);
    }
}

static void
prog_free_terminated(void)
{
    GList *next;

    for (GList *cur = active_progs; cur; cur = next) {
        struct prog_env *cur_prog = cur->data;
        next = cur->next;

        if (cur_prog->exec_pt < 0 || !cur_prog->owner) {
            prog_free(cur_prog);
        }
    }
}

static void
prog_trigger_idle_mobs(void)
{
    for (GList *cit = first_living(creatures); cit; cit = next_living(cit)) {
        struct creature *ch = cit->data;
        if (ch->prog_marker || !GET_NPC_PROGOBJ(ch)) {
            continue;
        } else if (is_fighting(ch)) {
            trigger_prog_combat(ch, PROG_TYPE_MOBILE);
        } else {
            trigger_prog_idle(ch, PROG_TYPE_MOBILE);
        }
    }
}

static void
prog_trigger_idle_rooms(void)
{
    struct zone_data *zone;
    struct room_data *room;

    for (zone = zone_table; zone; zone = zone->next) {
        if (ZONE_FLAGGED(zone, ZONE_FROZEN)
            || zone->idle_time >= ZONE_IDLE_TIME) {
            continue;
        }

        for (room = zone->world; room; room = room->next) {
            if (GET_ROOM_PROGOBJ(room) && !room->prog_marker) {
                trigger_prog_idle(room, PROG_TYPE_ROOM);
            }
        }
    }
}

void
prog_update(void)
{
    prog_trigger_idle_mobs();
    prog_trigger_idle_rooms();

    prog_execute_and_mark();
    prog_tick++;

    prog_free_terminated();
}

void
prog_update_pending(void)
{
    prog_execute_and_mark();
}

size_t
free_prog_count(void)
{
    return g_trash_stack_height(&dead_progs);
}

size_t
prog_count(bool total)
{
    if (total) {
        return g_list_length(active_progs) + free_prog_count();
    } else {
        return g_list_length(active_progs);
    }
}
