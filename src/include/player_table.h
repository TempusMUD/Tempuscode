#ifndef __PLAYER_TABLE_H__
#define __PLAYER_TABLE_H__

#include <vector>
#include <pair.h>
#include <algorithm>
using namespace std;

/** A class representing the "name" half of a player table entry. **/
class NameEntry;
/** A class representing the "player id" half of a player table entry. **/
class IDEntry;



/**
 * A Class representing the "name" half of a player table.
 * Contains NameEntry's.
**/
class NameTable : public vector<NameEntry> {
    public:
        NameTable() : vector<NameEntry>() { }
        void sort();
    private:
};

/**
 * A Class representing the "ID" half of a player table.
 * Contains NameEntry's.
**/
class IDTable : public vector<IDEntry> {
    public:
        IDTable() : vector<IDEntry>() { }
        void sort();
    private:
};



/**
 * An ID->Name mapping for Players.
 * Represented as a pair of sorted vectors of name/id pairs. One sorted by
 * name and the other by id.  Lookups are done via a binary search of the
 * appropriate vector.
**/
class PlayerTable 
{
    private:
        IDTable idTable;
        NameTable nameTable;
        int top_id;
    public:
        /** Creates a blank PlayerTable  **/
        PlayerTable();

        /** Returns true if and only if the given id is present in the player table. **/
        bool exists( long id );
        /** Returns true if and only if the given name is present in the player table. **/
        bool exists( const char* name );
        
        /** returns the char's name or NULL if not found. **/
        const char *getName( long id );
        /** returns chars id or 0 if not found **/
        long getID( const char *name );

        /** returns chars id or 0 if not found **/
        long operator[](const char *name){ return getID(name); }
        /** Returns the char's name or NULL if not found. **/
        const char* operator[](long id) { return getName(id); }

        /** Adds the given player info to this PlayerTable.  **/
        bool add( long id, const char* name );
        /** Removes the entry in this player table that corressponds to the given id. **/
        bool remove( long id );
        /** Removes the entry in this player table that corressponds to the given name. **/
        bool remove( const char* name );
        
        /** Sorts the player table. **/
        void sort();
        /** Retrieves the largest player id in the table **/
        int getTopIDNum();
};




/**
 * A class representing the "name" half of a player table entry.
 * It is nothing more than a char* / playerid pair
 *
 * Comparison operators have been overloaded for convinience with
 * the lexographical value of the "char*" being compared.
 *
 *  Equivilance to both char* and long have been provided for
 *  convinience.
**/
class NameEntry : public pair<char*, long> {
    public:
        NameEntry( long id, char* name );
        NameEntry( const NameEntry &n );
        ~NameEntry();

        long getID();
        const char* getName();
        bool operator==(long id) const;
        bool operator==( const char *name) const;
        bool operator==(NameEntry &e) const;
        bool operator!=(NameEntry &e) const;
        bool operator<(NameEntry &e) const;
        bool operator>(NameEntry &e) const;
        bool operator<(long id) const;
        bool operator>(long id) const;
        bool operator<(const char* name) const;
        bool operator>(const char* name) const;
};

inline bool operator==(long id, const NameEntry &e) { 
    return e == id; 
}
inline bool operator<(long id, const NameEntry &e) { 
    return e > id; 
}
inline bool operator>(long id, const NameEntry &e) { 
    return e < id; 
}
inline bool operator==(const char* name, const NameEntry &e) { 
    return e == name;
}
inline bool operator<(const char* name, const NameEntry &e) { 
    return e > name;
}
inline bool operator>(const char* name, const NameEntry &e) { 
    return e < name;
}



/**
 * A class representing the "player id" half of a player table entry.
 * It is nothing more than a char* / playerid pair
 *
 * Comparison operators have been overloaded for convinience with
 * the value if the player id being compared.
 *
 * Equivilance to both char* and long have been provided for
 * convinience.
**/
class IDEntry : public pair<long, char*> {
    public:
        IDEntry( long id, char* name);
        IDEntry( const IDEntry &e );
        // Destructory does NOT free name.
        ~IDEntry();
        
        long getID();
        const char* getName();
        bool operator==(IDEntry &e) const;
        bool operator!=(IDEntry &e) const;
        bool operator<(IDEntry &e) const;
        bool operator>(IDEntry &e) const;
        bool operator==( const char *name) const;
        bool operator==(long id) const;
        bool operator<(long id) const;
        bool operator>(long id) const;
};

inline bool operator==(long id, const IDEntry &e) { 
    return e == id; 
}
inline bool operator<(long id, const IDEntry &e) { 
    return e > id; 
}
inline bool operator>(long id, const IDEntry &e) { 
    return e < id; 
}
inline bool operator==(const char* name, const IDEntry &e) { 
    return e == name;
}

#endif // __PLAYER_TABLE_H__
