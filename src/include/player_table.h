#ifndef _PLAYER_TABLE_H_
#define _PLAYER_TABLE_H_

#include <vector>
#include <utility>
#include <algorithm>
using namespace std;

struct Creature;

/**
 * An ID->Name mapping for Players.
 * Represented as a pair of sorted vectors of name/id pairs. One sorted by
 * name and the other by id.  Lookups are done via a binary search of the
 * appropriate vector.
**/
class PlayerTable 
{
    public:
        /** Creates a blank PlayerTable  **/
        PlayerTable();

        /** Returns true if the given id is present in the player table. **/
        bool exists( long id );
        /** Returns true if the given name is present in the player table. **/
        bool exists( const char* name );
        
        /** returns the char's name or NULL if not found. **/
        const char *getName( long id );
        /** returns chars id or 0 if not found **/
        long getID( const char *name ) const;
		// returns account id or 0 if not found
		long getAccountID(const char *name) const;
		long getAccountID(long id) const;

        /** returns chars id or 0 if not found **/
        long operator[](const char *name){ return getID(name); }
        /** Returns the char's name or NULL if not found. **/
        const char* operator[](long id) { return getName(id); }

        /** Retrieves the largest player id in the table **/
        long getTopIDNum();
		
		/** loads the named victim into the provided Creature **/
		bool loadPlayer( const char* name, Creature *victim ) const;

		/** loads the victim with the given id into the provided Creature **/
		bool loadPlayer( const long id, Creature *victim ) const;
	
		/** Returns the number of id->name mappings in this player table. **/
		size_t size() const;
};

extern PlayerTable playerIndex;

#endif // __PLAYER_TABLE_H__
