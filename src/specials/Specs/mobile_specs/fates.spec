//
// File: fates.spec					 -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//


struct room_list_struct {
	struct room_data *room;
	struct room_list_struct *next;
};

int fate_timers[3] = {0,0,0};
SPECIAL(fate)
{
	struct Creature *fate = (struct Creature *) me;
	struct room_data *dest = NULL;
	dynamic_text_file *dyntext = NULL;
	char dyn_name[64];
	char s[1024];
	struct room_list_struct *roomlist = NULL, *cur_room_list_item=NULL;
	char *roomlist_buf=NULL; 
	char *roomlist_buf_top = NULL;
	struct room_data *temp_room = NULL;
	int num_rooms = 0, the_room = 0;
	int which_fate;

    if( spec_mode != SPECIAL_TICK ) return 0;  
	// Don't want the zone goin to sleep and trapping her.
	if (fate->in_room) {
		fate->in_room->zone->idle_time = 0;
	}

  	if (cmd)
		return 0;
	if (FIGHTING(fate))
		return 0;
	if(!fate->in_room)
		return 0;

	// If there is a player in the room, don't do anything.
	if(player_in_room(fate->in_room))
		return 0;


	// Who is she?
	switch(GET_MOB_VNUM(fate)){
		case FATE_VNUM_LOW:
			strcpy(dyn_name,"fatelow");
			which_fate = 0;
			break;
		case FATE_VNUM_MID:
			strcpy(dyn_name,"fatemid");
			which_fate = 1;
			break;
		case FATE_VNUM_HIGH:
			strcpy(dyn_name,"fatehigh");
			which_fate = 2;
			break;
		default:
			return 0;
	}

	// Is it time to leave?
	if(fate_timers[which_fate] > 0) { // Not time to leave yet.
		fate_timers[which_fate] -= 10; // Specials are called every 10 seconds.
		return 1;
	}

	// It's time to leave.
	// Start the timer for the next jump.
	fate_timers[which_fate] = 60 * dice(30,6); // 25 mins to 2.5 hours.

	// find the dyntext of the rooms we need.
	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
		if (!strcasecmp(dyn_name, dyntext->filename))
			break;
	}
	if(!dyntext) {
		slog("SYSERR: Fate unable to access dyntext doc.(%s)\r\n", dyn_name);
		return 1;
	}
	// If the file is null, return
	if(!dyntext->buffer) {
		do_say(fate,"Hmm... Where should I go to next?",0,0);
		return 1;
	}

	// Grab the rooms out of the buffer
	roomlist_buf_top = new char[strlen(dyntext->buffer) + 1];
	roomlist_buf = roomlist_buf_top;
	strcpy(roomlist_buf,dyntext->buffer);
	
	// Copy over the buf, ignoring all non numbers
	while(*roomlist_buf) {
		if(!isdigit(*roomlist_buf))
			*roomlist_buf = ' ';
		roomlist_buf++;
	}
	roomlist_buf = roomlist_buf_top;
	// Build list of pointers to rooms
	skip_spaces(&roomlist_buf);
	while(*roomlist_buf) {
		roomlist_buf = one_argument(roomlist_buf,s);
		temp_room = real_room(atoi(s));
		if(temp_room && temp_room != fate->in_room
			&& !player_in_room(temp_room)) {
			cur_room_list_item = new room_list_struct;
			cur_room_list_item->room = temp_room;
			cur_room_list_item->next = roomlist;
			roomlist = cur_room_list_item;
			num_rooms++;
		}
		skip_spaces(&roomlist_buf);
	}
	delete [] roomlist_buf_top;
	// Didnt find any rooms. :P
	if(!roomlist) {
		do_say(fate,"Hmm... Where should I go to next?",0,0);
		return 1;
	}

	// Find room we want.
	the_room = number(0,num_rooms);

	cur_room_list_item = roomlist;
	for(int i = 1;i < the_room;i++) {
		cur_room_list_item = cur_room_list_item->next;
	}
	// got the room
	dest = cur_room_list_item->room;
	// delete the list
	while(roomlist) {
		cur_room_list_item = roomlist->next;
		delete roomlist;
		roomlist = cur_room_list_item;
	}
	
	act("$n disappears into a green mist.", FALSE, fate, 0, 0, TO_ROOM);
	char_from_room(fate,false);
	char_to_room(fate, dest,false);
	fate->in_room->zone->idle_time = 0;
	act("$n appears out of a green mist.", FALSE, fate, 0, 0, TO_ROOM);
	return 1;
}



