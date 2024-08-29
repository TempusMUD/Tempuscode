#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>
#include <check.h>

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
#include "accstr.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "strutil.h"
#include "prog.h"
#include "quest.h"
#include "help.h"
#include "editor.h"
#include "testing.h"

extern int current_mob_idnum;
extern GHashTable *rooms;
extern GHashTable *mob_prototypes;
extern GHashTable *obj_prototypes;

static struct creature *ch = NULL;

extern GList *quests;
struct quest *make_quest(long owner_id,
                         int owner_level,
                         int type,
                         const char *name);
void free_quest(struct quest *quest);
struct quest *load_quest(xmlNodePtr n, xmlDocPtr doc);
void save_quest(struct quest *quest, FILE *out);
struct qplayer_data *quest_player_by_idnum(struct quest *quest, int idnum);
bool banned_from_quest(struct quest *quest, int id);
void add_quest_player(struct quest *quest, int id);
bool remove_quest_player(struct quest *quest, int id);
bool can_join_quest(struct quest *quest, struct creature *ch);
bool crashsave(struct creature *ch);
ACMD(do_quest);
ACMD(do_qcontrol);

static void
fixture_make_player(void)
{
    ch = make_test_player("foo", "Foo");
    crashsave(ch);
}

static void
fixture_destroy_player(void)
{
    destroy_test_player(ch);
}

START_TEST(test_next_quest_vnum)
{
    int next_quest_vnum(void);

    ck_assert_int_eq(next_quest_vnum(), 1);

    struct quest *q = malloc(sizeof(*q));
    q->vnum = 5;
    quests = g_list_prepend(quests, q);
    ck_assert_int_eq(next_quest_vnum(), 6);
    free(q);
    g_list_free(quests);
    quests = NULL;
}
END_TEST
START_TEST(test_make_destroy_quest)
{
    struct quest *q = make_quest(2, 72, 0, "Test quest");
    ck_assert_ptr_nonnull(q);

    ck_assert_int_eq(q->owner_id, 2);
    ck_assert_int_eq(q->owner_level, 72);
    ck_assert_int_eq(q->type, 0);
    ck_assert_str_eq(q->name, "Test quest");

    free_quest(q);
}
END_TEST

struct quest *
random_quest(void)
{
    struct quest *q = make_quest(2, 72, 0, "Test quest");

    q->started = time(NULL);
    q->ended = q->started + 10;
    q->max_players = number(1, 10);
    q->maxlevel = number(1, 49);
    q->minlevel = number(1, 49);
    q->maxgen = number(0, 10);
    q->mingen = number(0, 10);
    q->awarded = number(0, 5);
    q->penalized = number(0, 5);
    q->flags = number(0, 65535);
    q->loadroom = number(3000, 5000);
    free(q->description);
    q->description = strdup("This is a test quest.");
    free(q->updates);
    q->updates = strdup("Test quest updates");

    return q;
}

void
add_test_questor(struct quest *q, int idnum, int flags, int deaths, int mobkills, int pkills)
{
    struct qplayer_data *qp;

    CREATE(qp, struct qplayer_data, 1);
    qp->idnum = idnum;
    qp->flags = flags;
    qp->deaths = deaths;
    qp->mobkills = mobkills;
    qp->pkills = pkills;

    q->players = g_list_prepend(q->players, qp);
}

void
add_test_quest_ban(struct quest *q, int idnum)
{
    q->bans = g_list_prepend(q->bans, GINT_TO_POINTER(idnum));
}

void
compare_quests(struct quest *a, struct quest *b)
{
    ck_assert_int_eq(a->max_players, b->max_players);
    ck_assert_int_eq(a->awarded, b->awarded);
    ck_assert_int_eq(a->penalized, b->penalized);
    ck_assert_int_eq(a->vnum, b->vnum);
    ck_assert_int_eq(a->owner_id, b->owner_id);
    ck_assert_int_eq(a->started, b->started);
    ck_assert_int_eq(a->ended, b->ended);
    ck_assert_int_eq(a->flags, b->flags);
    ck_assert_int_eq(a->owner_level, b->owner_level);
    ck_assert_int_eq(a->minlevel, b->minlevel);
    ck_assert_int_eq(a->maxlevel, b->maxlevel);
    ck_assert_int_eq(a->mingen, b->mingen);
    ck_assert_int_eq(a->maxgen, b->maxgen);
    ck_assert_int_eq(a->loadroom, b->loadroom);
    ck_assert_int_eq(a->type, b->type);
    ck_assert_str_eq(a->name, b->name);
    ck_assert_pstr_eq(a->description, b->description);
    ck_assert_pstr_eq(a->updates, b->updates);

    GList *al = a->players;
    GList *bl = b->players;
    while (al && bl) {
        struct qplayer_data *ap = al->data;
        struct qplayer_data *bp = bl->data;

        ck_assert_int_eq(ap->idnum, bp->idnum);
        ck_assert_int_eq(ap->flags, bp->flags);
        ck_assert_int_eq(ap->deaths, bp->deaths);
        ck_assert_int_eq(ap->mobkills, bp->mobkills);
        ck_assert_int_eq(ap->pkills, bp->pkills);

        al = al->next;
        bl = bl->next;
    }
    ck_assert_ptr_null(al);
    ck_assert_ptr_null(bl);

    al = a->bans;
    bl = b->bans;
    while (al && bl) {
        ck_assert_ptr_eq(al->data, bl->data);
        al = al->next;
        bl = bl->next;
    }
    ck_assert_ptr_null(al);
    ck_assert_ptr_null(bl);
}

START_TEST(test_save_load_quest)
{
    struct quest *q = random_quest();

    add_test_questor(q, 2, QP_IGNORE, 3, 4, 5);
    add_test_questor(q, 5, QP_MUTE, 6, 7, 8);
    add_test_quest_ban(q, 6);
    add_test_quest_ban(q, 7);

    FILE *out = fopen(test_path("quest.test"), "w");

    save_quest(q, out);
    fclose(out);

    xmlDocPtr doc = xmlParseFile(test_path("quest.test"));
    xmlNodePtr root = xmlDocGetRootElement(doc);

    struct quest *lq = load_quest(root, doc);

    compare_quests(q, lq);

    xmlFreeDoc(doc);
    free_quest(lq);
    free_quest(q);
}
END_TEST
START_TEST(test_quest_player_by_idnum)
{
    struct quest *q = random_quest();
    struct qplayer_data *qp;

    add_test_questor(q, 2, QP_IGNORE, 3, 4, 5);

    qp = quest_player_by_idnum(q, 100);
    ck_assert_ptr_null(qp);

    qp = quest_player_by_idnum(q, 2);
    ck_assert_ptr_nonnull(qp);
    ck_assert_int_eq(qp->idnum, 2);
    free_quest(q);
}
END_TEST
START_TEST(test_banned_from_quest)
{
    struct quest *q = random_quest();

    add_test_questor(q, 2, QP_IGNORE, 3, 4, 5);
    add_test_quest_ban(q, 7);

    ck_assert(!banned_from_quest(q, 2));
    ck_assert(banned_from_quest(q, 7));
    free_quest(q);
}
END_TEST
START_TEST(test_add_remove_quest_player)
{
    struct quest *q = random_quest();

    test_creature_to_world(ch);

    add_quest_player(q, GET_IDNUM(ch));
    ck_assert_ptr_nonnull(quest_player_by_idnum(q, GET_IDNUM(ch)));
    remove_quest_player(q, GET_IDNUM(ch));
    ck_assert_ptr_null(quest_player_by_idnum(q, GET_IDNUM(ch)));
    free_quest(q);
}
END_TEST
START_TEST(test_qcontrol_create)
{
    quests = NULL;
    GET_LEVEL(ch) = LVL_IMMORT;
    do_qcontrol(ch, "create trivia Test quest", 0, 0);
    ck_assert_int_eq(g_list_length(quests), 1);

    struct quest *q = quests->data;

    ck_assert_str_eq(q->name, "Test quest");
    ck_assert_int_eq(q->type, 0);
    ck_assert_int_eq(q->owner_id, GET_IDNUM(ch));
    ck_assert_int_eq(q->owner_level, GET_LEVEL(ch));
    ck_assert_int_eq(g_list_length(q->players), 1);

    struct qplayer_data *p = q->players->data;

    ck_assert_int_eq(p->idnum, GET_IDNUM(ch));
    free_quest(q);
}
END_TEST
START_TEST(test_qcontrol_end)
{
    struct quest *q = random_quest();
    quests = g_list_prepend(quests, q);

    add_quest_player(q, GET_IDNUM(ch));
    q->ended = 0;

    GET_LEVEL(ch) = LVL_IMMORT;
    do_qcontrol(ch, tmp_sprintf("end %d", q->vnum), 0, 0);

    ck_assert_int_ne(q->ended, 0);
    ck_assert_ptr_null(q->players);
    free_quest(q);
}
END_TEST
START_TEST(test_qcontrol_add)
{
    struct creature *tch = make_test_player("xernst", "Xernst");

    test_creature_to_world(ch);
    test_creature_to_world(tch);

    struct quest *q = random_quest();
    quests = g_list_prepend(quests, q);
    q->owner_id = GET_IDNUM(ch);
    q->ended = 0;
    q->players = NULL;

    GET_LEVEL(ch) = LVL_IMMORT;
    do_qcontrol(ch, tmp_sprintf("add Xernst %d", q->vnum), 0, 0);
    ck_assert_msg(strstr(ch->desc->io->write_buf->str, "added") != NULL,
                "qcontrol add yielded output '%s'", ch->desc->io->write_buf->str);
    ck_assert_msg(g_list_length(q->players) == 1,
                "Expected one player, got %d players",
                g_list_length(q->players));
    struct qplayer_data *p = q->players->data;
    ck_assert_int_eq(p->idnum, GET_IDNUM(tch));
    free_quest(q);
    destroy_test_player(tch);
}
END_TEST
START_TEST(test_qcontrol_show)
{}
END_TEST
START_TEST(test_qcontrol_kick)
{
    struct creature *tch = make_test_player("xernst", "Xernst");

    test_creature_to_world(ch);
    test_creature_to_world(tch);

    struct quest *q = random_quest();
    quests = g_list_prepend(quests, q);
    q->owner_id = GET_IDNUM(ch);
    q->ended = 0;
    q->players = NULL;

    add_quest_player(q, GET_IDNUM(tch));

    GET_LEVEL(ch) = LVL_IMMORT;
    do_qcontrol(ch, tmp_sprintf("kick Xernst %d", q->vnum), 0, 0);
    ck_assert_msg(strstr(ch->desc->io->write_buf->str, "kicked") != NULL,
                "qcontrol kick yielded output '%s'", ch->desc->io->write_buf->str);
    ck_assert_msg(g_list_length(q->players) == 0,
                "Expected 0 players, got %d players",
                g_list_length(q->players));
    destroy_test_player(tch);
    free_quest(q);
}
END_TEST
START_TEST(test_qcontrol_flags)
{}
END_TEST
START_TEST(test_qcontrol_comment)
{}
END_TEST
START_TEST(test_qcontrol_describe)
{}
END_TEST
START_TEST(test_qcontrol_update)
{}
END_TEST
START_TEST(test_qcontrol_ban)
{}
END_TEST
START_TEST(test_qcontrol_unban)
{}
END_TEST
START_TEST(test_qcontrol_mute)
{}
END_TEST
START_TEST(test_qcontrol_unmute)
{}
END_TEST
START_TEST(test_qcontrol_level)
{}
END_TEST
START_TEST(test_qcontrol_minlev)
{}
END_TEST
START_TEST(test_qcontrol_maxlev)
{}
END_TEST
START_TEST(test_qcontrol_mingen)
{}
END_TEST
START_TEST(test_qcontrol_maxgen)
{}
END_TEST
START_TEST(test_qcontrol_mload)
{}
END_TEST
START_TEST(test_qcontrol_purge)
{}
END_TEST
START_TEST(test_qcontrol_save)
{}
END_TEST
START_TEST(test_qcontrol_help)
{}
END_TEST
START_TEST(test_qcontrol_switch)
{}
END_TEST
START_TEST(test_qcontrol_title)
{}
END_TEST
START_TEST(test_qcontrol_oload)
{}
END_TEST
START_TEST(test_qcontrol_trans)
{}
END_TEST
START_TEST(test_qcontrol_award)
{}
END_TEST
START_TEST(test_qcontrol_penalize)
{}
END_TEST
START_TEST(test_qcontrol_restore)
{}
END_TEST
START_TEST(test_qcontrol_loadroom)
{}
END_TEST
START_TEST(test_can_join_quest_1)
{
    struct quest *q = random_quest();

    q->flags = QUEST_NOJOIN;
    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_2)
{
    struct quest *q = random_quest();

    q->flags = 0;
    GET_LEVEL(ch) = LVL_AMBASSADOR;

    ck_assert(can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_3)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->maxgen = number(0, 9);
    GET_REMORT_GEN(ch) = q->maxgen + 1;

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_4)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->mingen = number(1, 10);
    GET_REMORT_GEN(ch) = q->mingen - 1;

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_5)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->maxlevel = number(0, 48);
    GET_REMORT_GEN(ch) = q->maxlevel + 1;

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_6)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->minlevel = number(2, 50);
    GET_REMORT_GEN(ch) = q->minlevel - 1;

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_7)
{
    struct quest *q = random_quest();

    q->flags = 0;
    ch->account->quest_banned = true;

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_8)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->bans = g_list_prepend(q->bans, GINT_TO_POINTER(GET_IDNUM(ch)));

    ck_assert(!can_join_quest(q, ch));
    free_quest(q);
}
END_TEST
START_TEST(test_can_join_quest_9)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->maxgen = 10;
    q->mingen = 0;
    q->maxlevel = LVL_AMBASSADOR;
    q->minlevel = 0;

    ck_assert_msg(can_join_quest(q, ch), "couldn't join quest");
    free_quest(q);
}
END_TEST
START_TEST(test_quest_list)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->ended = 0;
    q->maxgen = 10;
    q->mingen = 0;
    q->maxlevel = LVL_AMBASSADOR;
    q->minlevel = 0;

    quests = g_list_prepend(quests, q);
    do_quest(ch, "list", 0, 0);
    ck_assert_msg(strstr(ch->desc->io->write_buf->str, q->name) != NULL,
                "Quest name not found in '%s'", ch->desc->io->write_buf->str);
    free_quest(q);
    g_list_free(quests);
    quests = NULL;
}
END_TEST
START_TEST(test_quest_join_leave)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->ended = 0;
    q->maxgen = 10;
    q->mingen = 0;
    q->maxlevel = LVL_AMBASSADOR;
    q->minlevel = 0;

    quests = g_list_prepend(quests, q);
    do_quest(ch, tmp_sprintf("join %d", q->vnum), 0, 0);
    ck_assert_int_eq(GET_QUEST(ch), q->vnum);
    ck_assert_ptr_nonnull(quest_player_by_idnum(q, GET_IDNUM(ch)));
    ck_assert(strstr(ch->desc->io->write_buf->str,
                       "You have joined quest 'Test quest'") != NULL);

    ch->desc->io->write_buf->str[0] = '\0';
    ch->desc->io->write_buf->len = 0;
    do_quest(ch, tmp_sprintf("leave %d", q->vnum), 0, 0);
    ck_assert_int_eq(GET_QUEST(ch), 0);
    ck_assert_ptr_null(quest_player_by_idnum(q, GET_IDNUM(ch)));
    ck_assert_msg(strstr(ch->desc->io->write_buf->str,
                       "You have left quest 'Test quest'") != NULL,
                "quest leave yielded output '%s'", ch->desc->io->write_buf->str);
    free_quest(q);
}
END_TEST

Suite *
quest_suite(void)
{
    Suite *s = suite_create("quest");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, test_tempus_boot, test_tempus_cleanup);
    tcase_add_checked_fixture(tc_core, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_core, test_next_quest_vnum);
    tcase_add_test(tc_core, test_make_destroy_quest);
    tcase_add_test(tc_core, test_save_load_quest);
    tcase_add_test(tc_core, test_quest_player_by_idnum);
    tcase_add_test(tc_core, test_banned_from_quest);
    tcase_add_test(tc_core, test_add_remove_quest_player);
    tcase_add_test(tc_core, test_can_join_quest_1);
    tcase_add_test(tc_core, test_can_join_quest_2);
    tcase_add_test(tc_core, test_can_join_quest_3);
    tcase_add_test(tc_core, test_can_join_quest_4);
    tcase_add_test(tc_core, test_can_join_quest_5);
    tcase_add_test(tc_core, test_can_join_quest_6);
    tcase_add_test(tc_core, test_can_join_quest_7);
    tcase_add_test(tc_core, test_can_join_quest_8);
    tcase_add_test(tc_core, test_can_join_quest_9);
    tcase_add_test(tc_core, test_quest_list);
    tcase_add_test(tc_core, test_quest_join_leave);
    suite_add_tcase(s, tc_core);

    TCase *tc_qcontrol = tcase_create("qcontrol");
    tcase_add_checked_fixture(tc_qcontrol, test_tempus_boot, test_tempus_cleanup);
    tcase_add_checked_fixture(tc_qcontrol, fixture_make_player, fixture_destroy_player);
    tcase_add_test(tc_qcontrol, test_qcontrol_create);
    tcase_add_test(tc_qcontrol, test_qcontrol_end);
    tcase_add_test(tc_qcontrol, test_qcontrol_add);
    tcase_add_test(tc_qcontrol, test_qcontrol_show);
    tcase_add_test(tc_qcontrol, test_qcontrol_kick);
    tcase_add_test(tc_qcontrol, test_qcontrol_flags);
    tcase_add_test(tc_qcontrol, test_qcontrol_comment);
    tcase_add_test(tc_qcontrol, test_qcontrol_describe);
    tcase_add_test(tc_qcontrol, test_qcontrol_update);
    tcase_add_test(tc_qcontrol, test_qcontrol_ban);
    tcase_add_test(tc_qcontrol, test_qcontrol_unban);
    tcase_add_test(tc_qcontrol, test_qcontrol_mute);
    tcase_add_test(tc_qcontrol, test_qcontrol_unmute);
    tcase_add_test(tc_qcontrol, test_qcontrol_level);
    tcase_add_test(tc_qcontrol, test_qcontrol_minlev);
    tcase_add_test(tc_qcontrol, test_qcontrol_maxlev);
    tcase_add_test(tc_qcontrol, test_qcontrol_mingen);
    tcase_add_test(tc_qcontrol, test_qcontrol_maxgen);
    tcase_add_test(tc_qcontrol, test_qcontrol_mload);
    tcase_add_test(tc_qcontrol, test_qcontrol_purge);
    tcase_add_test(tc_qcontrol, test_qcontrol_save);
    tcase_add_test(tc_qcontrol, test_qcontrol_help);
    tcase_add_test(tc_qcontrol, test_qcontrol_switch);
    tcase_add_test(tc_qcontrol, test_qcontrol_title);
    tcase_add_test(tc_qcontrol, test_qcontrol_oload);
    tcase_add_test(tc_qcontrol, test_qcontrol_trans);
    tcase_add_test(tc_qcontrol, test_qcontrol_award);
    tcase_add_test(tc_qcontrol, test_qcontrol_penalize);
    tcase_add_test(tc_qcontrol, test_qcontrol_restore);
    tcase_add_test(tc_qcontrol, test_qcontrol_loadroom);
    suite_add_tcase(s, tc_qcontrol);

    return s;
}
