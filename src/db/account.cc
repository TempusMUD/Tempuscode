#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <utility>
#include "utils.h"
#include "desc_data.h"
#include "interpreter.h"
#include "player_table.h"
#include "tmpstr.h"
#include "account.h"
#include "comm.h"
#include "security.h"
#include "clan.h"
#include "handler.h"
#include "db.h"

const char *ansi_levels[] = {
	"none",
	"normal",
	"complete",
	"\n"
};

char *get_account_file_path(long id);

long Account::_top_id = 0;
vector <Account *> Account::_cache;

void
Account::boot(void)
{
	long count, idx;
	PGresult *res;

	slog("Reading player records");

	// Add the players to the player index -- accounts are added to the
	// account index as they are requested
	res = sql_query("select idnum, name, account from players");
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++)
		playerIndex.add(atol(PQgetvalue(res, idx, 0)),
			PQgetvalue(res, idx, 1),
			atoi(PQgetvalue(res, idx, 2)),
			false);
	PQclear(res);

	res = sql_query("select MAX(idnum) from accounts");
	_top_id = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

	if (playerIndex.size())
		slog("... %d character%s loaded", playerIndex.size(), (playerIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No characters loaded");
	playerIndex.sort();
}

size_t
Account::cache_size(void)
{
	return _cache.size();
}

Account::Account(void)
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
    _bank_past = 0;
    _bank_future = 0;
	_term_height = DEFAULT_TERM_HEIGHT;
	_term_width = DEFAULT_TERM_WIDTH;
}

Account::~Account(void)
{
	if (_name)
		free(_name);
	if (_password)
		free(_password);
	if (_email)
		free(_email);
	if (_creation_addr)
		free(_creation_addr);
	if (_login_addr)
		free(_login_addr);

	Account::remove(this);
}

bool
Account::load(long idnum)
{
	long acct_count, field_count, field_idx;
	long count, idx;
	const char **fields;
	PGresult *res;

	res = sql_query("select idnum, name, password, email, date_part('epoch', creation_time) as creation_time, creation_addr, date_part('epoch', login_time) as login_time, login_addr, date_part('epoch', entry_time) as entry_time, ansi_level, term_height, term_width, bank_past, bank_future from accounts where idnum=%ld", idnum);
	acct_count = PQntuples(res);

	if (acct_count > 1) {
		slog("SYSERR: search for account %ld returned more than one match", idnum);
		return false;
	}

	if (acct_count < 1)
		return false;

	// Get field names and put them in an array
	field_count = PQnfields(res);
	fields = new (const char *)[field_count];
	for (field_idx = 0;field_idx < field_count;field_idx++)
		fields[field_idx] = PQfname(res, field_idx);

	for (field_idx = 0;field_idx < field_count;field_idx++)
		this->set(fields[field_idx],
			PQgetvalue(res, 0, field_idx));
	PQclear(res);
	delete [] fields;

	// Now we add the players to the account and player index
	res = sql_query("select idnum from players where account=%ld", idnum);
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++)
		this->add_player(atol(PQgetvalue(res, idx, 0)));
	PQclear(res);

	// Add trusted players to account
	res = sql_query("select player from trusted where account=%ld", idnum);
	count = PQntuples(res);
	for (idx = 0;idx < count;idx++)
		this->add_trusted(atol(PQgetvalue(res, idx, 0)));
	PQclear(res);

	std::sort(_cache.begin(),_cache.end(), Account::cmp());
	return true;
}

void
Account::set(const char *key, const char *val)
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
	else if (!strcmp(key, "bank_past"))
		_bank_past = atoll(val);
	else if (!strcmp(key, "bank_future"))
		_bank_future = atoll(val);
	else
		slog("Invalid account field %s set to %s", key, val);
}

void
Account::add_player(long idnum)
{
	_chars.push_back(idnum);
}

void
Account::add_trusted(long idnum)
{
	_trusted.push_back(idnum);
}

Account* 
Account::retrieve(int id) 
{
	vector <Account *>::iterator it;
	Account *acct;
	
	// First check to see if we already have it in memory
	it = lower_bound( _cache.begin(), _cache.end(), id, Account::cmp() );
	if( it != _cache.end() && (*it)->get_idnum() == id )
		return *it;
	
	// Apprently, we don't, so look it up on the db
	acct = new Account;
	if (acct->load(id)) {
		_cache.push_back(acct);
		return acct;
	}
	return NULL;
}

bool 
Account::exists( int accountID )
{
	PGresult *res;
	bool result;

	res = sql_query("select idnum from accounts where idnum=%d", accountID);
	result = PQntuples(res) > 0;
	PQclear(res);

    return result;
}

Account* 
Account::retrieve(const char *name) 
{
	vector <Account *>::iterator it;
	Account *acct;
	PGresult *res;
	int acct_id;

	// First check to see if we already have it in memory
	for (it = _cache.begin();it != _cache.end();it++) {
		acct = *it;
		if (!strcasecmp(acct->get_name(), name))
			return acct;
	}

	// Apprently, we don't, so look it up on the db
	res = sql_query("select idnum from accounts where name='%s'",
		tmp_sqlescape(name));
	if (PQntuples(res) != 1)
		return NULL;
	acct_id = atoi(PQgetvalue(res, 0, 0));
	PQclear(res);

	// Now try to load it
	acct = new Account;
	if (acct->load(acct_id)) {
		_cache.push_back(acct);
		return acct;
	}
	return NULL;
}

Account *
Account::create(const char *name, descriptor_data *d)
{
	Account *result;

	_top_id++;
	result = new Account;
	result->initialize(name, d, _top_id);
	_cache.push_back(result);
	std::sort(_cache.begin(),_cache.end(), Account::cmp());
	return result;
}

bool
Account::remove(Account *acct)
{
	vector <Account *>::iterator it;

	for (it = _cache.begin();it != _cache.end();it++)
		if (*it == acct) {
			_cache.erase(it);
			return true;
		}
	
	return false;
}


// Create a brand new character
Creature *
Account::create_char(const char *name)
{
	Creature *ch;
	int i;

	ch = new Creature(true);

    GET_NAME(ch) = strdup(tmp_capitalize(tmp_tolower(name)));
    ch->char_specials.saved.idnum = playerIndex.getTopIDNum() + 1;
	_chars.push_back(GET_IDNUM(ch));
	sql_exec("insert into players (idnum, name, account) values (%ld, '%s', %d)",
		GET_IDNUM(ch), tmp_sqlescape(name), _id);
	
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
	ch->player_specials->saved.quest_points = 0;
	ch->player_specials->saved.quest_id = 0;
	ch->player_specials->saved.qlog_level = 0;

	GET_REMORT_CLASS(ch) = -1;
    ch->player.weight = 100;
    ch->player.height = 100;

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.mana = GET_MAX_MANA(ch);
	ch->points.move = GET_MAX_MOVE(ch);
	ch->points.armor = 100;

	SET_BIT(PRF_FLAGS(ch), PRF_COMPACT | PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_AUTOEXIT | PRF_NOSPEW);
	SET_BIT(PRF2_FLAGS(ch), PRF2_AUTO_DIAGNOSE | PRF2_AUTOPROMPT | PRF2_DISPALIGN | PRF2_NEWBIE_HELPER);

	
	playerIndex.add( ch->char_specials.saved.idnum, GET_NAME(ch), _id );

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
Account::delete_char(Creature *ch)
{
	vector<long>::iterator it;
	clan_data *clan;
	Creature *real_ch;

	// Remove character from game
	real_ch = get_char_in_world_by_idnum(GET_IDNUM(ch));
	if (real_ch) {
		send_to_char(real_ch, "This character has been deleted!  Goodbye!\r\n");
		real_ch->purge(false);
	}
	// Remove character from clan
	clan = real_clan(GET_CLAN(ch));
	if (clan) {
		struct clanmember_data *member, *temp;
		
		member = real_clanmember(GET_IDNUM(ch), clan);
		if (member) {
			REMOVE_FROM_LIST(member, clan->member_list, next);
			save_clans();
		}
	}

	// Remove character from any access groups
	list<Security::Group>::iterator group;

	for (group = Security::groups.begin();group != Security::groups.end();group++)
		group->removeMember(GET_IDNUM(ch));
	Security::saveGroups();

	// Remove character from account
	it = lower_bound(_chars.begin(), _chars.end(), GET_IDNUM(ch));
	if (it != _chars.end())
		_chars.erase(it);
	sql_exec("delete from players where idnum=%ld", GET_IDNUM(ch));

	// Remove character from player index
	playerIndex.remove(GET_IDNUM(ch));

}

bool
Account::authenticate(const char *pw)
{
	if(_password == NULL || *_password == '\0') {
		slog("SYSERR: Account %s[%d] has NULL password. Setting to guess.", _name, _id );
		set_password(pw);
	}
	return !strcmp(_password, crypt(pw, _password));
}

void
Account::login(descriptor_data *d)
{
	slog("login: %s[%d] from %s", _name, get_idnum(), d->host);
	set_desc_state(CXN_MENU, d);
}

void
Account::logout(descriptor_data *d, bool forced)
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
Account::initialize(const char *name, descriptor_data *d, int idnum)
{
	_id = idnum;
	_name = strdup(name);
	_password = NULL;
	_creation_time = _login_time = time(0);
	_creation_addr = strdup(d->host);
	_login_addr = strdup(d->host);
	_ansi_level = 0;
	_term_height = 22;
	_term_width = 80;
	_bank_past = 0;
	_bank_future = 0;

	slog("new account: %s[%d] from %s", _name, idnum, d->host);
	sql_exec("insert into accounts (idnum, name, creation_time, creation_addr, login_time, login_addr, ansi_level, term_height, term_width, bank_past, bank_future) values (%d, '%s', now(), '%s', now(), '%s', 0, %d, %d, 0, 0)",
		idnum, tmp_sqlescape(name), tmp_sqlescape(d->host),
		tmp_sqlescape(d->host), DEFAULT_TERM_HEIGHT, DEFAULT_TERM_WIDTH);
}

void
Account::set_password(const char *pw)
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
Account::deposit_past_bank(long long amt)
{
	if (amt > 0)
		set_past_bank(_bank_past + amt);
}

void
Account::deposit_future_bank(long long amt)
{
	if (amt > 0)
		set_future_bank(_bank_future + amt);
}

void
Account::withdraw_past_bank(long long amt)
{
	if (amt <= 0)
		return;
	if (amt > _bank_past)
		amt = _bank_past;
	set_past_bank(_bank_past - amt);
}

void
Account::withdraw_future_bank(long long amt)
{
	if (amt <= 0)
		return;
	if (amt > _bank_future)
		amt = _bank_future;
	set_future_bank(_bank_future - amt);
}

void
Account::set_email_addr(const char *addr)
{
	_email = strdup(addr);
	sql_exec("update accounts set email='%s' where idnum=%d",
		tmp_sqlescape(_email), _id);
}

long
Account::get_char_by_index(int idx)
{
	return _chars[idx - 1];
}

bool
Account::invalid_char_index(int idx)
{
	return (idx < 1 || idx > (int)_chars.size());
}

void
Account::move_char(long id, Account *dest)
{
	vector<long>::iterator it;
	char *name;

	// Remove character from account
	it = lower_bound(_chars.begin(), _chars.end(), id);
	if (it != _chars.end())
		_chars.erase(it);

	// Get the player's name before we delete from player table
	name = tmp_strdup(playerIndex.getName(*it));
	playerIndex.remove(*it);
	playerIndex.add(*it, name, dest->_id );
	dest->_chars.push_back(id);
	sql_exec("update players set account=%d where idnum=%ld",
		dest->_id, id);
}

void
Account::exhume_char( Creature *exhumer, long id )
{
	if( playerIndex.exists(id) ) {
		send_to_char(exhumer, "That character has already been exhumed.\r\n");
		return;
	}

	// load char from file
	Creature* victim = new Creature(true);
	if( victim->loadFromXML(id) ) {
		playerIndex.add(id, GET_NAME(victim), _id, true);
		_chars.push_back( id );
		send_to_char(exhumer, "%s exhumed.\r\n", 
					tmp_capitalize( GET_NAME(victim) ) );
	} else {
		send_to_char(exhumer, "Unable to load character %ld.\r\n", id );
	}
	delete victim;
}

bool
Account::deny_char_entry(Creature *ch)
{
	descriptor_data *d;
	int remorts = 0, mortals = 0;

    // Admins and full wizards can multi-play all they want
    if (Security::isMember(ch, "WizardFull"))
        return false;
    if (Security::isMember(ch, "AdminFull"))
        return false;	

	for (d = descriptor_list;d;d = d->next) {
		if (d->account == this &&
				d->input_mode == CXN_PLAYING &&
				d->creature) {
            // Admins and full wizards can multi-play all they want
			if (Security::isMember(d->creature, "WizardFull"))
				return false;
            if (Security::isMember(d->creature, "AdminFull"))
				return false;
            // builder can have on a tester and vice versa.
            if (Security::isMember(d->creature, "OLC") && ch->isTester())
				return false;
            if (ch->isTester() && Security::isMember(d->creature, "OLC"))
				return false;
			if (IS_REMORT(d->creature))
				remorts++;
			else
				mortals++;
		}
	}

	if (remorts)
		return true;
	if (mortals < 2)
		return false;

	return true;
}

bool 
Account::is_logged_in() const
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
Account::update_last_entry(void)
{
	_entry_time = time(0);
	sql_exec("update accounts set entry_time=now() where idnum=%d", _id);
}

bool
Account::isTrusted(long idnum)
{
	vector<long>::iterator it;

	for (it = _chars.begin();it != _chars.end();it++)
		if (*it == idnum)
			return true;

	for (it = _trusted.begin();it != _trusted.end();it++)
		if (*it == idnum)
			return true;

	return false;
}

void
Account::trust(long idnum)
{
	vector<long>::iterator it;

	for (it = _trusted.begin();it != _trusted.end();it++)
		if (*it == idnum)
			return;

	_trusted.push_back(idnum);
	sql_exec("insert into trusted (account, player) values (%d, %ld)",
		_id, idnum);
}

void
Account::distrust(long idnum)
{
	vector<long>::iterator it;

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
Account::displayTrusted(Creature *ch)
{
	vector<long>::iterator it;
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
Account::set_ansi_level(int level)
{
	_ansi_level = level;
	sql_exec("update accounts set ansi_level=%d where idnum=%d",
		_ansi_level, _id);
}

void
Account::set_past_bank(long long amt)
{
	_bank_past = amt;
	sql_exec("update accounts set bank_past=%lld where idnum=%d",
		_bank_past, _id);
}

void
Account::set_future_bank(long long amt)
{
	_bank_future = amt;
	sql_exec("update accounts set bank_future=%lld where idnum=%d",
		_bank_future, _id);
}

void
Account::set_term_height(int height)
{
	if (height < 2)
		height = 2;
	if (height > 200)
		height = 200;
	_term_height = height;
	sql_exec("update accounts set term_height=%d where idnum=%d",
		_term_height, _id);
}

void
Account::set_term_width(int width)
{
	if (width < 2)
		width = 2;
	if (width > 200)
		width = 200;
	_term_width = width;
	sql_exec("update accounts set term_width=%d where idnum=%d",
		_term_width, _id);
}

