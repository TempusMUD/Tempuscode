#include <string.h>
#include "player_table.h"
#include "utils.h"
#include "db.h"

/* The global player index */
PlayerTable playerIndex;

/**
 *  Creates a blank PlayerTable
**/
PlayerTable::PlayerTable() 
{ 
}

long
PlayerTable::getTopIDNum()
{
	PGresult *res;
	long result;

	res = sql_query("select MAX(idnum) from players");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

    return result;
}

/** loads the named victim into the provided Creature **/
bool
PlayerTable::loadPlayer(const char* name, Creature *victim) const 
{
	long id = getID(name);
	return loadPlayer(id, victim);
}

/** loads the victim with the given id into the provided Creature **/
bool
PlayerTable::loadPlayer(const long id, Creature *victim) const 
{
	if(id <= 0) {
		return false;
	}
	return victim->loadFromXML(id);
}


/**
 * Returns true if and only if the given id is present in the player table.
**/
bool
PlayerTable::exists(long id) 
{
	PGresult *res;
	bool result;

	res = sql_query("select COUNT(*) from players where idnum=%ld", id);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = (atoi(PQgetvalue(res, 0, 0)) == 1);
	PQclear(res);

    return result;
}

/**
 * Returns true if and only if the given name is present in the player table.
**/
bool PlayerTable::exists(const char* name) 
{
	PGresult *res;
	bool result;

	res = sql_query("select COUNT(*) from players where name ilike '%s'",
		tmp_sqlescape(name));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = (atoi(PQgetvalue(res, 0, 0)) == 1);
	PQclear(res);

    return result;
}

/**
 * returns the char's name or NULL if not found.
**/
const char *
PlayerTable::getName(long id) 
{
	PGresult *res;
	char *result;

	res = sql_query("select name from players where idnum=%ld", id);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return NULL;
	if (PQntuples(res) == 1)
		result = tmp_strdup(PQgetvalue(res, 0, 0));
	else
		return NULL;
	PQclear(res);

    return result;
}

/**
 * returns chars id or 0 if not found
 *
**/
long
PlayerTable::getID(const char *name) const 
{
	PGresult *res;
	long result;

	res = sql_query("select idnum from players where name ilike '%s'",
		tmp_sqlescape(name));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

    return result;
}

long
PlayerTable::getAccountID(const char *name) const
{
	PGresult *res;
	long result;

	res = sql_query("select account from players where name ilike '%s'",
		tmp_sqlescape(name));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

    return result;
}

long
PlayerTable::getAccountID(long id) const
{
	PGresult *res;
	long result;

	res = sql_query("select account from players where idnum=%ld", id);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

    return result;
}

size_t
PlayerTable::size(void) const
{
	PGresult *res;
	long result;

	res = sql_query("select COUNT(*) from players");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

    return result;
}
