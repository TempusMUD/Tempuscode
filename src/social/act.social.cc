/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

//
// File: act.social.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;

/* extern functions */
char *fread_action(FILE * fl, int nr);

/* local globals */
static int list_top = -1;

struct social_messg {
    int act_nr;
    int hide;
    int min_victim_position;        /* Position of victim */

    /* No argument was supplied */
    char *char_no_arg;
    char *others_no_arg;

    /* An argument was there, and a victim was found */
    char *char_found;                /* if NULL, read no further, ignore args */
    char *others_found;
    char *vict_found;

    /* An argument was there, but no victim was found */
    char *not_found;

    /* The victim turned out to be the character */
    char *char_auto;
    char *others_auto;
}           *soc_mess_list = NULL;



int find_action(int cmd)
{
    int bot, top, mid;

    bot = 0;
    top = list_top;

    if (top < 0)
        return (-1);

    for (;;) {
        mid = (bot + top) >> 1;

        if (soc_mess_list[mid].act_nr == cmd)
            return (mid);
        if (bot >= top)
            return (-1);

        if (soc_mess_list[mid].act_nr > cmd)
            top = --mid;
        else
            bot = ++mid;
    }
}



ACMD(do_action)
{
    int act_nr;
    struct social_messg *action;
    struct char_data *vict = NULL;
    struct obj_data *obj = NULL;

    /*struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);*/

    if ((act_nr = find_action(cmd)) < 0) {
        send_to_char("That action is not supported.\r\n", ch);
        return;
    }
    action = &soc_mess_list[act_nr];

    if (action->char_found) {
        one_argument(argument, buf);
    } else {
        *buf = '\0';
    }
    if (!*buf) {
        send_to_char(action->char_no_arg, ch);
        send_to_char("\r\n", ch);
        act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
        return;
    }
    if (!(vict = get_char_room_vis(ch, buf))) {
        if (!(obj = get_obj_in_list_vis(ch, buf, ch->in_room->contents)) &&
            !(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
            act(action->not_found, action->hide, ch, 0, vict, TO_CHAR);
            return;
        } else {
            if ((vict = read_mobile(1599))) {    /* Social Thang */
                vict->player.short_descr = str_dup(obj->short_description);
                char_to_room(vict, ch->in_room);
            } else {
                act(action->not_found, action->hide, ch, 0, vict, TO_CHAR);
                slog("SYSERR: Social Thang 1599 cannot be read.");
                return;
            }
        }
    }
    if (vict == ch) {
        send_to_char(action->char_auto, ch);
        send_to_char("\r\n", ch);
        act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
    } else {
        if (vict->getPosition() < action->min_victim_position)
            act("$N is not in a proper position for that.",
                FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
        else {
            act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
            act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
            act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
        
        }
    }
    if (obj && IS_MOB(vict))
        vict->extract( FALSE );
}

ACMD(do_point)
{
    struct char_data *vict = NULL;
    struct obj_data *obj = NULL;
    int dir, i;

    skip_spaces(&argument);
    if (!*argument) {
        send_to_char("You point whereto?\r\n", ch);
        act("$n points in all directions, seemingly confused.",
            TRUE,ch,0,0,TO_ROOM);
        return;
    }

    if ((dir = search_block(argument, dirs, FALSE)) >= 0) {
        sprintf(buf, "$n points %sward.", dirs[dir]);
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        sprintf(buf, "You point %sward.", dirs[dir]);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if ((vict = get_char_room_vis(ch, argument))) {
        if (vict == ch) {
            send_to_char("You point at yourself.\r\n", ch);
            act("$n points at $mself.", TRUE, ch, 0, 0, TO_ROOM);
            return;
        }
        act("You point at $M.", FALSE, ch, 0, vict, TO_CHAR);
        act("$n points at $N.", TRUE, ch, 0, vict, TO_NOTVICT);
        act("$n points at you.", TRUE, ch, 0, vict, TO_VICT);
        return;
    }

    if ((obj = get_object_in_equip_vis(ch, argument, ch->equipment, &i)) ||
        (obj = get_obj_in_list_vis(ch, argument, ch->carrying)) ||
        (obj = get_obj_in_list_vis(ch, argument, ch->in_room->contents))) {
        act("You point at $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n points at $p.", TRUE, ch, obj, 0, TO_ROOM);
        return;
    }

    sprintf(buf, "You don't see any '%s' here.\r\n", argument);
    send_to_char(buf, ch);

}

  
ACMD(do_insult)
{
    struct char_data *victim;

    one_argument(argument, arg);

    if (*arg) {
        if (!(victim = get_char_room_vis(ch, arg)))
            send_to_char("Can't hear you!\r\n", ch);
        else {
            if (victim != ch) {
                sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
                send_to_char(buf, ch);

                switch (number(0, 2)) {
                case 0:
                    if (GET_SEX(ch) == SEX_MALE) {
                        if (GET_SEX(victim) == SEX_MALE)
                            act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
                        else
                            act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
                    } else {                /* Ch == Woman */
                        if (GET_SEX(victim) == SEX_MALE)
                            act("$n accuses you of having the smallest... (brain?)",
                                FALSE, ch, 0, victim, TO_VICT);
                        else
                            act("$n tells you that you'd lose a beauty contest against a troll.",
                                FALSE, ch, 0, victim, TO_VICT);
                    }
                    break;
                case 1:
                    act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
                    break;
                default:
                    act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
                    break;
                }                        /* end switch */

                act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
            } else {                        /* ch == victim */
                send_to_char("You feel insulted.\r\n", ch);
            }
        }
    } else
        send_to_char("I'm sure you don't want to insult *everybody*...\r\n", ch);
}


char *fread_action(FILE * fl, int nr)
{
    char buf[MAX_STRING_LENGTH], *rslt;

    fgets(buf, MAX_STRING_LENGTH, fl);
    if (feof(fl)) {
        fprintf(stderr, "fread_action - unexpected EOF near action #%d", nr);
        safe_exit(1);
    }
    if (*buf == '#')
        return (NULL);
    else {
        *(buf + strlen(buf) - 1) = '\0';
        CREATE(rslt, char, strlen(buf) + 1);
        strcpy(rslt, buf);
        return (rslt);
    }
}


#define MAX_SOCIALS 500

void boot_social_messages(void)
{
    FILE *fl;
    int nr, i, hide, min_pos, curr_soc = -1;
    char next_soc[100];
    struct social_messg temp;
    struct social_messg tmp_soc_mess_list[MAX_SOCIALS];

    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE, "r"))) {
        sprintf(buf, "Can't open socials file '%s'", SOCMESS_FILE);
        perror(buf);
        safe_exit(1);
    }
    /* count socials & allocate space */
    for (nr = 0; *cmd_info[nr].command != '\n'; nr++)
        if (cmd_info[nr].command_pointer == do_action)
            list_top++;

    /* now read 'em */
    for (;;) {
        fscanf(fl, " %s ", next_soc);
        if (*next_soc == '$')
            break;
        if ((nr = find_command(next_soc)) < 0) {
            sprintf(buf, "Unknown social '%s' in social file", next_soc);
            slog(buf);
        }
        if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
            fprintf(stderr, "Format error in social file near social '%s'\n",
                    next_soc);
            safe_exit(1);
        }
        /* read the stuff */
        curr_soc++;
        if (curr_soc >= MAX_SOCIALS) {
            slog("Too many socials.  Increase MAX_SOCIALS in act.social.c");
            safe_exit(1);
        }
        tmp_soc_mess_list[curr_soc].act_nr = nr;
        tmp_soc_mess_list[curr_soc].hide = hide;
        tmp_soc_mess_list[curr_soc].min_victim_position = min_pos;

        tmp_soc_mess_list[curr_soc].char_no_arg = fread_action(fl, nr);
        tmp_soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr);
        tmp_soc_mess_list[curr_soc].char_found = fread_action(fl, nr);

        /* if no char_found, the rest is to be ignored */
        if (!tmp_soc_mess_list[curr_soc].char_found) {
            tmp_soc_mess_list[curr_soc].others_found = NULL;
            tmp_soc_mess_list[curr_soc].vict_found   = NULL;
            tmp_soc_mess_list[curr_soc].not_found    = NULL;
            tmp_soc_mess_list[curr_soc].others_auto  = NULL;
            continue;
        }

        tmp_soc_mess_list[curr_soc].others_found = fread_action(fl, nr);
        tmp_soc_mess_list[curr_soc].vict_found = fread_action(fl, nr);
        tmp_soc_mess_list[curr_soc].not_found = fread_action(fl, nr);
        if ((tmp_soc_mess_list[curr_soc].char_auto = fread_action(fl, nr)))
            tmp_soc_mess_list[curr_soc].others_auto = fread_action(fl, nr);
        else
            tmp_soc_mess_list[curr_soc].others_auto = NULL;
    }

    /* close file & set top */
    fclose(fl);

    CREATE(soc_mess_list, struct social_messg, list_top + 1);

    for (curr_soc = 0, i = 0; curr_soc < MAX_SOCIALS && i < list_top; curr_soc++)
        if (tmp_soc_mess_list[curr_soc].act_nr >= 0) {
            soc_mess_list[i] = tmp_soc_mess_list[curr_soc];
            i++;
        }

    /* now, sort 'em */
    for (curr_soc = 0; curr_soc < list_top; curr_soc++) {
        min_pos = curr_soc;
        for (i = curr_soc + 1; i <= list_top; i++)
            if (soc_mess_list[i].act_nr < soc_mess_list[min_pos].act_nr)
                min_pos = i;
        if (curr_soc != min_pos) {
            temp = soc_mess_list[curr_soc];
            soc_mess_list[curr_soc] = soc_mess_list[min_pos];
            soc_mess_list[min_pos] = temp;
        }
    }
}


void show_social_messages(struct char_data *ch, char *arg)
{
    int i, j, l;
    struct social_messg *action;

    if (!*arg)
        send_to_char("What social?\r\n", ch);
    else {
        for (l = strlen(arg), i = 0; *cmd_info[i].command != '\n'; i++) {
            if (!strncmp(cmd_info[i].command, arg, l)) {
                if (GET_LEVEL(ch) >= cmd_info[i].minimum_level) {
                    break;
                } 
            } 
        }
        if (*cmd_info[i].command == '\n')
            send_to_char("No such social.\r\n", ch);
        else if ((j = find_action(i)) < 0) 
            send_to_char("That action is not supported.\r\n", ch);
        else {
            action = &soc_mess_list[j];
      
            sprintf(buf, "Action '%s', Hide-invis : %s, Min Vict Pos: %d\r\n",
                    cmd_info[i].command, YESNO(action->hide), 
                    action->min_victim_position);
            sprintf(buf, "%schar_no_arg  : %s\r\n", buf, action->char_no_arg);   
            sprintf(buf, "%sothers_no_arg: %s\r\n", buf, action->others_no_arg);
            sprintf(buf, "%schar_found   : %s\r\n", buf, action->char_found);
            if (action->others_found) {
                sprintf(buf, "%sothers_found : %s\r\n", buf, action->others_found);
                sprintf(buf, "%svict_found   : %s\r\n", buf, action->vict_found);
                sprintf(buf, "%snot_found    : %s\r\n", buf, action->not_found);
                sprintf(buf, "%schar_auto    : %s\r\n", buf, action->char_auto);
                sprintf(buf, "%sothers_auto  : %s\r\n", buf, action->others_auto);
            }
      
            send_to_char(buf, ch);

            return;
        }
    }
}
void free_socials(void)
{
    int i;

    for (i = 0; i <= list_top; i++) {
        if (soc_mess_list[i].char_no_arg)
            free(soc_mess_list[i].char_no_arg);
        if (soc_mess_list[i].others_no_arg)
            free(soc_mess_list[i].others_no_arg);
        if (soc_mess_list[i].char_found)
            free(soc_mess_list[i].char_found);
        if (soc_mess_list[i].others_found)
            free(soc_mess_list[i].others_found);
        if (soc_mess_list[i].vict_found)
            free(soc_mess_list[i].vict_found);
        if (soc_mess_list[i].not_found)
            free(soc_mess_list[i].not_found);
        if (soc_mess_list[i].char_auto)
            free(soc_mess_list[i].char_auto);
        if (soc_mess_list[i].others_auto)
            free(soc_mess_list[i].others_auto);
    }
    free(soc_mess_list);
}
