#define KEY1_VNUM	83000
#define KEY2_VNUM	83001
#define KEY3_VNUM	83002

ACMD(do_put);

SPECIAL(laby_altar)
{
	obj_data *self = (obj_data *)me;
	obj_data *cur_obj;
	bool prev_keys[3], cur_keys[3];

	if (spec_mode != SPECIAL_CMD && !CMD_IS("put"))
		return 0;

	cur_keys[0] = prev_keys[0] = false;
	cur_keys[1] = prev_keys[1] = false;
	cur_keys[2] = prev_keys[2] = false;
	for (cur_obj = self->contains;cur_obj;cur_obj = cur_obj->next_content) {
		if (GET_OBJ_VNUM(cur_obj) == KEY1_VNUM)
			prev_keys[0] = true;
		if (GET_OBJ_VNUM(cur_obj) == KEY2_VNUM)
			prev_keys[1] = true;
		if (GET_OBJ_VNUM(cur_obj) == KEY3_VNUM)
			prev_keys[2] = true;
	}
	
	do_put(ch, argument, cmd, 0, 0);
	
	for (cur_obj = self->contains;cur_obj;cur_obj = cur_obj->next_content) {
		if (GET_OBJ_VNUM(cur_obj) == KEY1_VNUM)
			cur_keys[0] = true;
		if (GET_OBJ_VNUM(cur_obj) == KEY2_VNUM)
			cur_keys[1] = true;
		if (GET_OBJ_VNUM(cur_obj) == KEY3_VNUM)
			cur_keys[2] = true;
	}

	// If they add a key, they need some correct feedback.
	if ((!prev_keys[0] && cur_keys[0]) ||
			(!prev_keys[1] && cur_keys[1]) ||
			(!prev_keys[2] && cur_keys[2])) {

		if (!(cur_keys[0] && cur_keys[1] && cur_keys[2])) {
			send_to_char(ch, "A bright light flashes from the altar.\r\n");
			return 1;
		}
	}

	// Remove all contents - they shouldn't be putting just anything in there
	while (self->contains)
		extract_obj(self->contains);

	send_to_char(ch,
		"As you place it within the altar, a bright light sears across your vision.\r\n"
		"You feel your body being annihilated and restored by the powers of Fate,\r\n"
		"until at last, you fall into the blissful darkness...\r\n");
	set_desc_state(CXN_REMORT_AFTERLIFE, ch->desc);
	return 1;
}
