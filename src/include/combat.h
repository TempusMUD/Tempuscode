
#define COMBAT_DIR "combat"
#define COMBATFILENAME "combat/combatlog"

#define COMBAT_STARTED (1 << 0)  // Combat has already started
#define COMBAT_OPEN    (1 << 1)  // People can still join
#define COMBAT_INVITE  (1 << 2)  // Only invited people can join
#define COMBAT_DRAW    (1 << 3)  // Combat was a draw(all players killed)

#define CTYPE_CLANWAR         0      // The fight is a clanwar
#define CTYPE_ONE_ONE         1      // One on one combat
#define CTYPE_FREE_FOR_ALL    2      // last man standing wins

#define COMBAT_FLAGGED(cm, bits) (IS_SET(cm->flags, bits))

#define MAX_COMBAT_NAME 31
#define MAX_COMBAT_DESC 1024
#define MAX_TEAM_MEMBERS 10
#define MAX_TEAMS        3

#define CLOG_OFF    0
#define CLOG_BRIEF  1
#define CLOG_NORM   2
#define CLOG_COMP   3

#define IN_COMBAT(ch) (char_in_combat_vnum(ch) >= 0)
#define COMBAT_CREATOR(ch,combat) (GET_IDNUM(ch) == combat->creator)

typedef struct carena_data {
    long zone;
    long booty_room;
    int used;		
    struct carena_data *next;	
}carena_data;

typedef struct cplayer_data {
    long         idnum;
    int          flags;
    int          team;
    int          approved;
    struct cplayer_data *next;
} cplayer_data;



typedef struct combat_data {
    int vnum;
    int creator;
    int flags;	
    int owner_level;
    int max_combatants;
    int type;
    int num_teams;
    int max_teams;
    int sacrificed;
    struct room_data *booty_room;
    struct carena_data *arena;
    long winner;
    long fee;
    time_t started;
    time_t ended;
    char *name;
    char *description;
    cplayer_data *players;
    ubyte num_players;	
    struct combat_data *next;
} combat_data;


void list_combat_players(CHAR *ch, combat_data *combat, char *outbuf);
void comlog(CHAR *ch, char *str, int file, int to_char);
combat_data* create_combat(CHAR *ch, int type, char *name);
struct combat_data* combat_by_vnum(int num);	
cplayer_data * idnum_in_combat(int idnum, combat_data *quest);
struct room_data *random_arena_room(struct combat_data *combat,CHAR* ch);
struct carena_data* create_arena(long the_zone, long the_room);
struct carena_data* arena_by_num(int num);

int boot_combat(void);
int add_idnum_to_combat(int idnum, combat_data *combat);
int remove_idnum_from_combat(int idnum, combat_data *combat);
int find_combat_type(char *argument);
int check_battles(combat_data *combat);
int check_teams(combat_data *combat);
int end_battle(combat_data *combat);
int trans_combatants(struct combat_data *combat);
int char_in_combat_vnum(CHAR* ch);
int build_arena_list(void);

void say_to_combat(char* argument, CHAR* ch, combat_data *combat);
void send_to_combat(char* argument, combat_data *combat);
void remove_combat(struct combat_data *combat);
void remove_players(struct combat_data *combat);
void remove_player(CHAR* ch);
void add_arena(struct carena_data *arena);
void return_sacrifice(CHAR* ch);
void return_sacrifices(struct combat_data *combat);
void show_arenas(CHAR* ch);
void combat_loop(CHAR* ch, CHAR* killer);
void random_arena(CHAR* ch, combat_data* combat);
void combat_reimburse(CHAR *ch, combat_data* combat);

void do_ccontrol_create (CHAR *ch, char *argument, int com);
void do_ccontrol_usage(CHAR *ch, int com);
void do_ccontrol_options(CHAR *ch);
void do_ccontrol_show(CHAR *ch, char *argument);
void do_ccontrol_join(CHAR *ch,char* argument);
void do_ccontrol_open(CHAR *ch);
void do_ccontrol_close(CHAR *ch);
void do_ccontrol_start(CHAR *ch);
void do_ccontrol_sacrifice(CHAR *ch, char* argument);
void do_ccontrol_arena(CHAR *ch, char* argument);
void do_ccontrol_say(CHAR* ch, char* argument);
void do_ccontrol_describe(CHAR* ch);
void do_ccontrol_end(CHAR *ch);
void do_ccontrol_leave(CHAR *ch);
void do_ccontrol_approve(CHAR *ch);
void do_ccontrol_fee(CHAR*ch, char* arg);
void do_ccontrol_reimburse(CHAR* ch);

//Wiz combat options
void do_ccontrol_wizoptions(CHAR* ch, char* argument);
void do_ccontrol_destroy(CHAR* ch, char* argument);
void do_ccontrol_lock(CHAR* ch);

















