#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <vector>
#include "xml_utils.h"

using namespace std;

struct Creature;
struct descriptor_data;

const int DEFAULT_TERM_HEIGHT = 22;
const int DEFAULT_TERM_WIDTH = 80;

void boot_accounts(void);

class Account {
	public:
		Account(void);
		~Account(void);

		bool load_from_xml(xmlDocPtr doc, xmlNodePtr root);
		bool save_to_xml(void);

		bool authenticate(const char *password);
		void login(descriptor_data *d);
		void logout(descriptor_data *d, bool forced);
		void initialize(const char *name, descriptor_data *d, int idnum);

		inline const char *get_name(void) const { return _name; }
		inline int get_idnum(void) const { return _id; }
		inline void set_idnum(int idnum) { _id = idnum; }

		inline int get_ansi_level(void) { return _ansi_level; }
		inline void set_ansi_level(int level) { _ansi_level = level; }
		inline int get_term_height(void) { return _term_height; }
		inline int get_term_width(void) { return _term_width; }
		void set_email_addr(const char *addr);

		Creature *create_char(const char *name);
		void delete_char(Creature *ch);
		long get_char_by_index(int idx);
		Creature *get_creature_by_index(int idx);
		bool invalid_char_index(int idx);
		bool deny_char_entry(void);

		inline long long get_past_bank(void) { return _bank_past; }
		inline long long get_future_bank(void) { return _bank_future; }
		inline void set_past_bank(long long amt) { _bank_past = amt; }
		inline void set_future_bank(long long amt) { _bank_future = amt; }
		void deposit_past_bank(long long amt);
		void deposit_future_bank(long long amt);
		void withdraw_past_bank(long long amt);
		void withdraw_future_bank(long long amt);

		void set_password(const char *password);

	private:
		// Internal
		int _id;
		char *_name;
		char *_password;
		// Administration
		char *_email;
		time_t _creation_time;
		char *_creation_addr;
		time_t _login_time;
		char *_login_addr;
		// Account-wide references
		unsigned char _ansi_level;
		unsigned int _term_height;
		unsigned int _term_width;
		// Game data
		vector<int> _chars;
		long long _bank_past;
		long long _bank_future;
};

class AccountIndex : public vector<Account *> 
{
	class cmp {
		public:
			bool operator()(const Account *s1, const Account *s2) const
				{ return s1->get_idnum() < s2->get_idnum(); }
			bool operator()(const Account *s1, int id) const
				{ return s1->get_idnum() < id; }
	};
	public:
		AccountIndex() : vector<Account *>(), _top_id(0) {}

		// retrieves the account with the given id or NULL
		Account *find_account(const char *name);
		Account *find_account(int id);

		Account *create_account(const char *name, descriptor_data *d);
		bool add(Account *acct);
		bool remove(Account *acct);

		void sort();
	private:
		long _top_id;
};

extern AccountIndex accountIndex;

#endif
