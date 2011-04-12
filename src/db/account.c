#ifdef HAS_CONFIG_H
#include "config.h"
#endif
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <crypt.h>
#include "utils.h"
#include "desc_data.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "account.h"
#include "comm.h"
#include "security.h"
#include "clan.h"
#include "quest.h"
#include "handler.h"
#include "db.h"
#include "mail.h"
#include "players.h"

const char *ansi_levels[] = {
    "none",
    "sparse",
    "normal",
    "complete",
    "\n"
};

const char *compact_levels[] = {
    "off",
    "minimal",
    "partial",
    "full",
    "\n"
};

char *get_account_file_path(long id);

long account_top_id = 0;
GHashTable *account_cache = NULL;

void
account_boot(void)
{
    PGresult *res;

    account_cache = g_hash_table_new(g_direct_hash, g_direct_equal);

    slog("Getting max account idnum");

    res = sql_query("select MAX(idnum) from accounts");
    account_top_id = atol(PQgetvalue(res, 0, 0));

    slog("Getting character count");
    if (player_count())
        slog("... %zd character%s in db", player_count(),
            (player_count() == 1) ? "" : "s");
    else
        slog("WARNING: No characters loaded");
}

size_t
account_cache_size(void)
{
    return g_hash_table_size(account_cache);
}

void
free_account(struct account *acct)
{
    free(acct->name);
    free(acct->password);
    free(acct->email);
    free(acct->creation_addr);
    free(acct->login_addr);
    g_list_free(acct->chars);
    g_list_free(acct->trusted);
    free(acct);
}

void
load_players(struct account *account)
{
    long count, idx;
    PGresult *res;

    g_list_free(account->chars);
    account->chars = NULL;

    res =
        sql_query("select idnum from players where account=%d order by idnum",
        account->id);
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++)
        account->chars = g_list_prepend(account->chars,
            GINT_TO_POINTER(atol(PQgetvalue(res, idx, 0))));
    account->chars = g_list_reverse(account->chars);
}

void
account_add_trusted(struct account *account, long idnum)
{
    account->trusted =
        g_list_prepend(account->trusted, GINT_TO_POINTER(idnum));
}

void
load_trusted(struct account *account)
{
    PGresult *res;
    int count, idx;

    g_list_free(account->trusted);
    account->trusted = NULL;

    res =
        sql_query("select player from trusted where account=%d", account->id);
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++)
        account_add_trusted(account, atol(PQgetvalue(res, idx, 0)));
    account->trusted = g_list_reverse(account->trusted);
}

void
preload_accounts(const char *conditions)
{
    long acct_count, field_count, field_idx;
    const char **fields;
    PGresult *res;

    res =
        sql_query
        ("select idnum, name, password, email, date_part('epoch', creation_time) as creation_time, creation_addr, date_part('epoch', login_time) as login_time, login_addr, date_part('epoch', entry_time) as entry_time, ansi_level, compact_level, term_height, term_width, banned, reputation, quest_points, quest_banned, bank_past, bank_future from accounts where %s",
        conditions);
    acct_count = PQntuples(res);

    if (acct_count < 1)
        return;

    // Get field names and put them in an array
    field_count = PQnfields(res);
    fields = (const char **)malloc(sizeof(const char *) * field_count);
    for (field_idx = 0; field_idx < field_count; field_idx++)
        fields[field_idx] = PQfname(res, field_idx);

    int acct_idx;
    for (acct_idx = 0; acct_idx < acct_count; acct_idx++) {
        // Make sure we don't reload one that's already in the cache
        long idnum = atol(PQgetvalue(res, acct_idx, 0));

        if (g_hash_table_lookup(account_cache, GINT_TO_POINTER(idnum)))
            continue;

        // Create a new account and load it up
        struct account *new_acct;
        CREATE(new_acct, struct account, 1);
        for (field_idx = 0; field_idx < field_count; field_idx++)
            account_set(new_acct,
                fields[field_idx], PQgetvalue(res, acct_idx, field_idx));

        load_players(new_acct);
        load_trusted(new_acct);

        g_hash_table_insert(account_cache, GINT_TO_POINTER(idnum), new_acct);
        slog("Account %ld preloaded from database", idnum);
    }
    free(fields);
}

bool
load_account(struct account *account, long idnum)
{
    long acct_count, field_count, field_idx;
    const char **fields;
    PGresult *res;

    res =
        sql_query
        ("select idnum, name, password, email, date_part('epoch', creation_time) as creation_time, creation_addr, date_part('epoch', login_time) as login_time, login_addr, date_part('epoch', entry_time) as entry_time, ansi_level, compact_level, term_height, term_width, banned, reputation, quest_points, quest_banned, bank_past, bank_future from accounts where idnum=%ld",
        idnum);
    acct_count = PQntuples(res);

    if (acct_count > 1) {
        errlog("search for account %ld returned more than one match", idnum);
        return false;
    }

    if (acct_count < 1)
        return false;

    // Get field names and put them in an array
    field_count = PQnfields(res);
    fields = (const char **)malloc(sizeof(const char *) * field_count);
    for (field_idx = 0; field_idx < field_count; field_idx++)
        fields[field_idx] = PQfname(res, field_idx);

    for (field_idx = 0; field_idx < field_count; field_idx++)
        account_set(account, fields[field_idx], PQgetvalue(res, 0, field_idx));
    free(fields);

    load_players(account);
    load_trusted(account);

    slog("Account %d loaded from database", account->id);
    g_hash_table_insert(account_cache, GINT_TO_POINTER(idnum), account);
    return true;
}

bool
account_reload(struct account * account)
{
    free(account->name);
    free(account->password);
    free(account->email);
    free(account->creation_addr);
    free(account->login_addr);
    g_list_free(account->trusted);
    account->trusted = NULL;
    g_list_free(account->chars);
    account->chars = NULL;
    g_hash_table_remove(account_cache, GINT_TO_POINTER(account->id));
    return load_account(account, account->id);
}

void
account_set(struct account *account, const char *key, const char *val)
{
    if (!strcmp(key, "idnum"))
        account->id = atol(val);
    else if (!strcmp(key, "name"))
        account->name = strdup(val);
    else if (!strcmp(key, "password"))
        account->password = strdup(val);
    else if (!strcmp(key, "email"))
        account->email = strdup(val);
    else if (!strcmp(key, "ansi_level"))
        account->ansi_level = atoi(val);
    else if (!strcmp(key, "compact_level"))
        account->compact_level = atoi(val);
    else if (!strcmp(key, "term_height"))
        account->term_height = atoi(val);
    else if (!strcmp(key, "term_width"))
        account->term_width = atoi(val);
    else if (!strcmp(key, "creation_time"))
        account->creation_time = atol(val);
    else if (!strcmp(key, "creation_addr"))
        account->creation_addr = strdup(val);
    else if (!strcmp(key, "login_time"))
        account->login_time = atol(val);
    else if (!strcmp(key, "login_addr"))
        account->login_addr = strdup(val);
    else if (!strcmp(key, "entry_time"))
        account->entry_time = atol(val);
    else if (!strcmp(key, "banned"))
        account->banned = !strcasecmp(val, "T");
    else if (!strcmp(key, "reputation"))
        account->reputation = atoi(val);
    else if (!strcmp(key, "quest_points"))
        account->quest_points = atoi(val);
    else if (!strcmp(key, "quest_banned"))
        account->quest_banned = !strcasecmp(val, "T");
    else if (!strcmp(key, "bank_past"))
        account->bank_past = atoll(val);
    else if (!strcmp(key, "bank_future"))
        account->bank_future = atoll(val);
    else
        slog("Invalid account field %s set to %s", key, val);
}

struct account *
account_by_idnum(int id)
{
    // First check to see if we already have it in memory
    struct account *acct = g_hash_table_lookup(account_cache,
                                               GINT_TO_POINTER(id));
    if (acct)
        return acct;

    // Apparently, we don't, so look it up on the db
    CREATE(acct, struct account, 1);
    if (load_account(acct, id))
        return acct;
    free_account(acct);
    return NULL;
}

gboolean
account_name_matches(gpointer key, gpointer value, gpointer user_data)
{
    struct account *this_acct = (struct account *)value;
    return !strcasecmp(this_acct->name, (char *)user_data);
}



struct account *
account_by_name(char *name)
{
    PGresult *res;
    int acct_id;

    // First check to see if we already have it in memory
    struct account *acct = g_hash_table_find(account_cache,
                                             account_name_matches,
                                             name);

    if (acct)
        return acct;

    // Apprently, we don't, so look it up on the db
    res = sql_query("select idnum from accounts where lower(name)='%s'",
        tmp_sqlescape(tmp_tolower(name)));
    if (PQntuples(res) != 1)
        return NULL;
    acct_id = atoi(PQgetvalue(res, 0, 0));

    // Now try to load it
    CREATE(acct, struct account, 1);
    if (load_account(acct, acct_id))
        return acct;

    free(acct);
    return NULL;
}

struct account *
account_create(const char *name, struct descriptor_data *d)
{
    struct account *result;

    account_top_id++;
    CREATE(result, struct account, 1);
    account_initialize(result, name, d, account_top_id);
    g_hash_table_insert(account_cache, GINT_TO_POINTER(account_top_id),
        result);
    return result;
}

int
count_account_gens(struct account *account)
{
    struct creature *tmp_ch;
    int count = 0;

    for (int idx = 1; !invalid_char_index(account, idx); idx++) {
        // test for file existence
        tmp_ch = load_player_from_xml(get_char_by_index(account, idx));
        if (!tmp_ch)
            continue;

        count += GET_REMORT_GEN(tmp_ch);

        free_creature(tmp_ch);
    }

    return count;
}

int
chars_available(struct account *account)
{
    return count_account_gens(account) / 10 + 10 -
        g_list_length(account->chars);
}

// Create a brand new character
struct creature *
account_create_char(struct account *account, const char *name)
{
    struct creature *ch;
    int i;

    if (chars_available(account) <= 0)
        return NULL;

    ch = make_creature(true);

    ch->player.name = strdup(tmp_capitalize(tmp_tolower(name)));
    ch->char_specials.saved.idnum = top_player_idnum() + 1;
    account->chars =
        g_list_append(account->chars, GINT_TO_POINTER(GET_IDNUM(ch)));

    sql_exec
        ("insert into players (idnum, name, account) values (%ld, '%s', %d)",
        GET_IDNUM(ch), tmp_sqlescape(name), account->id);

    // New characters shouldn't get old mail.
    if (has_mail(GET_IDNUM(ch))) {
        if (purge_mail(GET_IDNUM(ch)) > 0) {
            errlog("Purging pre-existing mailfile for new character.(%s)",
                GET_NAME(ch));
        }
    }
    // *** if this is our first player --- he be God ***
    if (GET_IDNUM(ch) == 1) {
        GET_EXP(ch) = 160000000;
        GET_LEVEL(ch) = LVL_GRIMP;

        ch->points.max_hit = 666;
        ch->points.max_mana = 555;
        ch->points.max_move = 444;

        GET_HOME(ch) = HOME_MODRIAN;
        ch->player_specials->saved.load_room = -1;
        ch->player_specials->saved.home_room = 1204;
    } else {
        ch->points.max_hit = 100;
        ch->points.max_mana = 100;
        ch->points.max_move = 82;

        GET_HOME(ch) = HOME_NEWBIE_SCHOOL;
        ch->player_specials->saved.load_room = -1;
        ch->player_specials->saved.home_room = -1;
    }
    ch->player_specials->rentcode = RENT_CREATING;

    set_title(ch, NULL);

    ch->player.short_descr = NULL;
    ch->player.long_descr = NULL;
    ch->player.description = NULL;

    ch->player.time.birth = time(0);
    ch->player.time.death = 0;
    ch->player.time.played = 0;
    ch->player.time.logon = time(0);

    for (i = 0; i < MAX_SKILLS; i++)
        ch->player_specials->saved.skills[i] = 0;

    for (i = 0; i < MAX_WEAPON_SPEC; i++) {
        ch->player_specials->saved.weap_spec[i].vnum = 0;
        ch->player_specials->saved.weap_spec[i].level = 0;
    }
    ch->player_specials->saved.imm_qp = 0;
    ch->player_specials->saved.quest_id = 0;
    ch->player_specials->saved.qlog_level = 0;

    GET_REMORT_CLASS(ch) = -1;
    ch->player.weight = 100;
    ch->player.height = 100;

    ch->points.hit = GET_MAX_HIT(ch);
    ch->points.mana = GET_MAX_MANA(ch);
    ch->points.move = GET_MAX_MOVE(ch);
    ch->points.armor = 100;

    SET_BIT(PRF_FLAGS(ch),
        PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_AUTOEXIT | PRF_NOSPEW |
        PRF_NOPLUG);
    SET_BIT(PRF2_FLAGS(ch),
        PRF2_AUTO_DIAGNOSE | PRF2_AUTOPROMPT | PRF2_DISPALIGN |
        PRF2_NEWBIE_HELPER);

    ch->char_specials.saved.affected_by = 0;
    ch->char_specials.saved.affected2_by = 0;
    ch->char_specials.saved.affected3_by = 0;

    for (i = 0; i < 5; i++)
        GET_SAVE(ch, i) = 0;

    GET_COND(ch, FULL) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 24);
    GET_COND(ch, THIRST) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 24);
    GET_COND(ch, DRUNK) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 0);

    POOFIN(ch) = NULL;
    POOFOUT(ch) = NULL;
    return ch;
}

void
account_distrust(struct account *account, long idnum)
{
    account->trusted = g_list_remove(account->trusted, GINT_TO_POINTER(idnum));
    sql_exec("delete from trusted where account=%d and player=%ld",
        account->id, idnum);
}

void
account_delete_char(struct account *account, struct creature *ch)
{
    void remove_bounties(int);
    struct clan_data *clan;
    int idx, count;
    PGresult *res;

    // Clear the owner of any clans this player might own in memory
    for (clan = clan_list; clan; clan = clan->next)
        if (clan->owner == GET_IDNUM(ch))
            clan->owner = 0;

    // Clear the owner of any clans this player might own on the db
    sql_exec("update clans set owner=null where owner=%ld", GET_IDNUM(ch));

    // Remove character from clan
    clan = real_clan(GET_CLAN(ch));
    if (clan) {
        struct clanmember_data *member, *temp;

        member = real_clanmember(GET_IDNUM(ch), clan);
        if (member) {
            REMOVE_FROM_LIST(member, clan->member_list, next);
        }
    }
    sql_exec("delete from clan_members where player=%ld", GET_IDNUM(ch));

    // TODO: Remove character from any access groups
    sql_exec("delete from sgroup_members where player=%ld", GET_IDNUM(ch));

    // Remove character from any quests they might have joined
    if (GET_QUEST(ch)) {
        struct quest *quest = quest_by_vnum(GET_QUEST(ch));

        if (quest) {
            remove_quest_player(quest, GET_IDNUM(ch));
            save_quests();
        }
    }
    // Remove character from trusted lists - we have to take the accounts
    // in memory into consideration when we do this, so we have to go
    // through each account
    res =
        sql_query("select account from trusted where player=%ld",
        GET_IDNUM(ch));
    count = PQntuples(res);
    for (idx = 0; idx < count; idx++) {
        struct account *acct = account_by_idnum(atoi(PQgetvalue(res, idx, 0)));
        if (acct)
            account_distrust(acct, GET_IDNUM(ch));
    }

    // Remove from the bounty list
    remove_bounties(GET_IDNUM(ch));
    sql_exec("delete from bounty_hunters where idnum=%ld or victim=%ld",
        GET_IDNUM(ch), GET_IDNUM(ch));

    // Disassociate author from board messages
    sql_exec("update board_messages set author=null where author=%ld",
        GET_IDNUM(ch));

    // Remove character from account
    account->chars = g_list_remove(account->chars,
                                   GINT_TO_POINTER(GET_IDNUM(ch)));
    sql_exec("delete from players where idnum=%ld", GET_IDNUM(ch));

    // Remove character from game
    if (ch->in_room) {
        send_to_char(ch,
            "A cold wind blows through your soul, and you disappear!\r\n");
        creature_purge(ch, false);
    }
}

bool
account_authenticate(struct account *account, const char *pw)
{
    struct crypt_data data = { initialized: 0 };

    if (account->password == NULL || *account->password == '\0') {
        errlog("Account %s[%d] has NULL password. Setting to guess.",
            account->name, account->id);
        account_set_password(account, pw);
    }
    return !strcmp(account->password, crypt_r(pw, account->password, &data));
}

void
account_login(struct account *account, struct descriptor_data *d)
{
    slog("login: %s[%d] from %s", account->name, account->id, d->host);
    set_desc_state(CXN_MENU, d);
}

void
account_logout(struct account *account, struct descriptor_data *d, bool forced)
{
    if (account->password) {
        account->login_time = time(NULL);
        if (account->login_addr)
            free(account->login_addr);
        account->login_addr = strdup(d->host);

        sql_exec
            ("update accounts set login_addr='%s', login_time='%s' where idnum=%d",
            tmp_sqlescape(account->login_addr), tmp_ctime(account->login_time),
            account->id);

        slog("%slogout: %s[%d] from %s", (forced) ? "forced " : "",
            account->name, account->id, account->login_addr);
    } else {
        slog("%slogout: unfinished account from %s", (forced) ? "forced " : "",
            account->login_addr);
    }

    set_desc_state(CXN_DISCONNECT, d);
}

void
account_initialize(struct account *account,
                   const char *name,
                   struct descriptor_data *d,
                   int idnum)
{
    account->id = idnum;
    account->name = strdup(name);
    if (strlen(account->name) > 20)
        account->name[20] = '\0';
    account->password = NULL;
    account->creation_time = account->login_time = time(0);
    account->creation_addr = strdup(d->host);
    account->login_addr = strdup(d->host);
    account->ansi_level = 0;
    account->compact_level = 0;
    account->term_height = 22;
    account->term_width = 80;
    account->bank_past = 0;
    account->bank_future = 0;
    account->banned = false;
    account->reputation = 0;
    account->quest_points = 0;
    account->quest_banned = false;

    slog("new account: %s[%d] from %s", account->name, idnum, d->host);
    sql_exec
        ("insert into accounts (idnum, name, creation_time, creation_addr, login_time, login_addr, ansi_level, compact_level, term_height, term_width, reputation, bank_past, bank_future) values (%d, '%s', now(), '%s', now(), '%s', 0, 0, %d, %d, 0, 0, 0)",
        idnum, tmp_sqlescape(name), tmp_sqlescape(d->host),
        tmp_sqlescape(d->host), DEFAULT_TERM_HEIGHT, DEFAULT_TERM_WIDTH);
}

bool
account_has_password(struct account *account)
{
    return (account->password != NULL);
}

void
account_set_password(struct account *account, const char *pw)
{
    char salt[13] = "$1$........$";
    int idx;

    for (idx = 3; idx < 12; idx++) {
        salt[idx] = my_rand() & 63;
        if (salt[idx] < 10)
            salt[idx] += '0';
        else if (salt[idx] < 36)
            salt[idx] = salt[idx] - 10 + 'A';
        else if (salt[idx] < 62)
            salt[idx] = salt[idx] - 36 + 'a';
        else if (salt[idx] == 63)
            salt[idx] = '.';
        else
            salt[idx] = '/';
    }
    free(account->password);
    account->password = strdup(crypt(pw, salt));
    sql_exec("update accounts set password='%s' where idnum=%d",
        tmp_sqlescape(account->password), account->id);
}

void
account_set_banned(struct account *account, bool banned)
{
    account->banned = banned;
    sql_exec("update accounts set banned='%s' where idnum=%d",
        account->banned ? "T" : "F", account->id);
}

void
account_set_reputation(struct account *account, int amt)
{
    if (amt >= 0) {
        account->reputation = amt;
        sql_exec("update accounts set reputation=%d where idnum=%d",
            account->reputation, account->id);
    }
}

void
account_gain_reputation(struct account *account, int amt)
{
    if (amt != 0) {
        account->reputation += amt;
        if (account->reputation < 0)
            account->reputation = 0;
        sql_exec("update accounts set reputation=%d where idnum=%d",
            account->reputation, account->id);
    }
}

void
account_set_past_bank(struct account *account, money_t amt)
{
    account->bank_past = amt;
    sql_exec("update accounts set bank_past=%lld where idnum=%d",
        account->bank_past, account->id);
}

void
account_set_future_bank(struct account *account, money_t amt)
{
    account->bank_future = amt;
    sql_exec("update accounts set bank_future=%lld where idnum=%d",
        account->bank_future, account->id);
}

void
deposit_past_bank(struct account *account, money_t amt)
{
    if (amt > 0)
        account_set_past_bank(account, account->bank_past + amt);
}

void
deposit_future_bank(struct account *account, money_t amt)
{
    if (amt > 0)
        account_set_future_bank(account, account->bank_future + amt);
}

void
withdraw_past_bank(struct account *account, money_t amt)
{
    if (amt <= 0)
        return;
    if (amt > account->bank_past)
        amt = account->bank_past;
    account_set_past_bank(account, account->bank_past - amt);
}

void
withdraw_future_bank(struct account *account, money_t amt)
{
    if (amt <= 0)
        return;
    if (amt > account->bank_future)
        amt = account->bank_future;
    account_set_future_bank(account, account->bank_future - amt);
}

void
account_set_email_addr(struct account *account, const char *addr)
{
    account->email = strdup(addr);
    if (strlen(account->email) > 60)
        account->email[60] = '\0';
    sql_exec("update accounts set email='%s' where idnum=%d",
        tmp_sqlescape(account->email), account->id);
}

long
get_char_by_index(struct account *account, int idx)
{
    return GPOINTER_TO_INT(g_list_nth_data(account->chars, idx - 1));
}

bool
invalid_char_index(struct account * account, int idx)
{
    return (idx < 1 || idx > g_list_length(account->chars));
}

void
account_move_char(struct account *account, long id, struct account *dest)
{
    // Remove character from account
    account->chars = g_list_remove(account->chars, GINT_TO_POINTER(id));

    // Get the player's name before we delete from player table
    dest->chars = g_list_prepend(dest->chars, GINT_TO_POINTER(id));
    sql_exec("update players set account=%d where idnum=%ld", dest->id, id);
}

void
account_exhume_char(struct account *account, struct creature *exhumer, long id)
{
    if (player_idnum_exists(id)) {
        send_to_char(exhumer, "That character has already been exhumed.\r\n");
        return;
    }
    // load char from file
    struct creature *victim;
    CREATE(victim, struct creature, 1);
    victim = load_player_from_xml(id);
    if (victim) {
        sql_exec
            ("insert into players (idnum, account, name) values (%ld, %d, '%s')",
            GET_IDNUM(victim), account->id, tmp_sqlescape(GET_NAME(victim)));
        load_players(account);
        send_to_char(exhumer, "%s exhumed.\r\n",
            tmp_capitalize(GET_NAME(victim)));
        slog("%s[%ld] exhumed into account %s[%d]", GET_NAME(victim),
            GET_IDNUM(victim), account->name, account->id);
        free_creature(victim);
    } else {
        send_to_char(exhumer, "Unable to load character %ld.\r\n", id);
    }
}

bool
account_deny_char_entry(struct account *account, struct creature *ch)
{
    // Admins and full wizards can multi-play all they want
    if (is_authorized(ch, MULTIPLAY, NULL))
        return false;

    bool override = false;
    bool found = false;

    for (GList *it = first_living(creatures);it;it = next_living(it)) {
        struct creature *tch = it->data;

        if (tch->account == account) {
            // Admins and full wizards can multi-play all they want
            if (is_authorized(ch, MULTIPLAY, NULL))
                override = true;
            // builder can have on a tester and vice versa.
            if (is_authorized(ch, LOGIN_WITH_TESTER, NULL) && is_tester(tch))
                override = true;
            if (is_tester(ch) && is_authorized(tch, LOGIN_WITH_TESTER, NULL))
                override = true;
            // We have a non-immortal already in the game, so they don't
            // get to come in
            found = true;
        }
    }
    if (override)
        return false;

    return found;
}

bool
account_is_logged_in(struct account * account)
{
    struct descriptor_data *d;
    for (d = descriptor_list; d != NULL; d = d->next) {
        if (d->account == account) {
            return true;
        }
    }
    return false;
}

void
account_update_last_entry(struct account *account)
{
    account->entry_time = time(0);
    sql_exec("update accounts set entry_time=now() where idnum=%d",
        account->id);
}

bool
is_trusted(struct account *account, long idnum)
{
    if (g_list_find(account->chars, GINT_TO_POINTER(idnum)))
        return true;

    if (g_list_find(account->trusted, GINT_TO_POINTER(idnum)))
        return true;

    return false;
}

void
account_trust(struct account *account, long idnum)
{
    if (g_list_find(account->trusted, GINT_TO_POINTER(idnum)))
        return;
    account->trusted =
        g_list_prepend(account->trusted, GINT_TO_POINTER(idnum));
    sql_exec("insert into trusted (account, player) values (%d, %ld)",
        account->id, idnum);
}

void
account_display_trusted(struct account *account, struct creature *ch)
{
    int col = 0;

    for (GList *it = account->trusted;it;it = it->next) {
        int idnum = GPOINTER_TO_INT(it->data);

        if (player_idnum_exists(idnum)) {
            send_to_char(ch, "%20s   ", player_name_by_idnum(idnum));
            col += 1;
            if (col > 2) {
                send_to_char(ch, "\r\n");
                col = 0;
            }
        }
    }
    if (col)
        send_to_char(ch, "\r\n");
}

void
account_set_ansi_level(struct account *account, int level)
{
    account->ansi_level = level;
    sql_exec("update accounts set ansi_level=%d where idnum=%d",
        account->ansi_level, account->id);
}

void
account_set_compact_level(struct account *account, int level)
{
    account->compact_level = level;
    sql_exec("update accounts set compact_level=%d where idnum=%d",
        account->compact_level, account->id);
}

void
account_set_term_height(struct account *account, int height)
{
    if (height < 0)
        height = 0;
    if (height > 200)
        height = 200;
    account->term_height = height;
    sql_exec("update accounts set term_height=%d where idnum=%d",
        account->term_height, account->id);
}

void
account_set_term_width(struct account *account, int width)
{
    if (width < 0)
        width = 0;
    if (width > 200)
        width = 200;
    account->term_width = width;
    sql_exec("update accounts set term_width=%d where idnum=%d",
        account->term_width, account->id);
}

void
account_set_quest_points(struct account *account, int qp)
{
    if (qp < 0)
        qp = 0;
    account->quest_points = qp;
    sql_exec("update accounts set quest_points=%d where idnum=%d",
        account->quest_points, account->id);
}

void
account_set_quest_banned(struct account *account, bool banned)
{
    account->quest_banned = banned;
    sql_exec("update accounts set quest_banned='%s' where idnum=%d",
        account->quest_banned ? "T" : "F", account->id);
}

int
hasCharLevel(struct account *account, int level)
{
    int idx = 1;
    struct creature *tmp_ch;

    while (!invalid_char_index(account, idx)) {
        tmp_ch = load_player_from_xml(get_char_by_index(account, idx));

        if (!tmp_ch)
            return 0;

        if (GET_LEVEL(tmp_ch) >= level) {
            free_creature(tmp_ch);
            return idx;
        }

        free_creature(tmp_ch);

        idx++;
    }

    return 0;
}

int
account_chars_available(struct account *account)
{
    return (count_account_gens(account) / 10
        + 10 - g_list_length(account->chars));
}

int
account_char_count(struct account *account)
{
    return g_list_length(account->chars);
}
