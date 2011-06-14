#include <check.h>

#include "creature.h"
#include "room_data.h"
#include "zone_data.h"
#include "handler.h"
#include "quest.h"
#include "utils.h"
#include "testing.h"

extern int current_mob_idnum;
extern GHashTable *rooms;
extern GHashTable *mob_prototypes;
extern GHashTable *obj_prototypes;

static struct creature *ch = NULL;
static struct zone_data *zone = NULL;
static struct room_data *room_a = NULL, *room_b = NULL;

extern GList *quests;
struct quest *make_quest(long owner_id,
                         int owner_level,
                         int type,
                         const char *name);
void free_quest(struct quest *quest);
struct quest *load_quest(xmlNodePtr n, xmlDocPtr doc);
void save_quest(struct quest *quest, FILE *out);
struct qplayer_data *quest_player_by_idnum(struct quest *quest, int idnum);
bool banned_from_quest(struct quest * quest, int id);
void add_quest_player(struct quest *quest, int id);
bool remove_quest_player(struct quest *quest, int id);
bool can_join_quest(struct quest *quest, struct creature * ch);
bool crashsave(struct creature *ch);

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

    fail_unless(next_quest_vnum() == 1);

    struct quest *q = malloc(sizeof(*q));
    q->vnum = 5;
    quests = g_list_prepend(quests, q);
    fail_unless(next_quest_vnum() == 6);
    free(q);
    g_list_free(quests);
    quests = NULL;
}
END_TEST

START_TEST(test_make_destroy_quest)
{
    struct quest *q = make_quest(2, 72, QTYPE_TRIVIA, "Test quest");
    fail_unless(q != NULL);

    fail_unless(q->owner_id == 2);
    fail_unless(q->owner_level == 72);
    fail_unless(q->type == QTYPE_TRIVIA);
    fail_unless(!strcmp(q->name, "Test quest"));

    free_quest(q);
}
END_TEST

struct quest *
random_quest(void)
{
    struct quest *q = make_quest(2, 72, QTYPE_TRIVIA, "Test quest");

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
    q->description = strdup("This is a test quest.");
    q->updates = strdup("Test quest updates");

    struct qplayer_data *qp;

    CREATE(qp, struct qplayer_data, 1);
    qp->idnum = 2;
    qp->flags = QP_IGNORE;
    qp->deaths = 3;
    qp->mobkills = 4;
    qp->pkills = 5;

    q->players = g_list_prepend(q->players, qp);

    CREATE(qp, struct qplayer_data, 1);
     qp->idnum = 5;
    qp->flags = QP_MUTE;
    qp->deaths = 6;
    qp->mobkills = 7;
    qp->pkills = 8;

    q->players = g_list_prepend(q->players, qp);

    q->bans = g_list_prepend(q->bans, GINT_TO_POINTER(6));
    q->bans = g_list_prepend(q->bans, GINT_TO_POINTER(7));

    return q;
}

void
compare_quests(struct quest *a, struct quest *b)
{
    fail_unless(a->max_players == b->max_players);
    fail_unless(a->awarded == b->awarded);
    fail_unless(a->penalized == b->penalized);
    fail_unless(a->vnum == b->vnum);
    fail_unless(a->owner_id == b->owner_id);
    fail_unless(a->started == b->started);
    fail_unless(a->ended == b->ended);
    fail_unless(a->flags == b->flags);
    fail_unless(a->owner_level == b->owner_level);
    fail_unless(a->minlevel == b->minlevel);
    fail_unless(a->maxlevel == b->maxlevel);
    fail_unless(a->mingen == b->mingen);
    fail_unless(a->maxgen == b->maxgen);
    fail_unless(a->loadroom == b->loadroom);
    fail_unless(a->type == b->type);
    fail_unless(!strcmp(a->name, b->name));
    fail_unless((a->description == NULL && b->description == NULL)
                || !strcmp(a->description, b->description));
    fail_unless((a->updates == NULL && b->updates == NULL)
                || !strcmp(a->updates, b->updates));

    GList *al = a->players;
    GList *bl = b->players;
    while (al && bl) {
        struct qplayer_data *ap = al->data;
        struct qplayer_data *bp = bl->data;

        fail_unless(ap->idnum == bp->idnum);
        fail_unless(ap->flags == bp->flags);
        fail_unless(ap->deaths == bp->deaths);
        fail_unless(ap->mobkills== bp->mobkills);
        fail_unless(ap->pkills == bp->pkills);

        al = al->next;
        bl = bl->next;
    }
    fail_unless(al == NULL && bl == NULL);

    al = a->bans;
    bl = b->bans;
    while (al && bl) {
        fail_unless(al->data == bl->data);
        al = al->next;
        bl = bl->next;
    }
    fail_unless(al == NULL && bl == NULL);
}

START_TEST(test_save_load_quest)
{
    struct quest *q = random_quest();

    FILE *out = fopen("/tmp/quest.test", "w");

    save_quest(q, out);
    fclose(out);

    xmlDocPtr doc = xmlParseFile("/tmp/quest.test");
    xmlNodePtr root = xmlDocGetRootElement(doc);

    struct quest *lq = load_quest(root, doc);

    compare_quests(q, lq);
}
END_TEST

START_TEST(test_quest_player_by_idnum)
{
    struct quest *q = random_quest();
    struct qplayer_data *qp;

    qp = quest_player_by_idnum(q, 100);
    fail_unless(qp == NULL);

    qp = quest_player_by_idnum(q, 2);
    fail_unless(qp != NULL);
    fail_unless(qp->idnum == 2);
}
END_TEST

START_TEST(test_banned_from_quest)
{
    struct quest *q = random_quest();

    fail_if(banned_from_quest(q, 2));
    fail_unless(banned_from_quest(q, 7));
}
END_TEST

START_TEST(test_add_remove_quest_player)
{
    struct quest *q = random_quest();

    add_quest_player(q, GET_IDNUM(ch));
    fail_unless(quest_player_by_idnum(q, GET_IDNUM(ch)) != NULL);
    remove_quest_player(q, GET_IDNUM(ch));
    fail_if(quest_player_by_idnum(q, GET_IDNUM(ch)) != NULL);
}
END_TEST

START_TEST(test_can_join_quest_1)
{
    struct quest *q = random_quest();

    q->flags = QUEST_NOJOIN;
    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_2)
{
    struct quest *q = random_quest();

    q->flags = 0;
    GET_LEVEL(ch) = LVL_AMBASSADOR;

    fail_unless(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_3)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->maxgen = number(0, 9);
    GET_REMORT_GEN(ch) = q->maxgen + 1;

    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_4)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->mingen = number(1, 10);
    GET_REMORT_GEN(ch) = q->mingen - 1;

    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_5)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->maxlevel = number(0, 48);
    GET_REMORT_GEN(ch) = q->maxlevel + 1;

    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_6)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->minlevel = number(2, 50);
    GET_REMORT_GEN(ch) = q->minlevel - 1;

    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_7)
{
    struct quest *q = random_quest();

    q->flags = 0;
    ch->account->quest_banned = true;

    fail_if(can_join_quest(q, ch));
}
END_TEST

START_TEST(test_can_join_quest_8)
{
    struct quest *q = random_quest();

    q->flags = 0;
    q->bans = g_list_prepend(q->bans, GINT_TO_POINTER(GET_IDNUM(ch)));

    fail_if(can_join_quest(q, ch));
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

    fail_unless(can_join_quest(q, ch), "couldn't join quest: %s", ch->desc->small_outbuf);
}
END_TEST

Suite *
quest_suite(void)
{
    Suite *s = suite_create("quest");

    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, test_tempus_boot, NULL);
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
    suite_add_tcase(s, tc_core);

    return s;
}
