#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <utility>
#include "xml_utils.h"
#include "utils.h"
#include "desc_data.h"
#include "interpreter.h"
#include "player_table.h"
#include "tmpstr.h"
#include "account.h"
#include "comm.h"

const char *ansi_levels[] = {
	"none",
	"normal",
	"complete",
	"\n"
};

AccountIndex accountIndex;
char *get_account_file_path(long id);

void
boot_accounts(void)
{
	DIR *dir;
	dirent *file;
	char dir_path[256], path[256];
	long acct_count = 0;
	xmlDocPtr doc;
	xmlNodePtr node;
	Account *new_acct;
	int num;

	slog("Reading player accounts");

	for (num = 1;num < 10;num++) {
		snprintf(dir_path, 255, "players/accounts/%d", num);
		dir = opendir(dir_path);
		if (!dir) {
			slog("SYSERR: directory '%s' does not exist", dir_path);
			return;
		}

		while ((file = readdir(dir)) != NULL) {
			if (!rindex(file->d_name, '.'))
				continue;
			if (strcmp(rindex(file->d_name, '.'), ".acct"))
				continue;

			snprintf(path, 255, "%s/%s", dir_path, file->d_name);
			doc = xmlParseFile(path);
			if (!doc) {
				slog("SYSERR: XML parse error while loading %s", path);
				continue;
			}

			node = xmlDocGetRootElement(doc);
			if (!node) {
				xmlFreeDoc(doc);
				slog("SYSERR: XML file %s is empty", path);
				continue;
			}

			if (!xmlMatches(node->name, "account")) {
				xmlFreeDoc(doc);
				slog("SYSERR: First XML node in %s is not an account", path);
				continue;
			}
			
			new_acct = new Account;
			new_acct->load_from_xml(doc, node);
			xmlFreeDoc(doc);

			accountIndex.add(new_acct);
			acct_count++;
		}
	}
	if (accountIndex.size())
		slog("... %d account%s loaded", accountIndex.size(), (accountIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No accounts loaded");
	if (playerIndex.size())
		slog("... %d character%s loaded", playerIndex.size(), (playerIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No characters loaded");

}

Account::Account(void)
	: _chars()
{
	time_t now;
	
	now = time(NULL);
	_id = 0;
	_name = NULL;
	_password = NULL;
	_email = NULL;
	_creation_time = now;
	_login_time = now;
	_creation_addr = NULL;
	_login_addr = NULL;
	_active_cxn = NULL;
	_active_char = NULL;
	_ansi_level = 0;
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

	accountIndex.remove(this);
}

bool
Account::load_from_xml(xmlDocPtr doc, xmlNodePtr root)
{
	int num;
	char *str;

	_id = xmlGetIntProp(root, "idnum");
	for (xmlNodePtr node = root->xmlChildrenNode;node;node = node->next) {
		if (xmlMatches(node->name, "display")) {
			_ansi_level = xmlGetIntProp(node, "ansi");
			_term_height = xmlGetIntProp(node, "height");
			_term_width = xmlGetIntProp(node, "width");
		} else if (xmlMatches(node->name, "login")) {
			_name = xmlGetProp(node, "name");
			_password = xmlGetProp(node, "password");
		} else if (xmlMatches(node->name, "email")) {
			_email = (char *)xmlNodeGetContent(node);
		} else if (xmlMatches(node->name, "creation")) {
			_creation_time = xmlGetIntProp(node, "time");
			_creation_addr = xmlGetProp(node, "addr");
		} else if (xmlMatches(node->name, "lastlogin")) {
			_login_time = xmlGetIntProp(node, "time");
			_login_addr = xmlGetProp(node, "addr");
		} else if (xmlMatches(node->name, "bank")) {
			_bank_past = xmlGetIntProp(node, "past");
			_bank_future = xmlGetIntProp(node, "future");
		} else if (xmlMatches(node->name, "character")) {
			num = xmlGetIntProp(node, "idnum");
			str = xmlGetProp(node, "name");
			_chars.push_back(num);
			playerIndex.add(num, str, _id, false);
		} else if (!xmlMatches(node->name, "text")) {
			slog("Can't happen at %s:%d", __FILE__, __LINE__);
			return false;
		}
	}

	return true;
}

bool
Account::save_to_xml(void)
{
	vector<int>::iterator cur_pc;
	char *path;
	FILE *ouf;

	ouf = fopen(get_account_file_path(_id), "w");

	if (!ouf) {
		slog("Unable to open XML account file '%s' for writing: %s", path,
			strerror(errno));
		return false;
	}

	fprintf(ouf, "<account idnum=\"%d\">\n", _id);
	fprintf(ouf, "\t<login name=\"%s\" password=\"%s\"/>\n", _name, _password);
	if (_email)
		fprintf(ouf, "\t<email>%s</email>\n", _email);
	fprintf(ouf, "\t<creation time=\"%lu\" addr=\"%s\"/>\n", _creation_time,
		_creation_addr);
	fprintf(ouf, "\t<lastlogin time=\"%lu\" addr=\"%s\"/>\n", _login_time,
		_login_addr);
	fprintf(ouf, "\t<display ansi=\"%u\" height=\"%u\" width=\"%u\"/>\n",
		_ansi_level, _term_height, _term_width);
	for (cur_pc = _chars.begin();cur_pc != _chars.end();cur_pc++) {
		fprintf(ouf, "\t<character idnum=\"%d\" name=\"%s\"/>\n",
			*cur_pc, playerIndex.getName(*cur_pc));
	}

	fprintf(ouf, "</account>\n");
	fclose(ouf);

	return true;
}

Account* 
AccountIndex::find_account( int id ) 
{
	AccountIndex::iterator it = lower_bound( begin(), end(), id, AccountIndex::cmp() );
	if( it != end() && (*it)->get_idnum() == id )
		return *it;
	return NULL;
}

Account* 
AccountIndex::find_account(const char *name) 
{
	AccountIndex::iterator it;
	Account *acct;

	for (it = begin();it != end();it++) {
		acct = *it;
		if (!strcmp(acct->get_name(), name))
			return acct;
	}
	return NULL;
}

Account *
AccountIndex::create_account(const char *name, descriptor_data *d)
{
	Account *result;

	_top_id++;
	result = new Account;
	result->initialize(name, d, _top_id);
	this->push_back(result);
	sort();
	return result;
}

bool
AccountIndex::add(Account *acct)
{
	if (find_account(acct->get_idnum())) {
		slog("SYSERR: Attempt to add account with conflicting idnum");
		return false;
	}

	if (acct->get_idnum() > _top_id)
		_top_id = acct->get_idnum();
	this->push_back(acct);
	sort();
	return true;
}

bool
AccountIndex::remove(Account *acct)
{
	AccountIndex::iterator it;

	for (it = begin();it != end();it++)
		if (*it == acct) {
			erase(it);
			return true;
		}
	
	return false;
}

void
AccountIndex::sort(void)
{
	std::sort(begin(),end(), AccountIndex::cmp());
}


// Create a brand new character
Creature *
Account::create_char(const char *name)
{
	Creature *ch;
	int i;

	// create a player_special structure
	ch = new Creature;

	// *** if this is our first player --- he be God ***
	if (!playerIndex.exists(1)) {
		GET_EXP(ch) = 160000000;
		GET_LEVEL(ch) = LVL_GRIMP;

		ch->points.max_hit = 666;
		ch->points.max_mana = 555;
		ch->points.max_move = 444;

		GET_HOME(ch) = HOME_MODRIAN;
		ch->player_specials->saved.load_room = 1204;
	} else {
		ch->points.max_hit = 100;
		ch->points.max_mana = 100;
		ch->points.max_move = 82;

		GET_HOME(ch) = HOME_NEWBIE_SCHOOL;
		ch->player_specials->saved.load_room = -1;
	}

	GET_NAME(ch) = strdup(tmp_capitalize(tmp_tolower(name)));

	ch->char_specials.saved.idnum = playerIndex.getTopIDNum() + 1;
	_chars.push_back(GET_IDNUM(ch));

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
	/* make favors for sex ... and race */
	if (ch->player.sex == SEX_MALE) {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(110, 160) + GET_STR(ch);
			ch->player.height = number(160, 200) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(120, 160) + GET_STR(ch);
			ch->player.height = number(100, 125) + (GET_WEIGHT(ch) >> 4);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(140, 165) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(120, 180) + GET_STR(ch);
			ch->player.height = number(120, 200) + (GET_WEIGHT(ch) >> 4);
		} else if (GET_RACE(ch) == RACE_MINOTAUR) {
			ch->player.weight = number(200, 360) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		} else {
			ch->player.weight = number(130, 180) + GET_STR(ch);
			ch->player.height = number(140, 190) + (GET_WEIGHT(ch) >> 3);
		}
	} else {
		if (GET_RACE(ch) == RACE_HUMAN) {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_TABAXI) {
			ch->player.weight = number(80, 120) + GET_STR(ch);
			ch->player.height = number(160, 190) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_DWARF) {
			ch->player.weight = number(100, 140) + GET_STR(ch);
			ch->player.height = number(90, 115) + (GET_WEIGHT(ch) >> 4);
		} else if (IS_ELF(ch) || IS_DROW(ch)) {
			ch->player.weight = number(90, 130) + GET_STR(ch);
			ch->player.height = number(120, 155) + (GET_WEIGHT(ch) >> 3);
		} else if (GET_RACE(ch) == RACE_HALF_ORC) {
			ch->player.weight = number(110, 170) + GET_STR(ch);
			ch->player.height = number(110, 190) + (GET_WEIGHT(ch) >> 3);
		} else {
			ch->player.weight = number(90, 150) + GET_STR(ch);
			ch->player.height = number(140, 170) + (GET_WEIGHT(ch) >> 3);
		}
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.mana = GET_MAX_MANA(ch);
	ch->points.move = GET_MAX_MOVE(ch);
	ch->points.armor = 100;

	SET_BIT(PRF_FLAGS(ch), PRF_COMPACT | PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_AUTOEXIT | PRF_NOSPEW);
	SET_BIT(PRF2_FLAGS(ch), PRF2_AUTO_DIAGNOSE | PRF2_AUTOPROMPT | PRF2_DISPALIGN | PRF2_NEWBIE_HELPER);

	
	playerIndex.add( ch->char_specials.saved.idnum, GET_NAME(ch), _id );

	if (GET_LEVEL(ch) < LVL_GRIMP) {
		for (i = 1; i <= MAX_SKILLS; i++)
			SET_SKILL(ch, i, 0);
	} else {
		for (i = 1; i <= MAX_SKILLS; i++)
			SET_SKILL(ch, i, 100);
	}

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
	vector<int>::iterator it;

	it = lower_bound(_chars.begin(), _chars.end(), GET_IDNUM(ch));
	if (it != _chars.end())
		_chars.erase(it);
	playerIndex.remove(GET_IDNUM(ch));
}

bool
Account::authenticate(const char *pw)
{
	return !strcmp(_password, crypt(pw, _password));
}

void
Account::login(descriptor_data *d)
{
	if (_active_cxn) {
		slog("login: %s from %s, disconnecting %s",
			_name, _login_addr, _active_cxn->host);
		set_desc_state(_active_cxn->input_mode, d);

		set_desc_state(CXN_DISCONNECT, d);
		_active_cxn->creature = NULL;
		_active_cxn->account = NULL;
		if (_active_char) {
			d->creature = _active_char;
			_active_char->desc = d;
			send_to_char(d->creature, "You take over your own body, already in use!\r\n");
			mudlog(GET_INVIS_LVL(_active_char), NRM, true,
				"%s has reconnected to %s [%s]",
				_name, GET_NAME(_active_char), _login_addr);
		}
	} else if (_active_char) {
		slog("login: %s from %s", _name, _login_addr);
		d->creature = _active_char;
		_active_char->desc = d;
		set_desc_state(CXN_PLAYING, d);
		mudlog(GET_INVIS_LVL(_active_char), NRM, true,
			"%s has reconnected to %s [%s]",
			_name, GET_NAME(_active_char), _login_addr);
		send_to_char(_active_char, "You take over your own body!\r\n");
	} else {
		slog("login: %s from %s", _name, _login_addr);
		set_desc_state(CXN_MENU, d);
	}
	_active_cxn = d;
}

void
Account::logout(bool forced)
{
	if (!_active_cxn) {
		slog("SYSERR: account '%s' logout without active link", _name);
		return;
	}

	if (_password) {
		_login_time = time(NULL);
		if (_login_addr)
			free(_login_addr);
		_login_addr = strdup(_active_cxn->host);

		save_to_xml();
		slog("%slogout: %s from %s", (forced) ? "forced ":"",
			_name, _login_addr);
	} else {
		slog("%slogout: unfinished account from %s", (forced) ? "forced ":"",
			_login_addr);
	}

	set_desc_state(CXN_DISCONNECT, _active_cxn);
	_active_cxn = NULL;
}

void
Account::initialize(const char *name, descriptor_data *d, int idnum)
{
	_id = idnum;
	_name = strdup(name);
	_active_cxn = d;
	slog("new account: #%d (%s) from %s", idnum, _name, _login_addr);
}

void
Account::set_password(const char *pw)
{
	char salt[3] = "**";

	salt[0] = rand() & 63;
	if (salt[0] < 10)
		salt[0] += '0';
	else if (salt[0] < 36)
		salt[0] = salt[0] - 10 + 'A';
	else if (salt[0] < 62)
		salt[0] = salt[0] - 36 + 'a';
	else if (salt[0] == 63)
		salt[0] = '.';
	else
		salt[0] = '/';
	salt[1] = rand() & 63;
	if (salt[1] < 10)
		salt[1] += '0';
	else if (salt[1] < 36)
		salt[1] = salt[1] - 10 + 'A';
	else if (salt[1] < 62)
		salt[1] = salt[1] - 36 + 'a';
	else if (salt[1] == 63)
		salt[1] = '.';
	else
		salt[1] = '/';
	_password = strdup(crypt(pw, salt));
}

void
Account::deposit_past_bank(long long amt)
{
	// STUB
}

void
Account::deposit_future_bank(long long amt)
{
	// STUB
}

void
Account::withdraw_past_bank(long long amt)
{
	// STUB
}

void
Account::withdraw_future_bank(long long amt)
{
	// STUB
}

void
Account::set_email_addr(const char *addr)
{
	_email = strdup(addr);
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
