//
// File: matrix.c                       -- Part of TempusMUD
//       general functions governing life in Tempus' (vnum) cyberspace
//
// Copyright 1998 by John Watson, all rights reserved.
//

#include "structs.h"
#include "db.h"
#include "comm.h"
#include "matrix.h"

//
// The network interface for Tempus needs to be revamped...
//

//
// network menus
//

void 
show_net_menu1_to_descriptor(struct descriptor_data *d)
{
    strcpy(buf, "\r\n");
    sprintf(buf, "%s          Primary Net Menu\r\n", buf);
    sprintf(buf, "%s\r\n"
	    "        (0)    -     Disconnect from the network.\r\n", buf);
    sprintf(buf, "%s        (1)    -     Enter program database.\r\n", buf);
    sprintf(buf, "%s        (2)    -     List current users.\r\n\r\n", buf);
    sprintf(buf, "%s      You may choose one of the above options.\r\n\r\n"
	    "  >", buf);

    SEND_TO_Q(buf, d);
}

void 
show_net_progmenu1_to_descriptor(struct descriptor_data *d)
{
    strcpy(buf, "\r\n");
    sprintf(buf, "%s          Program Database\r\n", buf);
    sprintf(buf, "%s\r\n"
	    "        (0)    -     Previous menu.  (Primary)\r\n", buf);
    sprintf(buf, "%s        (1)    -     Public Cyborg functions.\r\n\r\n", buf);
    sprintf(buf, "%s      You may choose one of the above options.\r\n"
	    "  >", buf);

    SEND_TO_Q(buf, d);
}
