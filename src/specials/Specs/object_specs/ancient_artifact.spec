#include "bomb.h"

SPECIAL(ancient_artifact)
{
	obj_data *obj = (obj_data *) me;

	if (spec_mode != SPECIAL_COMBAT)
		return 0;

	// Same algorithm as do_casting_objon uses
	if (number(0, MAX(2, LVL_GRIMP + 28 - GET_LEVEL(ch) - GET_INT(ch))))
		return 0;

	if ((number(0, 1) || GET_LEVEL(ch) >= LVL_AMBASSADOR)) {
		// mega-blast makes mob lose 10% of current hp
		strcpy(buf,
			"A bright blue beam erupts from $p with a screaming roar!");
		send_to_char(ch, CCCYN(ch, C_NRM));
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		send_to_char(ch, CCNRM(ch, C_NRM));
		act(buf, TRUE, ch, obj, 0, TO_ROOM);
		strcpy(buf, "$N screams silently as $E briefly fades from existence!");
		act(buf, FALSE, ch, obj, FIGHTING(ch), TO_CHAR);
		act(buf, TRUE, ch, obj, FIGHTING(ch), TO_ROOM);
		GET_HIT(FIGHTING(ch)) -= GET_HIT(FIGHTING(ch)) / 10;
	} else if (number(0, 99)) {
		strcpy(buf, "$p rumbles disquietingly in your hands.");
		send_to_char(ch, CCCYN(ch, C_NRM));
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		send_to_char(ch, CCNRM(ch, C_NRM));
		strcpy(buf, "$p rumbles disquietingly in $n's hands.");
		act(buf, TRUE, ch, obj, 0, TO_ROOM);
	} else {
		// self-destruct
		bomb_radius_list *rad_elem, *next_elem;
		room_data *room = ch->in_room;

		add_bomb_room(room, -1, 35);
		sort_rooms();

		for (rad_elem = bomb_rooms; rad_elem; rad_elem = next_elem) {
			next_elem = rad_elem->next;

			bomb_damage_room(obj->short_description,
				BOMB_ARTIFACT,
				35,
				rad_elem->room,
				find_first_step(rad_elem->room, room, GOD_TRACK),
				rad_elem->power);
			free(rad_elem);
			bomb_rooms = next_elem;
		}

	}

	return 0;
}
