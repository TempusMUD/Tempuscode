#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <vector>
#include "xml_utils.h"

using namespace std;

struct Creature;
struct descriptor_data;

const int DEFAULT_TERM_HEIGHT = 22;
const int DEFAULT_TERM_WIDTH = 80;
const char *ACCOUNTS_PREFIX = "players/accounts";

void boot_accounts(void);

class Account {
	public:
		Account(void);
		~Account(void);

		bool load_from_xml(xmlDocPtr doc, xmlNodePtr root);
		bool save_to_xml(void);

		inline int get_id(void) const { return _id; }
		bool authenticate(const char *password);
		Creature *create_char(void);

		long long get_past_bank(void);
		long long get_future_bank(void);
		void set_past_bank(long long amt);
		void set_future_bank(long long amt);
		void deposit_past_bank(long long amt);
		void deposit_future_bank(long long amt);
		void withdraw_past_bank(long long amt);
		void withdraw_future_bank(long long amt);

		void set_name(const char *name);
		void set_password(const char *password);
		void set_cxn(descriptor_data *d);

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
		descriptor_data *_active_cxn;
		Creature *_active_char;
		long long _bank_past;
		long long _bank_future;
};

inline bool operator==(const Account &a, const Account &b) { 
    return a.get_id() == b.get_id();
}
inline bool operator<(const Account &a, const Account &b) { 
    return a.get_id() < b.get_id();
}
inline bool operator>(const Account &a, const Account &b) { 
    return a.get_id() > b.get_id();
}

inline bool operator==(const Account &a, int id ) { return a.get_id() == id; }
inline bool operator<(const Account &a, int id ) { return a.get_id() < id; }
inline bool operator>(const Account &a, int id ) { return a.get_id() > id; }
inline bool operator==(int id , const Account &b) { return id == b.get_id(); }
inline bool operator<(int id, const Account &b) { return id < b.get_id(); }
inline bool operator>(int id, const Account &b) { return id > b.get_id(); }


class AccountIndex : public vector<Account> 
{
	public:
		AccountIndex() : vector<Account>() {}

		// retrieves the account with the given id or NULL
		Account* find_account( int id );
		// returns true if the account exists
		bool account_exists( int id );
		void sort();
};

extern AccountIndex accountIndex;

#endif
