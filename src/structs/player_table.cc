#include <string.h>
#include "player_table.h"
#include "utils.h"

/* The global player index */
PlayerTable playerIndex;

/**
 *  Creates a blank PlayerTable
**/
PlayerTable::PlayerTable() 
: idTable(), nameTable(), top_id(0)
{ 
}

void
PlayerTable::clear(void)
{
	idTable.clear();
	nameTable.clear();
	top_id = 0;
}

int PlayerTable::getTopIDNum() {
    return top_id;
}

/** loads the named victim into the provided Creature **/
bool PlayerTable::loadPlayer( const char* name, Creature *victim ) const 
{
	long id = getID( name );
	return loadPlayer(id, victim);
}

/** loads the victim with the given id into the provided Creature **/
bool PlayerTable::loadPlayer( const long id, Creature *victim ) const 
{
	if( id <= 0 ) {
		return false;
	}
	return victim->loadFromXML( id );
}


/**
 * Returns true if and only if the given id is present in the player table.
**/
bool PlayerTable::exists( long id ) 
{
    return binary_search( idTable.begin(), idTable.end(), id );
}

/**
 * Returns true if and only if the given name is present in the player table.
**/
bool PlayerTable::exists( const char* name ) 
{
    return binary_search( nameTable.begin(), nameTable.end(), name );
}

/**
 * returns the char's name or NULL if not found.
**/
const char *PlayerTable::getName( long id ) 
{
    IDTable::iterator it;
    it = lower_bound( idTable.begin(), idTable.end(), id );
    if( it != idTable.end() && (*it) == id ) {
        return (*it).second;
    }
    return NULL;
}

/**
 * returns chars id or 0 if not found
 *
**/
long PlayerTable::getID( const char *name ) const 
{
    NameTable::const_iterator it;
    it = lower_bound( nameTable.begin(), nameTable.end(), name );
    if( it != nameTable.end() && (*it) == name ) {
        return (*it).second;
    }
    return 0;
}

/**
 * Adds the given player info to this PlayerTable.
 *
 * NOTE: name will be duplicated and stored without alteration.
 * @param sortTable if true, the player table will be sorted after insertion
**/
bool PlayerTable::add( long id, const char* name, bool sortTable  ) 
{
    if( exists(id) )
        return false;
    top_id = MAX(id, top_id);
    idTable.push_back(IDEntry(id,strdup(name)));
    nameTable.push_back(NameEntry(id,strdup(name)));
	if( sortTable )
		sort();
    return true;
}

/**
 * Removes the entry in this player table that corressponds to the
 * given id.
**/
bool PlayerTable::remove( long id ) 
{
    NameTable::iterator nit;
    IDTable::iterator iit;

    iit = lower_bound( idTable.begin(), idTable.end(), id );
    nit = lower_bound( nameTable.begin(), nameTable.end(), (*iit).second );

    if( nit != nameTable.end() && (*nit) == id 
    && iit != idTable.end() && (*iit) == id ) 
    {
        nameTable.erase(nit);
        idTable.erase(iit);
        top_id = idTable[ idTable.size() -1 ].getID();
        return true;
    }
    return false; 
}

/**
 * Removes the entry in this player table that corressponds to the
 * given name.
**/
bool PlayerTable::remove( const char* name ) 
{
    NameTable::iterator nit;
    IDTable::iterator iit;

    nit = lower_bound( nameTable.begin(), nameTable.end(), name );
    iit = lower_bound( idTable.begin(), idTable.end(), (*nit).second );

    if( nit != nameTable.end() && (*nit) == name 
    && iit != idTable.end() && (*iit) == name ) 
    {
        nameTable.erase(nit);
        idTable.erase(iit);
        return true;
    }
    return false; 
}

/**
 * Sorts the player table.
 * Calls sort on the two contained table halfs ( id and name ).
**/
void PlayerTable::sort() 
{
    idTable.sort();
    nameTable.sort();
}

// Sorts the name table
void NameTable::sort() 
{
    std::sort( begin(), end() );
}
// Sorts the id table
void IDTable::sort() 
{
    std::sort( begin(), end() );
}


/***               NAME ENTRY               ***/

// Constructor

NameEntry::NameEntry( long id, char* name ) 
: pair<char*, long>( name, id ) 
{
    //
}

// Copy Constructor
NameEntry::NameEntry( const NameEntry &n ) {
	*this = n;
}

// Destructor
NameEntry::~NameEntry() {
    free(first);
}

long NameEntry::getID() {
    return second;
}
const char* NameEntry::getName() {
    return first;
}

NameEntry &NameEntry::operator=(const NameEntry &e ) {
	if( this->first != NULL )
		free(this->first);
    this->first = strdup(e.first);
    this->second = e.second;
	return *this;
}
bool NameEntry::operator==(long id) const { 
    return second == id; 
}
bool NameEntry::operator==(const char *name) const { 
    return strcasecmp(first,name) == 0; 
}
bool NameEntry::operator==(NameEntry &e) const { 
    return first == e.first; 
}
bool NameEntry::operator!=(NameEntry &e) const { 
    return first != e.first; 
}
bool NameEntry::operator<(NameEntry &e) const { 
    return ( strcasecmp(first,e.first) < 0);
}
bool NameEntry::operator>(NameEntry &e) const { 
    return strcasecmp(first,e.first) > 0;
}
bool NameEntry::operator<(long id) const { 
    return second < id; 
}
bool NameEntry::operator>(long id) const { 
    return second > id; 
}
bool NameEntry::operator<(const char *name) const { 
    return strcasecmp(first,name) < 0; 
}
bool NameEntry::operator>(const char *name) const { 
    return strcasecmp(first,name) > 0; 
}




/***               ID ENTRY               ***/

//Constructor
IDEntry::IDEntry( long id, char* name) 
: pair< long, char*>( id, name )
{ }
// Copy constructor
IDEntry::IDEntry( const IDEntry &e ) {
    this->second = strdup(e.second);
    this->first = e.first;
}
// Destructor
IDEntry::~IDEntry() {
    free(second);
}


long IDEntry::getID() {
    return first;
}
const char* IDEntry::getName() {
    return second;
}
IDEntry& IDEntry::operator=(const IDEntry &e ) {
    this->first = e.first;
	if( this->second != NULL )
		free(this->second);
    this->second = strdup(e.second);
	return *this;
}
bool IDEntry::operator==(long id) const { 
    return first == id; 
}
bool IDEntry::operator==(const char *name) const { 
    return strcasecmp(second,name) == 0; 
}
bool IDEntry::operator==(IDEntry &e) const { 
    return first == e.first; 
}
bool IDEntry::operator!=(IDEntry &e) const { 
    return first != e.first; 
}
bool IDEntry::operator<(IDEntry &e) const { 
    return first < e.first; 
}
bool IDEntry::operator>(IDEntry &e) const { 
    return first > e.first; 
}
bool IDEntry::operator<(long id) const { 
    return first < id; 
}
bool IDEntry::operator>(long id) const { 
    return first > id; 
}
