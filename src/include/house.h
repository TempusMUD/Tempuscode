#ifndef _HOUSE_H_
#define _HOUSE_H_

#include <vector>
#include "tmpstr.h"

static inline char* 
get_house_file_path( int id )
{
	return tmp_sprintf( "players/housing/%d/%04d.dat", (id % 10), id );
}

// Modes used for match_houses
enum HC_SearchModes { INVALID=0, HC_OWNER, HC_LANDLORD, HC_GUEST };

class House 
{
	public:
		static const unsigned int MAX_GUESTS = 50;
		static const int MAX_ITEMS = 50;

		enum Type { INVALID = 0, PRIVATE, PUBLIC, RENTAL };

		const char* getTypeName();
		const char* getTypeShortName();
		Type getTypeFromName( const char* name );

	private:
		// unique identifier of this house
		int id;
		// vnum house rooms
		typedef std::vector<room_num> RoomList;
		RoomList rooms;
		// date this house was built
		time_t created;
		// type of ownership: personal, public, rental
		Type type;
		// idnum of house owner's account
		int ownerID;
		// idnums of house's guests
		typedef std::vector<long> GuestList;
		GuestList guests;
		// The slumlord that built the house.
		long landlord;
		// the rate per date of rent charged on this house
		int rentalRate;
		bool loadRoom( xmlNodePtr node );
	public:

		House( int idnum, int owner, room_num first );
		House( const House &h );
		House();

		// saves this house to the given file
		bool save();
		// initializes this house from the given file
		bool load( const char* filename );
		
		// retrieves this house's unique identifier
		int getID() const { return id; }
		// retrieves the unique id of the account that owns this house
		int getOwnerID() const { return ownerID; }
		// returns true if the given creature belongs to the owner account
		bool isOwner( Creature *ch ) const ;
		// sets the owner account id to the given id
		void setOwnerID( int id ) { ownerID = id; }
		// retrieves the player id of the builder and maintainer of this house
		long getLandlord() const { return landlord; }
		// sets the builder and maintainer of this house
		void setLandlord( long lord ) { landlord = lord; }
		// retrieves the date this house was created
		time_t getCreated() const { return created; }
		// retrieves the enumerated type of this house
		Type getType() const { return type; }
		// sets the enumerated type of this house
		void setType( Type type ) { this->type = type; }

		// retrieves the number of characters that are guests of this house
		unsigned int getGuestCount() const { return guests.size(); }
		// retrieves the idnum of the character at the given index
		long getGuest( int index ) const { return guests[index]; }
		// adds the given guest id to this houses guest list
		bool addGuest( long guest );
		// Removes the given guest id from this houses guest list
		bool removeGuest( long guest );
		// returns true if the given creature is a guest of this house.
		bool isGuest( Creature *c ) const; // todo use sorted/binary search
		// returns true if the given player id is a guest of this house.
		bool isGuest( long idnum ) const;// todo use sorted/binary search

		// retrieves the number of rooms saved as part of this house
		unsigned int getRoomCount() const { return rooms.size(); }
		// retrieves the room_num of the room at the given index
		room_num getRoom( int index ) const { return rooms[index]; }
		// adds the given room to this houses room list
		bool addRoom( room_num room );
		// removes the given room from this houses room list
		bool removeRoom( room_num room );
		// returns true if the given room is part of this house
		bool hasRoom( room_num room ) const;
		// returns true if the given room is part of this house
		bool hasRoom( room_data *room ) const;

		// retrieves the rent charged per day on this house
		int getRentalRate() const { return rentalRate; }
		// sets the rent charged per day on this house
		void setRentalRate( int rate ) { rentalRate = rate; }

		// calculates the daily rent cost for this house
		int calcRentCost() const;
		// counts the objects contained in this house
		int calcObjectCount() const;
        // counts the objects contained in the given room
        int calcObjectCount( room_data* room ) const;

		void listRooms( Creature *ch, bool showContents = false );
		void listGuests( Creature *ch );
		void display( Creature *ch );
		
		House& operator=( const House &h );
		// Comparators for sorting
		bool operator==( const House &h ) const { return id == h.id; }
		bool operator!=( const House &h ) const { return id != h.id; }
		bool operator< ( const House &h ) const { return id <  h.id; }
		bool operator> ( const House &h ) const { return id >  h.id; }
};


class HouseControl : private std::vector<House*> 
{
	private:
		// the last time rent was paid.
		time_t lastCollection;
		int topId;
		class HouseComparator { 
			public:
				bool operator()( House *a, House *b ) {
					return a->getID() < b->getID();
				}
		};
		
	public:
		HouseControl() : lastCollection(0), topId(0) { }
		// saves the house control file and all house contents
		void save();
		// loads all house contents
		void load();
		// charges rent on each house
		void collectRent();
		// Updates the prototype object's "number in houses" counts
		void countObjects();
		// returns true if the given creature can enter the given house room
		bool canEnter( Creature *c, room_num room );
		// returns true if the given creature can edit the house
		// that contains the given room
		bool canEdit( Creature *c, room_data *room );
		// returns true if the given creature can edit the given house
		bool canEdit( Creature *c, House *house );
		// retrieves the house that contains the given room
		House* findHouseByRoom( room_num room );
		// retrieves the house with the given unique id
		House* findHouseById( int id );
		// returns the number of houses in this house control
		unsigned int getHouseCount() const ;
		// returns the house at the given index
		House* getHouse( int index );
		// attemps to create a house owned by owner with the given room range
		bool createHouse( int owner, room_num firstRoom, room_num lastRoom );
		// destroys the house with the given id
		bool destroyHouse( int id );
		// destroys the given house
		bool destroyHouse( House *house );

		void displayHouses( list<House*> houses, Creature *ch );
};
// The global housing project.
extern HouseControl Housing;

// a kludge hold over
static inline bool
House_can_enter( Creature *ch, room_num room )
{
	return Housing.canEnter( ch, room );
}


#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)
char* print_room_contents(Creature *ch, room_data *real_house_room, bool showContents = false);
int recurs_obj_cost(struct obj_data *obj, bool mode, struct obj_data *top_o);
int recurs_obj_contents(struct obj_data *obj, struct obj_data *top_o);
#endif
