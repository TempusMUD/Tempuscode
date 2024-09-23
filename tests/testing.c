#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>
#include <check.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "interpreter.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "players.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"
#include "zone_data.h"
#include "char_class.h"
#include "testing.h"
#include "language.h"

extern PGconn *sql_cxn;
extern GList *creatures;
extern GHashTable *creature_map;
extern GHashTable *rooms;
extern GMainLoop *main_loop;
extern FILE *qlogfile;

void boot_tongues(const char *path);
void boot_spells(const char *path);
void boot_sectors(const char *path);
void extract_creature(struct creature *ch, enum cxn_state con_state);
void free_account(struct account *acct);

const char *
test_path(char *relpath)
{
    static char buf[PATH_MAX + 1];

    snprintf(buf, sizeof(buf), "/tmp/tempustest-%s/", getenv("LOGNAME"));
    (void)mkdir(buf, 0755);
    strcat_s(buf, sizeof(buf), relpath);

    return buf;
}

void
test_tempus_boot(void)
{
    tmp_string_init();
    if (!sql_cxn) {
        sql_cxn = PQconnectdb("user=realm dbname=devtempus");
    }

    if (!sql_cxn) {
        slog("Couldn't allocate postgres connection!");
        safe_exit(1);
    }
    if (PQstatus(sql_cxn) != CONNECTION_OK) {
        slog("Couldn't connect to postgres!: %s", PQerrorMessage(sql_cxn));
        safe_exit(1);
    }

    creatures = NULL;
    rooms = g_hash_table_new(g_direct_hash, g_direct_equal);
    mob_prototypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    obj_prototypes = g_hash_table_new(g_direct_hash, g_direct_equal);
    creature_map = g_hash_table_new(g_direct_hash, g_direct_equal);

    if (chdir("../../lib") < 0) {
        slog("Couldn't change directory to lib");
    }

    account_boot();
    boot_tongues("etc/tongues.xml");
    boot_races("etc/races.xml");
    boot_spells("etc/spells.xml");
    boot_sectors("etc/sectors.xml");
    qlogfile = fopen(QLOGFILENAME, "a");

    struct zone_data *zone = make_zone(1);
    struct room_data *room_a = make_room(zone, 1);
    struct room_data *room_b = make_room(zone, 2);
    link_rooms(room_a, room_b, NORTH);

    main_loop = g_main_loop_new(NULL, false);
}

void
test_tempus_cleanup(void)
{
    tmp_string_cleanup();
    fclose(qlogfile);
}

int
random_char_class(void)
{
    int choices[] = { CLASS_MAGIC_USER, CLASS_CLERIC, CLASS_BARB, CLASS_THIEF,
                      CLASS_KNIGHT, CLASS_RANGER, CLASS_MONK, CLASS_CYBORG,
                      CLASS_PSIONIC, CLASS_PHYSIC, CLASS_MONK,
                      CLASS_MERCENARY, CLASS_BARD, -1 };
    int len = 0;

    while (choices[len] != -1) {
        len++;
    }

    return choices[number(0, len - 1)];
}

void
randomize_creature(struct creature *ch, int char_class)
{
    time_t now = time(NULL);

    GET_CLASS(ch) = (char_class == CLASS_UNDEFINED) ? random_char_class() : char_class;

    GET_MAX_HIT(ch) = number(100, 1000);
    GET_MAX_MANA(ch) = number(100, 1000);
    GET_MAX_MOVE(ch) = number(100, 1000);
    GET_HIT(ch) = number(1, GET_MAX_HIT(ch));
    GET_MANA(ch) = number(1, GET_MAX_MANA(ch));
    GET_MOVE(ch) = number(1, GET_MAX_MOVE(ch));
    GET_AC(ch) = number(-300, 100);
    GET_GOLD(ch) = number(0, 1000000);
    GET_CASH(ch) = number(0, 1000000);
    GET_LEVEL(ch) = number(1, 49);
    GET_EXP(ch) = exp_scale[(int)GET_LEVEL(ch)];
    GET_HITROLL(ch) = number(0, 125);
    GET_DAMROLL(ch) = number(0, 125);
    GET_SEX(ch) = number(0, 2);
    GET_RACE(ch) = number(0,7);
    GET_HEIGHT(ch) = number(1,200);
    GET_WEIGHT(ch) = number(1,200);
    GET_ALIGNMENT(ch) = number(-1000,1000);
    GET_REMORT_GEN(ch) = number(0, 10);
    if (GET_REMORT_GEN(ch) > 0) {
        GET_REMORT_CLASS(ch) = random_char_class();
    }
    if (IS_CYBORG(ch)) {
        GET_OLD_CLASS(ch) = number(0, 2);
        GET_TOT_DAM(ch) = number(0, 10);
        GET_BROKE(ch) = number(0, 65535);
    }
    ch->player.time.birth = now - number(100000, 1000000);
    ch->player.time.death = now - number(0, 100000);
    ch->player.time.logon = now - number(0, 100000);
    ch->player.time.played = number(0, 100000);

    ch->real_abils.str = number(3, 35);
    ch->real_abils.intel = number(3, 25);
    ch->real_abils.wis = number(3, 25);
    ch->real_abils.dex = number(3, 25);
    ch->real_abils.con = number(3, 25);
    ch->real_abils.cha = number(3, 25);
    GET_COND(ch, FULL) = number(0, 20);
    GET_COND(ch, THIRST) = number(0, 20);
    GET_COND(ch, DRUNK) = number(0, 20);
    GET_WIMP_LEV(ch) = number(0, GET_MAX_HIT(ch) / 2);
    GET_LIFE_POINTS(ch) = number(0, 100);
    GET_CLAN(ch) = number(0, 10);
    GET_HOME(ch) = number(0, 10);
    GET_HOMEROOM(ch) = number(3000, 5000);
    GET_LOADROOM(ch) = number(3000, 5000);
    GET_QUEST(ch) = number(1, 60);
    GET_IMMORT_QP(ch) = number(1, 10);
    GET_QUEST_ALLOWANCE(ch) = number(1, 10);
    ch->char_specials.saved.act = number(0, 65535) & ~NPC_ISNPC;
    ch->player_specials->saved.plr2_bits = number(0, 65535);
    ch->player_specials->saved.pref = number(0, 65535);
    ch->player_specials->saved.pref2 = number(0, 65535);
    GET_TONGUE(ch) = number(0, 34);

    for (int i = 0; i < number(0, MAX_WEAPON_SPEC); i++) {
        GET_WEAP_SPEC(ch, i).vnum = number(1, 30000);
        GET_WEAP_SPEC(ch, i).level = number(1, 5);
    }
    free(GET_TITLE(ch));
    GET_TITLE(ch) = strdup("");
}

void
test_creature_to_world(struct creature *ch)
{
    creatures = g_list_prepend(creatures, ch);
    if (IS_NPC(ch)) {
        g_hash_table_insert(creature_map, GINT_TO_POINTER(-NPC_IDNUM(ch)), ch);
    } else {
        g_hash_table_insert(creature_map, GINT_TO_POINTER(GET_IDNUM(ch)), ch);
    }

    char_to_room(ch, real_room(1), false);
    GET_POSITION(ch) = POS_STANDING;
}

gboolean
dummy_handler(GIOChannel *io,
              GIOCondition condition,
              gpointer data)
{
    return true;
}

gboolean
dummy_timer(gpointer data)
{
    return true;
}


struct creature *
make_test_player(const char *acct_name, const char *char_name)
{
    struct account *acct;
    struct descriptor_data *desc;

    sql_exec("delete from players where account=99999");
    sql_exec("delete from accounts where idnum=99999");
    CREATE(desc, struct descriptor_data, 1);
    memset(desc, 0, sizeof(struct descriptor_data));
    strcpy(desc->host, "127.0.0.1");
    desc->login_time = time(NULL);


    desc->io = g_io_channel_new_file("/dev/null", "w+", NULL);
    desc->in_watcher = g_io_add_watch(desc->io, G_IO_IN | G_IO_HUP, dummy_handler, desc);
    desc->err_watcher = g_io_add_watch(desc->io, G_IO_ERR, dummy_handler, desc);
    desc->input_handler = g_timeout_add(100, dummy_timer, desc);

    desc->input = g_queue_new();

    CREATE(acct, struct account, 1);
    account_initialize(acct, acct_name, desc, 99999);
    g_hash_table_insert(account_cache, GINT_TO_POINTER(account_top_id), acct);

    struct creature *ch = account_create_char(acct, char_name, 99999);
    ch->desc = desc;
    ch->account = acct;

    GET_EXP(ch) = 1;

    return ch;
}

void
destroy_test_player(struct creature *ch)
{
    account_delete_char(ch->account, ch);
    g_hash_table_remove(account_cache, GINT_TO_POINTER(ch->account->id));
    sql_exec("delete from accounts where idnum=%d", ch->account->id);
    free_account(ch->account);
    if (ch->in_room) {
        extract_creature(ch, CXN_DISCONNECT);
    } else {
        free_creature(ch);
    }
}

void
compare_creatures(struct creature *ch, struct creature *tch)
{
    ck_assert_str_eq(GET_NAME(ch), GET_NAME(tch));
    ck_assert_int_eq(GET_IDNUM(ch), GET_IDNUM(tch));
    ck_assert_int_eq(GET_HIT(ch), GET_HIT(tch));
    ck_assert_int_eq(GET_MANA(ch), GET_MANA(tch));
    ck_assert_int_eq(GET_MOVE(ch), GET_MOVE(tch));
    ck_assert_int_eq(GET_MAX_HIT(ch), GET_MAX_HIT(tch));
    ck_assert_int_eq(GET_MAX_MANA(ch), GET_MAX_MANA(tch));
    ck_assert_int_eq(GET_MAX_MOVE(ch), GET_MAX_MOVE(tch));
    ck_assert_int_eq(GET_GOLD(ch), GET_GOLD(tch));
    ck_assert_int_eq(GET_CASH(ch), GET_CASH(tch));
    ck_assert_int_eq(GET_EXP(ch), GET_EXP(tch));
    ck_assert_int_eq(GET_HITROLL(ch), GET_HITROLL(tch));
    ck_assert_int_eq(GET_DAMROLL(ch), GET_DAMROLL(tch));
    ck_assert_int_eq(GET_LEVEL(ch), GET_LEVEL(tch));
    ck_assert_int_eq(GET_SEX(ch), GET_SEX(tch));
    ck_assert_int_eq(GET_RACE(ch), GET_RACE(tch));
    ck_assert_int_eq(GET_HEIGHT(ch), GET_HEIGHT(tch));
    ck_assert_int_eq(GET_WEIGHT(ch), GET_WEIGHT(tch));
    ck_assert_int_eq(GET_ALIGNMENT(ch), GET_ALIGNMENT(tch));
    ck_assert_int_eq(GET_CLASS(ch), GET_CLASS(tch));
    ck_assert_int_eq(GET_REMORT_CLASS(ch), GET_REMORT_CLASS(tch));
    ck_assert_int_eq(GET_REMORT_GEN(ch), GET_REMORT_GEN(tch));
    ck_assert(!memcmp(&ch->player.time, &tch->player.time, sizeof(ch->player.time)));
    ck_assert(!memcmp(&ch->real_abils, &tch->real_abils, sizeof(ch->real_abils)));
    ck_assert_int_eq(GET_COND(ch, FULL), GET_COND(tch, FULL));
    ck_assert_int_eq(GET_COND(ch, THIRST), GET_COND(tch, THIRST));
    ck_assert_int_eq(GET_COND(ch, DRUNK), GET_COND(tch, DRUNK));
    ck_assert_int_eq(GET_WIMP_LEV(ch), GET_WIMP_LEV(tch));
    ck_assert_int_eq(GET_LIFE_POINTS(ch), GET_LIFE_POINTS(tch));
    ck_assert_int_eq(GET_CLAN(ch), GET_CLAN(tch));
    ck_assert_int_eq(GET_HOME(ch), GET_HOME(tch));
    ck_assert_int_eq(GET_HOMEROOM(ch), GET_HOMEROOM(tch));
    ck_assert_int_eq(GET_LOADROOM(ch), GET_LOADROOM(tch));
    ck_assert_int_eq(GET_QUEST(ch), GET_QUEST(tch));
    ck_assert_int_eq(ch->char_specials.saved.act, tch->char_specials.saved.act);
    ck_assert_int_eq(ch->player_specials->saved.plr2_bits, tch->player_specials->saved.plr2_bits);
    ck_assert_int_eq(ch->player_specials->saved.pref, tch->player_specials->saved.pref);
    ck_assert_int_eq(ch->player_specials->saved.pref2, tch->player_specials->saved.pref2);
    ck_assert_int_eq(GET_TONGUE(ch), GET_TONGUE(tch));
    for (int i = 0; i < MAX_WEAPON_SPEC; i++) {
        ck_assert_int_eq(GET_WEAP_SPEC(ch, i).vnum, GET_WEAP_SPEC(tch, i).vnum);
        ck_assert_int_eq(GET_WEAP_SPEC(ch, i).level, GET_WEAP_SPEC(tch, i).level);
    }
    ck_assert_str_eq(GET_TITLE(ch), GET_TITLE(tch));
}

static int next_mob_vnum = 3000;
extern struct player_special_data dummy_mob;
void recalculate_based_on_level(struct creature *mob_p);

int
make_random_mobile(void)
{
    struct creature *mob;

    CREATE(mob, struct creature, 1);
    CREATE(mob->mob_specials.shared, struct mob_shared_data, 1);

    mob->mob_specials.shared->vnum = next_mob_vnum++;
    mob->mob_specials.shared->number = 0;
    mob->mob_specials.shared->proto = mob;

    mob->player_specials = &dummy_mob;

    mob->player.name = strdup("test mobile");
    mob->player.short_descr = strdup("a test mobile");
    mob->player.long_descr = strdup("A test mobile is here waiting to be tested.");

    SET_BIT(NPC_FLAGS(mob), NPC_ISNPC);
    GET_TONGUE(mob) = TONGUE_COMMON;

    mob->real_abils.str = 11;
    mob->real_abils.intel = 11;
    mob->real_abils.wis = 11;
    mob->real_abils.dex = 11;
    mob->real_abils.con = 11;
    mob->real_abils.cha = 11;

    GET_LEVEL(mob) = 1;

    mob->points.hit = 10;
    mob->points.max_hit = 10;
    mob->points.mana = 10;
    mob->points.max_mana = 10;
    mob->points.move = 50;
    mob->points.max_move = 50;

    mob->mob_specials.shared->damnodice = 5;
    mob->mob_specials.shared->damsizedice = 2;

    mob->player.char_class = CLASS_NORMAL;
    mob->player.race = RACE_MOBILE;

    GET_EXP(mob) = 100;
    GET_MORALE(mob) = 100;
    GET_NPC_LAIR(mob) = -1;
    GET_NPC_LEADER(mob) = -1;

    mob->char_specials.position = POS_STANDING;
    mob->mob_specials.shared->default_pos = POS_STANDING;
    mob->player.sex = SEX_NEUTER;

    mob->player.weight = 200;
    mob->player.height = 198;

    mob->player.remort_char_class = -1;
    recalculate_based_on_level(mob);

    for (int j = 0; j < 3; j++) {
        GET_COND(mob, j) = -1;
    }

    mob->aff_abils = mob->real_abils;

    g_hash_table_insert(mob_prototypes, GINT_TO_POINTER(mob->mob_specials.shared->vnum), mob);

    return mob->mob_specials.shared->vnum;
}

static int next_obj_vnum = 3000;

int
make_random_object(void)
{
    struct obj_data *obj;
    CREATE(obj, struct obj_data, 1);
    CREATE(obj->shared, struct obj_shared_data, 1);
    obj->shared->vnum = next_obj_vnum++;
    obj->shared->number = 0;
    obj->shared->house_count = 0;
    obj->shared->func = NULL;
    obj->shared->proto = obj;
    obj->shared->owner_id = 0;

    obj->name = strdup("test name");
    obj->aliases = strdup("test aliases");
    obj->line_desc = strdup("test line desc");
    obj->action_desc = strdup("test action desc");
    obj->soilage = 84732;
    obj->unique_id = 34827429;
    obj->creation_time = time(NULL);
    obj->creation_method = CREATED_IMM;
    obj->creator = 732;

    obj->worn_on = -1;

    obj->obj_flags.weight = 1.0;
    obj->obj_flags.extra2_flags = 0;

    g_hash_table_insert(obj_prototypes, GINT_TO_POINTER(GET_OBJ_VNUM(obj)), obj);

    return obj->shared->vnum;
}

void
compare_objects(struct obj_data *obj_a, struct obj_data *obj_b)
{
    ck_assert_str_eq(obj_a->name, obj_b->name);
    ck_assert_str_eq(obj_a->aliases, obj_b->aliases);
    ck_assert_str_eq(obj_a->line_desc, obj_b->line_desc);
    ck_assert_str_eq(obj_a->action_desc, obj_b->action_desc);
    ck_assert_int_eq(obj_a->plrtext_len, obj_b->plrtext_len);
    ck_assert_msg(obj_a->worn_on == obj_b->worn_on, "obj_a->worn_on = %d, obj_b->worn_on = %d", obj_a->worn_on, obj_b->worn_on);
    ck_assert_int_eq(obj_a->soilage, obj_b->soilage);
    ck_assert_int_eq(obj_a->unique_id, obj_b->unique_id);
    ck_assert_int_eq(obj_a->creation_time, obj_b->creation_time);
    ck_assert_int_eq(obj_a->creation_method, obj_b->creation_method);
    ck_assert_int_eq(obj_a->creator, obj_b->creator);
    ck_assert_int_eq(obj_a->obj_flags.value[0], obj_b->obj_flags.value[0]);
    ck_assert_int_eq(obj_a->obj_flags.value[1], obj_b->obj_flags.value[1]);
    ck_assert_int_eq(obj_a->obj_flags.value[2], obj_b->obj_flags.value[2]);
    ck_assert_int_eq(obj_a->obj_flags.value[3], obj_b->obj_flags.value[3]);
    ck_assert_int_eq(obj_a->obj_flags.type_flag, obj_b->obj_flags.type_flag);
    ck_assert_int_eq(obj_a->obj_flags.wear_flags, obj_b->obj_flags.wear_flags);
    ck_assert_int_eq(obj_a->obj_flags.extra_flags, obj_b->obj_flags.extra_flags);
    ck_assert_int_eq(obj_a->obj_flags.extra2_flags, obj_b->obj_flags.extra2_flags);
    ck_assert_int_eq(obj_a->obj_flags.extra3_flags, obj_b->obj_flags.extra3_flags);
    ck_assert_int_eq(obj_a->obj_flags.weight, obj_b->obj_flags.weight);
    ck_assert_int_eq(obj_a->obj_flags.timer, obj_b->obj_flags.timer);
    ck_assert_int_eq(obj_a->obj_flags.bitvector[0], obj_b->obj_flags.bitvector[0]);
    ck_assert_int_eq(obj_a->obj_flags.bitvector[1], obj_b->obj_flags.bitvector[1]);
    ck_assert_int_eq(obj_a->obj_flags.bitvector[2], obj_b->obj_flags.bitvector[2]);
    ck_assert_int_eq(obj_a->obj_flags.material, obj_b->obj_flags.material);
    ck_assert_int_eq(obj_a->obj_flags.max_dam, obj_b->obj_flags.max_dam);
    ck_assert_int_eq(obj_a->obj_flags.damage, obj_b->obj_flags.damage);
    ck_assert_int_eq(obj_a->obj_flags.sigil_idnum, obj_b->obj_flags.sigil_idnum);
    ck_assert_int_eq(obj_a->obj_flags.sigil_level, obj_b->obj_flags.sigil_level);

    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
        ck_assert_int_eq(obj_a->affected[i].location, obj_a->affected[i].location);
        ck_assert_int_eq(obj_a->affected[i].modifier, obj_a->affected[i].modifier);
    }

    // TODO: test temp object affects
}
