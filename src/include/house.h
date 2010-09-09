#ifndef _HOUSE_H_
#define _HOUSE_H_

#include "defs.h"
#include "xml_utils.h"
#include "tmpstr.h"

struct creature;
struct obj_data;
struct room_data;

static inline char*
get_house_file_path( int id )
{
	return tmp_sprintf( "players/housing/%d/%04d.dat", (id % 10), id );
}

// Modes used for match_houses
enum HC_SearchModes { HC_INVALID=0, HC_OWNER, HC_LANDLORD, HC_GUEST };
enum house_type { INVALID = 0, PRIVATE, PUBLIC, RENTAL, CLAN };

static const unsigned int MAX_HOUSE_GUESTS = 50;
static const int MAX_HOUSE_ITEMS = 50;

struct house
{
    // unique identifier of this house
    int id;
    // date this house was built
    time_t created;
    // type of ownership: personal, public, rental
    enum house_type type;
    // idnum of house owner's account
    int owner_id;
    // The slumlord that built the house.
    long landlord;
    // the rate per date of rent charged on this house
    int rental_rate;
    // The left over gold amount from selling off items to
    // cover unpaid rent.  This is never given to the char.
    long rent_overflow;

    // idnums of house's guests
    // The list of id's of characters allowed to enter this house
    GList *guests;
    // vnum house rooms
    // The list of room numbers that make up this house
    GList *rooms;

    // the repossession notices created when objects are sold
    // to cover rent cost.
    struct txt_block *repo_notes;

};

extern time_t last_house_collection;
extern int top_house_id;
extern GList *houses;

#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)
char* print_room_contents(struct creature *ch, struct room_data *real_house_room, bool showContents);
int recurs_obj_cost(struct obj_data *obj, bool mode, struct obj_data *top_o);
int recurs_obj_contents(struct obj_data *obj, struct obj_data *top_o);
bool can_enter_house(struct creature *ch, room_num room_idnum);
void load_houses(void);
void update_objects_housed_count(void);
struct house *find_house_by_idnum(int idnum);
struct house *find_house_by_owner(int idnum);
struct house *find_house_by_clan(int idnum);
struct house *find_house_by_room(room_num room_idnum);
void collect_housing_rent(void);
bool save_house(struct house *house);
void save_houses(void);
int repo_note_count(struct house *house);
void house_notify_repossession(struct house *house, struct creature *ch);
int room_rent_cost(struct house *house, struct room_data *room);
int count_objects_in_room(struct room_data *room);
void list_house_rooms(struct house *house, struct creature *ch, bool show_contents);

#endif
