//
// File: tarrasque.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

#define T_SLEEP   0
#define T_ACTIVE  1
#define T_RETURN  2
#define T_ERROR   3

#define EXIT_RM_MODRIAN 24878
#define EXIT_RM_ELECTRO -1
#define LAIR_RM       24918
#define BELLY_RM      24919

#define JRM_BASE_OUT  24903
#define JRM_TOP_OUT   24900
#define JRM_BASE_IN   24914
#define JRM_TOP_IN    24916

#define T_SLEEP_LEN   100
#define T_ACTIVE_LEN  200
#define T_POOP_LEN    50

struct room_data *belly_rm = NULL;

static unsigned int diurnal_timer = 0;
static unsigned int poop_timer = 0;
static bool pursuit = false;

room_num inner_tunnel[] = { 24914, 24915, 24916, 24917 };
room_num outer_tunnel[] = { 24903, 24902, 24901, 24900, 24878 };

void
tarrasque_jump(struct creature *tarr, int jump_mode)
{

    struct room_data *up_room = NULL;
    int i;
    act("$n takes a flying leap upwards into the chasm!",
        false, tarr, NULL, NULL, TO_ROOM);

    for (i = 1; i <= (jump_mode == T_ACTIVE ? 4 : 3); i++) {
        if (!(up_room = real_room(jump_mode == T_ACTIVE ?
                    outer_tunnel[i] : inner_tunnel[i]))) {
            break;
        }
        char_from_room(tarr, false);
        char_to_room(tarr, up_room, false);

        if ((jump_mode == T_ACTIVE && i == 4) ||
            (jump_mode == T_RETURN && i == 3)) {
            act("$n comes flying up from the chasm and lands with an earthshaking footfall!", false, tarr, NULL, NULL, TO_ROOM);
        } else {
            act("$n comes flying past from below, and disappears above you!",
                false, tarr, NULL, NULL, TO_ROOM);
        }
    }
}

bool
tarrasque_lash(struct creature *tarr, struct creature *vict)
{
    bool is_dead;

    WAIT_STATE(tarr, 3 RL_SEC);

    is_dead = damage(tarr, vict, NULL,
        GET_DEX(vict) < number(5, 28) ?
        (dice(20, 20) + 100) : 0, TYPE_TAIL_LASH, WEAR_LEGS);
    if (!is_dead && vict &&
        GET_POSITION(vict) >= POS_STANDING && GET_DEX(vict) < number(10, 24)) {
        GET_POSITION(vict) = POS_RESTING;
    }

    return is_dead;
}

bool
tarrasque_gore(struct creature * tarr, struct creature * vict)
{
    WAIT_STATE(tarr, 2 RL_SEC);

    act("$n charges forward!!", false, tarr, NULL, NULL, TO_ROOM);

    return damage(tarr, vict, NULL, (GET_DEX(vict) < number(5, 28)) ?
        (dice(30, 20) + 300) : 0, TYPE_GORE_HORNS, WEAR_BODY);
}

bool
tarrasque_trample(struct creature * tarr, struct creature * vict)
{
    if (vict && GET_DEX(vict) < number(5, 25)) {
        GET_POSITION(vict) = POS_SITTING;
        return damage(tarr, vict, NULL,
            dice(20, 40) + 300, TYPE_TRAMPLING, WEAR_BODY);
    }

    return 0;
}

void
tarrasque_swallow(struct creature *tarr, struct creature *vict)
{
    act("You suddenly pounce and devour $N whole!", false, tarr, NULL, vict, TO_CHAR);
    act("$n suddenly pounces and devours $N whole!", false, tarr, NULL, vict,
        TO_NOTVICT);
    send_to_char(vict, "The tarrasque suddenly leaps into the the air,\r\n");
    send_to_char(vict,
        "The last thing you hear is the bloody crunch of your body.\r\n");
    death_cry(vict);
    mudlog(GET_INVIS_LVL(vict), NRM, true,
        "%s swallowed by %s at %s (%d)",
        GET_NAME(vict), GET_NAME(tarr),
        tarr->in_room->name, tarr->in_room->number);
    char_from_room(vict, false);
    char_to_room(vict, belly_rm, false);
    act("The body of $n flies in from the mouth.", 1, vict, NULL, NULL, TO_ROOM);
    GET_HIT(vict) = -15;
    die(vict, tarr, TYPE_SWALLOW);
}

void
tarrasque_poop(struct creature *tarr, struct obj_data *obj)
{
    // The tarr doesn't poop while fighting, sleeping, or in its lair
    if (GET_POSITION(tarr) == POS_FIGHTING ||
        GET_POSITION(tarr) < POS_STANDING ||
        tarr->in_room->number == LAIR_RM) {
        return;
    }
    // Poop last object out if no object given
    if (obj == NULL) {
        obj = belly_rm->contents;
        if (obj == NULL)
            return;
        while (obj->next_content != NULL)
            obj = obj->next_content;
    }

    act("$p is pooped out.", false, NULL, obj, NULL, TO_ROOM);
    obj_from_room(obj);
    obj_to_room(obj, tarr->in_room);
    act("You strain a bit, and $p plops out.", false, tarr, obj, NULL, TO_CHAR);
    act("$n strains a bit, and $p plops out.", false, tarr, obj, NULL, TO_ROOM);

    poop_timer = 0;
}

void
tarrasque_digest(struct creature *tarr)
{
    struct obj_data *obj, *next_obj;
    bool pooped = false;

    if (!belly_rm) {
        slog("Tarrasque can't find his own belly in digest()!");
        return;
    }
    if (!belly_rm->contents)
        return;

    for (obj = belly_rm->contents; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (GET_OBJ_TYPE(obj) != ITEM_TRASH &&
            !IS_PLASTIC_TYPE(obj) && !IS_GLASS_TYPE(obj)) {
            obj = damage_eq(NULL, obj, 20, SPELL_ACIDITY);

            // If we have an object at this point, it needs to be excreted.  But
            // don't poop more than once at a time
            if (obj && !pooped) {
                pooped = true;
                tarrasque_poop(tarr, obj);
            }
        }
    }
}

int
tarrasque_die(struct creature *tarr, struct creature *killer)
{
    struct obj_data *obj, *next_obj;

    // Log death so we can get some stats
    slog("DEATH: %s killed by %s",
        GET_NAME(tarr), killer ? GET_NAME(killer) : "(NULL)");

    // When the tarrasque dies, everything in its belly must be put in its
    // corpse.  Acidified, of course.
    for (obj = belly_rm->contents; obj; obj = next_obj) {
        next_obj = obj->next_content;
        obj_from_room(obj);
        obj_to_char(obj, tarr);
    }

    // We wnat everything else to be normal
    return 0;
}

int
tarrasque_fight(struct creature *tarr)
{
    struct creature *vict = NULL, *vict2 = NULL;

    if (!is_fighting(tarr)) {
        errlog("FIGHTING(tarr) == NULL in tarrasque_fight!!");
        return 0;
    }
    // Select two lucky individuals who will be our random target dummies
    // this fighting pulse
    vict = get_char_random_vis(tarr, tarr->in_room);
    if (vict) {
        if (g_list_find(tarr->fighting, vict)
            || PRF_FLAGGED(vict, PRF_NOHASSLE))
            vict = NULL;
        vict2 = get_char_random_vis(tarr, tarr->in_room);
        if (vict2 && (g_list_find(tarr->fighting, vict2) || vict == vict2 ||
                PRF_FLAGGED(vict2, PRF_NOHASSLE)))
            vict2 = NULL;
    }
    // In a tarrasquian charge, the person whom the tarrasque is fighting
    // is hit first, then two other random people (if any).  As an added
    // bonus, the tarrasque will then be attacking the last person hit
    if (!number(0, 5)) {
        WAIT_STATE(tarr, 2 RL_SEC);

        tarrasque_gore(tarr, random_opponent(tarr));
        if (vict) {
            if (!tarrasque_trample(tarr, vict)) {
                add_combat(tarr, vict, true);
                add_combat(vict, tarr, false);
            }

        }
        if (vict2) {
            if (!tarrasque_trample(tarr, vict2)) {
                add_combat(tarr, vict2, true);
                add_combat(vict2, tarr, false);
            }
        }
        return 1;
    }
    // Tarrasquian tail lashing involves knocking down and damaging the
    // intrepid adventurer the tarr is fighting, plus the two random
    // individuals.
    if (!number(0, 5)) {
        act("$n lashes out with $s tail!!", false, tarr, NULL, NULL, TO_ROOM);
        WAIT_STATE(tarr, 3 RL_SEC);
        tarrasque_lash(tarr, random_opponent(tarr));

        if (vict)
            tarrasque_lash(tarr, vict);
        if (vict2)
            tarrasque_lash(tarr, vict2);
        return 1;
    }

    if (number(0, 5))
        return 0;

    // There's a chance, however small, that the tarrasque is going to swallow
    // the character whole, no matter how powerful.  If they make their saving
    // throw, they just get hurt.  The player being fought is most likely to
    // get swallowed.
    vict = random_opponent(tarr);
    if (vict) {
        if (GET_DEX(vict) < number(5, 23) &&
            !mag_savingthrow(vict, 50, SAVING_ROD)) {
            tarrasque_swallow(tarr, vict);
        } else {
            damage(tarr, vict, NULL, GET_DEX(vict) < number(5, 28) ?
                (dice(40, 20) + 200) : 0, TYPE_BITE, WEAR_BODY);
        }
    }

    return 1;
}

int
tarrasque_follow(struct creature *tarr)
{
    struct creature *ch, *vict, *vict2;
    int dir;

    pursuit = false;
    ch = NPC_HUNTING(tarr);
    if (!ch)
        return 0;

    vict = random_opponent(tarr);
    vict2 = NULL;
    if (!vict)
        vict = get_char_random_vis(tarr, tarr->in_room);
    if (vict) {
        vict2 = get_char_random_vis(tarr, tarr->in_room);
        if (g_list_find(tarr->fighting, vict) || vict == vict2)
            vict2 = NULL;
    }

    if (vict)
        tarrasque_trample(tarr, vict);
    if (vict2)
        tarrasque_trample(tarr, vict2);

    dir = find_first_step(tarr->in_room, ch->in_room, STD_TRACK);
    perform_move(tarr, dir, MOVE_NORM, true);

    return 0;
}

SPECIAL(tarrasque)
{

    static int mode = T_ACTIVE, checked = false;
    struct creature *tarr = (struct creature *)me;
    struct room_data *rm = NULL;

    if (!IS_TARRASQUE(tarr)) {
        tarr->mob_specials.shared->func = NULL;
        REMOVE_BIT(NPC_FLAGS(tarr), NPC_SPEC);
        errlog("There is only one true tarrasque");
        return 0;
    }

    if (spec_mode == SPECIAL_CMD) {
        if (CMD_IS("status") && GET_LEVEL(ch) >= LVL_IMMORT) {
            const char *mode_str = NULL;

            switch (mode) {
            case T_SLEEP:
                mode_str = tmp_sprintf("Asleep (%d/%d)", diurnal_timer,
                    T_SLEEP_LEN);
                break;
            case T_ACTIVE:
                mode_str = tmp_sprintf("Active (%d/%d)", diurnal_timer,
                    T_ACTIVE_LEN);
                break;
            case T_RETURN:
                mode_str = "Returning to lair";
                break;
            default:
                slog("Can't happen in tarrasque()");
            }
            send_to_char(ch,
                "Tarrasque status: %s,  poop (%d/%d)\r\n",
                mode_str, poop_timer, T_POOP_LEN);
            return 1;
        }
        if (CMD_IS("reload") && GET_LEVEL(ch) > LVL_DEMI) {
            mode = T_SLEEP;
            diurnal_timer = 0;
            if ((rm = real_room(LAIR_RM))) {
                char_from_room(tarr, false);
                char_to_room(tarr, rm, false);
            }
            send_to_char(ch, "Tarrasque reset.\r\n");
            return 1;
        }

        if ((tarr == ch) &&
            (CMD_IS("swallow") || CMD_IS("lash") || CMD_IS("gore") ||
                CMD_IS("trample"))) {
            struct creature *vict;

            skip_spaces(&argument);
            if (*argument) {
                vict = get_char_room_vis(tarr, argument);
                if (!vict) {
                    send_to_char(ch, "You don't see them here.\r\n");
                    return 1;
                }
            } else {
                vict = random_opponent(tarr);
                if (vict == NULL) {
                    send_to_char(ch, "Yes, but WHO?\r\n");
                    return 1;
                }
            }
            if (!CMD_IS("swallow") && !ok_to_attack(tarr, vict, true))
                return 1;
            else if (!ok_to_attack(tarr, vict, false))
                send_to_char(ch, "You aren't all that hungry, for once.\r\n");
            else if (GET_LEVEL(vict) >= LVL_IMMORT)
                send_to_char(ch, "Maybe that's not such a great idea...\r\n");
            else if (CMD_IS("swallow"))
                tarrasque_swallow(tarr, vict);
            else if (CMD_IS("gore"))
                tarrasque_gore(tarr, vict);
            else if (CMD_IS("trample"))
                tarrasque_trample(tarr, vict);
            else if (CMD_IS("lash"))
                tarrasque_lash(tarr, vict);
            return 1;
        }
        if (mode == T_SLEEP && !AWAKE(tarr))
            diurnal_timer += T_SLEEP_LEN / 10;
        return 0;
    }

    if (spec_mode == SPECIAL_DEATH)
        return tarrasque_die(tarr, ch);

    if (spec_mode == SPECIAL_LEAVE && ch != tarr && !NPC_HUNTING(tarr)) {
        start_hunting(tarr, ch);
        pursuit = true;
        return 0;
    }

    if (spec_mode != SPECIAL_TICK)
        return 0;

    if (!checked) {
        checked = true;
        if (!(belly_rm = real_room(BELLY_RM))) {
            errlog("Tarrasque can't find his belly!");
            tarr->mob_specials.shared->func = NULL;
            REMOVE_BIT(NPC_FLAGS(tarr), NPC_SPEC);
            return 1;
        }
    }

    tarrasque_digest(tarr);
    diurnal_timer++;
    poop_timer++;

    // If someone flees, we go after em immediately
    if (pursuit)
        return tarrasque_follow(tarr);

    if (is_fighting(tarr))
        return tarrasque_fight(tarr);

    if (poop_timer > T_POOP_LEN) {
        tarrasque_poop(tarr, NULL);
        return 1;
    }

    switch (mode) {
    case T_SLEEP:
        if (diurnal_timer > T_SLEEP_LEN) {
            GET_POSITION(tarr) = POS_STANDING;
            if (!add_path_to_mob(tarr, path_vnum_by_name("tarr_exit_mod"))) {
                errlog("error assigning tarr_exit_mod path to tarrasque.");
                mode = T_ERROR;
                return 1;
            }

            mode = T_ACTIVE;
            diurnal_timer = 0;
        } else if (tarr->in_room->number == LAIR_RM && AWAKE(tarr) &&
            !tarr->in_room->people->next) {
            act("$n goes to sleep.", false, tarr, NULL, NULL, TO_ROOM);
            GET_POSITION(tarr) = POS_SLEEPING;
        }
        break;

    case T_ACTIVE:
        if (diurnal_timer > T_ACTIVE_LEN) {
            mode = T_RETURN;
            diurnal_timer = 0;

            if (!add_path_to_mob(tarr, path_vnum_by_name("tarr_return_mod"))) {
                errlog("error assigning tarr_return_mod path to tarrasque.");
                mode = T_ERROR;
                return 1;
            }
            return 1;
        }

        if (tarr->in_room->number == outer_tunnel[0]) {
            tarrasque_jump(tarr, T_ACTIVE);
            return 1;
        }

        if (tarr->in_room->people->next) {
            for (GList * it = first_living(tarr->in_room->people); it; it = next_living(it)) {
                struct creature *tch = it->data;
                if (!IS_NPC(tch) && GET_LEVEL(tch) < 10) {
                    if (GET_POSITION(tch) < POS_STANDING)
                        GET_POSITION(tch) = POS_STANDING;
                    act("You are overcome with terror at the sight of $N!",
                        false, tch, NULL, tarr, TO_CHAR);
                    do_flee(tch, tmp_strdup(""), 0, 0);
                }
            }
        }

        break;

    case T_RETURN:{

            if (tarr->in_room->number == LAIR_RM) {
                mode = T_SLEEP;
                GET_POSITION(tarr) = POS_SLEEPING;
                act("$n lies down and falls asleep.", false, tarr, NULL, NULL,
                    TO_ROOM);
                diurnal_timer = 0;
                return 1;
            }

            if (tarr->in_room->number == inner_tunnel[0]) {
                tarrasque_jump(tarr, T_RETURN);
                return 1;
            }
            if (tarr->in_room->people->next) {
                for (GList * it = first_living(tarr->in_room->people); it; it = next_living(it)) {
                    struct creature *tch = it->data;
                    if (!IS_NPC(tch) && GET_LEVEL(tch) < 10) {
                        if (GET_POSITION(tch) < POS_STANDING)
                            GET_POSITION(tch) = POS_STANDING;
                        act("You are overcome with terror at the sight of $N!",
                            false, tch, NULL, tarr, TO_CHAR);
                        do_flee(tch, tmp_strdup(""), 0, 0);
                    }
                }
            }

            break;
        }
    default:
        break;
    }
    return 0;
}
