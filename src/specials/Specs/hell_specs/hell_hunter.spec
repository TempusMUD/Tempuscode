//
// File: hell_hunter.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

typedef struct {
    int m_vnum;
    int weapon;
    char prob;
} Hunter_data;

typedef struct {
    int o_vnum;
    int level;
} Hunt_data;

#define H_SPINED   16900 
#define H_ABISHAI  16901 
#define H_BEARDED  16902
#define H_BARBED   16903
#define H_BONE     16904
#define H_HORNED   16905

#define H_REGULATOR 16907

#define SAFE_ROOM_BITS \
ROOM_PEACEFUL | ROOM_NOMOB | ROOM_NOTRACK | ROOM_NOMAGIC | ROOM_NOTEL | \
ROOM_ARENA | ROOM_NORECALL | ROOM_GODROOM | ROOM_HOUSE | ROOM_DEATH

SPECIAL(shop_keeper);

Hunter_data hunters[10][4] = {
    {{0, -1, 0},  // level 0 - padding
     {0, -1, 0},
     {0, -1, 0},
     {0, -1, 0}},
    {{H_BARBED,  -1, 100},  // level 1 - avernus
     {H_SPINED,  -1,  50},
     {H_ABISHAI, -1,  60},
     {H_ABISHAI, -1,  60}},
    {{H_BARBED,  -1, 100},  // level 2 - dis
     {H_BARBED,  -1,  70},
     {H_ABISHAI, -1,  50},
     {H_SPINED,  -1,  50}},
    {{H_BARBED,  -1, 100},  // level 3 - minauros
     {H_BARBED,  -1,  70},
     {H_BEARDED, -1,  70},
     {H_SPINED,  -1,  50}},
    {{H_BARBED,  -1, 100},  // level 4 - phlegethos
     {H_BONE,    -1,  70},
     {H_BEARDED, -1,  70},
     {H_SPINED,  -1,  70}},
    {{H_HORNED,  -1, 100},  // level 5 - stygia
     {H_BONE,    -1,  50},
     {H_BEARDED, -1,  90},
     {H_SPINED,  -1,  70}},
    {{H_HORNED,  -1, 100},  // level 6 - malbolge
     {H_HORNED,  -1,  60},
     {H_SPINED,  -1,  70},
     {H_SPINED,  -1,  70}}
};

Hunt_data items[] = {
    {16118, 6},   // whip of pain
    {16119, 6},   // bracelet of pain
    {16146, 6},   // spiked shin guards
    {16510, 5},   // flesh bracelet
    {16147, 5},   // enchanted loincloth
    {16150, 5},   // claws of geryon
    {16149, 5},   // mode of degreelessness
    {16140, 4},   // huge military fork
    {16116, 3},   // sword of wounding
    {16110, 3},   // shield of armageddon
    {16111, 3},   // mace of disruption
    {16130, 2},   // inq hood
    {16108, 2},   // staff of striking
    {16128, 1},   // scales of tiamat
    {-1, -1}
};

SPECIAL(hell_hunter_brain)
{
    static int counter = 1;
    static int freq =   80;
    struct obj_data *obj = NULL, *weap = NULL;
    struct char_data *mob = NULL, *vict = NULL;
    int i, j;
    int num_devils = 0, regulator = 0;

    if (cmd) {
	if (CMD_IS("status")) {
	    sprintf(buf, "Counter is at %d, freq %d.\r\n", counter, freq);
	    send_to_char(buf, ch);
	    for (i = 0; items[i].o_vnum != -1; i++) {
		if (!(obj = real_object_proto(items[i].o_vnum)))
		    continue;
		sprintf(buf, "%3d. [%5d] %30s %3d/%3d\r\n",
			i, items[i].o_vnum, obj->short_description,
			obj->shared->number, obj->shared->house_count);
		send_to_char(buf, ch);
	    }
	    return 1;
	}
	if (CMD_IS("activate")) {
	    skip_spaces(&argument);
	
	    if (*argument) {
		freq = atoi(argument);
		sprintf(buf, "Frequency set to %d.\n", freq);
		send_to_char(buf, ch);
		return 1;
	    }

	    counter = 1;
	}
	else
	    return 0;
    }

    if (--counter > 0) {
	return 0;
    }

    counter = freq;

    for (obj = object_list; obj; vict = NULL, obj = obj->next) {

	if (!obj->in_room && !(vict = obj->carried_by) && !(vict = obj->worn_by))
	    continue;

	if (vict && (IS_NPC(vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
		     // some rooms are safe
		     ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) ||
		     // can't go to isolated zones
		     ZONE_FLAGGED(vict->in_room->zone, ZONE_ISOLATED) ||
		     // ignore shopkeepers
		     (IS_NPC(vict) && shop_keeper == GET_MOB_SPEC(vict)) ||
		     // don't go to heaven
		     vict->in_room->zone->number == 430)) {
	    continue;
	}
	if ( vict && IS_SOULLESS(vict)) {
		send_to_char("You feel the eyes of hell look down upon you.\r\n",ch);
		continue;
	}

	if (obj->in_room && 
	    (ROOM_FLAGGED(obj->in_room, SAFE_ROOM_BITS) ||
	     ZONE_FLAGGED(obj->in_room->zone, ZONE_ISOLATED))) {
	    continue;
	}

	for (i = 0; items[i].o_vnum != -1; i++)
	    if (items[i].o_vnum == GET_OBJ_VNUM(obj))
		break;

	if (items[i].o_vnum == -1) {
	    continue;
	}

	// try to skip the first person sometimes
	if (vict && !number(0, 2)) {
	    continue;
	}

	for (j = 0; j < 4; j++) {
	    if (number(0, 100) <= hunters[items[i].level][j].prob) { // probability
		if (!(mob = read_mobile(hunters[items[i].level][j].m_vnum))) {
		    slog("SYSERR: Unable to load mob in hell_hunter_brain()");
		    return 0;
		}
		if (hunters[items[i].level][j].weapon >= 0 &&
		    (weap = read_object(hunters[items[i].level][j].weapon))) {
		    if (equip_char(mob, weap, WEAR_WIELD, MODE_EQ)) { // mob equipped
			slog("SYSERR: (non-critical) Hell Hunter killed by eq.");
			return 1;                       // return if equip killed mob
		    }
		}

		num_devils++;

		if (vict) {
		    HUNTING(mob) = vict;
            SET_BIT(MOB_FLAGS(mob),MOB_SPIRIT_TRACKER);
	
		    if (!IS_NPC(vict) && GET_REMORT_GEN(vict)) {
			
			// hps GENx
			GET_MAX_HIT(mob) = GET_HIT(mob) = MIN(10000, (GET_MAX_HIT(mob) + GET_MAX_HIT(vict)));
			// damroll GENx/3
			GET_DAMROLL(mob) = MIN(50, (GET_DAMROLL(mob) + GET_REMORT_GEN(vict)));
			// hitroll GENx/3
			GET_HITROLL(mob) = MIN(50, (GET_HITROLL(mob) + GET_REMORT_GEN(vict)));
			
		    }
		}

		char_to_room(mob, vict ? vict->in_room : obj->in_room);
		act("$n steps suddenly out of an infernal conduit from the outer planes!", FALSE, mob, 0, 0, TO_ROOM);

	    }
	}

	if ( vict && !IS_NPC(vict) && GET_REMORT_GEN(vict) && number(0, GET_REMORT_GEN(vict)) > 1 ) {
	    
	    if ( ! ( mob = read_mobile(H_REGULATOR) ) )
		slog("SYSERR: Unable to load hell hunter regulator in hell_hunter_brain.");
	    else {
		regulator = 1;
		HUNTING(mob) = vict;
		char_to_room(mob, vict->in_room);
		act("$n materializes suddenly from a stream of hellish energy!", FALSE, mob, 0, 0, TO_ROOM);
	    }
	}
	    
	sprintf(buf, "HELL: %d Devils%s sent after obj %s (%s@%d)",
		num_devils,
		regulator ? " (+reg)" : "",
		obj->short_description,
		vict ? GET_NAME(vict) : "Nobody",
		obj->in_room ? obj->in_room->number :
		(vict && vict->in_room) ? vict->in_room->number : -1);
	mudlog(buf, CMP, vict ? GET_INVIS_LEV(vict) : 0, TRUE);
	sprintf(buf, "%d Devils%s sent after obj %s (%s@%d)",
		num_devils,
		regulator ? " (+reg)" : "",
		obj->short_description, vict ? "$N" : "Nobody",
		obj->in_room ? obj->in_room->number :
		(vict && vict->in_room) ? vict->in_room->number : -1);
	act(buf, FALSE, ch, 0, vict, TO_ROOM);
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
	return 1;
    }

    if ( cmd ) {
	send_to_char("Falling through, no match.\r\n", ch);
    } else {

	// we fell through, lets check again sooner than freq
	counter = freq >> 3;
    }

    return 0;
}
  
SPECIAL(hell_hunter)
{

    struct obj_data *obj = NULL, *t_obj = NULL;
    int i;
    struct char_data *vict = NULL, *devil = NULL;

    if (cmd)
	return 0;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {

	if (IS_CORPSE(obj)) {
	    for (t_obj = obj->contains; t_obj; t_obj = t_obj->next_content) {
		for (i = 0; items[i].o_vnum != -1; i++) {
		    if (items[i].o_vnum == GET_OBJ_VNUM(t_obj)) {
			act("$n takes $p from $P.", FALSE, ch, t_obj, obj, TO_ROOM);
			sprintf(buf, "HELL: %s looted %s at %d.", GET_NAME(ch), 
				t_obj->short_description, ch->in_room->number);
			mudlog(buf, CMP, 0, TRUE);
			extract_obj(t_obj);
			return 1;
		    }   
		}
	    }
	    continue;
	}	
	for (i = 0; items[i].o_vnum != -1; i++) {
	    if (items[i].o_vnum == GET_OBJ_VNUM(obj)) {
		act("$n takes $p.", FALSE, ch, obj, t_obj, TO_ROOM);
		sprintf(buf, "HELL: %s retrieved %s at %d.", GET_NAME(ch), 
			obj->short_description, ch->in_room->number);
		mudlog(buf, CMP, 0, TRUE);
		extract_obj(obj);
		return 1;
	    }   
	}
    }

    if (!FIGHTING(ch) && !HUNTING(ch) && !AFF_FLAGGED(ch, AFF_CHARM)) {
	act("$n vanishes into the mouth of an interplanar conduit.",
	    FALSE, ch, 0, 0, TO_ROOM);
	extract_char(ch, 1);
	return 1;
    }

    if ( GET_MOB_VNUM(ch) == H_REGULATOR ) {

	if ( GET_MANA(ch) < 100 ) {
	    act("$n vanishes into the mouth of an interplanar conduit.",
		FALSE, ch, 0, 0, TO_ROOM);
	    extract_char(ch, 1);
	    return 1;
	}

	for ( vict = ch->in_room->people; vict; vict = vict->next_in_room ) {

	    if ( vict == ch )
		continue;

	    // REGULATOR doesn't want anyone attacking him
	    if ( !IS_DEVIL(vict) && ch == FIGHTING(vict) ) {

		if ( ! ( devil = read_mobile(H_SPINED) ) ) {
		    slog("SYSERR: HH REGULATOR failed to load H_SPINED for defense.");
		    // set mana to zero so he will go away on the next loop
		    GET_MANA(ch) = 0;
		    return 1;
		}

		char_to_room(devil, ch->in_room);
		act("$n gestures... A glowing conduit flashes into existence!", FALSE, ch, 0, vict, TO_ROOM);
		act("...$n leaps out and attacks $N!", FALSE, devil, 0, vict, TO_NOTVICT);
		act("...$n leaps out and attacks you!", FALSE, devil, 0, vict, TO_VICT);
		
		stop_fighting(vict);
		hit(devil, vict, TYPE_UNDEFINED);
		WAIT_STATE(vict, 1 RL_SEC);

		return 1;
	    }
		
		
	    if ( IS_DEVIL(vict) && IS_NPC(vict) && 
		 GET_HIT(vict) < ( GET_MAX_HIT(vict) - 500 ) ) {

		act("$n opens a conduit of streaming energy to $N!\r\n"
		    "...$N's wounds appear to regenerate!", FALSE, ch, 0, vict, TO_ROOM);

		GET_HIT(vict) = MIN(GET_MAX_HIT(vict), GET_HIT(vict) + 500);
		GET_MANA(ch) -= 100;
		return 1;
	    }

	}		    
		
    }
	
    return 0;
}

#define BLADE_VNUM 16203
#define ARIOCH_LAIR 16284
#define PENTAGRAM_ROOM 15437
#define ARIOCH_LEAVE_MSG "A glowing portal spins into existance behind $n, who is then drawn backwards into the mouth of the conduit, and out of this plane.  The portal then spins down to a singular point and disappears."
#define ARIOCH_ARRIVE_MSG "A glowing portal spins into existance before you, and you see a dark figure approaching from the depths.  $n steps suddenly from the mouth of the conduit, which snaps shut behind $m."

SPECIAL(arioch)
{
    struct obj_data *blade = NULL, *obj = NULL;
    struct room_data *rm = NULL;
    struct char_data *vict = NULL;
    int i;

    if (cmd)
	return 0;

    if (ch->in_room->zone->number != 162) {
    
	if (!HUNTING(ch) && !FIGHTING(ch)) {
    
	    for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
		if (IS_CORPSE(obj)) {
		    for (blade = obj->contains; blade; blade = blade->next_content) {
			for (i = 0; items[i].o_vnum != -1; i++) {
			    if (BLADE_VNUM == GET_OBJ_VNUM(blade)) {
				act("$n takes $p from $P.", FALSE, ch, blade, obj, TO_ROOM);
				sprintf(buf, "HELL: %s looted %s at %d.", GET_NAME(ch), 
					blade->short_description, ch->in_room->number);
				mudlog(buf, CMP, 0, TRUE);
				if (!GET_EQ(ch, WEAR_WIELD)) {
				    obj_from_obj(blade);
				    obj_to_char(blade, ch);
				} else
				    extract_obj(blade);
				return 1;
			    }   
			}
		    }
		}	
		if (BLADE_VNUM == GET_OBJ_VNUM(obj)) {
		    act("$n takes $p.", FALSE, ch, obj, obj, TO_ROOM);
		    sprintf(buf, "HELL: %s retrieved %s at %d.", GET_NAME(ch), 
			    obj->short_description, ch->in_room->number);
		    mudlog(buf, CMP, 0, TRUE);
		    if (!GET_EQ(ch, WEAR_WIELD)) {
			obj_from_room(obj);
			obj_to_char(obj, ch);
		    } else
			extract_obj(obj);
		    return 1;
		}   
	    }
	    act(ARIOCH_LEAVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
	    char_from_room(ch);
	    char_to_room(ch, real_room(ARIOCH_LAIR));
	    act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
	    return 1;
	}
	if (GET_HIT(ch) < 800) {
	    act(ARIOCH_LEAVE_MSG,FALSE,ch,0,0,TO_ROOM);
	    char_from_room(ch);
	    char_to_room(ch, real_room(ARIOCH_LAIR));
	    act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
	    return 1;
	}
	return 0;
    }

    if (GET_EQ(ch, WEAR_WIELD))
	return 0;

    for (blade = object_list; blade; blade = blade->next) {
	if (GET_OBJ_VNUM(blade) == BLADE_VNUM &&
	    (((rm = blade->in_room) && !ROOM_FLAGGED(rm, SAFE_ROOM_BITS)) ||
	     (((vict = blade->carried_by) || (vict = blade->worn_by)) &&
	      !ROOM_FLAGGED(vict->in_room, SAFE_ROOM_BITS) &&
	      (!IS_NPC(vict) || shop_keeper != GET_MOB_SPEC(vict)) &&
	      !PRF_FLAGGED(vict, PRF_NOHASSLE) &&
	      CAN_SEE(ch, vict)))) {
	    if (vict) {
		HUNTING(ch) = vict;
		rm = vict->in_room;
	    }
	    act(ARIOCH_LEAVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
	    char_from_room(ch);
	    char_to_room(ch, rm);
	    act(ARIOCH_ARRIVE_MSG, FALSE, ch, 0, 0, TO_ROOM);
	    sprintf(buf, "HELL: Arioch ported to %s@%d",
		    vict ? GET_NAME(vict) : "Nobody", rm->number);
	    mudlog(buf, CMP, 0, TRUE);
	    return 1;
	}
    }
    return 0;
}
	


#undef H_SPINED
#undef H_ABISHAI
#undef H_BEARDED
#undef H_BARBED
#undef H_BONE
#undef H_HORNED
#undef H_REGULATOR
#undef SAFE_ROOM_BITS
