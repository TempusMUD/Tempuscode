/* ************************************************************************
*   File: castle.c                                      Part of CircleMUD *
*  Usage: Special procedures for King's Castle area                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se)     *
*  Coded by Sapowox (d90-jkr@nada.kth.se)                                 *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: duke.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "fight.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
void set_local_time(struct zone_data *zone,struct time_info_data *local_time);

/* IMPORTANT!
   The below defined number is the zone number of the Kings Castle.
   Change it to apply to your chosen zone number. The default zone
   number (On Alex and Alfa) is 80 (That is rooms and mobs have numbers
   in the 8000 series... */

#define Z_DUKES_C 152


/**********************************************************************\
|* Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se) *|
|* Coded by Sapowox (d90-jkr@nada.kth.se)                             *|
\**********************************************************************/

#define R_ROOM(zone, num) (real_room(((zone)*100)+(num)))

#define EXITN(room, door)     (room->dir_option[door])

#define CASTLE_ITEM(item) (Z_DUKES_C*100+(item))
/* note that castle_item() works necesarrily only for mobs and objs 
   our castle covers rooms from 15200-15499				*/


/* Routine member_of_staff */
/* Used to see if a character is a member of the castle staff */
/* Used mainly by BANZAI:ng NPC:s */
int member_of_staff(struct char_data * chChar)
{
    int ch_num;

    if (!IS_NPC(chChar))
	return (FALSE);

    ch_num = GET_MOB_VNUM(chChar);
    return (ch_num == CASTLE_ITEM(1) ||
	    (ch_num > CASTLE_ITEM(2) && ch_num < CASTLE_ITEM(15)) ||
	    (ch_num > CASTLE_ITEM(15) && ch_num < CASTLE_ITEM(18)) ||
	    (ch_num > CASTLE_ITEM(18) && ch_num < CASTLE_ITEM(30)));
}


/* Function member_of_royal_guard */
/* Returns TRUE if the character is a guard on duty, otherwise FALSE */
/* Used by Peter the captain of the royal guard */
int member_of_royal_guard(struct char_data * chChar)
{
    int ch_num;

    if (!chChar || !IS_NPC(chChar))
	return FALSE;

    ch_num = GET_MOB_VNUM(chChar);
    return (ch_num == CASTLE_ITEM(3) ||
	    ch_num == CASTLE_ITEM(6) ||
	    (ch_num > CASTLE_ITEM(7) && ch_num < CASTLE_ITEM(12)) ||
	    (ch_num > CASTLE_ITEM(23) && ch_num < CASTLE_ITEM(26)));
}


/* Function find_npc_by_name */
/* Returns a pointer to an npc by the given name */
/* Used by Tim and Tom */
struct char_data *find_npc_by_name(struct char_data * chAtChar, char *pszName,
				   int iLen)
{
    CharacterList::iterator it = chAtChar->in_room->people.begin();
    for( ; it != chAtChar->in_room->people.end(); ++it ) {
    	if (IS_NPC((*it)))
    	    if (!strncmp(pszName, (*it)->player.short_descr, iLen))
        		return ((*it));
    }
    return NULL;
}


/* Function find_guard */
/* Returns the pointer to a guard on duty. */
/* Used by Peter the Captain of the Royal Guard */
struct char_data *find_guard(struct char_data * chAtChar)
{

    CharacterList::iterator it = chAtChar->in_room->people.begin();
    for( ; it != chAtChar->in_room->people.end(); ++it ) {
    	if (!FIGHTING((*it)) && member_of_royal_guard((*it)))
    	    return (*it);
    }
    return NULL;
}


/* Function get_victim */
/* Returns a pointer to a randomly chosen character in the same room, */
/* fighting someone in the castle staff... */
/* Used by BANZAII-ing characters and King Welmar... */
struct char_data *get_victim(struct char_data * chAtChar)
{

    int iNum_bad_guys = 0, iVictim;
    CharacterList::iterator it = chAtChar->in_room->people.begin();
    for( ; it != chAtChar->in_room->people.end(); ++it ) {
    	if (FIGHTING((*it)) &&
    	    member_of_staff(FIGHTING((*it))))
    	    iNum_bad_guys++;
    }
    if (!iNum_bad_guys)
    	return NULL;

    iVictim = number(0, iNum_bad_guys);   /* How nice, we give them a chance */
    if (!iVictim)
    	return NULL;

    iNum_bad_guys = 0;
    it = chAtChar->in_room->people.begin();
    for( ; it != chAtChar->in_room->people.end(); ++it ) {
    	if (FIGHTING((*it)) &&
    	    member_of_staff(FIGHTING((*it))) &&
    	    ++iNum_bad_guys == iVictim)
    	    return (*it);
    }
    return NULL;
}


/* Function banzaii */
/* Makes a character banzaii on attackers of the castle staff */
/* Used by Guards, Tim, Tom, Dick, David, Peter, Master, King and Guards */
int banzaii(struct char_data * ch)
{

    struct char_data *chOpponent = NULL;

    if (!AWAKE(ch) || ch->getPosition() == POS_FIGHTING)
	return FALSE;

    if ((chOpponent = get_victim(ch))) {
	act("$n roars: 'Protect the Duchy of the Great Duke Araken!  BANZAIIII!!!'",
	    FALSE, ch, 0, 0, TO_ROOM);
	hit(ch, chOpponent, TYPE_UNDEFINED);
	return TRUE;
    }
    return FALSE;
}


/* Function do_npc_rescue */
/* Makes ch_hero rescue ch_victim */
/* Used by Tim and Tom */
int do_npc_rescue(struct char_data * ch_hero, struct char_data * ch_victim)
{

    struct char_data *ch_bad_guy = NULL;
    CharacterList::iterator it = ch_hero->in_room->people.begin();
    for( ; it != ch_hero->in_room->people.end(); ++it ) {
        if( FIGHTING((*it)) == ch_victim )
            ch_bad_guy = *it;
    }
    if (ch_bad_guy) {
    	if (ch_bad_guy == ch_hero)
    	    return FALSE;             /* NO WAY I'll rescue the one I'm fighting! */
    	act("You bravely rescue $N.\r\n", FALSE, ch_hero, 0, ch_victim, TO_CHAR);
    	act("You are rescued by $N, your loyal friend!\r\n",
    	    FALSE, ch_victim, 0, ch_hero, TO_CHAR);
    	act("$n heroically rescues $N.", FALSE, ch_hero, 0, ch_victim, TO_NOTVICT);

    	if (FIGHTING(ch_bad_guy))
    	    stop_fighting(ch_bad_guy);
    	if (FIGHTING(ch_hero))
    	    stop_fighting(ch_hero);

    	set_fighting(ch_hero, ch_bad_guy, TRUE);
    	set_fighting(ch_bad_guy, ch_hero, TRUE);
    	return TRUE;
    }
    return FALSE;
}


/* Procedure to block a person trying to enter a room. */
/* Used by Tim/Tom at Kings bedroom and Dick/David at treasury */
int block_way(struct char_data * ch, struct char_data * guard, int cmd, 
	      char *arg, int iIn_room, int iProhibited_direction)
{

    if (cmd != ++iProhibited_direction || (ch->player.short_descr &&
					   !strncmp(ch->player.short_descr, "Duke Araken", 11)))
	return FALSE;

    if (!CAN_SEE(guard, ch) || !AWAKE(guard))
	return FALSE;

    if ((ch->in_room == real_room(iIn_room)) && (cmd == iProhibited_direction)) {
	if (!member_of_staff(ch))
	    act("$N roars at $n and pushes $m back.",
		FALSE, ch, 0, guard, TO_ROOM);
	act("$N roars: 'Entrance is Prohibited!', and pushes you back.", FALSE, 
	    ch, 0, guard, TO_CHAR);
	return (TRUE);
    }
    return FALSE;
}


/* Routine to check if an object is trash... */
/* Used by James the Butler and the Cleaning Lady */
int is_trash(struct obj_data * i)
{
    // dont get sigilized items
    if (GET_OBJ_SIGIL_IDNUM(i))
	return FALSE;
    else if (GET_OBJ_VNUM(i) == QUAD_VNUM)
	return FALSE;

    else if (IS_SET(i->obj_flags.wear_flags, ITEM_WEAR_TAKE) &&
	     ((GET_OBJ_TYPE(i) == ITEM_DRINKCON) || (GET_OBJ_COST(i) <= 50)))
	return TRUE;
    else
	return FALSE;
}


/* Function fry_victim */
/* Finds a suitabe victim, and cast some _NASTY_ spell on him */
/* Used by King Welmar */
void fry_victim(struct char_data * ch)
{

    struct char_data *tch;

    if (ch->points.mana < 10)
	return;

    /* Find someone suitable to fry ! */

    if (!(tch = get_victim(ch)))
	return;

    switch (number(0, 8)) {
    case 1:
    case 2:
    case 3:
	act("You raise your hand in a dramatical gesture.", 1, ch, 0, 0, TO_CHAR);
	act("$n raises $s hand in a dramatical gesture.", 1, ch, 0, 0, TO_ROOM);
	cast_spell(ch, tch, 0, SPELL_COLOR_SPRAY);
	break;
    case 4:
    case 5:
	act("You concentrate and mumble to yourself.", 1, ch, 0, 0, TO_CHAR);
	act("$n concentrates, and mumbles to $mself.", 1, ch, 0, 0, TO_ROOM);
	cast_spell(ch, tch, 0, SPELL_HARM);
	break;
    case 6:
    case 7:
	act("You look deeply into the eyes of $N.", 1, ch, 0, tch, TO_CHAR);
	act("$n looks deeply into the eyes of $N.", 1, ch, 0, tch, TO_NOTVICT);
	act("You see an ill-boding flame in the eye of $n.", 1, ch, 0, tch, TO_VICT);
	cast_spell(ch, tch, 0, SPELL_FIREBALL);
	break;
    default:
	if (!number(0, 1))
	    cast_spell(ch, ch, 0, SPELL_HEAL);
	break;
    }

    ch->points.mana -= 10;

    return;
}


SPECIAL(duke_araken)
{
    struct time_info_data local_time;
    ACMD(do_gen_door);
  
    static char *monolog[] = {
	"$n proclaims 'Primus in regnis Geticis coronam'.",
	"$n proclaims 'regiam gessi, subiique regis'.",
	"$n proclaims 'munus et mores colui sereno'.",
	"$n proclaims 'principe dignos'."};

    static char bedroom_path[] = "s33004o1c1S.";

    static char throne_path[] = "W3o3cG52211rg.";

    static char monolog_path[] = "ABCDPPPP.";

    static char *path;
    static int index;
    static bool move = false;

    set_local_time(ch->in_room->zone, &local_time);

    if (!move) {
	if (local_time.hours==8 && ch->in_room == R_ROOM(Z_DUKES_C, 51)) {
	    move = true;
	    path = throne_path;
	    index = 0;
	} else if (local_time.hours==21 && ch->in_room == R_ROOM(Z_DUKES_C, 17)) {
	    move = true;
	    path = bedroom_path;
	    index = 0;
	} else if (local_time.hours==12 && ch->in_room == R_ROOM(Z_DUKES_C, 17)) {
	    move = true;
	    path = monolog_path;
	    index = 0;
	}
    }
    if (cmd || (ch->getPosition() < POS_SLEEPING) ||
	(ch->getPosition() == POS_SLEEPING && !move))
	return FALSE;

    if (ch->getPosition() == POS_FIGHTING) {
	fry_victim(ch);
	return FALSE;
    } else if (banzaii(ch))
	return FALSE;

    if (!move)
	return FALSE;

    switch (path[index]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
	perform_move(ch, path[index] - 0, MOVE_NORM, 1);
	break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
	act(monolog[path[index] - 'A'], FALSE, ch, 0, 0, TO_ROOM);
	break;
    case 'P':
	break;
    case 'W':
	ch->setPosition( POS_STANDING );
	act("$n awakens and stands up.", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 'S':
	ch->setPosition( POS_SLEEPING );
	act("$n lies down on $s beautiful bed and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 'r':
	ch->setPosition( POS_SITTING );
	act("$n sits down on his great throne.", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 's':
	ch->setPosition( POS_STANDING );
	act("$n stands up.", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 'G':
	act("$n says 'Good morning, trusted friends.'", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 'g':
	act("$n says 'Good morning, dear subjects.'", FALSE, ch, 0, 0, TO_ROOM);
	break;

    case 'o':
	do_gen_door(ch, "door", 0, SCMD_UNLOCK);
	do_gen_door(ch, "door", 0, SCMD_OPEN);
	break;

    case 'c':
	do_gen_door(ch, "door", 0, SCMD_CLOSE);
	do_gen_door(ch, "door", 0, SCMD_LOCK);
	break;

    case '.':
	move = false;
	break;
    }

    index++;
    return FALSE;
}


/* Function training_master */
/* Acts actions to the training room, if his students are present */
/* Also allowes warrior-char_class to practice */
/* Used by the Training Master */
SPECIAL(training_master)
{

    struct char_data *pupil1, *pupil2, *tch;

    if (!AWAKE(ch) || (ch->getPosition() == POS_FIGHTING))
	return FALSE;

    if (cmd)
	return FALSE;

    if (!banzaii(ch) && !number(0, 2)) {
	if ((pupil1 = find_npc_by_name(ch, "Brian", 5)) &&
	    (pupil2 = find_npc_by_name(ch, "Mick", 4)) &&
	    (!FIGHTING(pupil1) && !FIGHTING(pupil2))) {
	    if (number(0, 1)) {
		tch = pupil1;
		pupil1 = pupil2;
		pupil2 = tch;
	    }
	    switch (number(0, 7)) {
	    case 0:
		act("$n hits $N on $s head with a powerful blow.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("You hit $N on $s head with a powerful blow.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("$n hits you on your head with a powerful blow.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    case 1:
		act("$n hits $N in $s chest with a thrust.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("You manage to thrust $N in the chest.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("$n manages to thrust you in your chest.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    case 2:
		send_to_char("You command your pupils to bow\r\n.", ch);
		act("$n commands his pupils to bow.", FALSE, ch, 0, 0, TO_ROOM);
		act("$n bows before $N.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("$N bows before $n.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("You bow before $N, who returns your gesture.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("You bow before $n, who returns your gesture.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    case 3:
		act("$N yells at $n, as he fumbles and drops his sword.",
		    FALSE, pupil1, 0, ch, TO_NOTVICT);
		act("$n quickly picks up his weapon.", FALSE, pupil1, 0, 0, TO_ROOM);
		act("$N yells at you, as you fumble, losing your weapon.",
		    FALSE, pupil1, 0, ch, TO_CHAR);
		send_to_char("You quickly pick up your weapon again.", pupil1);
		act("You yell at $n, as he fumbles, losing his weapon.",
		    FALSE, pupil1, 0, ch, TO_VICT);
		break;
	    case 4:
		act("$N tricks $n, and slashes him across the back.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("$N tricks you, and slashes you across your back.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("You trick $n, and quickly slash him across his back.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    case 5:
		act("$n lunges a blow at $N but $N parries skillfully.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("You lunge a blow at $N but $E parries skillfully.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("$n lunges a blow at you, but you skillfully parry it.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    case 6:
		act("$n clumsily tries to kick $N, but misses.",
		    FALSE, pupil1, 0, pupil2, TO_NOTVICT);
		act("You clumsily miss $N with your poor excuse for a kick.",
		    FALSE, pupil1, 0, pupil2, TO_CHAR);
		act("$n fails an unusually clumsy attempt at kicking you.",
		    FALSE, pupil1, 0, pupil2, TO_VICT);
		break;
	    default:
		send_to_char("You show your pupils an advanced technique.", ch);
		act("$n shows his pupils an advanced technique.",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	    }
	}
    }
    return FALSE;
}


SPECIAL(tom)
{

    struct char_data *tom = (struct char_data *) me; 
    struct char_data *king, *tim;

    ACMD(do_follow);

    if (!AWAKE(ch))
	return FALSE;

    if ((!cmd) && (king = find_npc_by_name(ch, "Duke Araken", 11))) {
	if (!ch->master)
	    do_follow(ch, "Duke Araken", 0, 0);
	if (FIGHTING(king))
	    do_npc_rescue(ch, king);
    }
    if (!cmd)
	if ((tim = find_npc_by_name(ch, "Tim", 3)))
	    if (FIGHTING(tim) && 2 * GET_HIT(tim) < GET_HIT(ch))
		do_npc_rescue(ch, tim);

    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    return block_way(ch, tom, cmd, arg, CASTLE_ITEM(49), 1);
}


SPECIAL(tim)
{

    struct char_data *tim = (struct char_data *) me; 
    struct char_data *king, *tom;

    ACMD(do_follow);

    if (!AWAKE(ch))
	return FALSE;

    if ((!cmd) && (king = find_npc_by_name(ch, "Duke Araken", 11))) {
	if (!ch->master)
	    do_follow(ch, "Duke Araken", 0, 0);
	if (FIGHTING(king))
	    do_npc_rescue(ch, king);
    }
    if (!cmd)
	if ((tom = find_npc_by_name(ch, "Tom", 3)))
	    if (FIGHTING(tom) && 2 * GET_HIT(tom) < GET_HIT(ch))
		do_npc_rescue(ch, tom);

    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    return block_way(ch, tim, cmd, arg, CASTLE_ITEM(49), 1);
}


/* Routine for James the Butler */
/* Complains if he finds any trash... */
SPECIAL(James)
{

    struct obj_data *i;

    if (cmd || !AWAKE(ch) || (ch->getPosition() == POS_FIGHTING))
	return (FALSE);

    for (i = ch->in_room->contents; i; i = i->next_content)
	if (is_trash(i)) {
	    act("$n says: 'My oh my!  I ought to fire that lazy cleaning woman!'",
		FALSE, ch, 0, 0, TO_ROOM);
	    act("$n picks up a piece of trash.", FALSE, ch, 0, 0, TO_ROOM);
	    obj_from_room(i);
	    obj_to_char(i, ch);
	    return TRUE;
	} else
	    return FALSE;

    return FALSE;
}


/* Routine for the Cleaning Woman */
/* Picks up any trash she finds... */
SPECIAL(cleaning)
{
    struct obj_data *i, *next;

    if (cmd || !AWAKE(ch))
	return (FALSE);

    for (i = ch->in_room->contents; i; i = next) {
	next = i->next_content; 
	if (is_trash(i)) {
	    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
	    obj_from_room(i);
	    obj_to_char(i, ch);
	    return TRUE;
	} else
	    return FALSE;
    }
    return FALSE;
}


/* Routine CastleGuard */
/* Standard routine for ordinary castle guards */
SPECIAL(CastleGuard)
{

    if (cmd || !AWAKE(ch) || (ch->getPosition() == POS_FIGHTING))
	return FALSE;

    return (banzaii(ch));
}
SPECIAL(sleeping_soldier)
{
    if (cmd || (ch->getPosition() == POS_FIGHTING))
	return FALSE;
    if (!AWAKE(ch))
	switch (number(0, 60)) {
	case 0:
	    act("$n farts in $s sleep.", FALSE, ch, 0, 0, TO_ROOM);
	    break;
	case 1:
	    act("$n rolls over fitfully.", FALSE, ch, 0, 0, TO_ROOM);
	    break;
	case 2:
	    act("$n moans loudly.", FALSE, ch, 0, 0, TO_ROOM);
	    break;
	case 3:
	    act("$n begins to exhibit Rapid Eye Movement.", TRUE, ch, 0, 0, TO_ROOM);
	    break;
	case 4:
	    act("$n drools all over the place.", TRUE, ch, 0, 0, TO_ROOM);
	    break;
	}
    if (!AWAKE(ch))
	return 0;

    return (banzaii(ch));
}
SPECIAL(lounge_soldier)
{
    if (cmd || !AWAKE(ch) || (ch->getPosition() == POS_FIGHTING))
	return FALSE;
  
    switch(number(0, 60)) {
    case 0:
	act("$n grabs a dirty magazine and starts flipping through the pages.", 
	    TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You grab a dirty mag.\r\n", ch);
	break;
    case 1:
	act("$n counts up this month's pay and scowls.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You count your pay and scowl.\r\n", ch);
	break;
    case 2:
	act("$n wonders when $s next leave will be.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You wonder about leave.\r\n", ch);
	break;
    case 3:
	act("$n starts biting $s fingernails.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You bite your fingernails.\r\n", ch);
	break;
    case 4:
	act("$n starts biting $s toenails.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You bite your toenails.\r\n", ch);
	break;
    case 5:
	act("$n flexes $s muscles.",TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You flex.\r\n", ch);
	break;
    case 6:
	act("$n rolls up a blunt.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You roll a blunt.\r\n", ch);
	break;
    case 7:
	act("$n starts shadowboxing.", FALSE, ch, 0, 0, TO_ROOM);
	send_to_char("You shadowbox.\r\n", ch);
	break;
    case 8:
	act("$n scratches $s butt.", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("You scratch your butt.\r\n", ch);
	break;
    }
    return (banzaii(ch));
}


/* Routine DicknDave */
/* Routine for the guards Dick and David */
SPECIAL(DicknDavid)
{

    struct char_data * guard = (struct char_data *) me;
    if (!AWAKE(ch))
	return (FALSE);

    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    return (block_way(ch, guard, cmd, arg, 15453, 3));
}
SPECIAL(chess_guard_no_west)
{

    struct char_data * guard = (struct char_data *) me;
  
    if (!AWAKE(ch))
	return FALSE;
    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    if (GET_LEVEL(ch) > 39)
	return (block_way(ch, guard, cmd, arg, 15240, 3));
    else
	return FALSE;
}
SPECIAL(chess_guard_no_east)
{

    struct char_data * guard = (struct char_data *) me;
    if (!AWAKE(ch))
	return FALSE;
    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    if (GET_LEVEL(ch) > 39)
	return (block_way(ch, guard, cmd, arg, 15248, 1));
    else
	return FALSE;
}
SPECIAL(armory_guard)
{

    struct char_data * guard = (struct char_data *) me;
    if (!AWAKE(ch))
	return FALSE;
    if (!cmd && ch->getPosition() != POS_FIGHTING)
	banzaii(ch);

    return (block_way(ch, guard, cmd, arg, 15357, 1));
}
SPECIAL(armory_person)
{
  
    struct char_data * guard = (struct char_data *) me;
    if (!cmd || IS_NPC(ch))
	return FALSE;
  
    if (!CAN_SEE(guard, ch) || FIGHTING(guard))
	return FALSE;
  
    act("$n screams, 'This is a RESTRICTED AREA!!!'",FALSE,guard,0,0,TO_ROOM);
    hit(guard, ch, TYPE_UNDEFINED);
    return TRUE;

}

/* Routine peter */
/* Routine for Captain of the Guards. */
SPECIAL(peter)
{

    struct char_data *ch_guard;

    if (cmd || !AWAKE(ch) || ch->getPosition() == POS_FIGHTING)
	return (FALSE);

    if (banzaii(ch))
	return FALSE;

    if (!(number(0, 3)) && (ch_guard = find_guard(ch)))
	switch (number(0, 5)) {
	case 0:
	    act("$N comes sharply into attention as $n inspects $M.",
		FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("$N comes sharply into attention as you inspect $M.",
		FALSE, ch, 0, ch_guard, TO_CHAR);
	    act("You go sharply into attention as $n inspects you.",
		FALSE, ch, 0, ch_guard, TO_VICT);
	    break;
	case 1:
	    act("$N looks very small, as $n roars at $M.",
		FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("$N looks very small as you roar at $M.",
		FALSE, ch, 0, ch_guard, TO_CHAR);
	    act("You feel very small as $N roars at you.",
		FALSE, ch, 0, ch_guard, TO_VICT);
	    break;
	case 2:
	    act("$n gives $N some Royal directions.",
		FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("You give $N some Royal directions.",
		FALSE, ch, 0, ch_guard, TO_CHAR);
	    act("$n gives you some Royal directions.",
		FALSE, ch, 0, ch_guard, TO_VICT);
	    break;
	case 3:
	    act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
	    act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("$n growls: 'Those boots need polishing!'",
		FALSE, ch, 0, ch_guard, TO_ROOM);
	    act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
	    break;
	case 4:
	    act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
	    act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("$n growls: 'Straighten that collar!'",
		FALSE, ch, 0, ch_guard, TO_ROOM);
	    act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
	    break;
	default:
	    act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
	    act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
	    act("$n growls: 'That chain mail looks rusty!  CLEAN IT !!!'",
		FALSE, ch, 0, ch_guard, TO_ROOM);
	    act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
	    break;
	}

    return FALSE;
}


/* Procedure for Jerry and Michael in x08 of King's Castle.      */
/* Code by Sapowox modified by Pjotr.(Original code from Master) */

SPECIAL(jerry)
{

    struct char_data *gambler1, *gambler2, *tch;

    if (!AWAKE(ch) || (ch->getPosition() == POS_FIGHTING))
	return FALSE;

    if (cmd)
	return FALSE;

    if (!banzaii(ch) && !number(0, 2)) {
	if ((gambler1 = ch) &&
	    (gambler2 = find_npc_by_name(ch, "Michael", 7)) &&
	    (!FIGHTING(gambler1) && !FIGHTING(gambler2))) {
	    if (number(0, 1)) {
		tch = gambler1;
		gambler1 = gambler2;
		gambler2 = tch;
	    }
	    switch (number(0, 5)) {
	    case 0:
		act("$n rolls the dice and cheers loudly at the result.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You roll the dice and cheer. GREAT!",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n cheers loudly as $e rolls the dice.",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    case 1:
		act("$n curses the Goddess of Luck roundly as he sees $N's roll.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You curse the Goddess of Luck as $N rolls.",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n swears angrily. You are in luck!",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    case 2:
		act("$n sighs loudly and gives $N some gold.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You sigh loudly at the pain of having to give $N some gold.",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n sighs loudly as $e gives you your rightful win.",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    case 3:
		act("$n smiles remorsefully as $N's roll tops his.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You smile sadly as you see that $N beats you. Again.",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n smiles remorsefully as your roll tops his.",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    case 4:
		act("$n excitedly follows the dice with his eyes.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You excitedly follow the dice with your eyes.",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n excitedly follows the dice with his eyes.",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    default:
		act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
		    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
		act("You say 'Well, my luck has to change soon' and shake the dice.",
		    FALSE, gambler1, 0, gambler2, TO_CHAR);
		act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
		    FALSE, gambler1, 0, gambler2, TO_VICT);
		break;
	    }
	}
    }
    return FALSE;
}

SPECIAL(dukes_chamber)
{
    struct room_data *other_room;
    struct room_direction_data *back = 0;

    if (!CMD_IS("pull") && !CMD_IS("push") && !CMD_IS("get") && !CMD_IS("take")
	&& !CMD_IS("open"))
	return 0;
    skip_spaces(&argument);
  
    if (strncasecmp(argument, "book", 4) &&
	strncasecmp(argument, "shelf", 5) &&
	strncasecmp(argument, "lever", 5) &&
	strncasecmp(argument, "on lever", 8) &&
	strncasecmp(argument, "on shelf", 8) &&
	strncasecmp(argument, "on book", 7))
	return 0;

    if (CMD_IS("open") && !strncasecmp(argument, "book", 4)) {
	send_to_char("It's securely lodged in the bookshelf.\r\n", ch);
	return 1;
    }
    if ((other_room = EXIT(ch, WEST)->to_room) != NULL)
	if ((back = other_room->dir_option[rev_dir[WEST]]))
	    if (back->to_room != ch->in_room) {
		back = 0;
		send_to_char(" back= 0", ch);
	    }

    send_to_char("You pull firmly on the protruding book...\r\n", ch);
    act("$n pulls on a book in the bookshelf.", TRUE, ch, 0, 0, TO_ROOM);

    if (IS_SET(EXIT(ch, WEST)->exit_info, EX_CLOSED)) {
	send_to_room("A small section of the bookshelf rotates open, revealing a small room\r\n"
		     "to the west.\r\n", ch->in_room);
  
	TOGGLE_BIT(EXITN(ch->in_room, WEST)->exit_info, EX_CLOSED);
	if (back && IS_SET(back->exit_info, EX_CLOSED)) {
	    TOGGLE_BIT(EXITN(other_room, rev_dir[WEST])->exit_info, EX_CLOSED);
	    send_to_room("The east wall rotates open, revealing a large chamber.\r\n", other_room);
	}
	return 1;
    } else {
	send_to_room("A small section of the bookshelf rotates closed.\r\n", ch->in_room);
  
	TOGGLE_BIT(EXITN(ch->in_room, WEST)->exit_info, EX_CLOSED);
	if (back && !IS_SET(back->exit_info, EX_CLOSED)) {
	    TOGGLE_BIT(EXITN(other_room, rev_dir[WEST])->exit_info, EX_CLOSED);
	    send_to_room("The east wall rotates closed.\r\n", other_room);
	}
	return 1;
    }
}
SPECIAL(wiz_library)
{
    struct room_data *book_room = real_room(15494);
    struct obj_data *book;
  
    if (!CMD_IS("look") && !CMD_IS("examine") && !CMD_IS("get") && !CMD_IS("take"))
	return 0;

    for (book = book_room->contents; book; book = book->next_content)
	if (GET_OBJ_VNUM(book) == 15250)
	    break;

    if (!book || GET_OBJ_VNUM(book) != 15250)
	return 0;

    skip_spaces(&argument);

    if (CMD_IS("look") || CMD_IS("examine")) {
	if (strncasecmp(argument, "bookshelf", 9) &&
	    strncasecmp(argument, "books", 5)     &&
	    strncasecmp(argument, "shelf", 5)     &&
	    strncasecmp(argument, "scrolls", 7)   &&
	    strncasecmp(argument, "tablets", 7))
	    return 0;
	send_to_char("The bookshelf is filled with books, but one particular book near the\r\n"
		     "top shelf catches your eye.  It is odd, because the book is bound in\r\n"
		     "black leather, but it seems to glow faintly...\r\n", ch);
	act("$n examines the bookshelf.", TRUE, ch, 0, 0, TO_ROOM);
	return 1;
    }
    if (CMD_IS("get") || CMD_IS("take")) {
	if (strncasecmp(argument, "book", 4))
	    return 0;
	act("You take $p from the shelf.", FALSE, ch, book, 0, TO_CHAR);
	act("$n takes $p from the shelf.", TRUE, ch, book, 0, TO_ROOM);
	obj_from_room(book);
	obj_to_char(book, ch);
	return 1;
    }
    return 0;
}
SPECIAL(book_int)
{
    int book_number = 15250;
    struct obj_data *book;

    if (!CMD_IS("read"))
	return 0;
  
    for (book = ch->carrying; book; book = book->next_content)
	if (GET_OBJ_VNUM(book) == book_number)
	    break;
  
    if (!book || GET_OBJ_VNUM(book) != book_number)
	return 0;

    skip_spaces(&argument);

    if (strncasecmp(argument, "book", 4) &&
	strncasecmp(argument, "cognizance", 10))
	return 0;

    send_to_char("You study the pages of the ancient book.\r\n", ch);
    act("$n reads $p.", TRUE, ch, book, 0, TO_ROOM);

    if (ch->real_abils.intel >= 19)
	send_to_char("You do not feel enlightened.\r\n", ch);
    else {
	send_to_char("A wave of enlightenment passes over you.\r\n", ch);
	act("A bright light appears above the head of $n", FALSE, ch, 0, 0, TO_ROOM);
	ch->real_abils.intel++;
	save_char(ch, NULL);
	WAIT_STATE(ch, PULSE_VIOLENCE*2);
    }
    act("$p fades into a different reality, and is gone.", FALSE, ch, book, 0, TO_CHAR);
    obj_from_char(book);
    extract_obj(book);
    return 1;
}
