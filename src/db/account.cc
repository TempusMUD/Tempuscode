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
#include "security.h"
#include "clan.h"
#include "handler.h"

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

	slog("Reading player accounts");

	for (int num = 0;num < 10;num++) {
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

		closedir(dir);
	}
	if (accountIndex.size())
		slog("... %d account%s loaded", accountIndex.size(), (accountIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No accounts loaded");
	if (playerIndex.size())
		slog("... %d character%s loaded", playerIndex.size(), (playerIndex.size() == 1) ? "":"s");
	else
		slog("WARNING: No characters loaded");
	playerIndex.sort();
	accountIndex.sort();
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
		} else if (xmlMatches(node->name, "lastentry")) {
			_entry_time = xmlGetIntProp(node, "time");
		} else if (xmlMatches(node->name, "bank")) {
			_bank_past = xmlGetLongLongProp(node, "past");
			_bank_future = xmlGetLongLongProp(node, "future");
		} else if (xmlMatches(node->name, "character")) {
			num = xmlGetIntProp(node, "idnum");
			str = xmlGetProp(node, "name");
			_chars.push_back(num);
			playerIndex.add(num, str, _id, false);
			free(str);
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
	vector<long>::iterator cur_pc;
	char *path = get_account_file_path(_id);
	FILE *ouf;

	ouf = fopen(path, "w");

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
	fprintf(ouf, "\t<lastentry time=\"%lu\"/>\n", _entry_time);
	fprintf(ouf, "\t<display ansi=\"%u\" height=\"%u\" width=\"%u\"/>\n",
		_ansi_level, _term_height, _term_width);
	fprintf(ouf, "\t<bank past=\"%lld\" future=\"%lld\"/>\n",
		_bank_past, _bank_future);
	for (cur_pc = _chars.begin();cur_pc != _chars.end();cur_pc++) {
		fprintf(ouf, "\t<character idnum=\"%ld\" name=\"%s\"/>\n",
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

bool 
AccountIndex::exists( int accountID ) const
{
    return binary_search( begin(), end(), accountID, AccountIndex::cmp() );
}

Account* 
AccountIndex::find_account(const char *name) 
{
	AccountIndex::iterator it;
	Account *acct;

	for (it = begin();it != end();it++) {
		acct = *it;
		if (!strcasecmp(acct->get_name(), name))
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
		slog("SYSERR: Attempt to add account with conflicting idnum %d", acct->get_idnum() );
		return false;
	}

	if (acct->get_idnum() > _top_id)
		_top_id = acct->get_idnum();
	this->push_back(acct);
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

	ch = new Creature(true);

    GET_NAME(ch) = strdup(tmp_capitalize(tmp_tolower(name)));
    ch->char_specials.saved.idnum = playerIndex.getTopIDNum() + 1;
	_chars.push_back(GET_IDNUM(ch));
	
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
	save_to_xml();

	// Remove character from player index
	playerIndex.remove(GET_IDNUM(ch));

}

bool
Account::authenticate(const char *pw)
{
	if( _password == NULL ) {
		slog("SYSERR: Account %s[%d] has NULL password. Setting to guess.", _name, _id );
		_password = strdup( crypt(pw, _password) );
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

		save_to_xml();
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
}

void
Account::set_password(const char *pw)
{
	char salt[13] = "$1$........$";
	int idx;

	for (idx = 3; idx < 12; idx++) {
		salt[idx] = rand() & 63;
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
}

void
Account::deposit_past_bank(long long amt)
{
	if (amt > 0)
		_bank_past += amt;
}

void
Account::deposit_future_bank(long long amt)
{
	if (amt > 0)
		_bank_future += amt;
}

void
Account::withdraw_past_bank(long long amt)
{
	if (amt > 0)
		_bank_past -= amt;
}

void
Account::withdraw_future_bank(long long amt)
{
	if (amt > 0)
		_bank_future -= amt;
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
		save_to_xml();
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
}
