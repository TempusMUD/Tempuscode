class hunter_data {
	public:
		hunter_data(int hunt_id, int vict) : idnum(hunt_id), vict_id(vict) {}
		bool operator==(int id) { return idnum == id; }

		int idnum;			// idnum of the hunter
		int vict_id;			// idnum of the hunted
};

vector <hunter_data> hunter_list;

void
load_bounty_data(void)
{
	PGresult *res;
	int idx, count;

	res = sql_query("select idnum, hunted from bounty_hunters");
	count = PQntuples(res);
	if (count < 1)
		return;

	for (idx = 0;idx < count;idx++)
		hunter_list.push_back(hunter_data(
			atoi(PQgetvalue(res, idx, 0)),
			atoi(PQgetvalue(res, idx, 1))));
}

int
get_bounty_amount(int idnum)
{
	Creature vict(true);
	
	if (!vict.loadFromXML(idnum)) {
		slog("SYSERR: Could not load victim in place_bounty");
		return 0;
	}

	if (!IS_CRIMINAL(&vict))
		return 0;
	
	return GET_REPUTATION(&vict) * 20000;
}

// Awards any bounties due to the killer - returns true if it was a
// legitimate bounty kill
bool
award_bounty(Creature *killer, Creature *vict)
{
	vector <hunter_data>::iterator hunter;
	struct follow_type *f;
	int count, amt, amt_left;

	if (IS_NPC(killer) || IS_NPC(vict))
		return false;

	// first find and remove the hunter record - if they aren't a registered
	// hunter, they don't get the bounty.. 
	hunter = find(hunter_list.begin(), hunter_list.end(), GET_IDNUM(killer));
	// no hunter record for this killer
	if (hunter == hunter_list.end())
		return false;
	// wrong victim for this hunter
	if (hunter->vict_id != GET_IDNUM(vict))
		return false;

	// erase record for this hunter
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
			vict->account->set_past_bank(0);
		} else {
			vict->account->set_past_bank(GET_PAST_BANK(vict) - amt_left);
			amt_left = 0;
		}
	}
	if (amt_left) {
		if (GET_FUTURE_BANK(vict) <= amt_left) {
			amt_left -= GET_FUTURE_BANK(vict);
			vict->account->set_future_bank(0);
		} else {
			vict->account->set_future_bank(GET_FUTURE_BANK(vict) - amt_left);
			amt_left = 0;
		}
	}

	// Don't want to make it easy to get rich...
	amt -= amt_left;
	if (!amt)
		return true;
	
	// Award them the amount
	if (!AFF_FLAGGED(killer, AFF_GROUP)) {
		send_to_char(killer, "You have been paid %d gold coins for killing %s!\r\n",
			amt, GET_NAME(vict));
		GET_GOLD(killer) += amt;
		mudlog(LVL_IMMORT, CMP, true, "%s paid %d gold for killing %s",
			GET_NAME(killer), amt, GET_NAME(vict));
	} else {
		if (killer->master)
			killer = killer->master;
		// count the people in the group
		count = 1;
		for (f = killer->followers;f;f = f->next)
			if (IS_AFFECTED(f->follower, AFF_GROUP) && !IS_NPC(f->follower))
				count++;

		amt /= count;

		mudlog(LVL_IMMORT, CMP, true, "%s's group paid %d gold each for killing %s",
			GET_NAME(killer), amt, GET_NAME(vict));

		send_to_char(killer, "You have been paid %d gold coins for your part in killing %s!\r\n",
			amt, GET_NAME(vict));
		GET_GOLD(killer) += amt;

		for (f = killer->followers;f;f = f->next)
			if (IS_AFFECTED(f->follower, AFF_GROUP) && !IS_NPC(f->follower)) {
				send_to_char(f->follower, "You have been paid %d gold coins for your part in killing %s!\r\n",
					amt, GET_NAME(vict));
				GET_GOLD(f->follower) += amt;
			}
	}

	return true;
}

int
register_bounty(Creature *self, Creature *ch, char *argument)
{
	const char USAGE[] = "Usage: register bounty <player>";
	vector <hunter_data>::iterator hunter;
	char *str, *vict_name;
	Creature vict(true);

	str = tmp_getword(&argument);
	vict_name = tmp_getword(&argument);
	if (!str || !is_abbrev(str, "bounty") || !vict_name) {
		send_to_char(ch, "%s\r\n", USAGE);
		return 1;
	}

	do_say(ch, tmp_sprintf("I'd like to register as a bounty hunter for %s.",
		tmp_capitalize(tmp_tolower(vict_name))), 0, 0, 0);
	

	if (IS_CRIMINAL(ch)) {
		do_say(self, "You're a criminal.  Scum like you don't get to be bounty hunters.", 0, 0, 0);
		return 1;
	}

	if (!playerIndex.exists(vict_name)) {
		do_say(self, "Never heard of that person.", 0, 0, 0);
		return 1;
	}

	if (!vict.loadFromXML(playerIndex.getID(vict_name))) {
		slog("SYSERR: Could not load victim in place_bounty");
		do_say(self, "Hmmm.  There seems to be a problem with that person.", 0, 0, 0);
		return 1;
	}

	if (GET_IDNUM(ch) == GET_IDNUM(&vict)) {
		do_say(self, "You can't be your own hunter, freak!", 0, 0, 0);
		return 1;
	}

	if (!IS_CRIMINAL(&vict)) {
		do_say(self, "That person isn't a criminal, jerk!", 0, 0, 0);
		return 1;
	}

	do_say(self, tmp_sprintf("Very well.  You are now registered as a bounty hunter for %s.", GET_NAME(&vict)), 0, 0, 0);

	hunter = find(hunter_list.begin(), hunter_list.end(), GET_IDNUM(ch));
	if (hunter == hunter_list.end()) {
		hunter_list.push_back(hunter_data(GET_IDNUM(ch), GET_IDNUM(&vict)));
		sql_exec("insert into bounty_hunters (idnum, victim) values (%ld, %ld)",
			GET_IDNUM(ch), GET_IDNUM(&vict));
	} else {
		hunter->vict_id = GET_IDNUM(&vict);
		sql_exec("update bounty_hunters set victim=%ld where idnum=%ld",
			GET_IDNUM(&vict), GET_IDNUM(ch));
	}
		
	return 1;
}

SPECIAL(bounty_clerk)
{
	if (spec_mode != SPECIAL_CMD)
		return 0;
	
	if (CMD_IS("register"))
		return register_bounty((Creature *)me, ch, argument);
	
	return 0;
}
