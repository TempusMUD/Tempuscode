#include "actions.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "tmpstr.h"
#include "screen.h"
#include "utils.h"

struct obj_data *
get_top_container(struct obj_data *obj)
{
    while (obj->in_obj) {
        obj = obj->in_obj;
    }
    return obj;
}

int
load_oedits(struct creature *ch)
{
    GHashTableIter iter;
    gpointer key, val;
    int count = 0;

    if (IS_NPC(ch))
        return 0;

    g_hash_table_iter_init(&iter, obj_prototypes);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct obj_data *obj = val;
        if (obj->shared->owner_id == GET_IDNUM(ch)) {
            ++count;
            struct obj_data *o = read_object(obj->shared->vnum);
            obj_to_char(o, ch);
            act("$p appears in your hands!", false, ch, o, NULL, TO_CHAR);
        }
    }

    return count;
}

int
retrieve_oedits(struct creature *ch)
{
    int count = 0;

    if (IS_NPC(ch))
        return 0;

    for (struct obj_data * obj = object_list; obj; obj = obj->next) {
        if (obj->shared->owner_id == GET_IDNUM(ch)) {
            // They already have it
            if (obj->worn_by == ch || obj->carried_by == ch) {
                continue;
            }
            if (obj->in_obj) {
                struct obj_data *tmp = get_top_container(obj);
                // They already have it
                if (tmp->carried_by == ch || tmp->worn_by == ch) {
                    continue;
                }
            }

            struct room_data *prev_obj_room = where_obj(obj);

            if (obj->worn_by && obj == GET_EQ(obj->worn_by, obj->worn_on)) {
                act("$p disappears off of your body!",
                    false, obj->worn_by, obj, NULL, TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_WORN);
            } else if (obj->worn_by
                && obj == GET_IMPLANT(obj->worn_by, obj->worn_on)) {
                act("$p disappears out of your body!", false, obj->worn_by,
                    obj, NULL, TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_IMPLANT);
            } else if (obj->worn_by
                && obj == GET_TATTOO(obj->worn_by, obj->worn_on)) {
                act("$p fades off of your body!", false, obj->worn_by, obj, NULL,
                    TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_TATTOO);
            } else if (obj->carried_by) {
                act("$p disappears out of your hands!", false,
                    obj->carried_by, obj, NULL, TO_CHAR);
                obj_from_char(obj);
            } else if (obj->in_room) {
                if (obj->in_room->people) {
                    act("$p fades out of existence!",
                        false, NULL, obj, NULL, TO_ROOM);
                }
                obj_from_room(obj);
            } else if (obj->in_obj != NULL) {
                obj_from_obj(obj);
            }

            if (prev_obj_room && ROOM_FLAGGED(prev_obj_room, ROOM_HOUSE)) {
                struct house *h = find_house_by_room(prev_obj_room->number);
                if (h)
                    save_house(h);
            }

            ++count;
            obj_to_char(obj, ch);
            act("$p appears in your hands!", false, ch, obj, NULL, TO_CHAR);
        }
    }
    return count;
}

SPECIAL(oedit_reloader)
{
    struct creature *self = (struct creature *)me;
    if (!cmd || (!CMD_IS("help") && !CMD_IS("retrieve"))) {
        return false;
    }
    if (!can_see_creature(self, ch)) {
        perform_say(self, "say", "Who's there?  I can't see you.");
        return true;
    }

    if (CMD_IS("help") && !*argument) {
        perform_say(self, "say",
            "If you want me to retrieve your property, just type 'retrieve'.");
    } else if (CMD_IS("retrieve")) {
        act("$n closes $s eyes in deep concentration.", true, self, NULL, NULL,
            TO_ROOM);
        int retrieved = retrieve_oedits(ch);
        int existing = load_oedits(ch);

        if (!existing) {
            perform_say_to(self, ch, "You own nothing that can be retrieved.");
        } else if (!retrieved) {
            perform_say_to(self, ch,
                "You already have all that could be retrieved.");
        } else {
            perform_say_to(self, ch, "That should be everything.");
        }
    } else {
        return false;
    }

    return true;
}
