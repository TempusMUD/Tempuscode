//
// File: tarrasque.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define T_SLEEP   0
#define T_ACTIVE  1
#define T_RETURN  2
#define T_ERROR   3

#define EXIT_RM_MODRIAN 24878
#define EXIT_RM_ELECTRO -1
#define LAIR_RM       24918
#define BELLY_RM      24919

#define JRM_BASE_OUT  24903
#define JRM_TOP_OUT   24900
#define JRM_BASE_IN   24914
#define JRM_TOP_IN    24916

#define T_SLEEP_LEN   100
#define T_ACTIVE_LEN  200

struct room_data *belly_rm = NULL;

room_num inner_tunnel[] = { 24914, 24915, 24916, 24917 };
room_num outer_tunnel[] = { 24903, 24902, 24901, 24900, 24878 };

void
tarrasque_jump(struct Creature *tarr, int jump_mode)
{

	struct room_data *up_room = NULL;
	int i;
	act("$n takes a flying leap upwards into the chasm!",
		FALSE, tarr, 0, 0, TO_ROOM);

	for (i = 1; i <= (jump_mode == T_ACTIVE ? 4 : 3); i++) {
		if (!(up_room = real_room(jump_mode == T_ACTIVE ?
					outer_tunnel[i] : inner_tunnel[i]))) {
			break;
		}
		char_from_room(tarr, false);
		char_to_room(tarr, up_room, false);

		if ((jump_mode == T_ACTIVE && i == 4) ||
			(jump_mode == T_RETURN && i == 3))
			act("$n comes flying up from the chasm and lands with an earthshaking footfall!", FALSE, tarr, 0, 0, TO_ROOM);
		else
			act("$n comes flying past from below, and disappears above you!",
				FALSE, tarr, 0, 0, TO_ROOM);
	}
}

void
tarrasque_swallow(Creature *tarr, Creature *vict)
{
	act("You suddenly pounce and devour $N whole!", 0, tarr, 0, vict, TO_CHAR);
	act("$n suddenly pounces and devours $N whole!", 0, tarr, 0, vict, TO_NOTVICT);
	send_to_char(vict, "The tarrasque suddenly leaps into the the air,\r\n"); 
	send_to_char(vict, "The last thing you hear is the bloody crunch of your body.\r\n");
	death_cry(vict);
	char_from_room(vict);
	char_to_room(vict, belly_rm);
	act("The chewed body of $n flies in from the mouth.", 1, vict, 0, 0, TO_ROOM);
	die(vict, tarr, TYPE_SWALLOW, false);
}

void
tarrasque_digest(Creature *tarr)
{
	obj_data *obj, *next_obj;
	bool pooped = false;

	if (!belly_rm) {
		slog("Tarrasque can't find his belly in digest()!");
		return;
	}
	if (!belly_rm->contents)
		return;

	for (obj = belly_rm->contents;obj;obj = next_obj) {
		next_obj = obj->next_content;
		if (GET_OBJ_TYPE(obj) != ITEM_TRASH)
			obj = damage_eq(NULL, obj, 20, SPELL_ACIDITY);

		// If we have an object at this point, it needs to be excreted.  But..
		// don't poop more than once at a time, and the tarr doesn't excrete
		// while fighting, sleeping, or in its lair
		if (obj &&
				!pooped &&
				!FIGHTING(tarr) &&
				tarr->getPosition() != POS_FIGHTING &&
				tarr->getPosition() >= POS_STANDING &&
				tarr->in_room->number != LAIR_RM) {
			pooped = true;
			act("$p is pooped out of the belly.", 0, NULL, obj, NULL, TO_ROOM);
			obj_from_room(obj);
			obj_to_room(obj, tarr->in_room);
			act("You strain a bit, and $p plops out.", 0, tarr, obj, NULL, TO_CHAR);
			act("$n strains a bit, and $p plops out.", 0, tarr, obj, NULL, TO_ROOM);
		}
	}
}

int
tarrasque_die(Creature *tarr)
{
	obj_data *obj, *next_obj;
	// When the tarrasque dies, everything in its belly must be put in its
	// corpse.  Acidified, of course.
	for (obj = belly_rm->contents;obj;obj = next_obj) {
		next_obj = obj->next_content;
		obj_from_room(obj);
		obj_to_char(obj, tarr);
	}

	// We wnat everything else to be normal
	return 0;
}

int
tarrasque_fight(struct Creature *tarr)
{
	struct Creature *vict = NULL, *vict2 = NULL;
	CreatureList::iterator it;
	bool is_dead;

	if (!FIGHTING(tarr)) {
		slog("SYSERR: FIGHTING(tarr) == NULL in tarrasque_fight!!");
		return 0;
	}

	// Select two lucky individuals who will be our random target dummies
	// this fighting pulse
	it = tarr->in_room->people.begin();
	for (; it != tarr->in_room->people.end(); ++it)
		if ((*it) != tarr &&
				(*it) != FIGHTING(tarr) &&
				tarr == FIGHTING((*it)) &&
				CAN_SEE(tarr, (*it)) &&
				!PRF_FLAGGED((*it), PRF_NOHASSLE)) {
			vict = (*it);
			break;
		}

	if (vict) {
		it = tarr->in_room->people.begin();
		for (; it != tarr->in_room->people.end(); ++it)
			if ((*it) != tarr && (*it) != FIGHTING(tarr) &&
				tarr == FIGHTING((*it)) &&
				CAN_SEE(tarr, (*it)) && !PRF_FLAGGED((*it), PRF_NOHASSLE)) {
				vict2 = (*it);
				break;
			}
	}

	// In a tarrasquian charge, the person whom the tarrasque is fighting
	// is hit first, then two other random people (if any).  As an added
	// bonus, the tarrasque will then be attacking the last person hit
	if (!number(1, 10)) {
		act("$n charges forward!!", FALSE, tarr, 0, 0, TO_ROOM);
		WAIT_STATE(tarr, 2 RL_SEC);

		is_dead = damage(tarr, FIGHTING(tarr),
				(GET_DEX(FIGHTING(tarr)) < number(5, 28)) ?
				(dice(30, 20) + 300) : 0, TYPE_GORE_HORNS, WEAR_BODY);
		if (is_dead) {
			tarr->setFighting(vict);
			vict = vict2;
			vict2 = NULL;
		}

		if (vict && GET_DEX(vict) < number(5, 25)) {
			is_dead = damage(tarr, vict,
				(GET_DEX(vict) < number(5, 28)) ?
				(dice(20, 40) + 300) : 0, TYPE_TRAMPLING, WEAR_BODY);
			if (is_dead)
				vict = NULL;
			else
				tarr->setFighting(vict);
		}

		if (vict2 && GET_DEX(vict2) < number(5, 25)) {
			is_dead = damage(tarr, vict2,
				(GET_DEX(vict2) < number(5, 28)) ?
				(dice(20, 40) + 300) : 0, TYPE_TRAMPLING, WEAR_BODY);
			if (!is_dead)
				vict2 = NULL;
			else
				tarr->setFighting(vict2);
		}
		return 1;
	}

	// Tarrasquian tail lashing involves knocking down and damaging the
	// intrepid adventurer the tarr is fighting, plus the two random
	// individuals.
	if (!number(1, 10)) {
		act("$n lashes out with $s tail!!", FALSE, tarr, 0, 0, TO_ROOM);
		WAIT_STATE(tarr, 3 RL_SEC);
		is_dead = damage(tarr, FIGHTING(tarr),
					GET_DEX(FIGHTING(tarr)) < number(5, 28) ?
					(dice(20, 20) + 100) : 0,
					TYPE_TAIL_LASH, WEAR_LEGS);
		if (!is_dead &&
				FIGHTING(tarr) &&
				(FIGHTING(tarr))->getPosition() == POS_FIGHTING &&
				GET_DEX(FIGHTING(tarr)) < number(10, 18) &&
				!PRF_FLAGGED(FIGHTING(tarr), PRF_NOHASSLE))
			(FIGHTING(tarr))->setPosition(POS_RESTING);
		if (is_dead) {
			tarr->setFighting(vict);
			vict = vict2;
			vict2 = NULL;
		}

		if (vict) {
			is_dead = damage(tarr, vict,
					GET_DEX(vict) < number(5, 28) ?
					(dice(20, 20) + 100) : 0,
					TYPE_TAIL_LASH, WEAR_LEGS);
			if (!is_dead &&
					vict->getPosition() == POS_FIGHTING
					&& GET_DEX(vict) < number(10, 18)
					&& !PRF_FLAGGED(vict, PRF_NOHASSLE))
				vict->setPosition(POS_RESTING);
			if (is_dead)
				vict = NULL;
		}

		if (vict2) {
			is_dead = damage(tarr, vict2,
					GET_DEX(vict2) < number(5, 28) ?
					(dice(20, 20) + 100) : 0,
					TYPE_TAIL_LASH, WEAR_LEGS);
			if (!is_dead &&
					vict2->getPosition() == POS_FIGHTING
					&& GET_DEX(vict2) < number(10, 18)
					&& !PRF_FLAGGED(vict2, PRF_NOHASSLE))
				vict2->setPosition(POS_RESTING);
			if (is_dead)
				vict2 = NULL;
		}
		return 1;
	}

	if (number(0, 20))
		return 0;

	// There's a chance, however small, that the tarrasque is going to swallow
	// the character whole, no matter how powerful.  If they make their saving
	// throw, they just get hurt.  The player being fought is most likely to
	// get swallowed.
	if (FIGHTING(tarr) || number(1,4)) {
		if (GET_DEX(FIGHTING(tarr)) < number(5, 23) &&
			!mag_savingthrow(FIGHTING(tarr), 50, SAVING_ROD)) {
			tarrasque_swallow(tarr, FIGHTING(tarr));
		} else {
			damage(tarr,
				FIGHTING(tarr), GET_DEX(FIGHTING(tarr)) < number(5, 28) ?
				(dice(40, 20) + 200) : 0, TYPE_BITE, WEAR_BODY);
		}
	} else if (vict) {
		if (GET_DEX(vict) < number(5, 23) &&
			!mag_savingthrow(vict, 50, SAVING_ROD)) {
			tarrasque_swallow(tarr, vict);
		} else {
			damage(tarr, vict, GET_DEX(vict) < number(5, 28) ?
				(dice(40, 20) + 200) : 0, TYPE_BITE, WEAR_BODY);
		}
	} else if (vict2) {
		if (GET_DEX(vict2) < number(5, 23) &&
			!mag_savingthrow(vict2, 50, SAVING_ROD)) {
			tarrasque_swallow(tarr, vict2);
		} else {
			damage(tarr, vict2, GET_DEX(vict2) < number(5, 28) ?
				(dice(40, 20) + 200) : 0, TYPE_BITE, WEAR_BODY);
		}
	}
	return 1;
}

SPECIAL(tarrasque)
{

	static int mode = 0, tframe = TIME_MODRIAN, checked = FALSE;
	static unsigned int timer = 0;
	struct Creature *tarr = (struct Creature *)me;
	struct room_data *rm = NULL;

	if (tarr->mob_specials.shared->vnum != 24800) {
		tarr->mob_specials.shared->func = NULL;
		REMOVE_BIT(MOB_FLAGS(tarr), MOB_SPEC);
		slog("SYSERR: There is only one true tarrasque");
		return 0;
	}

	if (spec_mode == SPECIAL_CMD) {
		if (CMD_IS("status") && GET_LEVEL(ch) >= LVL_IMMORT) {
			send_to_char(ch,
				"Tarrasque status: mode (%d), timer (%d), tframe (%d)\r\n",
				mode, timer, tframe);
			return 1;
		}
		if (CMD_IS("reload") && GET_LEVEL(ch) > LVL_DEMI) {
			mode = T_SLEEP;
			timer = 0;
			if ((rm = real_room(LAIR_RM))) {
				char_from_room(tarr, false);
				char_to_room(tarr, rm, false);
			}
			send_to_char(ch, "Tarrasque reset.\r\n");
			return 1;
		}
		if (mode == T_SLEEP && !AWAKE(tarr))
			timer += T_SLEEP_LEN / 10;
		return 0;
	}
	
	if (spec_mode == SPECIAL_DEATH)
		return tarrasque_die(tarr);

	if (spec_mode != SPECIAL_TICK)
		return 0;

	if (!checked) {
		checked = TRUE;
		if (!(belly_rm = real_room(BELLY_RM))) {
			slog("SYSERR: Tarrasque can't find his belly!");
			tarr->mob_specials.shared->func = NULL;
			REMOVE_BIT(MOB_FLAGS(tarr), MOB_SPEC);
			return 1;
		}
	}

	tarrasque_digest(tarr);
	timer++;

	if (FIGHTING(tarr))
		return tarrasque_fight(tarr);

	switch (mode) {
	case T_SLEEP:
		if (timer > T_SLEEP_LEN) {
			tarr->setPosition(POS_STANDING);
			tframe = TIME_MODRIAN;
			if (!add_path_to_mob(tarr, "tarr_exit_mod")) {
				slog("SYSERR: error assigning tarr_exit_mod path to tarrasque.");
				mode = T_ERROR;
				return 1;
			}

			mode = T_ACTIVE;
			timer = 0;
		} else if (tarr->in_room->number == LAIR_RM && AWAKE(tarr)) {
			act("$n goes to sleep.", FALSE, tarr, 0, 0, TO_ROOM);
			tarr->setPosition(POS_SLEEPING);
		}
		break;

	case T_ACTIVE:
		if (timer > T_ACTIVE_LEN) {
			mode = T_RETURN;
			timer = 0;

			if (!add_path_to_mob(tarr, "tarr_return_mod")) {
				slog("SYSERR: error assigning tarr_return_mod path to tarrasque.");
				mode = T_ERROR;
				return 1;
			}
			return 1;
		}

		if (tarr->in_room->number == outer_tunnel[0]) {
			tarrasque_jump(tarr, T_ACTIVE);
			return 1;
		}

		if (tarr->in_room->people.size() >= 1) {
			CreatureList::iterator it = tarr->in_room->people.begin();
			for (; it != tarr->in_room->people.end(); ++it) {
				if (!IS_NPC((*it)) && GET_LEVEL((*it)) < 10) {
					if ((*it)->getPosition() < POS_STANDING)
						(*it)->setPosition(POS_STANDING);
					act("You are overcome with terror at the sight of $N!",
						FALSE, (*it), 0, tarr, TO_CHAR);
					do_flee((*it), "", 0, 0);
				}
			}
		}

		break;

	case T_RETURN:{

			if (tarr->in_room->number == LAIR_RM) {
				mode = T_SLEEP;
				tarr->setPosition(POS_SLEEPING);
				act("$n lies down and falls asleep.", FALSE, tarr, 0, 0,
					TO_ROOM);
				timer = 0;
				return 1;
			}

			if (tarr->in_room->number == inner_tunnel[0]) {
				tarrasque_jump(tarr, T_RETURN);
				return 1;
			}
			if (tarr->in_room->people.size() >= 1) {
				CreatureList::iterator it = tarr->in_room->people.begin();
				for (; it != tarr->in_room->people.end(); ++it) {
					if (!IS_NPC((*it)) && GET_LEVEL((*it)) < 10) {
						if ((*it)->getPosition() < POS_STANDING)
							(*it)->setPosition(POS_STANDING);
						act("You are overcome with terror at the sight of $N!",
							FALSE, (*it), 0, tarr, TO_CHAR);
						do_flee((*it), "", 0, 0);
					}
				}
			}

			break;
		}
	default:
		break;
	}
	return 0;
}
