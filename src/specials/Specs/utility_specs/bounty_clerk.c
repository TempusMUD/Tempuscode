struct hunt_data {
    int idnum;                  // idnum of the hunter
    int vict_id;                // idnum of the hunted
};

GList *hunt_list;

struct hunt_data *
make_hunt_data(int hunt_id, int vict)
{
    struct hunt_data *data;

    CREATE(data, struct hunt_data, 1);
    data->idnum = hunt_id;
    data->vict_id = vict;

    return data;
}

void
load_bounty_data(void)
{
    PGresult *res;
    int idx, count;

    res = sql_query("select idnum, victim from bounty_hunters");
    count = PQntuples(res);
    if (count < 1)
        return;

    for (idx = 0; idx < count; idx++) {
        hunt_list =
            g_list_prepend(hunt_list, make_hunt_data(atoi(PQgetvalue(res, idx,
                        0)), atoi(PQgetvalue(res, idx, 1))));
    }
}

void
remove_bounties(int char_id)
{
    GList *next;

    for (GList * it = hunt_list; it; it = next) {
        struct hunt_data *hunt = it->data;
        next = it->next;

        if (hunt->vict_id == char_id || hunt->idnum == char_id)
            hunt_list = g_list_remove_link(hunt_list, it);
    }
}

int
get_bounty_amount(int idnum)
{
    struct creature *vict;

    vict = load_player_from_xml(idnum);
    if (!vict) {
        errlog("Could not load victim in place_bounty");
        return 0;
    }

    if (!IS_CRIMINAL(vict)) {
        free_creature(vict);
        return 0;
    }

    int amount = GET_REPUTATION(vict) * 20000;
    free_creature(vict);

    return amount;
}

struct hunt_data *
hunt_data_by_idnum(int hunter_id, int vict_id)
{
    for (GList * it = hunt_list; it; it = it->next) {
        struct hunt_data *hunt = it->data;

        if ((!hunter_id || hunt->idnum == hunter_id)
            && (!vict_id || hunt->vict_id == vict_id))
            return hunt;
    }

    return NULL;
}

bool
is_bountied(struct creature * hunter, struct creature * vict)
{
    return (hunt_data_by_idnum(GET_IDNUM(hunter), GET_IDNUM(vict)) != NULL);
}

int
get_hunted_id(int hunter_id)
{
    struct hunt_data *hunt;

    hunt = hunt_data_by_idnum(hunter_id, 0);
    if (!hunt)
        return 0;

    return hunt->vict_id;
}

// Awards any bounties due to the killer - returns true if it was a
// legitimate bounty kill
bool
award_bounty(struct creature * killer, struct creature * vict)
{
    struct follow_type *f;
    int count, amt, amt_left;
    struct hunt_data *hunt;

    if (IS_NPC(killer) || IS_NPC(vict))
        return false;

    hunt = hunt_data_by_idnum(GET_IDNUM(killer), GET_IDNUM(vict));
    if (!hunt)
        return false;

    hunt_list = g_list_remove(hunt_list, hunt);

    // erase record for this hunt
    sql_exec("delete from bounty_hunters where idnum=%ld", GET_IDNUM(killer));

    // Now find out how much money they get for killing the bastard
    amt = amt_left = get_bounty_amount(GET_IDNUM(vict));

    // Take the money out of the victim's account... in this order:
    // gold, cash, past bank, future bank
    if (amt_left) {
        if (GET_GOLD(vict) <= amt_left) {
            amt_left -= GET_GOLD(vict);
            GET_GOLD(vict) = 0;
        } else {
            GET_GOLD(vict) -= amt_left;
            amt_left = 0;
        }
    }
    if (amt_left) {
        if (GET_CASH(vict) <= amt_left) {
            amt_left -= GET_CASH(vict);
            GET_CASH(vict) = 0;
        } else {
            GET_CASH(vict) -= amt_left;
            amt_left = 0;
        }
    }
    if (amt_left) {
        if (GET_PAST_BANK(vict) <= amt_left) {
            amt_left -= GET_PAST_BANK(vict);
            account_set_past_bank(vict->account, 0);
        } else {
            account_set_past_bank(vict->account,
                GET_PAST_BANK(vict) - amt_left);
            amt_left = 0;
        }
    }
    if (amt_left) {
        if (GET_FUTURE_BANK(vict) <= amt_left) {
            amt_left -= GET_FUTURE_BANK(vict);
            account_set_future_bank(vict->account, 0);
        } else {
            account_set_future_bank(vict->account,
                GET_FUTURE_BANK(vict) - amt_left);
            amt_left = 0;
        }
    }
    // Don't want to make it easy to get rich...
    amt -= amt_left;
    if (!amt)
        return true;

    // Award them the amount
    if (!AFF_FLAGGED(killer, AFF_GROUP)) {
        send_to_char(killer,
            "You have been paid %d gold coins for killing %s!\r\n", amt,
            GET_NAME(vict));
        GET_GOLD(killer) += amt;
        mudlog(LVL_IMMORT, CMP, true, "%s paid %d gold for killing %s",
            GET_NAME(killer), amt, GET_NAME(vict));
    } else {
        if (killer->master)
            killer = killer->master;
        // count the people in the group
        count = 1;
        for (f = killer->followers; f; f = f->next)
            if (AFF_FLAGGED(f->follower, AFF_GROUP) && !IS_NPC(f->follower))
                count++;

        amt /= count;

        mudlog(LVL_IMMORT, CMP, true,
            "%s's group paid %d gold each for killing %s", GET_NAME(killer),
            amt, GET_NAME(vict));

        send_to_char(killer,
            "You have been paid %d gold coins for your part in killing %s!\r\n",
            amt, GET_NAME(vict));
        GET_GOLD(killer) += amt;

        for (f = killer->followers; f; f = f->next)
            if (AFF_FLAGGED(f->follower, AFF_GROUP) && !IS_NPC(f->follower)) {
                send_to_char(f->follower,
                    "You have been paid %d gold coins for your part in killing %s!\r\n",
                    amt, GET_NAME(vict));
                GET_GOLD(f->follower) += amt;
            }
    }

    return true;
}

int
register_bounty(struct creature *self, struct creature *ch, char *argument)
{
    const char USAGE[] = "Usage: register bounty <player>";
    char *str, *vict_name;
    struct creature *vict;

    str = tmp_getword(&argument);
    vict_name = tmp_getword(&argument);
    if (!str || !is_abbrev(str, "bounty") || !vict_name) {
        send_to_char(ch, "%s\r\n", USAGE);
        return 1;
    }

    perform_say(ch, "inquire",
        tmp_sprintf("I'd like to register as a bounty hunter for %s.",
            tmp_capitalize(tmp_tolower(vict_name))));

    if (IS_CRIMINAL(ch)) {
        perform_say(self, "say",
            "You're a criminal.  Scum like you don't get to be bounty hunters.");
        return 1;
    }

    if (!player_name_exists(vict_name)) {
        perform_say(self, "say", "Never heard of that person.");
        return 1;
    }

    vict = load_player_from_xml(player_idnum_by_name(vict_name));
    if (!vict) {
        errlog("Could not load victim in place_bounty");
        perform_say(self, "say",
            "Hmmm.  There seems to be a problem with that person.");
        return 1;
    }

    if (GET_IDNUM(ch) == GET_IDNUM(vict)) {
        perform_say(self, "say", "You can't be your own hunter, freak!");
        return 1;
    }

    if (!IS_CRIMINAL(vict)) {
        perform_say(self, "say", "That person isn't a criminal, jerk!");
        return 1;
    }

    perform_say(self, "state",
        tmp_sprintf
        ("Very well.  You are now registered as a bounty hunter for %s.",
            GET_NAME(vict)));


    struct hunt_data *hunt =
        hunt_data_by_idnum(GET_IDNUM(ch), GET_IDNUM(vict));
    if (!hunt) {
        hunt = make_hunt_data(GET_IDNUM(ch), GET_IDNUM(vict));
        hunt_list = g_list_prepend(hunt_list, hunt);
        sql_exec
            ("insert into bounty_hunters (idnum, victim) values (%ld, %ld)",
            GET_IDNUM(ch), GET_IDNUM(vict));
    } else {
        hunt->vict_id = GET_IDNUM(vict);
        sql_exec("update bounty_hunters set victim=%ld where idnum=%ld",
            GET_IDNUM(vict), GET_IDNUM(ch));
    }

    return 1;
}

SPECIAL(bounty_clerk)
{
    if (spec_mode != SPECIAL_CMD)
        return 0;

    if (CMD_IS("register"))
        return register_bounty((struct creature *)me, ch, argument);

    return 0;
}
