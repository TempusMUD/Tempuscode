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
retrieve_oedits(struct creature *ch, struct obj_data *inc_obj)
{
    int count = 0;

    if (IS_NPC(ch)) {
        return 0;
    }

    for (struct obj_data *obj = object_list; obj; obj = obj->next) {
        if (inc_obj->shared->vnum == obj->shared->vnum && obj->shared->owner_id == GET_IDNUM(ch)) {
            if (obj->worn_by == ch || obj->carried_by == ch) {
                send_to_char(ch, "%s glows bright orange.\r\n", obj->name);
                count++;
                continue;
            }
            if (obj->in_obj) {
                struct obj_data *tmp = get_top_container(obj);
                if (tmp->carried_by == ch || tmp->worn_by == ch) {
                    send_to_char(ch, "%s glows a faint orange, revealing owned items within.\r\n", tmp->name);
                    count++;
                    continue;
                }
            }

            struct room_data *prev_obj_room = where_obj(obj);

            if (obj->worn_by && obj == GET_EQ(obj->worn_by, obj->worn_on)) {
                act("$p disappears off of your body!",
                    false, obj->worn_by, obj, NULL, TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_WORN);
                count++;
            } else if (obj->worn_by
                       && obj == GET_IMPLANT(obj->worn_by, obj->worn_on)) {
                act("$p disappears out of your body!", false, obj->worn_by,
                    obj, NULL, TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_IMPLANT);
                count++;
            } else if (obj->worn_by
                       && obj == GET_TATTOO(obj->worn_by, obj->worn_on)) {
                act("$p fades off of your body!", false, obj->worn_by, obj, NULL,
                    TO_CHAR);
                unequip_char(obj->worn_by, obj->worn_on, EQUIP_TATTOO);
                count++;
            } else if (obj->carried_by) {
                act("$p disappears out of your hands!", false,
                    obj->carried_by, obj, NULL, TO_CHAR);
                obj_from_char(obj);
                count++;
            } else if (obj->in_room) {
                if (obj->in_room->people) {
                    act("$p fades out of existence!",
                        false, NULL, obj, NULL, TO_ROOM);
                }
                obj_from_room(obj);
                count++;
            } else if (obj->in_obj != NULL) {
                obj_from_obj(obj);
                count++;
            }

            if (prev_obj_room && ROOM_FLAGGED(prev_obj_room, ROOM_HOUSE)) {
                struct house *h = find_house_by_room(prev_obj_room->number);
                if (h) {
                    save_house(h);
                }
            }

            obj_to_char(obj, ch);
            act("$p appears in your hands!", false, ch, obj, NULL, TO_CHAR);
        }
    }

    return count;
}

int
load_oedits(struct creature *ch)
{
    GHashTableIter iter;
    gpointer key, val;
    int count = 0;

    if (IS_NPC(ch)) {
        return 0;
    }

    g_hash_table_iter_init(&iter, obj_prototypes);
    while (g_hash_table_iter_next(&iter, &key, &val)) {
        struct obj_data *obj = val;
        if (obj->shared->owner_id == GET_IDNUM(ch)) {
            int num_world = retrieve_oedits(ch, obj);
            if (!num_world) {
                struct obj_data *o = read_object(obj->shared->vnum);
                obj_to_char(o, ch);
                act("$p appears in your hands!", false, ch, o, NULL, TO_CHAR);
                count++;
            }
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
        int loaded = load_oedits(ch);

        if (!loaded) {
            perform_say_to(self, ch, "I couldn't retrieve any owned items for you.");
        } else {
            perform_say_to(self, ch, "That should be everything you own.");
        }
    } else {
        return false;
    }

    return true;
}
