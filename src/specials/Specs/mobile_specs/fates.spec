//
// File: fates.spec					 -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//
#define FATE_VNUM_LOW 1205
#define FATE_VNUM_MID 1206
#define FATE_VNUM_HIGH 1207

#define FATE_TEST 1

struct room_list_struct {
	struct room_data *room;
	struct room_list_struct *next;
};

// From act.wizard.cc

SPECIAL(fate)
{
	static int timers[3] = {0,0,0};
	struct char_data *fate = (struct char_data *) me;
	struct char_data *c = NULL;
	struct room_data *dest = NULL;
	dynamic_text_file *dyntext = NULL;
	char dyn_name[64];
	char s[1024];
	struct room_list_struct *roomlist = NULL, *cur_room_list_item=NULL;
	char *roomlist_buf=NULL, roomlist_buf_top[MAX_STRING_LENGTH];
	struct room_data *temp_room = NULL;
	int num_rooms = 0, the_room = 0;
	int which_fate;
  
  	if (cmd)
		return 0;
	if (FIGHTING(fate))
		return 0;
	if(!fate->in_room)
		return 0;

	// Who is she?
	if(GET_MOB_VNUM(fate) == FATE_VNUM_LOW) {
		strcpy(dyn_name,"fatelow");
		which_fate = 0;
	} else if(GET_MOB_VNUM(fate) == FATE_VNUM_MID) {
		strcpy(dyn_name,"fatemid");
		which_fate = 1;
	} else if(GET_MOB_VNUM(fate) == FATE_VNUM_HIGH) {
		strcpy(dyn_name,"fatehigh");
		which_fate = 2;
	} else {
		return 0;
	}

	// If there is a player in the room, don't do anything.
	for(c = fate->in_room->people;c;c = c->next_in_room) {
		if(!IS_NPC(c) && GET_LEVEL(c) < LVL_AMBASSADOR)
			return 0;
	}

	// Don't want the zone goin to sleep and trapping her.
	if (fate->in_room) {
		fate->in_room->zone->idle_time = 0;
	}
	
	// Is it time to leave?
	if(timers[which_fate] > 0) { // Not time to leave yet.
		timers[which_fate] -= 10; // Specials are called every 10 seconds.
		return 1;
	} else {				// Time to leave
		#ifdef FATE_TEST
		timers[which_fate] = 20; // 25 mins to 2.5 hours.
		#endif
		#ifndef FATE_TEST
		timers[which_fate] = 60 * dice(30,6); // 25 mins to 2.5 hours.
		#endif
	}

	// It's time to leave.

	// find the dyntext of the rooms we need.
	for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
		if (!strcasecmp(dyn_name, dyntext->filename))
			break;
	}
	if(!dyntext) {
		sprintf(buf,"SYSERR: Fate %d unable to access dyntext doc.\r\n", GET_MOB_VNUM(fate));
		slog(buf);
		return 1;
	}
	// Grab the rooms out of the buffer
	if(dyntext->buffer) {
		roomlist_buf = roomlist_buf_top;
		strcpy(roomlist_buf,dyntext->buffer);
	} else {
		do_say(fate,"Hmm... Where should I go to next?",0,0);
		return 1;
	}
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
		if(temp_room && temp_room != fate->in_room) {
			cur_room_list_item = new room_list_struct;
			cur_room_list_item->room = temp_room;
			cur_room_list_item->next = roomlist;
			roomlist = cur_room_list_item;
			num_rooms++;
		}
		skip_spaces(&roomlist_buf);
	}
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
	char_from_room(fate);
	char_to_room(fate, dest);
	fate->in_room->zone->idle_time = 0;
	act("$n appears out of a green mist.", FALSE, fate, 0, 0, TO_ROOM);
	return 1;
}



