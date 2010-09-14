#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include "utils.h"
#include "db.h"

long
top_player_idnum(void)
{
	PGresult *res;
	long result;

	res = sql_query("select MAX(idnum) from players");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));

    return result;
}

/**
 * Returns true if and only if the given id is present in the player table.
**/
bool
player_idnum_exists(long id)
{
	PGresult *res;
	bool result;

	res = sql_query("select COUNT(*) from players where idnum=%ld", id);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = (atoi(PQgetvalue(res, 0, 0)) == 1);

    return result;
}

/**
 * Returns true if and only if the given name is present in the player table.
**/
bool
player_name_exists(const char* name)
{
	PGresult *res;
	int result;

	res = sql_query("select COUNT(*) from players where lower(name)='%s'",
		tmp_sqlescape(tmp_tolower(name)));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atoi(PQgetvalue(res, 0, 0));

    if (result > 1)
        errlog("Found %d characters named '%s'", result, tmp_tolower(name));

    return (result == 1);
}

/**
 * returns the char's name or NULL if not found.
**/
const char *
player_name_by_idnum(long id)
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

    return result;
}

/**
 * returns chars id or 0 if not found
 *
**/
long
player_idnum_by_name(const char *name)
{
	PGresult *res;
	long result;

	res = sql_query("select idnum from players where lower(name)='%s'",
		tmp_sqlescape(tmp_tolower(name)));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	if (PQntuples(res) == 1)
		result = atol(PQgetvalue(res, 0, 0));
	else
		result = 0;

    return result;
}

long
player_account_by_name(const char *name)
{
	PGresult *res;
	long result;

	res = sql_query("select account from players where lower(name)='%s'",
		tmp_sqlescape(tmp_tolower(name)));
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	if (PQntuples(res) == 1)
		result = atol(PQgetvalue(res, 0, 0));
	else
		result = 0;

    return result;
}

long
player_account_by_idnum(long id)
{
	PGresult *res;
	long result;

	res = sql_query("select account from players where idnum=%ld", id);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	if (PQntuples(res) == 1)
		result = atol(PQgetvalue(res, 0, 0));
	else
		result = 0;

    return result;
}

size_t
player_count(void)
{
	PGresult *res;
	long result;

	res = sql_query("select COUNT(*) from players");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		return 0;
	result = atol(PQgetvalue(res, 0, 0));

    return result;
}
