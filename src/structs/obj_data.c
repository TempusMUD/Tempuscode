#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>
#include <libpq-fe.h>
#include <libxml/parser.h>
#include <glib.h>

#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "race.h"
#include "creature.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "tmpstr.h"
#include "xml_utils.h"
#include "obj_data.h"
#include "spells.h"
#include "strutil.h"
#include "bomb.h"
#include "xmlc.h"

extern int no_plrtext;

struct extra_descr_data *locate_exdesc(char *word,
                                       struct extra_descr_data *list, bool exact);

struct obj_data *
make_object(void)
{
    struct obj_data *obj;

    CREATE(obj, struct obj_data, 1);

    obj->next = object_list;
    object_list = obj;

    obj->worn_on = -1;

    return obj;
}

/* release memory allocated for an obj struct */
void
free_object(struct obj_data *obj)
{
#define FREE_UNIQUE_FIELD(o,f) if (o->shared == NULL || obj->shared->proto == NULL || obj->shared->proto->f != obj->f) { free(obj->f); }
    FREE_UNIQUE_FIELD(obj, name);
    FREE_UNIQUE_FIELD(obj, aliases);
    FREE_UNIQUE_FIELD(obj, line_desc);
    FREE_UNIQUE_FIELD(obj, action_desc);
    FREE_UNIQUE_FIELD(obj, engraving);

    // This should probably check to make sure none of these are
    // shared with the proto.  In practice, these are likely to be
    // entirely disjoint if the first element is different.
    if (obj->shared == NULL
        || obj->shared->proto == NULL
        || obj->shared->proto->ex_description != obj->ex_description) {
        struct extra_descr_data *desc;
        while ((desc = obj->ex_description)) {
            obj->ex_description = desc->next;
            if (desc->keyword) {
                free(desc->keyword);
            }
            if (desc->description) {
                free(desc->description);
            }
            free(desc);
        }
    }

    while (obj->tmp_affects) {
        struct tmp_obj_affect *aff;

        aff = obj->tmp_affects;
        remove_object_affect(obj, aff);
    }
    free(obj);
}

const char *
get_worn_type(struct obj_data *obj)
{
    if (obj->worn_on == -1 || !obj->worn_by) {
        return "none";
    } else if (GET_EQ(obj->worn_by, obj->worn_on) == obj) {
        return "equipped";
    } else if (GET_IMPLANT(obj->worn_by, obj->worn_on) == obj) {
        return "implanted";
    } else if (GET_TATTOO(obj->worn_by, obj->worn_on) == obj) {
        return "tattooed";
    }

    return "unknown";
}

bool
is_slashing_weapon(struct obj_data *obj)
{
    return (IS_OBJ_TYPE(obj, ITEM_WEAPON)
            && (GET_OBJ_VAL(obj, 3) + TYPE_HIT == TYPE_SLASH));
}

float
modify_object_weight(struct obj_data *obj, float mod_weight)
{

    // if object is inside another object, recursively
    // go up and modify it
    if (obj->in_obj) {
        modify_object_weight(obj->in_obj, mod_weight);
    } else if (obj->worn_by) {
        if (GET_IMPLANT(obj->worn_by, obj->worn_on) == obj) {
            // implant, increase character weight
            GET_WEIGHT(obj->worn_by) += mod_weight;
        } else {
            // worn, increase worn weight
            IS_WEARING_W(obj->worn_by) += mod_weight;
        }
    } else if (obj->carried_by) {
        IS_CARRYING_W(obj->carried_by) += mod_weight;
    }

    obj->obj_flags.weight += mod_weight;

    return obj->obj_flags.weight;
}

bool
obj_is_unrentable(struct obj_data *obj)
{

    if (IS_OBJ_STAT(obj, ITEM_NORENT)
        || !OBJ_APPROVED(obj) || GET_OBJ_VNUM(obj) <= NOTHING
        || (IS_OBJ_TYPE(obj, ITEM_KEY) && GET_OBJ_VAL(obj, 1) == 0)
        || (IS_OBJ_TYPE(obj, ITEM_CIGARETTE) && GET_OBJ_VAL(obj, 3))
        || (IS_BOMB(obj) && obj->contains &&
            IS_FUSE(obj->contains) && FUSE_STATE(obj->contains))) {
        return true;
    }

    if (no_plrtext && obj->plrtext_len) {
        return true;
    }
    return false;
}

float
set_obj_weight(struct obj_data *obj, float new_weight)
{

    return (modify_object_weight(obj, new_weight - GET_OBJ_WEIGHT(obj)));
}

struct room_data *
find_object_room(struct obj_data *obj)
{
    if (obj->worn_by) {
        return obj->worn_by->in_room;
    } else if (obj->carried_by) {
        return obj->carried_by->in_room;
    } else if (obj->in_obj) {
        return find_object_room(obj->in_obj);
    } else if (obj->in_room) {
        return obj->in_room;
    }

    errlog("Object in limbo at %s:%d", __FILE__, __LINE__);
    return NULL;
}

// NEVER call obj function directly.  Use addAffect(), removeAffect(),
// or affectJoin() instead.
// add == true adds the affect, add == false deletes the affect
void
apply_object_affect(struct obj_data *obj, struct tmp_obj_affect *af, bool add)
{
    // Set or restore damage
    if (af->dam_mod && obj->obj_flags.max_dam > 0) {
        if (add) {
            obj->obj_flags.damage += af->dam_mod;
        } else {
            obj->obj_flags.damage -= af->dam_mod;
        }
    }
    // Set or restore maxdam
    if (af->maxdam_mod && obj->obj_flags.max_dam > 0) {
        if (add) {
            obj->obj_flags.max_dam += af->maxdam_mod;
        } else {
            obj->obj_flags.max_dam -= af->maxdam_mod;
        }
    }
    // Set or reset the value mods
    for (int i = 0; i < 4; i++) {
        if (af->val_mod[i] != 0) {
            if (add) {
                obj->obj_flags.value[i] += af->val_mod[i];
            } else {
                obj->obj_flags.value[i] -= af->val_mod[i];
            }
        }
    }
    // Set or restore type
    if (af->type_mod) {
        if (add) {
            af->old_type = obj->obj_flags.type_flag;
            obj->obj_flags.type_flag = af->type_mod;
        } else {
            obj->obj_flags.type_flag = af->old_type;
        }
    }
    // Set or reset wear positions
    if (af->worn_mod) {
        if (add) {
            check_bits_32(obj->obj_flags.wear_flags, &af->worn_mod);
            obj->obj_flags.wear_flags |= af->worn_mod;
        } else {
            obj->obj_flags.wear_flags &= ~(af->worn_mod);
        }
    }
    // set or reset extra flags
    if (af->extra_mod && af->extra_index) {
        int *oextra;
        if (af->extra_index == 1) {
            oextra = &obj->obj_flags.extra_flags;
        } else if (af->extra_index == 2) {
            oextra = &obj->obj_flags.extra2_flags;
        } else if (af->extra_index == 3) {
            oextra = &obj->obj_flags.extra3_flags;
        } else {
            errlog("Invalid extra index (%d) in apply_object_affect().",
                   af->extra_index);
            return;
        }

        if (add) {
            check_bits_32(*oextra, &af->extra_mod);
            *oextra |= af->extra_mod;
        } else {
            *oextra &= ~(af->extra_mod);
        }
    }
    // Set or reset weight
    if (af->weight_mod) {
        if (add) {
            modify_object_weight(obj, af->weight_mod);
        } else {
            modify_object_weight(obj, -af->weight_mod);
        }
    }
    // Set or reset affections
    // I probably could have done obj with less code but it would have been
    // much more difficult to understand
    for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (af->affect_mod[i] > 125) {
            af->affect_mod[i] = 125;
        } else if (af->affect_mod[i] < -125) {
            af->affect_mod[i] = -125;
        }

        if (af->affect_loc[i] != APPLY_NONE) {
            bool found = false;
            for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
                if (obj->affected[j].location == af->affect_loc[i]) {
                    found = true;
                    if (add) {
                        obj->affected[j].modifier += af->affect_mod[i];
                    } else {
                        obj->affected[j].modifier -= af->affect_mod[i];
                    }
                    break;
                }
            }

            if (!found) {
                for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
                    if (obj->affected[j].modifier == 0) {
                        obj->affected[j].location = APPLY_NONE;
                    }
                    if (obj->affected[j].location == APPLY_NONE) {
                        found = true;
                        obj->affected[j].location = af->affect_loc[i];
                        obj->affected[j].modifier = af->affect_mod[i];
                        break;
                    }
                }
            }

            normalize_applies(obj);

            if (!found) {
                errlog
                    ("No affect locations trying to alter object affect on obj vnum %d, id %ld",
                    GET_OBJ_VNUM(obj), obj->unique_id);
            }
        }
    }
}

void
add_object_affect(struct obj_data *obj, struct tmp_obj_affect *af)
{
    struct tmp_obj_affect *new_aff;

    CREATE(new_aff, struct tmp_obj_affect, 1);

    memcpy(new_aff, af, sizeof(struct tmp_obj_affect));
    new_aff->next = obj->tmp_affects;
    obj->tmp_affects = new_aff;

    apply_object_affect(obj, new_aff, true);
}

void
remove_object_affect(struct obj_data *obj, struct tmp_obj_affect *af)
{
    struct tmp_obj_affect *curr_aff;
    struct tmp_obj_affect *prev_aff = NULL;
    bool found = true;

    apply_object_affect(obj, af, false);

    curr_aff = obj->tmp_affects;

    while (curr_aff != NULL) {
        found = false;
        if (curr_aff == af) {
            found = true;
            if (prev_aff != NULL) {
                prev_aff->next = af->next;
            } else {
                obj->tmp_affects = curr_aff->next;
            }

            free(af);
            break;
        } else {
            prev_aff = curr_aff;
            curr_aff = curr_aff->next;
        }
    }

    if (!found) {
        errlog("Could not find matching temporary object affect to remove.");
    }
}

void
obj_affect_join(struct obj_data *obj, struct tmp_obj_affect *af, int dur_mode,
                int val_mode, int aff_mode)
{
    struct tmp_obj_affect *cur_aff = obj->tmp_affects;
    struct tmp_obj_affect tmp_aff;
    int j;
    bool found = false;

    for (; cur_aff != NULL; cur_aff = cur_aff->next) {
        if ((cur_aff->type == af->type) &&
            (cur_aff->extra_index == af->extra_index)) {
            memcpy(&tmp_aff, cur_aff, sizeof(struct tmp_obj_affect));
            if (dur_mode == AFF_ADD) {
                tmp_aff.duration = MIN(666, af->duration + tmp_aff.duration);
            } else if (dur_mode == AFF_AVG) {
                tmp_aff.duration = (af->duration + tmp_aff.duration) / 2;
            }

            for (int i = 0; i < 4; i++) {
                if (af->val_mod[i] != 0) {
                    if (val_mode == AFF_ADD) {
                        tmp_aff.val_mod[i] += af->val_mod[i];
                    } else if (val_mode == AFF_AVG) {
                        tmp_aff.val_mod[i] = (af->val_mod[i] +
                                              tmp_aff.val_mod[i]) / 2;
                    }
                }
            }

            // The next section for affects is really a cluster fuck
            // but i can't think of a better way to handle it
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (af->affect_loc[i] != APPLY_NONE) {
                    // Try to find a matching affect
                    for (int k = 0; k < MAX_OBJ_AFFECT; k++) {
                        if (af->affect_loc[i] == tmp_aff.affect_loc[k]) {
                            if (aff_mode == AFF_ADD) {
                                tmp_aff.affect_mod[k] += af->affect_mod[i];
                            } else if (aff_mode == AFF_AVG) {
                                tmp_aff.affect_mod[i] =
                                    (tmp_aff.affect_mod[k] +
                                     af->affect_mod[i]) / 2;
                            }
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                    // We didn't find a matching affect so just add it
                    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
                        if (tmp_aff.affect_loc[j] == APPLY_NONE) {
                            if (aff_mode != AFF_NOOP) {
                                tmp_aff.affect_loc[j] = af->affect_loc[i];
                                tmp_aff.affect_mod[j] = af->affect_mod[i];
                                break;
                            }
                        }
                    }

                    if (j == MAX_OBJ_AFFECT) {
                        errlog
                            ("Could not find free affect position in affectJoin().");
                    }
                }
            }

            if (af->weight_mod) {
                if (val_mode == AFF_ADD) {
                    tmp_aff.weight_mod += af->weight_mod;
                } else if (val_mode == AFF_AVG) {
                    tmp_aff.weight_mod =
                        (tmp_aff.weight_mod + af->weight_mod) / 2;
                }
                if (GET_OBJ_WEIGHT(obj) + tmp_aff.weight_mod < 0) {
                    tmp_aff.weight_mod = 0.01 - GET_OBJ_WEIGHT(obj);
                }
            }
            if (af->dam_mod) {
                if (val_mode == AFF_ADD) {
                    tmp_aff.dam_mod += af->dam_mod;
                } else if (val_mode == AFF_AVG) {
                    tmp_aff.dam_mod = (tmp_aff.dam_mod + af->dam_mod) / 2;
                }
            }

            if (af->maxdam_mod) {
                if (val_mode == AFF_ADD) {
                    tmp_aff.maxdam_mod += af->maxdam_mod;
                } else if (val_mode == AFF_AVG) {
                    tmp_aff.maxdam_mod =
                        (tmp_aff.maxdam_mod + af->maxdam_mod) / 2;
                }
            }

            remove_object_affect(obj, cur_aff);
            add_object_affect(obj, &tmp_aff);
            return;
        }
    }

    add_object_affect(obj, af);
}

void
normalize_applies(struct obj_data *obj)
{
    int i, j;

    for (i = 0, j = 0; i < MAX_OBJ_AFFECT; i++, j++) {
        while (j < MAX_OBJ_AFFECT
               && (obj->affected[j].location == APPLY_NONE
                   || obj->affected[j].modifier == 0)) {
            j++;
        }
        if (j < MAX_OBJ_AFFECT) {
            obj->affected[i] = obj->affected[j];
        } else {
            obj->affected[i].location = APPLY_NONE;
            obj->affected[i].modifier = 0;
        }

    }
}

struct tmp_obj_affect *
obj_has_affect(struct obj_data *obj, int spellnum)
{
    struct tmp_obj_affect *cur_aff = obj->tmp_affects;

    for (; cur_aff != NULL; cur_aff = cur_aff->next) {
        if (cur_aff->type == spellnum) {
            return cur_aff;
        }
    }

    return NULL;
}

int
equipment_position_of(struct obj_data *obj)
{
    int result;

    if (!obj->worn_by) {
        return -1;
    }

    for (result = 0; result < NUM_WEARS; result++) {
        if (GET_EQ(obj->worn_by, result) == obj) {
            return result;
        }
    }

    return -1;
}

int
implant_position_of(struct obj_data *obj)
{
    int result;

    if (!obj->worn_by) {
        return -1;
    }

    for (result = 0; result < NUM_WEARS; result++) {
        if (GET_IMPLANT(obj->worn_by, result) == obj) {
            return result;
        }
    }

    return -1;
}

int
count_contained_objs(struct obj_data *obj)
{
    struct obj_data *cur;
    int count = 0;

    for (cur = obj->contains; cur; cur = cur->next_content) {
        count++;
        if (IS_OBJ_TYPE(cur, ITEM_CONTAINER)) {
            count += count_contained_objs(cur);
        }
    }
    return count;
}

float
weigh_contained_objs(struct obj_data *obj)
{
    struct obj_data *cur;
    float weight = 0;

    for (cur = obj->contains; cur; cur = cur->next_content) {
        weight += GET_OBJ_WEIGHT(cur);
    }
    return weight;
}

struct obj_data *
load_object_from_xml(struct obj_data *container,
                     struct creature *victim, struct room_data *room, xmlNodePtr node)
{
    struct obj_data *obj;
    struct extra_descr_data *last_desc = NULL;
    int vnum = xmlGetIntProp(node, "vnum", 0);
    bool placed;
    char *str;

    placed = false;

    // Commenting out the code below may be a horrible idea, but I need
    // obj function to handle corpses.

    // NOTE: Obj is bad, but since the object is already allocated, we have
    // to do it obj way.  Functionality is copied from read_object(int)
    if (vnum > -1) {
        struct obj_data *prototype = real_object_proto(vnum);
        if (!prototype) {
            slog("Object #%d missing prototype being removed from %s's rent",
                 vnum, victim ? GET_NAME(victim) : "(null)");
            return NULL;
        }
        if (!OBJ_APPROVED(prototype) && (victim ? !IS_IMMORT(victim):true)) {
            slog("Object #%d with unapproved prototype being removed from %s's rent",
                 vnum, victim ? GET_NAME(victim) : "(null)");
            return NULL;
        }

        obj = make_object();

        obj->shared = prototype->shared;
        obj->shared->number++;

        obj->name = obj->shared->proto->name;
        obj->aliases = obj->shared->proto->aliases;
        obj->line_desc = obj->shared->proto->line_desc;
        obj->action_desc = obj->shared->proto->action_desc;
        obj->ex_description = obj->shared->proto->ex_description;
    } else {
        obj = make_object();
        obj->shared = null_obj_shared;
    }

    for (xmlNodePtr cur = node->xmlChildrenNode; cur; cur = cur->next) {
        if (xmlMatches(cur->name, "name")) {
            str = (char *)xmlNodeGetContent(cur);
            obj->name = strdup(tmp_gsub(str, "\n", "\r\n"));
            free(str);
        } else if (xmlMatches(cur->name, "aliases")) {
            obj->aliases = (char *)xmlNodeGetContent(cur);
        } else if (xmlMatches(cur->name, "engraving")) {
            obj->engraving = (char *)xmlNodeGetContent(cur);
        } else if (xmlMatches(cur->name, "line_desc")) {
            str = (char *)xmlNodeGetContent(cur);
            obj->line_desc =
                strdup(tmp_gsub(tmp_gsub(str, "\r", ""), "\n", "\r\n"));
            free(str);
        } else if (xmlMatches(cur->name, "extra_desc")) {
            struct extra_descr_data *desc;
            char *keyword;

            if (obj->shared->proto
                && obj->ex_description == obj->shared->proto->ex_description) {
                obj->ex_description =
                    exdesc_list_dup(obj->shared->proto->ex_description);
            }

            keyword = (char *)xmlGetProp(cur, (xmlChar *) "keywords");
            desc = locate_exdesc(fname(keyword), obj->ex_description, 1);
            if (!desc) {
                CREATE(desc, struct extra_descr_data, 1);
            } else {
                free(desc->keyword);
                free(desc->description);
            }
            desc->keyword = keyword;
            desc->description = (char *)xmlNodeGetContent(cur);
            if (last_desc) {
                last_desc->next = desc;
            } else {
                obj->ex_description = desc;
            }
            last_desc = desc;
        } else if (xmlMatches(cur->name, "action_desc")) {
            str = (char *)xmlNodeGetContent(cur);
            obj->action_desc = strdup(tmp_gsub(str, "\n", "\r\n"));
            free(str);
        } else if (xmlMatches(cur->name, "points")) {
            obj->obj_flags.type_flag = xmlGetIntProp(cur, "type", 0);
            obj->soilage = xmlGetIntProp(cur, "soilage", 0);
            obj->obj_flags.weight = xmlGetFloatProp(cur, "weight", 1);
            obj->obj_flags.material = xmlGetIntProp(cur, "material", 0);
            obj->obj_flags.timer = xmlGetIntProp(cur, "timer", 0);
            fix_object_weight(obj);
        } else if (xmlMatches(cur->name, "tracking")) {
            obj->unique_id = xmlGetIntProp(cur, "id", 0);
            obj->creation_method = xmlGetIntProp(cur, "method", 0);
            obj->creator = xmlGetIntProp(cur, "creator", 0);
            obj->creation_time = xmlGetIntProp(cur, "time", 0);
        } else if (xmlMatches(cur->name, "damage")) {
            obj->obj_flags.damage = xmlGetIntProp(cur, "current", 0);
            obj->obj_flags.max_dam = xmlGetIntProp(cur, "max", 0);
            obj->obj_flags.sigil_idnum = xmlGetIntProp(cur, "sigil_id", 0);
            obj->obj_flags.sigil_level = xmlGetIntProp(cur, "sigil_level", 0);
        } else if (xmlMatches(cur->name, "consignment")) {
            obj->consignor = xmlGetIntProp(cur, "consignor", 0);
            obj->consign_price = xmlGetIntProp(cur, "price", 0);
        } else if (xmlMatches(cur->name, "flags")) {
            char *flag = (char *)xmlGetProp(cur, (xmlChar *) "extra");
            obj->obj_flags.extra_flags = hex2dec(flag);
            free(flag);
            flag = (char *)xmlGetProp(cur, (xmlChar *) "extra2");
            obj->obj_flags.extra2_flags = hex2dec(flag);
            free(flag);
            flag = (char *)xmlGetProp(cur, (xmlChar *) "extra3");
            obj->obj_flags.extra3_flags = hex2dec(flag);
            free(flag);
        } else if (xmlMatches(cur->name, "worn")) {
            char *flag = (char *)xmlGetProp(cur, (xmlChar *) "possible");
            obj->obj_flags.wear_flags = hex2dec(flag);
            free(flag);
            int position = xmlGetIntProp(cur, "pos", 0);

            char *type = (char *)xmlGetProp(cur, (xmlChar *) "type");
            if (type != NULL) {
                if (victim && streq(type, "equipped")) {
                    equip_char(victim, obj, position, EQUIP_WORN);
                } else if (victim && streq(type, "implanted")) {
                    equip_char(victim, obj, position, EQUIP_IMPLANT);
                } else if (victim && streq(type, "tattooed")) {
                    equip_char(victim, obj, position, EQUIP_TATTOO);
                } else if (container) {
                    unsorted_obj_to_obj(obj, container);
                } else if (victim) {
                    unsorted_obj_to_char(obj, victim);
                } else if (room) {
                    unsorted_obj_to_room(obj, room);
                }
                placed = true;
            }
            free(type);
        } else if (xmlMatches(cur->name, "values")) {
            obj->obj_flags.value[0] = xmlGetIntProp(cur, "v0", 0);
            obj->obj_flags.value[1] = xmlGetIntProp(cur, "v1", 0);
            obj->obj_flags.value[2] = xmlGetIntProp(cur, "v2", 0);
            obj->obj_flags.value[3] = xmlGetIntProp(cur, "v3", 0);
        } else if (xmlMatches(cur->name, "affectbits")) {
            char *aff = (char *)xmlGetProp(cur, (xmlChar *) "aff1");
            obj->obj_flags.bitvector[0] = hex2dec(aff);
            free(aff);

            aff = (char *)xmlGetProp(cur, (xmlChar *) "aff2");
            obj->obj_flags.bitvector[1] = hex2dec(aff);
            free(aff);

            aff = (char *)xmlGetProp(cur, (xmlChar *) "aff3");
            obj->obj_flags.bitvector[2] = hex2dec(aff);
            free(aff);

        } else if (xmlMatches(cur->name, "affect")) {
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                if (obj->affected[i].location == 0
                    && obj->affected[i].modifier == 0) {
                    obj->affected[i].modifier =
                        xmlGetIntProp(cur, "modifier", 0);
                    obj->affected[i].location =
                        xmlGetIntProp(cur, "location", 0);
                    break;
                }
            }
        } else if (xmlMatches(cur->name, "object")) {
            load_object_from_xml(obj, victim, room, cur);
        } else if (xmlMatches(cur->name, "tmpaffect")) {
            struct tmp_obj_affect af;
            char *prop;

            memset(&af, 0x0, sizeof(af));
            af.level = xmlGetIntProp(cur, "level", 0);
            af.type = xmlGetIntProp(cur, "type", 0);
            af.duration = xmlGetIntProp(cur, "duration", 0);
            af.dam_mod = xmlGetIntProp(cur, "dam_mod", 0);
            af.maxdam_mod = xmlGetIntProp(cur, "maxdam_mod", 0);
            af.val_mod[0] = xmlGetIntProp(cur, "val_mod1", 0);
            af.val_mod[1] = xmlGetIntProp(cur, "val_mod2", 0);
            af.val_mod[2] = xmlGetIntProp(cur, "val_mod3", 0);
            af.val_mod[3] = xmlGetIntProp(cur, "val_mod4", 0);
            af.type_mod = xmlGetIntProp(cur, "type_mod", 0);
            af.old_type = xmlGetIntProp(cur, "old_type", 0);
            af.worn_mod = xmlGetIntProp(cur, "worn_mod", 0);
            af.extra_mod = xmlGetIntProp(cur, "extra_mod", 0);
            af.extra_index = xmlGetIntProp(cur, "extra_index", 0);
            af.weight_mod = xmlGetFloatProp(cur, "weight_mod", 0.01);
            for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
                prop = tmp_sprintf("affect_loc%d", i);
                af.affect_loc[i] = xmlGetIntProp(cur, prop, 0);
                prop = tmp_sprintf("affect_mod%d", i);
                af.affect_mod[i] = xmlGetIntProp(cur, prop, 0);
            }

            add_object_affect(obj, &af);
        }

    }

    normalize_applies(obj);

    if (!OBJ_APPROVED(obj)) {
        slog("Unapproved object %d being junked from %s's rent.",
             vnum, (victim && GET_NAME(victim)) ? GET_NAME(victim) : "(none)");
        extract_obj(obj);
        return NULL;
    }
    if (!placed) {
        if (victim != NULL) {
            obj_to_char(obj, victim);
        } else if (container != NULL) {
            obj_to_obj(obj, container);
        } else if (room != NULL) {
            obj_to_room(obj, room);
        }
    }
    return obj;
}

// SHOULD_WRITE_STR is a macro which returns whether or not str
// differs from its prototype's string, assuming that str is non-null,
// the prototype exists, and the prototype's string exists.  Defined
// as a macro since proto_str must not be evaluated if proto is null.
#define SHOULD_WRITE_STR(str, proto, proto_str) ((str) && ((proto) == NULL || (proto_str) == NULL || !streq((str), (proto_str))))

struct xmlc_node *xml_collect_obj_list(struct obj_data *obj_list);

struct xmlc_node *
xml_collect_extra_descs(struct extra_descr_data *desc_list)
{
    struct xmlc_node *result = NULL;

    for (struct extra_descr_data *desc = desc_list; desc; desc = desc->next) {
        result = xml_append_node(
            result,
            xml_node("extra_desc",
                     xml_str_attr("keywords", desc->keyword),
                     xml_text(desc->description),
                     NULL));
    }
    return result;
}

struct xmlc_node *
xml_collect_obj_affects(struct obj_data *obj)
{
    struct xmlc_node *result = NULL;

    for (int i = 0;i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location == 0) {
            continue;
        }
        result = xml_append_node(
            result,
            xml_node("affect",
                     xml_int_attr("modifier", obj->affected[i].modifier),
                     xml_int_attr("location", obj->affected[i].location),
                     NULL));
    }
    return result;
}

struct xmlc_node *
xml_collect_tmp_affect_affects(struct tmp_obj_affect *af)
{
    struct xmlc_node *result = NULL;

    for (int i = 0;i < MAX_OBJ_AFFECT; i++) {
        if (af->affect_loc[i] == 0) {
            continue;
        }
        result = xml_append_node(
            result,
            xml_int_attr(tmp_sprintf("affect_loc%d", i), af->affect_loc[i]));
        result = xml_append_node(
            result,
            xml_int_attr(tmp_sprintf("affect_mod%d", i), af->affect_mod[i]));
    }
    return result;
}

struct xmlc_node *
xml_collect_tmp_affects(struct tmp_obj_affect *af_head)
{
    struct xmlc_node *result = NULL;

    for (struct tmp_obj_affect *af = af_head; af; af = af->next) {
        result = xml_append_node(
            result,
            xml_node("tmpaffect",
                     xml_int_attr("level", af->level),
                     xml_int_attr("type", af->type),
                     xml_int_attr("duration", af->duration),
                     xml_int_attr("dam_mod", af->dam_mod),
                     xml_int_attr("maxdam_mod", af->maxdam_mod),
                     xml_int_attr("val_mod1", af->val_mod[0]),
                     xml_int_attr("val_mod2", af->val_mod[1]),
                     xml_int_attr("val_mod3", af->val_mod[2]),
                     xml_int_attr("val_mod4", af->val_mod[3]),
                     xml_int_attr("type_mod", af->type_mod),
                     xml_int_attr("old_type", af->old_type),
                     xml_int_attr("worn_mod", af->worn_mod),
                     xml_int_attr("extra_mod", af->extra_mod),
                     xml_int_attr("extra_index", af->extra_index),
                     xml_int_attr("weight_mod", af->weight_mod),
                     xml_splice(xml_collect_tmp_affect_affects(af)),
                     NULL));
    }
    return result;
}

struct xmlc_node *
xml_collect_obj(struct obj_data *obj)
{
    struct obj_data *proto = obj->shared->proto;

    // stash temp object affects
    struct tmp_obj_affect *af_head = obj->tmp_affects;
    for (struct tmp_obj_affect *af = af_head; af; af = af->next) {
        apply_object_affect(obj, af, false);
    }

    struct xmlc_node *result =
        xml_node("object",
                 xml_int_attr("vnum", obj->shared->vnum),
                 xml_if(SHOULD_WRITE_STR(obj->name, proto, proto->name),
                        xml_node("name", xml_text(obj->name), NULL)),
                 xml_if(SHOULD_WRITE_STR(obj->aliases, proto, proto->aliases),
                        xml_node("aliases", xml_text(obj->aliases), NULL)),
                 xml_if(SHOULD_WRITE_STR(obj->engraving, proto, proto->engraving),
                        xml_node("engraving", xml_text(obj->engraving), NULL)),
                 xml_if(SHOULD_WRITE_STR(obj->line_desc, proto, proto->line_desc),
                        xml_node("line_desc", xml_text(obj->line_desc), NULL)),
                 xml_if(!proto || obj->ex_description != proto->ex_description,
                        xml_splice(xml_collect_extra_descs(obj->ex_description))),
                 xml_if(SHOULD_WRITE_STR(obj->action_desc, proto, proto->action_desc),
                        xml_node("action_desc", xml_text(obj->action_desc), NULL)),
                 xml_node("points",
                          xml_int_attr("type", obj->obj_flags.type_flag),
                          xml_int_attr("soilage", obj->soilage),
                          xml_float_attr("weight", GET_OBJ_WEIGHT(obj) - weigh_contained_objs(obj)),
                          xml_int_attr("material", obj->obj_flags.material),
                          xml_int_attr("timer", obj->obj_flags.timer),
                          NULL),
                 xml_node("tracking",
                          xml_int_attr("id", obj->unique_id),
                          xml_int_attr("method", obj->creation_method),
                          xml_int_attr("creator", obj->creator),
                          xml_int_attr("time", obj->creation_time),
                          NULL),
                 xml_node("damage",
                          xml_int_attr("current", obj->obj_flags.damage),
                          xml_int_attr("max", obj->obj_flags.max_dam),
                          xml_int_attr("sigil_id", obj->obj_flags.sigil_idnum),
                          xml_int_attr("sigil_level", obj->obj_flags.sigil_level),
                          NULL),
                 xml_node("flags",
                          xml_hex_attr("extra", obj->obj_flags.extra_flags),
                          xml_hex_attr("extra2", obj->obj_flags.extra2_flags),
                          xml_hex_attr("extra3", obj->obj_flags.extra3_flags),
                          NULL),
                 xml_node("values",
                          xml_int_attr("v0", obj->obj_flags.value[0]),
                          xml_int_attr("v1", obj->obj_flags.value[1]),
                          xml_int_attr("v2", obj->obj_flags.value[2]),
                          xml_int_attr("v3", obj->obj_flags.value[3]),
                          NULL),
                 xml_node("affectbits",
                          xml_hex_attr("aff1", obj->obj_flags.bitvector[0]),
                          xml_hex_attr("aff2", obj->obj_flags.bitvector[1]),
                          xml_hex_attr("aff3", obj->obj_flags.bitvector[2]),
                          NULL),
                 xml_if(obj->consignor,
                        xml_node("consignment",
                                 xml_int_attr("consignor", obj->consignor),
                                 xml_int_attr("price", obj->consign_price),
                                 NULL)),
                 xml_splice(xml_collect_obj_affects(obj)),
                 xml_splice(xml_collect_tmp_affects(af_head)),

                 xml_splice(xml_collect_obj_list(obj->contains)),

                 // Intentionally done last since reading obj property in loadFromXML
                 // causes the eq to be worn on the character.
                 xml_node("worn",
                          xml_hex_attr("possible", obj->obj_flags.wear_flags),
                          xml_int_attr("pos", obj->worn_on),
                          xml_str_attr("type", get_worn_type(obj)),
                          NULL),
                 NULL);

    // Ok, we'll restore the affects to the object right here
    obj->tmp_affects = af_head;
    for (struct tmp_obj_affect *af = af_head; af; af = af->next) {
        apply_object_affect(obj, af, true);
    }

    return result;
}

struct xmlc_node *
xml_collect_obj_list(struct obj_data *obj_list)
{
    if (obj_list == NULL) {
        return NULL;
    }

    struct xmlc_node *result = xml_collect_obj(obj_list);
    struct xmlc_node *last_obj = result;

    for (obj_list = obj_list->next_content;obj_list;obj_list = obj_list->next_content) {
        last_obj->next = xml_collect_obj(obj_list);
        last_obj = last_obj->next;
    }
    return result;
}


struct xmlc_node *
xml_collect_obj_glist(GList *obj_list)
{
    if (obj_list == NULL) {
        return NULL;
    }

    struct xmlc_node *result = xml_collect_obj((struct obj_data *)obj_list->data);
    struct xmlc_node *last_obj = result;

    for (obj_list = obj_list->next;obj_list;obj_list = obj_list->next) {
        last_obj->next = xml_collect_obj((struct obj_data *)obj_list->data);
        last_obj = last_obj->next;
    }
    return result;
}

void
write_corpse_file(struct obj_data *corpse)
{
    xml_output(get_corpse_file_path(CORPSE_IDNUM(corpse)),
               xml_node("corpse",
                        xml_splice(xml_collect_obj(corpse)),
                        NULL));
}


const char *
obj_cond(struct obj_data *obj)
{

    int num;

    if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_MAX_DAM(obj) == -1) {
        return "unbreakable";
    } else if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
        return "<broken>";
    } else if (GET_OBJ_MAX_DAM(obj) == 0) {
        return "frail";
    } else if (GET_OBJ_DAM(obj) >= GET_OBJ_MAX_DAM(obj)) {
        num = 0;
    } else {
        num = ((GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj)) * 100 /
               GET_OBJ_MAX_DAM(obj));
    }

    if (num == 0) {
        return "perfect";
    } else if (num < 10) {
        return "excellent";
    } else if (num < 30) {
        return "good";
    } else if (num < 50) {
        return "fair";
    } else if (num < 60) {
        return "worn";
    } else if (num < 70) {
        return "shabby";
    } else if (num < 90) {
        return "bad";
    }

    return "terrible";
}

const char *
obj_cond_color(struct obj_data *obj, int color_level, enum display_mode mode)
{
    int num;

    if (GET_OBJ_DAM(obj) == -1 || GET_OBJ_MAX_DAM(obj) == -1) {
        return tmp_sprintf("%sunbreakable%s",
                           termcode(mode, color_level, C_CMP, TERM_GRN),
                           termcode(mode, color_level, C_CMP, TERM_NRM));
    } else if (IS_OBJ_STAT2(obj, ITEM2_BROKEN)) {
        return "<broken>";
    } else if (GET_OBJ_MAX_DAM(obj) == 0) {
        return "frail";
    } else if (GET_OBJ_DAM(obj) >= GET_OBJ_MAX_DAM(obj)) {
        num = 0;
    } else {
        num = ((GET_OBJ_MAX_DAM(obj) - GET_OBJ_DAM(obj)) * 100 /
               GET_OBJ_MAX_DAM(obj));
    }

    if (num == 0) {
        return "perfect";
    } else if (num < 10) {
        return "excellent";
    } else if (num < 30) {
        return tmp_sprintf("%sgood%s", termcode(mode, color_level, C_NRM, TERM_CYN), termcode(mode, color_level, C_NRM, TERM_NRM));
    } else if (num < 50) {
        return tmp_sprintf("%sfair%s", termcode(mode, color_level, C_NRM, TERM_CYN), termcode(mode, color_level, C_NRM, TERM_NRM));
    } else if (num < 60) {
        return tmp_sprintf("%sworn%s", termcode(mode, color_level, C_NRM, TERM_YEL), termcode(mode, color_level, C_NRM, TERM_NRM));
    } else if (num < 70) {
        return tmp_sprintf("%sshabby%s", termcode(mode, color_level, C_NRM, TERM_YEL), termcode(mode, color_level, C_NRM, TERM_NRM));
    } else if (num < 90) {
        return tmp_sprintf("%sbad%s", termcode(mode, color_level, C_NRM, TERM_YEL), termcode(mode, color_level, C_NRM, TERM_NRM));
    }

    return tmp_sprintf("%sterrible%s", termcode(mode, color_level, C_NRM, TERM_RED), termcode(mode, color_level, C_NRM, TERM_NRM));
}

void
fix_object_weight(struct obj_data *obj)
{
    float new_weight = GET_OBJ_WEIGHT(obj);

    if (IS_OBJ_TYPE(obj, ITEM_POTION)) {
        new_weight = 0.5;
    } else if (IS_OBJ_TYPE(obj, ITEM_SYRINGE)) {
        new_weight = 0.3;
    } else if (IS_OBJ_TYPE(obj, ITEM_PILL)) {
        new_weight = 0.1;
    } else if (IS_OBJ_TYPE(obj, ITEM_PEN)) {
        new_weight = 0.2;
    } else if (IS_OBJ_TYPE(obj, ITEM_CIGARETTE)) {
        new_weight = 0.1;
    } else if (IS_OBJ_TYPE(obj, ITEM_BULLET)) {
        new_weight = 0.05;
    } else if (IS_OBJ_TYPE(obj, ITEM_MICROCHIP)) {
        new_weight = 0.05;
    } else if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
        // rings
        new_weight = 0.1;
    } else if (CAN_WEAR(obj, ITEM_WEAR_EAR)) {
        // earrings
        new_weight = 0.1;
    } else if (new_weight == 0.0) {
        new_weight = 0.01;
    } else if (obj->obj_flags.weight < 0.0) {
        if (obj->shared->proto && obj->shared->proto != obj) {
            slog("Illegal object %d weight %.2f - setting to %.2f from prototype",
                 GET_OBJ_VNUM(obj),
                 GET_OBJ_WEIGHT(obj),
                 GET_OBJ_WEIGHT(obj->shared->proto));
            new_weight = obj->shared->proto->obj_flags.weight;
        } else {
            if (obj->shared->proto) {
                slog("Illegal prototype object %d weight %.2f - setting to 1",
                     GET_OBJ_VNUM(obj),
                     GET_OBJ_WEIGHT(obj));
            } else {
                slog("Illegal prototype-less object weight %.2f - setting to 1",
                     GET_OBJ_WEIGHT(obj));
            }
            new_weight = 1;
        }
    }

    set_obj_weight(obj, new_weight);
}
