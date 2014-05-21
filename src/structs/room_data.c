#ifdef HAS_CONFIG_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <glib.h>

#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "prog.h"
#include "search.h"
#include "flow_room.h"

/**
 * init_room_affect:
 * @raff room affect to initialize
 *
 * Sets the fields of the room affect to their initial values.
 **/
void
init_room_affect(struct room_affect_data *raff, int level, int spell, int owner)
{
    raff->level = level;
    raff->spell_type = spell;
	raff->owner = owner;
	raff->flags = 0;
	raff->type = 0;
	raff->duration = 0;
    for (int i = 0;i < 4;i++) {
        raff->val[i] = 0;
    }

	raff->description = NULL;
    raff->next = NULL;
}

/**
 * free_room:
 * @room the #room_data to free
 *
 * Frees the memory occupied by a room, extracting all the objects
 * within.
 **/
void
free_room(struct room_data *room)
{
    for (struct extra_descr_data *exd = room->ex_description;
         exd != NULL;
         exd = room->ex_description) {
        room->ex_description = exd->next;
        free(exd->keyword);
        free(exd->description);
        free(exd);
    }
    for (struct special_search_data *srch = room->search;
         srch != NULL;
         srch = room->search) {
        room->search = srch->next;
        free(srch->command_keys);
        free(srch->keywords);
        free(srch->to_room);
        free(srch->to_vict);
        free(srch);
    }
    for (struct room_trail_data *trail = room->trail;
         trail != NULL;
         trail = room->trail) {
        room->trail = trail->next;
        free(trail->name);
        free(trail->aliases);
        free(trail);
    }
    for (int i = 0; i < NUM_DIRS; i++) {
        if (!room->dir_option[i]) {
            free(room->dir_option[i]->general_description);
            free(room->dir_option[i]->keyword);
        }
    }

    while (room->affects) {
        affect_from_room(room, room->affects);
    }

    free(room->name);
    free(room->description);
    free(room->sounds);
    free(room->prog);
    free(room->progobj);
    prog_state_free(room->prog_state);

    free(room);
}

int
count_room_exits(struct room_data *room)
{
    int idx, result = 0;

    for (idx = 0; idx < NUM_OF_DIRS; idx++)
        if (room->dir_option[idx] &&
            room->dir_option[idx]->to_room &&
            room->dir_option[idx]->to_room != room)
            result++;

    return result;
}

void
link_rooms(struct room_data *room_a, struct room_data *room_b, int dir)
{
    if (!ABS_EXIT(room_a, dir)) {
        CREATE(room_a->dir_option[dir],
               struct room_direction_data, 1);
    }
    ABS_EXIT(room_a, dir)->to_room = room_b;

    if (!ABS_EXIT(room_b, dir)) {
        CREATE(room_b->dir_option[rev_dir[dir]],
               struct room_direction_data, 1);
    }
    ABS_EXIT(room_b, rev_dir[dir])->to_room = room_a;
}

int
creature_occupancy(struct creature *ch)
{
    if (IS_PC(ch) && IS_IMMORT(ch))
        return 0;
    else if (GET_HEIGHT(ch) > 1000)
        return 3;
    else if (GET_HEIGHT(ch) > 500)
        return 2;
    else if (GET_HEIGHT(ch) > 50)
        return 1;

    return 0;
}

bool
will_fit_in_room(struct creature * ch, struct room_data * room)
{
    int i = 0;

    if (MAX_OCCUPANTS(room) == 0)
        return true;

    // If you're mounted, the size of the mount determines how much
    // space you take up, not your own size
    if (MOUNTED_BY(ch))
        i += creature_occupancy(MOUNTED_BY(ch));
    else
        i += creature_occupancy(ch);

    for (GList * it = first_living(room->people); it; it = next_living(it)) {
        struct creature *tch = it->data;
        i += creature_occupancy(tch);
    }

    return (i <= MAX_OCCUPANTS(room));
}
