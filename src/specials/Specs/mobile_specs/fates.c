//
// File: fates.spec                  -- Part of TempusMUD
//
// Copyright 1999 by John Watson & John Rothe, all rights reserved.
//

struct room_list_struct {
    struct room_data *room;
    struct room_list_struct *next;
};

int fate_timers[3] = { 0, 0, 0 };

SPECIAL(fate)
{
    struct creature *fate = (struct creature *)me;
    struct room_data *dest = NULL;
    dynamic_text_file *dyntext = NULL;
    char dyn_name[64];
    char s[1024];
    struct room_list_struct *roomlist = NULL, *cur_room_list_item = NULL;
    char *roomlist_buf = NULL;
    char *roomlist_buf_top = NULL;
    struct room_data *temp_room = NULL;
    int num_rooms = 0, the_room = 0;
    int which_fate;

    if (spec_mode == SPECIAL_CMD) {
        if (!is_authorized(ch, CONTROL_FATE, NULL)) {
            return 0;
        } else if (CMD_IS("status")) {
            send_to_char(ch, "Fate timers: %d, %d, %d\r\n",
                fate_timers[0], fate_timers[1], fate_timers[2]);
            return 1;
        } else if (CMD_IS("reset")) {
            send_to_char(ch, "Resetting fate timers.\r\n");
            fate_timers[0] = fate_timers[1] = fate_timers[2] = 0;
            return 1;
        } else {
            send_to_char(ch,
                "Fate - Available commands are: status, reset\r\n");
        }
        return 0;
    }

    if (spec_mode != SPECIAL_TICK)
        return 0;

    // Don't want the zone goin to sleep and trapping her.
    if (fate->in_room) {
        fate->in_room->zone->idle_time = 0;
    }

    if (is_fighting(fate))
        return 0;
    if (!fate->in_room)
        return 0;

    // If there is a player in the room, don't do anything.
    if (player_in_room(fate->in_room))
        return 0;

    // Who is she?
    switch (GET_NPC_VNUM(fate)) {
    case FATE_VNUM_LOW:
        strcpy(dyn_name, "fatelow");
        which_fate = 0;
        break;
    case FATE_VNUM_MID:
        strcpy(dyn_name, "fatemid");
        which_fate = 1;
        break;
    case FATE_VNUM_HIGH:
        strcpy(dyn_name, "fatehigh");
        which_fate = 2;
        break;
    default:
        return 0;
    }

    // Is it time to leave?
    if (fate_timers[which_fate] > 0) {  // Not time to leave yet.
        fate_timers[which_fate] -= 10;  // Specials are called every 10 seconds.
        return 1;
    }
    // It's time to leave.
    // Start the timer for the next jump.
    fate_timers[which_fate] = 60 * dice(30, 6); // 25 mins to 2.5 hours.

    // find the dyntext of the rooms we need.
    for (dyntext = dyntext_list; dyntext; dyntext = dyntext->next) {
        if (!strcasecmp(dyn_name, dyntext->filename))
            break;
    }
    if (!dyntext) {
        errlog("Fate unable to access %s dyntext doc.", dyn_name);
        return 1;
    }
    // If the file is null, return
    if (!dyntext->buffer) {
        perform_say(fate, "say", "Hmm... Where should I go to next?");
        return 1;
    }
    // Grab the rooms out of the buffer
    CREATE(roomlist_buf_top, char, strlen(dyntext->buffer) + 1);
    roomlist_buf = roomlist_buf_top;
    strcpy(roomlist_buf, dyntext->buffer);

    // Copy over the buf, ignoring all non numbers
    while (*roomlist_buf) {
        if (!isdigit(*roomlist_buf))
            *roomlist_buf = ' ';
        roomlist_buf++;
    }
    roomlist_buf = roomlist_buf_top;
    // Build list of pointers to rooms
    skip_spaces(&roomlist_buf);
    while (*roomlist_buf) {
        roomlist_buf = one_argument(roomlist_buf, s);
        temp_room = real_room(atoi(s));
        if (temp_room && temp_room != fate->in_room
            && !player_in_room(temp_room)) {
            CREATE(cur_room_list_item, struct room_list_struct, 1);
            cur_room_list_item->room = temp_room;
            cur_room_list_item->next = roomlist;
            roomlist = cur_room_list_item;
            num_rooms++;
        }
        skip_spaces(&roomlist_buf);
    }
    free(roomlist_buf_top);
    // Didnt find any rooms. :P
    if (!roomlist) {
        perform_say(fate, "say", "Hmm... Where should I go to next?");
        return 1;
    }
    // Find room we want.
    the_room = number(0, num_rooms);

    cur_room_list_item = roomlist;
    for (int i = 1; i < the_room; i++) {
        cur_room_list_item = cur_room_list_item->next;
    }
    // got the room
    dest = cur_room_list_item->room;
    // delete the list
    while (roomlist) {
        cur_room_list_item = roomlist->next;
        free(roomlist);
        roomlist = cur_room_list_item;
    }

    slog("FATE: Fate #%d moving to %d.  Timer reset to %d.",
        which_fate, dest->number, fate_timers[which_fate]);

    act("$n disappears into a green mist.", false, fate, NULL, NULL, TO_ROOM);
    char_from_room(fate, false);
    char_to_room(fate, dest, false);
    fate->in_room->zone->idle_time = 0;
    act("$n appears out of a green mist.", false, fate, NULL, NULL, TO_ROOM);
    return 1;
}
