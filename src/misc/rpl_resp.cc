//
// File: rpl_resp.c                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/*
*******************************************************************************
***                                                                         ***
*******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "actions.h"
#include "rpl_resp.h"

#define RPL(name) \
void (name) (struct Creature *ch)

void rpl_give_gold(struct Creature *ch, struct Creature *vict, char *desc);
void rpl_give_obj(struct Creature *ch, struct Creature *vict, char *desc);
void rpl_user_spec(struct Creature *ch, struct Creature *vict, char *desc);
char buf_temp[256];

/* 
******************************************************************************
***                                                                        ***
***            add proto types for reply spec's here                       ***
***                                                                        ***
****************************************************************************** 
*/

RPL(rpl_test);


void
reply_respond(struct Creature *ch, struct Creature *vict, char *desc)
{
	char *desc2 = desc;
	if (desc[0] == '%') {
		desc2++;
		if (desc2[0] == 'G')
			rpl_give_gold(ch, vict, desc2);
		if (desc2[0] == 'O')
			rpl_give_obj(ch, vict, desc2);
		if (desc2[0] == 'U')
			rpl_user_spec(ch, vict, desc2);

	} else {
        perform_tell(vict, ch, desc);
		act("$t tells something to $T", FALSE, vict, 0, ch, TO_ROOM);
	}
}

void
rpl_give_gold(struct Creature *ch, struct Creature *vict, char *desc)
{
	short num;

	num = atoi(++desc);

	sprintf(buf_temp, "%s%s gives you:%d gold.\r\n", CCRED(ch, C_NRM),
		GET_NAME(vict), num);
	act(buf_temp, FALSE, ch, 0, 0, TO_CHAR);
	GET_GOLD(ch) += num;
}

void
rpl_give_obj(struct Creature *ch, struct Creature *vict, char *desc)
{

	struct obj_data *new_obj;
	short num = atoi(++desc);
	new_obj = read_object(num);
	if (new_obj != NULL) {
		obj_to_char(new_obj, ch);
		sprintf(buf, "%s%s gives you a:%s.\r\n", CCRED(ch, C_NRM),
			GET_NAME(vict), new_obj->aliases);
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
	}

}

void
rpl_user_spec(struct Creature *ch, struct Creature *vict, char *desc)
{
	desc++;						/* move pointer beyond 'U ' */
	desc++;						/* move pointer beyond 'U ' */

	if (!strncasecmp(desc, "TEST", 4)) {
		rpl_test(ch);
		return;
	}

}

/*
*****************************************************************************
***                                                                       ***
***                                                                       ***
***   Special reply functions here                                        ***
***                                                                       ***
***                                                                       ***
*****************************************************************************
*/

RPL(rpl_test)
{

	slog("test spec called \n");

}
