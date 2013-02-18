SPECIAL(mage_teleporter)
{
    struct creature *self = (struct creature *)me;
    struct obj_data *vstone;
    char *arg;
    int cost;
    struct room_data *dest_room;

    if (spec_mode != SPECIAL_CMD)
        return 0;

    if (!CMD_IS("buy") && !CMD_IS("offer"))
        return 0;

    // Make sure they follow the format
    if (!is_abbrev(tmp_getword(&argument), "teleport")) {
        perform_say(self, "say",
            tmp_sprintf
            ("Type %s teleport <location> to get a quick trip to that location.",
                cmd_info[cmd].command));
        return 1;
    }
    // Now find a vstone in the room with that alias
    arg = tmp_getword(&argument);
    if (!*arg) {
        perform_say(self, "say", "You want to go where?");
        return 1;
    }
    // The vstones must be notake and have an owner value of 0
    // This prevents activation of player-made vstones
    vstone = self->in_room->contents;
    while (vstone && !(IS_OBJ_TYPE(vstone, ITEM_VSTONE)
            && isname(arg, vstone->aliases)
            && !(GET_OBJ_WEAR(vstone) & ITEM_WEAR_TAKE)
            && GET_OBJ_VAL(vstone, 1) == 0))
        vstone = vstone->next_content;
    if (!vstone) {
        perform_say(self, "say", "We don't have a v-stone to that location!");
        return 1;
    }

    cost = vstone->shared->cost * GET_LEVEL(ch);
    if (IS_MAGE(ch))
        cost /= 4;
    cost = adjusted_price(ch, self, cost);
    if (GET_GOLD(ch) < cost || CMD_IS("offer")) {
        if (can_see_creature(self, ch))
            perform_say_to(self, ch,
                tmp_sprintf
                ("It will cost you %d coins to be transported to %s", cost,
                    tmp_capitalize(fname(vstone->aliases))));
        else
            perform_say(ch, "say",
                tmp_sprintf
                ("It will cost you %d coins to be transported to %s", cost,
                    tmp_capitalize(fname(vstone->aliases))));
        return 1;
    }

    dest_room = real_room(GET_OBJ_VAL(vstone, 0));
    if (!dest_room) {
        perform_say(self, "say",
            "Hmmm.  That place doesn't seem to exist anymore.");
        return 1;
    }

    perform_say(self, "say", tmp_sprintf("Very well, %s.", GET_NAME(ch)));
    act("You stares at $N and utters, 'horosafh'.", true, self, NULL, ch,
        TO_CHAR);
    act("$n stares at you and utters, 'horosafh'.", true, self, NULL, ch,
        TO_VICT);
    act("$n stares at $N and utters, 'horosafh'.", true, self, NULL, ch,
        TO_NOTVICT);
    char_from_room(ch, true);
    char_to_room(ch, dest_room, true);
    return 1;
}
