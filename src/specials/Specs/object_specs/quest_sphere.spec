/*
	Rename
	Quad
	LP
	Pracs
	!break
	oedit
*/

static void
quest_weapon_enchant(Creature *ch, obj_data *obj, int lvl)
{
	int i;
	for (i = MAX_OBJ_AFFECT - 1; i >= 0; i--) {
		if (obj->affected[i].location == APPLY_HITROLL ||
				obj->affected[i].location == APPLY_DAMROLL) {
			obj->affected[i].location = APPLY_NONE;
			obj->affected[i].modifier = 0;
		} else if (i < MAX_OBJ_AFFECT - 2 && obj->affected[i].location) {
			obj->affected[i + 2].location = obj->affected[i].location;
			obj->affected[i + 2].modifier = obj->affected[i].modifier;
		}
	}

	obj->affected[0].location = APPLY_HITROLL;
	obj->affected[0].modifier = MAX(2, number(2, 4)) +
		(lvl >= 50) + (lvl >= 56) + (lvl >= 60) + (lvl >= 67);

	obj->affected[1].location = APPLY_DAMROLL;
	obj->affected[1].modifier = MAX(2, number(2, 4)) +
		(lvl >= 50) + (lvl >= 56) + (lvl >= 60) + (lvl >= 67);

	if (IS_GOOD(ch)) {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
		act("$p glows a bright blue.", FALSE, ch, obj, 0, TO_CHAR);
	} else if (IS_EVIL(ch)) {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
		act("$p glows a bright red.", FALSE, ch, obj, 0, TO_CHAR);
	} else {
		act("$p glows a bright yellow.", FALSE, ch, obj, 0, TO_CHAR);
	}

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC | ITEM_GLOW);
}

static void
quest_armor_enchant(Creature *ch, obj_data *obj, int lvl)
{
	int i;

	for (i = 0; i < MAX_OBJ_AFFECT - 1; i++) {
		if (obj->affected[i].location == APPLY_AC ||
			obj->affected[i].location == APPLY_SAVING_BREATH ||
			obj->affected[i].location == APPLY_SAVING_SPELL ||
			obj->affected[i].location == APPLY_SAVING_PARA) {
			obj->affected[i].location = APPLY_NONE;
			obj->affected[i].modifier = 0;
		}
	}

	obj->affected[0].location = APPLY_AC;
	obj->affected[0].modifier = -(lvl >> 3) - 5;

	obj->affected[1].location = APPLY_SAVING_PARA;
	obj->affected[1].modifier = -(4 + (lvl >= 53));

	obj->affected[2].location = APPLY_SAVING_BREATH;
	obj->affected[2].modifier = -(4 + (lvl >= 53));

	obj->affected[3].location = APPLY_SAVING_SPELL;
	obj->affected[3].modifier = -(4 + (lvl >= 53));

	if (IS_GOOD(ch)) {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
		act("$p glows a bright blue.", FALSE, ch, obj, 0, TO_CHAR);
	} else if (IS_EVIL(ch)) {
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
		act("$p glows a bright red.", FALSE, ch, obj, 0, TO_CHAR);
	} else {
		act("$p glows a bright yellow.", FALSE, ch, obj, 0, TO_CHAR);
	}

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC | ITEM_GLOW);
}

bool
quest_sphere_carrier_bad(Creature *carrier)
{
	if (GET_LEVEL(carrier) > LVL_IMMORT)
		return false;
	if (IS_NPC(carrier) && GET_MOB_SPEC(carrier) == vendor)
		return false;
	return true;
}

SPECIAL(quest_sphere)
{
	obj_data *self = (obj_data *)me;
	obj_data *targ_obj = NULL;
	char *line, *key, *param, *targ_str;
	bool quad = false, nobreak = false, need_targ = false;
	int enchant_lvl = 0, lp = 0;

	if (spec_mode == SPECIAL_TICK) {
		// The only place spheres can exist indefinitely is in the hands
		// of an immortal or a vendor
		if (self->worn_by && quest_sphere_carrier_bad(self->worn_by))
			GET_OBJ_VAL(self, 0)--;
		else if (self->carried_by && quest_sphere_carrier_bad(self->carried_by))
			GET_OBJ_VAL(self, 0)--;
		else if (self->in_obj || self->in_room)
			GET_OBJ_VAL(self, 0)--;

		if (!GET_OBJ_VAL(self, 0)) {
			if (self->worn_by) {
				act("$p dissolves into fine sand, which slips through your fingers...",
					true, self->worn_by, self, 0, TO_CHAR);
				act("$p dissolves into fine sand in $n's hands.",
					true, self->worn_by, self, 0, TO_ROOM);
			} else if (self->carried_by) {
				act("$p dissolves into fine sand and blows away on the floor...",
					true, self->carried_by, self, 0, TO_CHAR);
				act("$p dissolves into fine sand in $n's hands.",
					true, self->carried_by, self, 0, TO_ROOM);
			} else if (self->in_room && self->in_room->people.size()) {
				send_to_room(tmp_capitalize(tmp_sprintf("%s dissolves into fine sand and is blown away...\r\n",
					self->name)), self->in_room);
			} else {
				// in_obj is the only case left.  it just silently
				// disappears in this case
			}
			extract_obj(self);
		}
		return true;
	}

	if (spec_mode != SPECIAL_CMD)
		return false;
	
	if (!CMD_IS("use") || GET_EQ(ch, WEAR_HOLD) != self)
		return false;

	// Check to make sure they want to use the sphere
	targ_str = tmp_getword(&argument);
	if (!isname(targ_str, self->aliases))
		return false;

	targ_str = tmp_getword(&argument);
	if (*targ_str) {
		targ_obj = get_obj_in_list_all(ch, targ_str, ch->carrying);
		if (!targ_obj) {
			send_to_char(ch, "You don't seem to have any %ss.\r\n", targ_str);
			return true;
		}
	}

	if (!GET_OBJ_PARAM(self))
		return false;

	// We get to do our affect now
	param = GET_OBJ_PARAM(self);
	while ((line = tmp_getline(&param)) != NULL) {
		key = tmp_getword(&line);
		if (!strcmp(key, "quad"))
			quad = true;
		else if (!strcmp(key, "nobreak")) {
			nobreak = true;
			need_targ = true;
		} else if (!strcmp(key, "lp"))
			lp = (*line) ? atoi(line):2;
		else if (!strcmp(key, "enchant")) {
			enchant_lvl = (*line) ? atoi(line):51;
		need_targ = true;
		} else {
			mudlog(LVL_IMMORT, CMP, false,
				"Invalid directive in obj vnum #%d", GET_OBJ_VNUM(self));
			send_to_char(ch, "This is broken.  Please inform an imm.\r\n");
			return true;
		}
	}

	if (need_targ && !targ_obj) {
		send_to_char(ch, "You have to use %s on something!\r\n",
			self->name);
		return true;
	}

	if (enchant_lvl) {
		// This is where we hate ourselves
		if (IS_IMPLANT(targ_obj) ||
				IS_OBJ_STAT(targ_obj, ITEM_MAGIC) ||
				IS_OBJ_STAT(targ_obj, ITEM_MAGIC_NODISPEL) ||
				IS_OBJ_STAT(targ_obj, ITEM_BLESS) ||
				IS_OBJ_STAT(targ_obj, ITEM_DAMNED) ||
				(GET_OBJ_TYPE(targ_obj) != ITEM_WEAPON &&
					GET_OBJ_TYPE(targ_obj) != ITEM_ARMOR)) {
			send_to_char(ch, "You can't enchant that!\r\n");
			return true;
		}
		if (GET_OBJ_TYPE(targ_obj) == ITEM_WEAPON)
			quest_weapon_enchant(ch, targ_obj, enchant_lvl);
		else if (GET_OBJ_TYPE(targ_obj) == ITEM_ARMOR)
			quest_armor_enchant(ch, targ_obj, enchant_lvl);
		else {
			mudlog(LVL_IMMORT, CMP, true, "Can't happen at %s:%d",
				__FILE__, __LINE__);
			send_to_char(ch, "Something broke.  Notify an immortal.\r\n");
			return true;
		}
	}

	if (quad)
		call_magic(ch, ch, NULL, SPELL_QUAD_DAMAGE, LVL_GRIMP, CAST_SPELL);

	if (nobreak) {
		act("$p becomes immune to breaking!", 1, ch, targ_obj, 0, TO_CHAR);
		GET_OBJ_MAX_DAM(targ_obj) = -1;
		GET_OBJ_DAM(targ_obj) = -1;
	}
	if (lp) {
		send_to_char(ch, "You gain %d lifepoints!\r\n", lp);
		GET_LIFE_POINTS(ch) += lp;
	}

	act("$p disappears from your hands in a puff of smoke!",
		1, ch, self, 0, TO_CHAR);
	act("$p disappears from $n's hands in a puff of smoke!",
		0, ch, self, 0, TO_ROOM);
	if (targ_obj) {
		targ_str = tmp_sprintf("%s qsphere-%d %s", targ_obj->aliases,
			GET_OBJ_COST(self), GET_NAME(ch));
		free(targ_obj->aliases);
		targ_obj->aliases = str_dup(targ_str);
		mudlog(GET_LEVEL(ch), CMP, true,
			"%s has used %s on %s", GET_NAME(ch), self->name,
			targ_obj->name);
	} else
		mudlog(GET_LEVEL(ch), CMP, true,
			"%s has used %s", GET_NAME(ch), self->name);
	extract_obj(self);

	return true;
}
