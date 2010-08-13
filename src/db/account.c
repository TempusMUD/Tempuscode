#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
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
GList account_cache;

void
account_boot(void)
{
	PGresult *res;

	slog("Getting max account idnum");

	res = sql_query("select MAX(idnum) from accounts");
	account_top_id = atol(PQgetvalue(res, 0, 0));

	slog("Getting character count");
	if (playerIndex.size())
		slog("... %zd character%s in db", playerIndex.size(), (playerIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No characters loaded");
}

size_t
struct account_cache_size(void)
{
	return _cache.size();
}

struct account_struct account(void)
	: _chars(), _trusted()
{
	time_t now;

	now = time(NULL);
	_id = 0;
	_name = NULL;
	_password = NULL;
	_email = NULL;
	_creation_time = now;
	_login_time = now;
	_entry_time = 0;
	_creation_addr = NULL;
	_login_addr = NULL;
	_ansi_level = 0;
	_compact_level = 0;
    _bank_past = 0;
    _bank_future = 0;
    _banned = false;
	_reputation = 0;
	_quest_points = 0;
	_quest_banned = false;
	_term_height = DEFAULT_TERM_HEIGHT;
	_term_width = DEFAULT_TERM_WIDTH;
}

struct account_~struct account(void)
{
    free(_name);
    free(_password);
    free(_email);
    free(_creation_addr);
    free(_login_addr);

	struct account_remove(this);
}

void
struct account_preload(const char *conditions)
{
	long acct_count, field_count, field_idx;
	const char **fields;
	PGresult *res;

	res = sql_query("select idnum, name, password, email, date_part('epoch', creation_time) as creation_time, creation_addr, date_part('epoch', login_time) as login_time, login_addr, date_part('epoch', entry_time) as entry_time, ansi_level, compact_level, term_height, term_width, banned, reputation, quest_points, quest_banned, bank_past, bank_future from accounts where %s", conditions);
	acct_count = PQntuples(res);

	if (acct_count < 1)
		return;

	// Get field names and put them in an array
	field_count = PQnfields(res);
	fields = new const char *[field_count];
	for (field_idx = 0;field_idx < field_count;field_idx++)
		fields[field_idx] = PQfname(res, field_idx);

    for (int acct_idx = 0;acct_idx < acct_count;acct_idx++) {
        // Make sure we don't reload one that's already in the cache
        long idnum = atol(PQgetvalue(res, acct_idx, 0));
        vector<struct account *>_iterator it;

        it = lower_bound(_cache.begin(), _cache.end(), idnum, struct account_cmp());
        if (it != _cache.end() && (*it)->get_idnum() == idnum)
            continue;

        // Create a new account and load it up
        struct account *new_acct = new struct account;
        for (field_idx = 0;field_idx < field_count;field_idx++)
            new_acct->set(fields[field_idx],
                      PQgetvalue(res, acct_idx, field_idx));

        new_acct->load_players();
        new_acct->load_trusted();

        _cache.push_back(new_acct);
        std_sort(_cache.begin(),_cache.end(), struct account::cmp());
        slog("struct account %ld preloaded from database", idnum);
    }
	delete [] fields;
}

bool
struct account_load(long idnum)
{
	long acct_count, field_count, field_idx;
	const char **fields;
	PGresult *res;

	res = sql_query("select idnum, name, password, email, date_part('epoch', creation_time) as creation_time, creation_addr, date_part('epoch', login_time) as login_time, login_addr, date_part('epoch', entry_time) as entry_time, ansi_level, compact_level, term_height, term_width, banned, reputation, quest_points, quest_banned, bank_past, bank_future from accounts where idnum=%ld", idnum);
	acct_count = PQntuples(res);

	if (acct_count > 1) {
		errlog("search for account %ld returned more than one match", idnum);
		return false;
	}

	if (acct_count < 1)
		return false;

	// Get field names and put them in an array
	field_count = PQnfields(res);
	fields = new const char *[field_count];
	for (field_idx = 0;field_idx < field_count;field_idx++)
		fields[field_idx] = PQfname(res, field_idx);

	for (field_idx = 0;field_idx < field_count;field_idx++)
		this->set(fields[field_idx],
			PQgetvalue(res, 0, field_idx));
	delete [] fields;

	this->load_players();
    this->load_trusted();

	slog("struct account %d loaded from database", _id);
	_cache.push_back(this);
	std_sort(_cache.begin(),_cache.end(), struct account::cmp());
	return true;
}

bool
struct account_reload(void)
{
	vector<struct account *>_iterator it;

	_trusted.clear();
	_chars.clear();
	it = lower_bound(_cache.begin(), _cache.end(), _id, struct account_cmp());
	if (it != _cache.end() && (*it)->get_idnum() == _id)
		_cache.erase(it);

	return this->load(_id);
}

void
struct account_set(const char *key, const char *val)
{
	if (!strcmp(key, "idnum"))
		_id = atol(val);
	else if (!strcmp(key, "name"))
		_name = strdup(val);
	else if (!strcmp(key, "password"))
		_password = strdup(val);
	else if (!strcmp(key, "email"))
		_email = strdup(val);
	else if (!strcmp(key, "ansi_level"))
		_ansi_level = atoi(val);
	else if (!strcmp(key, "compact_level"))
		_compact_level = atoi(val);
	else if (!strcmp(key, "term_height"))
		_term_height = atoi(val);
	else if (!strcmp(key, "term_width"))
		_term_width = atoi(val);
	else if (!strcmp(key, "creation_time"))
		_creation_time = atol(val);
	else if (!strcmp(key, "creation_addr"))
		_creation_addr = strdup(val);
	else if (!strcmp(key, "login_time"))
		_login_time = atol(val);
	else if (!strcmp(key, "login_addr"))
		_login_addr = strdup(val);
	else if (!strcmp(key, "entry_time"))
		_entry_time = atol(val);
	else if (!strcmp(key, "banned"))
		_banned = !strcasecmp(val, "T");
	else if (!strcmp(key, "reputation"))
		_reputation = atoi(val);
	else if (!strcmp(key, "quest_points"))
		_quest_points = atoi(val);
	else if (!strcmp(key, "quest_banned"))
		_quest_banned = !strcasecmp(val, "T");
	else if (!strcmp(key, "bank_past"))
		_bank_past = atoll(val);
	else if (!strcmp(key, "bank_future"))
		_bank_future = atoll(val);
	else
		slog("Invalid account field %s set to %s", key, val);
}

void
struct account_load_players(void)
{
	long count, idx;
	PGresult *res;
	struct creature *tmp_ch = new struct creature(true);

	_chars.clear();
	res = sql_query("select idnum from players where account=%d order by idnum", _id);
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++)
		_chars.push_back(atol(PQgetvalue(res, idx, 0)));

    if (chars_available() < 0) {
        // First, find all the characters with NEW rentcode and
        // whack them as long as we still have more than 10 characters;
		for (int x = 0; x < get_char_count(); x++) {
			tmp_ch->loadFromXML(_chars[x]);
			if (tmp_ch->player_specials->rentcode == RENT_NEW_CHAR ||
				tmp_ch->player_specials->rentcode == RENT_CREATING)
				delete_char(tmp_ch);
			if (chars_available() >= 0)
				break;
		}
	}

	if (chars_available() < 0) {
        // Second, if we still have more than 10 characters, lets delete
        // the characters that have QUIT and are less than level 25
         for (int x = 0; x < get_char_count(); x++) {
            tmp_ch->loadFromXML(_chars[x]);
            if (tmp_GET_LEVEL(ch) <= 25 &&
                tmp_ch->player_specials->rentcode == RENT_QUIT)
                delete_char(tmp_ch);
			if (chars_available() >= 0)
				break;
        }
    }

    // We've deleted as many characters as I feel is prudent.  If this
	// account still has more than the maximum, make sure they only
	// have access to the max.
    if (chars_available() < 0) {
        vector<long>_iterator vi = _chars.begin();
        vi += _chars.size() + chars_available();
        _chars.erase(vi, _chars.end());
    }

	delete tmp_ch;
}

void
struct account_load_trusted(void)
{
    PGresult *res;

    res = sql_query("select player from trusted where account=%d", _id);
    int count = PQntuples(res);
    for (int idx = 0;idx < count;idx++)
        this->add_trusted(atol(PQgetvalue(res, idx, 0)));
}

void
struct account_add_trusted(long idnum)
{
	_trusted.push_back(idnum);
}

struct account*
struct account_retrieve(int id)
{
	vector <struct account *>_iterator it;
	struct account *acct;

	// First check to see if we already have it in memory
	it = lower_bound( _cache.begin(), _cache.end(), id, struct account_cmp() );
	if( it != _cache.end() && (*it)->get_idnum() == id )
		return *it;

	// Apprently, we don't, so look it up on the db
	acct = new struct account;
	if (acct->load(id))
		return acct;
	delete acct;
	return NULL;
}

bool
struct account_exists( int accountID )
{
	PGresult *res;
	bool result;

	res = sql_query("select idnum from accounts where idnum=%d", accountID);
	result = PQntuples(res) > 0;

    return result;
}

struct account*
struct account_retrieve(const char *name)
{
	vector <struct account *>_iterator it;
	struct account *acct;
	PGresult *res;
	int acct_id;

	// First check to see if we already have it in memory
	for (it = _cache.begin();it != _cache.end();it++) {
		acct = *it;
		if (!strcasecmp(acct->get_name(), name))
			return acct;
	}

	// Apprently, we don't, so look it up on the db
	res = sql_query("select idnum from accounts where lower(name)='%s'",
		tmp_sqlescape(tmp_tolower(name)));
	if (PQntuples(res) != 1)
		return NULL;
	acct_id = atoi(PQgetvalue(res, 0, 0));

	// Now try to load it
	acct = new struct account;
	if (acct->load(acct_id))
		return acct;

	delete acct;
	return NULL;
}

struct account*
struct account_retrieve(struct creature *ch)
{
	int acct_id;

    // NPCs don't have accounts!
    if (IS_NPC(ch))
        return NULL;

    // If we already have an account loaded for this creature
    // just return it
    if (ch->account)
        return ch->account;

    acct_id = playerIndex.getstruct accountID(GET_IDNUM(ch));
    if (!acct_id)
        return NULL;

    return struct account_retrieve(acct_id);
}

struct account *
struct account_create(const char *name, descriptor_data *d)
{
	struct account *result;

	account_top_id++;
	result = new struct account;
	result->initialize(name, d, account_top_id);
	_cache.push_back(result);
	std_sort(_cache.begin(),_cache.end(), struct account::cmp());
	return result;
}

bool
struct account_remove(struct account *acct)
{
	vector <struct account *>_iterator it;

	for (it = _cache.begin();it != _cache.end();it++)
		if (*it == acct) {
			_cache.erase(it);
			return true;
		}

	return false;
}

int
struct account_chars_available()
{
	return countGens() / 10 + 10 - get_char_count();
}

// Create a brand new character
struct creature *
struct account_create_char(const char *name)
{
	struct creature *ch;
	int i;

    if (chars_available() <= 0)
        return NULL;

	ch = new struct creature(true);

    GET_NAME(ch) = strdup(tmp_capitalize(tmp_tolower(name)));
    ch->char_specials.saved.idnum = playerIndex.getTopIDNum() + 1;
	_chars.push_back(GET_IDNUM(ch));
	sql_exec("insert into players (idnum, name, account) values (%ld, '%s', %d)",
		GET_IDNUM(ch), tmp_sqlescape(name), _id);

    // New characters shouldn't get old mail.
    if(has_mail(GET_IDNUM(ch))) {
        if(purge_mail(GET_IDNUM(ch))>0) {
            errlog("Purging pre-existing mailfile for new character.(%s)",
                   GET_NAME(ch));
        }
    }

    // *** if this is our first player --- he be God ***
	if( GET_IDNUM(ch) == 1) {
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
		ch->player_specials->saved.weap_spec[i].vnum = -1;
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

	SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_AUTOEXIT | PRF_NOSPEW | PRF_NOPLUG);
	SET_BIT(PRF2_FLAGS(ch), PRF2_AUTO_DIAGNOSE | PRF2_AUTOPROMPT | PRF2_DISPALIGN | PRF2_NEWBIE_HELPER);

	ch->char_specials.saved.affected_by = 0;
	ch->char_specials.saved.affected2_by = 0;
	ch->char_specials.saved.affected3_by = 0;

	for (i = 0; i < 5; i++)
		GET_SAVE(ch, i) = 0;

	GET_COND(ch, FULL) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 24);
	GET_COND(ch, THIRST) = (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 24);
	GET_COND(ch, DRUNK) =  (GET_LEVEL(ch) == LVL_GRIMP ? -1 : 0);

	POOFIN(ch) = NULL;
	POOFOUT(ch) = NULL;
	return ch;
}

void
struct account_delete_char(struct creature *ch)
{
	void remove_bounties(int);
	vector<long>_iterator it;
	clan_data *clan;
	int idx, count;
	PGresult *res;
	struct account *acct;

	// Clear the owner of any clans this player might own in memory
	for (clan = clan_list;clan;clan = clan->next)
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

	// Remove character from any access groups
	list<Security_Group>::iterator group;

	for (group = Security_groups.begin();group != Security::groups.end();group++)
		group->removeMember(GET_IDNUM(ch));
	sql_exec("delete from sgroup_members where player=%ld", GET_IDNUM(ch));

    // Remove character from any quests they might have joined
    if (GET_QUEST(ch)) {
        Quest *quest = quest_by_vnum(GET_QUEST(ch));

        if (quest) {
            quest->removePlayer(GET_IDNUM(ch));
            save_quests();
        }
    }

	// Remove character from trusted lists - we have to take the accounts
	// in memory into consideration when we do this, so we have to go
	// through each account
	res = sql_query("select account from trusted where player=%ld", GET_IDNUM(ch));
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++) {
		acct = struct account_retrieve(atoi(PQgetvalue(res, idx, 0)));
		if (acct)
			acct->distrust(GET_IDNUM(ch));
	}

	// Remove from the bounty list
	remove_bounties(GET_IDNUM(ch));
	sql_exec("delete from bounty_hunters where idnum=%ld or victim=%ld",
		GET_IDNUM(ch), GET_IDNUM(ch));

	// Disassociate author from board messages
	sql_exec("update board_messages set author=null where author=%ld",
		GET_IDNUM(ch));

	// Remove character from account
	it = lower_bound(_chars.begin(), _chars.end(), GET_IDNUM(ch));
	if (it != _chars.end())
		_chars.erase(it);
	sql_exec("delete from players where idnum=%ld", GET_IDNUM(ch));

	// Remove character from game
	if (ch->in_room) {
		send_to_char(ch, "A cold wind blows through your soul, and you disappear!\r\n");
		ch->purge(false);
	}
}

bool
struct account_authenticate(const char *pw)
{
	if(_password == NULL || *_password == '\0') {
		errlog("struct account %s[%d] has NULL password. Setting to guess.", _name, _id );
		set_password(pw);
	}
	return !strcmp(_password, crypt(pw, _password));
}

void
struct account_login(descriptor_data *d)
{
	slog("login: %s[%d] from %s", _name, get_idnum(), d->host);
	set_desc_state(CXN_MENU, d);
}

void
struct account_logout(descriptor_data *d, bool forced)
{
	if (_password) {
		_login_time = time(NULL);
		if (_login_addr)
			free(_login_addr);
		_login_addr = strdup(d->host);

		sql_exec("update accounts set login_addr='%s', login_time='%s' where idnum=%d",
			tmp_sqlescape(_login_addr), tmp_ctime(_login_time), _id);

		slog("%slogout: %s[%d] from %s", (forced) ? "forced ":"",
             _name, get_idnum(), _login_addr);
	} else {
		slog("%slogout: unfinished account from %s", (forced) ? "forced ":"",
			_login_addr);
	}

	set_desc_state(CXN_DISCONNECT, d);
}

void
struct account_initialize(const char *name, descriptor_data *d, int idnum)
{
	_id = idnum;
	_name = strdup(name);
	if (strlen(_name) > 20)
		_name[20] = '\0';
	_password = NULL;
	_creation_time = _login_time = time(0);
	_creation_addr = strdup(d->host);
	_login_addr = strdup(d->host);
	_ansi_level = 0;
	_compact_level = 0;
	_term_height = 22;
	_term_width = 80;
	_bank_past = 0;
	_bank_future = 0;
    _banned = false;
    _reputation = 0;
	_quest_points = 0;
	_quest_banned = false;

	slog("new account: %s[%d] from %s", _name, idnum, d->host);
	sql_exec("insert into accounts (idnum, name, creation_time, creation_addr, login_time, login_addr, ansi_level, compact_level, term_height, term_width, reputation, bank_past, bank_future) values (%d, '%s', now(), '%s', now(), '%s', 0, 0, %d, %d, 0, 0, 0)",
		idnum, tmp_sqlescape(name), tmp_sqlescape(d->host),
		tmp_sqlescape(d->host), DEFAULT_TERM_HEIGHT, DEFAULT_TERM_WIDTH);
}

void
struct account_set_password(const char *pw)
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
	_password = strdup(crypt(pw, salt));
	sql_exec("update accounts set password='%s' where idnum=%d",
		tmp_sqlescape(_password), _id);
}

void
struct account_set_banned(bool banned)
{
	_banned = banned;
	sql_exec("update accounts set banned='%s' where idnum=%d",
		_banned ? "T":"F", _id);
}

void
struct account_set_reputation(int amt)
{
	if (amt >= 0) {
		_reputation = amt;
		sql_exec("update accounts set reputation=%d where idnum=%d",
			_reputation, _id);
	}
}

void
struct account_gain_reputation(int amt)
{
    if (amt != 0) {
        _reputation += amt;
        if (_reputation < 0)
            _reputation = 0;
        sql_exec("update accounts set reputation=%d where idnum=%d",
                 _reputation, _id);
    }
}

void
struct account_deposit_past_bank(money_t amt)
{
	if (amt > 0)
		set_past_bank(_bank_past + amt);
}

void
struct account_deposit_future_bank(money_t amt)
{
	if (amt > 0)
		set_future_bank(_bank_future + amt);
}

void
struct account_withdraw_past_bank(money_t amt)
{
	if (amt <= 0)
		return;
	if (amt > _bank_past)
		amt = _bank_past;
	set_past_bank(_bank_past - amt);
}

void
struct account_withdraw_future_bank(money_t amt)
{
	if (amt <= 0)
		return;
	if (amt > _bank_future)
		amt = _bank_future;
	set_future_bank(_bank_future - amt);
}

void
struct account_set_email_addr(const char *addr)
{
	_email = strdup(addr);
	if (strlen(_email) > 60)
		_email[60] = '\0';
	sql_exec("update accounts set email='%s' where idnum=%d",
		tmp_sqlescape(_email), _id);
}

long
struct account_get_char_by_index(int idx)
{
	return _chars[idx - 1];
}

bool
struct account_invalid_char_index(int idx)
{
	return (idx < 1 || idx > (int)_chars.size());
}

void
struct account_move_char(long id, struct account *dest)
{
	vector<long>_iterator it;

	// Remove character from account
	it = lower_bound(_chars.begin(), _chars.end(), id);
	if (it != _chars.end())
		_chars.erase(it);

	// Get the player's name before we delete from player table
	dest->_chars.push_back(id);
	sort(dest->_chars.begin(), dest->_chars.end());
	sql_exec("update players set account=%d where idnum=%ld",
		dest->_id, id);
}

void
struct account_exhume_char( struct creature *exhumer, long id )
{
	if( playerIndex.exists(id) ) {
		send_to_char(exhumer, "That character has already been exhumed.\r\n");
		return;
	}

	// load char from file
	struct creature* victim = new struct creature(true);
	if( victim->loadFromXML(id) ) {
		sql_exec("insert into players (idnum, account, name) values (%ld, %d, '%s')",
			GET_IDNUM(victim), _id, tmp_sqlescape(GET_NAME(victim)));
		this->load_players();
		send_to_char(exhumer, "%s exhumed.\r\n",
					tmp_capitalize( GET_NAME(victim) ) );
		slog("%s[%ld] exhumed into account %s[%d]", GET_NAME(victim),
			GET_IDNUM(victim), _name, _id);
	} else {
		send_to_char(exhumer, "Unable to load character %ld.\r\n", id );
	}
	delete victim;
}

bool
struct account_deny_char_entry(struct creature *ch)
{
	struct creature *tch;

    // Admins and full wizards can multi-play all they want
    if (Security_isMember(ch, "WizardFull"))
        return false;
    if (Security_isMember(ch, "AdminFull"))
        return false;

	struct creatureList_iterator cit = characterList.begin();
	for (;cit != characterList.end(); ++cit) {
		tch = *cit;
		if (tch->account == this) {
            // Admins and full wizards can multi-play all they want
			if (Security_isMember(tch, "WizardFull"))
				return false;
            if (Security_isMember(tch, "AdminFull"))
				return false;
            // builder can have on a tester and vice versa.
            if (Security_isMember(tch, "OLC") && ch->isTester())
				return false;
            if (ch->isTester() && Security_isMember(tch, "OLC"))
				return false;
			// We have a non-immortal already in the game, so they don't
			// get to come in
			return true;
		}
	}

	return false;
}

bool
struct account_is_logged_in() const
{
    list<descriptor_data*> connections;
	for( descriptor_data *d = descriptor_list; d != NULL; d = d->next ) {
		if( d->account == this ) {
            return true;
		}
	}
    return false;
}

void
struct account_update_last_entry(void)
{
	_entry_time = time(0);
	sql_exec("update accounts set entry_time=now() where idnum=%d", _id);
}

bool
struct account_isTrusted(long idnum)
{
	vector<long>_iterator it;

	for (it = _chars.begin();it != _chars.end();it++)
		if (*it == idnum)
			return true;

	for (it = _trusted.begin();it != _trusted.end();it++)
		if (*it == idnum)
			return true;

	return false;
}

void
struct account_trust(long idnum)
{
	vector<long>_iterator it;

	for (it = _trusted.begin();it != _trusted.end();it++)
		if (*it == idnum)
			return;

	_trusted.push_back(idnum);
	sql_exec("insert into trusted (account, player) values (%d, %ld)",
		_id, idnum);
}

void
struct account_distrust(long idnum)
{
	vector<long>_iterator it;

	for (it = _trusted.begin();it != _trusted.end();it++) {
		if (*it == idnum) {
			_trusted.erase(it);
			sql_exec("delete from trusted where account=%d and player=%ld",
				_id, idnum);
			return;
		}
	}
}

void
struct account_displayTrusted(struct creature *ch)
{
	vector<long>_iterator it;
	int col = 0;

	for (it = _trusted.begin();it != _trusted.end();it++) {
		if (playerIndex.exists(*it)) {
			send_to_char(ch, "%20s   ", playerIndex.getName(*it));
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
struct account_set_ansi_level(int level)
{
	_ansi_level = level;
	sql_exec("update accounts set ansi_level=%d where idnum=%d",
		_ansi_level, _id);
}

void
struct account_set_compact_level(int level)
{
	_compact_level = level;
	sql_exec("update accounts set compact_level=%d where idnum=%d",
		_compact_level, _id);
}

void
struct account_set_past_bank(money_t amt)
{
	_bank_past = amt;
	sql_exec("update accounts set bank_past=%lld where idnum=%d",
		_bank_past, _id);
}

void
struct account_set_future_bank(money_t amt)
{
	_bank_future = amt;
	sql_exec("update accounts set bank_future=%lld where idnum=%d",
		_bank_future, _id);
}

void
struct account_set_term_height(int height)
{
	if (height < 0)
		height = 0;
	if (height > 200)
		height = 200;
	_term_height = height;
	sql_exec("update accounts set term_height=%d where idnum=%d",
		_term_height, _id);
}

void
struct account_set_term_width(int width)
{
	if (width < 0)
		width = 0;
	if (width > 200)
		width = 200;
	_term_width = width;
	sql_exec("update accounts set term_width=%d where idnum=%d",
		_term_width, _id);
}

void
struct account_set_quest_points(int qp)
{
	if (qp < 0)
		qp = 0;
	_quest_points = qp;
	sql_exec("update accounts set quest_points=%d where idnum=%d",
		_quest_points, _id);
}

void
struct account_set_quest_banned(bool banned)
{
	_quest_banned = banned;
	sql_exec("update accounts set quest_banned='%s' where idnum=%d",
		_quest_banned ? "T":"F", _id);
}

int struct account_hasCharLevel(int level)
{
    int idx = 1;
    struct creature *tmp_ch = new struct creature(true);

    while (!this->invalid_char_index(idx)) {
        tmp_ch->clear();

        if (!tmp_ch->loadFromXML(this->get_char_by_index(idx)))
            return 0;

        if (tmp_GET_LEVEL(ch) >= level) {
            delete tmp_ch;
            return idx;
        }

        idx++;
    }

    delete tmp_ch;

    return 0;
}

int
struct account_hasCharGen(int gen)
{
    struct creature *tmp_ch = new struct creature(true);
	struct stat st;
	char *path;
    int idx;

    for (idx = 1;!this->invalid_char_index(idx);idx++) {
        tmp_ch->clear();

		// test for file existence
		path = get_player_file_path(this->get_char_by_index(idx));
        if (stat(path, &st) < 0)
			continue;
		if (!tmp_ch->loadFromXML(path))
			continue;

        if (GET_REMORT_GEN(tmp_ch) >= gen) {
            delete tmp_ch;
            return idx;
        }
    }

    delete tmp_ch;

    return 0;
}

int
struct account_countGens()
{
    struct creature *tmp_ch = new struct creature(true);
	struct stat st;
	char *path;
    int idx;
    int genCount = 0;

    for (idx = 1;!this->invalid_char_index(idx);idx++) {
        tmp_ch->clear();

		// test for file existence
		path = get_player_file_path(this->get_char_by_index(idx));
        if (stat(path, &st) < 0)
			continue;
		if (!tmp_ch->loadFromXML(path))
			continue;

        genCount += GET_REMORT_GEN(tmp_ch);
    }

    delete tmp_ch;

    return genCount;
}
