#ifdef HAS_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "structs.h"
#include "room_data.h"
#include "utils.h"
#include "prog.h"

void
free_room(struct room_data *room)
{
    free(room->name);
    free(room->description);
    free(room->sounds);
    free(room->prog);
    free(room->progobj);
    prog_state_free(room->prog_state);
    struct extra_descr_data *exd, *next_exd;
    for (exd = room->ex_description; exd; exd = next_exd) {
        next_exd = exd->next;
        free(exd->keyword);
        free(exd->description);
        free(exd);
    }

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
