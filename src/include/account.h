#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <vector>

using namespace std;

struct Creature;
struct descriptor_data;

const int DEFAULT_TERM_HEIGHT = 22;
const int DEFAULT_TERM_WIDTH = 80;

extern const char *ansi_levels[];
extern const char *compact_levels[];

class Account {
	public:
		static void boot(void);
		static Account *create(const char *name, descriptor_data *d);
		static Account *retrieve(const char *name);
		static Account *retrieve(int id);
        static bool exists(int accountID);
		static bool remove(Account *acct);
		static size_t cache_size(void);

		Account(void);
		~Account(void);

		inline bool has_password(void) const { return _password != NULL; }
		bool authenticate(const char *password);
		void login(descriptor_data *d);
		void logout(descriptor_data *d, bool forced);
        bool is_logged_in() const;
		void initialize(const char *name, descriptor_data *d, int idnum);
		bool load(long idnum);

		inline const char *get_name(void) const { return _name; }
		inline int get_idnum(void) const { return _id; }
		inline void set_idnum(int idnum) { _id = idnum; }

		inline int get_ansi_level(void) { return _ansi_level; }
		void set_ansi_level(int level);
		inline int get_compact_level(void) { return _compact_level; }
		void set_compact_level(int level);
		inline int get_term_height(void) { return _term_height; }
		void set_term_height(int height);
		inline int get_term_width(void) { return _term_width; }
		void set_term_width(int width);

        inline const char *get_email_addr(void) const { return _email; }
		void set_email_addr(const char *addr);

		Creature *create_char(const char *name);
		void delete_char(Creature *ch);
		long get_char_by_index(int idx);
		Creature *get_creature_by_index(int idx);
		bool invalid_char_index(int idx);
        unsigned int get_char_count() { return _chars.size(); }
        long get_char( int index ) { return _chars[index]; }
		bool deny_char_entry(Creature *ch);
		void move_char(long id, Account *dest);
		// Attempts to add the given orphaned character to this account
		void exhume_char( Creature *exhumer, long id );

		inline int get_reputation(void) { return _reputation; }
		void gain_reputation(int amt);

		inline int get_quest_points(void) { return _quest_points; }
		void set_quest_points(int amt);

		inline long long get_past_bank(void) { return _bank_past; }
		inline long long get_future_bank(void) { return _bank_future; }
		void set_past_bank(long long amt);
		void set_future_bank(long long amt);
		void deposit_past_bank(long long amt);
		void deposit_future_bank(long long amt);
		void withdraw_past_bank(long long amt);
		void withdraw_future_bank(long long amt);

		void set_password(const char *password);
		
		inline const char* get_login_addr() { return _login_addr; }
		inline const char* get_creation_addr() { return _creation_addr; }
		
        inline time_t get_login_time() { return _login_time; }
        inline time_t get_creation_time() { return _creation_time; }
		inline time_t get_entry_time() { return _entry_time; }
		void update_last_entry(void);
        
		bool isTrusted(long idnum);
		void trust(long idnum);
		void distrust(long idnum);
		inline bool trustsNobody(void) { return _trusted.empty(); }
		void displayTrusted(Creature *ch);

		void set(const char *key, const char *val);
		void add_player(long idnum);
		void add_trusted(long idnum);

		class cmp {
			public:
				bool operator()(const Account *s1, const Account *s2) const
					{ return s1->get_idnum() < s2->get_idnum(); }
				bool operator()(const Account *s1, int id) const
					{ return s1->get_idnum() < id; }
				bool operator()(int id, const Account *s1) const
					{ return id < s1->get_idnum(); }
		};

	private:
		static int long _top_id;
		static vector <Account *> _cache;

		// Internal
		int _id;
		char *_name;
		char *_password;
		// Administration
		char *_email;
		time_t _creation_time;		// time account was created
		char *_creation_addr;
		time_t _login_time;			// last time account was logged into
		char *_login_addr;
		time_t _entry_time;			// last time char entered game
		// Account-wide references
		unsigned char _ansi_level;
		unsigned char _compact_level;
		unsigned int _term_height;
		unsigned int _term_width;
		// Game data
		vector<long> _chars;
		vector<long> _trusted;
		int _reputation;
		int _quest_points;
		long long _bank_past;
		long long _bank_future;
};

#endif
