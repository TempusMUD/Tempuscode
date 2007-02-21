ACMD(do_give);

struct mob_mugger_data {
	int idnum;					// idnum of player on shit list
	int vnum;					// vnum of object desired
	int deaths;					// For tracking whether the player has died
	byte timer;					// how long has the mob been waiting
};

SPECIAL(mugger)
{
	// Areas that muggers can see
	const int mug_eq[] = {
		WEAR_WIELD, WEAR_WIELD_2, WEAR_NECK_1, WEAR_NECK_2, WEAR_BODY,
		WEAR_HEAD, WEAR_ARMS, WEAR_HANDS, WEAR_WRIST_R, WEAR_WRIST_L,
		WEAR_HOLD, WEAR_EYES, WEAR_BACK, WEAR_BELT, WEAR_FACE,
		WEAR_SHIELD, WEAR_ABOUT 
	};
	const int MUG_MAX = 17;

	Creature *self = (Creature *)me;
	mob_mugger_data *mug;
	int idx;
	Creature *vict, *found_vict;
	obj_data *obj;
	CreatureList::iterator it;

	if (spec_mode != SPECIAL_TICK && spec_mode != SPECIAL_CMD)
		return 0;
	
	mug = (mob_mugger_data *)self->mob_specials.func_data;

	// Handle the give command
	if (spec_mode == SPECIAL_CMD && CMD_IS("give") && mug) {
		do_give(ch, argument, cmd, 0, 0);
		// Check to see if they gave me what I want
		for (obj = self->carrying;obj;obj = obj->next_content) {
			if (GET_OBJ_VNUM(obj) == mug->vnum) {
				if (GET_IDNUM(ch) == mug->idnum) {
                    vict = self->findRandomCombat();
					if (self->numCombatants() && !IS_NPC(vict)) {
						do_say(self, tmp_sprintf("Ha!  Let this be a lesson to you, %s!", GET_DISGUISED_NAME(self, vict)), 0, 0, 0);
                        self->removeAllCombat();
					} else {
						do_say(self, tmp_sprintf("Good move, %s!", GET_DISGUISED_NAME(self, ch)), 0, 0, 0);
					}
				} else
					do_say(self, "Ok, that will work.", 0, 0, 0);
				free(self->mob_specials.func_data);
				self->mob_specials.func_data = NULL;
				return 1;
			}
		}
		return 1;
	}

	if (spec_mode == SPECIAL_CMD)
		return 0;
	
	if (self->numCombatants() || self->isHunting())
		return 0;

	// We're not mugging anyone, so look for a new victim
	if (!mug) {
		found_vict = NULL;
		for (it = ch->in_room->people.begin(); it != ch->in_room->people.end();++it) {
			vict = *it;
			if (check_infiltrate(vict, ch))
				continue;
			if (IS_NPC(vict)
				&& can_see_creature(ch, vict)
				&& cityguard == GET_MOB_SPEC(vict)) {
				act("$n glances warily at $N", true, ch, 0, vict, TO_ROOM);
				return 1;
			}
			if (vict == self
					|| !can_see_creature(self, vict)
					|| IS_NPC(vict)
					|| !AWAKE(vict)
					|| GET_LEVEL(vict) < 7
					|| GET_LEVEL(vict) > (GET_LEVEL(self) + 5)
					|| GET_LEVEL(vict) + 15 < GET_LEVEL(self))
				continue;
			if (!found_vict
					|| GET_LEVEL(vict) < GET_LEVEL(found_vict)
					|| GET_HIT(vict) < GET_HIT(found_vict)
					|| !random_fractional_3())
				found_vict = vict;
		}

		if (!found_vict)
			return 0;
		
		vict = found_vict;
		act("You examine $N.", true, ch, 0, vict, TO_CHAR);
		act("$n examines you.", true, ch, 0, vict, TO_VICT);
		act("$n examines $N.", true, ch, 0, vict, TO_NOTVICT);

		for (idx = 0;idx < MUG_MAX;idx++) {
			obj = GET_EQ(vict, mug_eq[idx]);
			if (!obj
					|| IS_OBJ_STAT(obj, ITEM_NODROP)
					|| IS_OBJ_STAT2(obj, ITEM2_NOREMOVE)
					|| !can_see_object(self, obj)
					|| !obj->shared->proto
					|| obj->name != obj->shared->proto->name)
				continue;

            perform_say_to(self, vict,
                           tmp_sprintf("I see you are using %s.  I believe I could appreciate it much more than you.  Give it to me now.", obj->name));
			CREATE(mug, mob_mugger_data, 1);
			mug->idnum = GET_IDNUM(vict);
			mug->vnum = GET_OBJ_VNUM(obj);
			mug->timer = 0;
			mug->deaths = GET_PC_DEATHS(vict);
			self->mob_specials.func_data = mug;
			return 1;
		}
		return 0;
	}

	obj = ch->in_room->contents;
	while (obj) {
		// See if the wimps dropped it
		if (GET_OBJ_VNUM(obj) == mug->vnum
				&& can_see_object(self, obj)
				&& obj->shared->proto
				&& obj->name == obj->shared->proto->name) {

			if (obj->in_obj) {
				act(tmp_sprintf("You snicker and loot $p from %s", obj->in_obj->name),
					false, self, obj, 0, TO_CHAR);
				act(tmp_sprintf("$n snickers and loots $p from %s", obj->in_obj->name),
					false, self, obj, 0, TO_ROOM);
				obj_from_obj(obj);
			} else {
				act("You snicker and pick up $p.", false, self, obj, 0, TO_CHAR);
				act("$n snickers and picks up $p.", false, self, obj, 0, TO_ROOM);
				obj_from_room(obj);
			}
			obj_to_char(obj, self);
			free(mug);
			self->mob_specials.func_data = NULL;
			return 1;
		}

		// Search through corpses
		if (obj->contains)
			obj = obj->contains;
		else if (!obj->next_content && obj->in_obj)
			obj = obj->in_obj->next;
		else
			obj = obj->next_content;
	}


	// A mugging is in progress
	vict = NULL;
	for (it = ch->in_room->people.begin(); it != ch->in_room->people.end();it++)
		if (!IS_NPC(*it)
				&& can_see_creature(self, *it)
				&& GET_IDNUM(*it) == mug->idnum)
			vict = *it;
	if (!vict) {
		vict = get_char_in_world_by_idnum(mug->idnum);
		if (!vict
				|| GET_PC_DEATHS(vict) > mug->deaths
				|| !can_see_creature(self, vict)) {
			do_say(self, "Curses, foiled again!", 0, 0, 0);
			free(mug);
			self->mob_specials.func_data = NULL;
			return 1;
		}

		if (self->isHunting() != vict) {
			do_gen_comm(ch, tmp_sprintf("You're asking for it, %s!", GET_NAME(vict)), 0, SCMD_SHOUT, 0);
			self->startHunting(vict);
		}
		return 1;
	}

	obj = real_object_proto(mug->vnum);
	if (!obj) {
		errlog("Mugger's desired object not found in database");
		free(mug);
		self->mob_specials.func_data = NULL;
	}

	switch (mug->timer) {
	case 1:
		perform_say_to(ch, vict, tmp_sprintf("I'm warning you.  Give %s to me now or face the consequences!", obj->name));
		// Fall through
	case 2:
	case 3:
		perform_say_to(ch, vict, tmp_sprintf("You've got %d seconds to give it to me!", (5 - mug->timer) * 4));
		break;
	case 4:
		perform_say_to(ch, vict, "You've got 4 seconds to give it to me, OR ELSE!");
		break;
	case 5:
        perform_say_to(ch, vict, "You asked for it!");
		mug->timer = 3;
		best_attack(self, vict);
		break;

		mug->timer++;
		return 1;
	}

	return 0;
}
