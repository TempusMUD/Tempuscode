/* ************************************************************************
*   File: holodeck.c                                    Part of TempusMUD *
*         See header file: holodeck.h                                     *
*                                                                         *
*  CircleMUD:                                                             *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

#include "holodeck.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct time_info_data time_info;
extern struct command_info cmd_info[];

/*   external functions  */
long asciiflag_conv (char *flag);

#define HC_ROOM	10038	/* Holodeck Computer Room     		*/
#define HC	10000   /* Holodeck Computer		        */
#define HCIT	10000   /* Holodeck Computer Interface Terminal */

HoloZone *FirstHoloZone = NULL;

SPECIAL(holodeck_computer);
SPECIAL(holodeck_rm);
SPECIAL(hcit);

/*   holodeck utility prototypes   */
HoloZone	*holo_newzone (struct char_data *ch);
HoloZone	*holo_loadzone (struct char_data *ch, int num);
int		holo_savezone (struct char_data *ch, HoloZone *zone);
int		holo_delzone (struct char_data *ch, int num);
HoloZone	*holo_findzone (struct room_data *room);
char		*holo_loadwld (FILE *file, HoloZone *zone);
int		holo_savewld (FILE *file, HoloZone *zone);
int		holo_addroom (HoloRoom *this, HoloZone *zone);
void		holo_rmrooms (HoloZone *zone);
void		holo_freezone (int num);
void		num2str(char *str, int num);
int		holo_fread_string(FILE *file, char *string, int length);
char		*holo_parse_room(FILE *file, int virtual_nr, HoloRoom *room);

SPECIAL(holodeck_rm)
{
  HoloZone	*zone;
  int			num;
  char		buf[256];
  char		arg1[256];
  char		arg2[256];

  if (!cmd)
    return FALSE;

  zone = holo_findzone(ch->in_room);

  if (CMD_IS("computer"))  {
    two_arguments(argument, arg1, arg2);
    if (!strncasecmp(arg2,"program",4))  {
      if (!strcasecmp(arg1,"save"))  {
	if (!zone)  {
	  send_to_char("HC tells you, 'A program is not currently running.'\r\n",ch);
	  return TRUE;
	}
	if (holo_savezone(ch,zone))
	  sprintf(buf,"HC tells you, 'Program %d saved.'\r\n",zone->zonenum);
	else
	  sprintf(buf,"HC tells you, 'Error occured while saving program %d.'\r\n",zone->zonenum);
	send_to_char(buf,ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"new"))  {
	if (zone)  {
	  send_to_char("HC tells you, 'This Holodeck is already running a program.'\r\n",ch);
	  return TRUE;
	}
	if (!(zone = holo_newzone(ch)))  {
	  send_to_char("HC tells you, 'A new program could not be registered.'\r\n",ch);
	  return TRUE;
	}
	sprintf(buf,"HC tells you, 'Registering new program %d.'\r\n",zone->zonenum);
	send_to_char(buf,ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"end"))  {
	if (!zone)  {
	  send_to_char("HC tells you, 'A program is not currently running.'\r\n",ch);
	  return TRUE;
	}
	if (holo_savezone(ch,zone))  {
	  sprintf(buf,"HC tells you, 'Program %d saved and terminated.'\r\n",zone->zonenum);
	  holo_freezone(zone->zonenum);
	}
	else
	  sprintf(buf,"HC tells you, 'Error occured while saving program %d.'\r\n",zone->zonenum);
	send_to_char(buf,ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"abort"))  {
	if (!zone)  {
	  send_to_char("HC tells you, 'A program is not currently running.'\r\n",ch);
	  return TRUE;
	}
	sprintf(buf,"HC tells you, 'Discarding changes and closing program %d.'\r\n",zone->zonenum);
	send_to_char(buf,ch);
	holo_freezone(zone->zonenum);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"show"))  {
	send_to_char("HC tells you, 'Unable to list your known programs.'\r\n",ch);
	return TRUE;
      }
      else  {
	send_to_char("HC tells you, 'Invalid program request.'\r\n",ch);
      }
    }
    else if (!strcasecmp(arg2,"mode"))  {
      if (!strcasecmp(arg1,"room"))  {
	send_to_char("HC tells you, 'Entering room editing mode.'\r\n",ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"mobile"))  {
	send_to_char("HC tells you, 'Mobile editing mode currently unavailable.'\r\n",ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"object"))  {
	send_to_char("HC tells you, 'Object editing mode currently unavailable.'\r\n",ch);
	return TRUE;
      }
      else if (!strcasecmp(arg1,"zone"))  {
	send_to_char("HC tells you, 'Zone editing mode currently unavailable.'\r\n",ch);
	return TRUE;
      }
      else  {
	send_to_char("HC tells you, 'Invalid mode.'\r\n",ch);
	return TRUE;
      }
    }
    else if (!strcasecmp(arg1,"help"))  {
      send_to_char("Available commands:\r\n",ch);
      send_to_char("   computer {new,save,end,abort} program\r\n",ch);
      send_to_char("   computer {load,delete} <number>\r\n",ch);
      send_to_char("   computer {room,object,zone,mobile} mode\r\n \r\n",ch);
      send_to_char("   computer {help,status,public,private} \r\n \r\n",ch);
      send_to_char("   computer show programs\r\n \r\n",ch);
      return TRUE;
    }
    else if (!strcasecmp(arg1,"status"))  {
      send_to_char("HC tells you, 'Status request ignored.'\r\n",ch);
      return TRUE;
    }
    else if (!strcasecmp(arg1,"load"))  {
      if (zone)  {
	send_to_char("HC tells you, 'This Holodeck is already running a program.'\r\n",ch);
	return TRUE;
      }
      num = strtol(arg2,NULL,10);
      if (!(zone = holo_loadzone(ch, num)))  {
	sprintf(buf,"HC tells you, 'Could not load program %d'\r\n",num);
	send_to_char(buf,ch);
	return TRUE;
      }
      sprintf(buf,"HC tells you, 'Program %d loaded.'\r\n",zone->zonenum);
      send_to_char(buf,ch);
      return TRUE;
    }
    else if (!strcasecmp(arg1,"delete"))  {
      num = strtol(arg2,NULL,10);
      if (holo_delzone(ch,num))
	sprintf(buf,"HC tells you, 'Program %d deleted'.\r\n",num);
      else
	sprintf(buf,"HC tells you, 'Delete request for program %d failed'.\r\n",num);
      send_to_char(buf,ch);
      return TRUE;
    }
    else  {
      send_to_char("HC tells you, 'Invalid request'.\r\n",ch);
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(holodeck_computer)
{

  if (CMD_IS("computer"))  {
    send_to_char("The Holodeck Computer ignores you.\r\n", ch);
    return TRUE;
  }

  return FALSE;
}

SPECIAL(hcit)
{
  if (CMD_IS("computer"))  {
    send_to_char("The HCIT seems to be malfunctioning.\r\n", ch);
    return TRUE;
  }

  return FALSE;
}


/* ****************************************************************
* Holodeck utility routines
**************************************************************** */

HoloZone *holo_findzone(struct room_data *room)
{
  HoloZone *this = FirstHoloZone;

  while ((this != NULL) && (this->holodeck != room->number))
    this = this->nextzone;

  return this;
}

HoloZone *holo_newzone(struct char_data *ch)
{
  int			start	= HOLODECK_ZONE_START;
  int			stop	= HOLODECK_ZONE_STOP;
  int			inc		= HOLODECK_ZONE_INC;
  HoloZone	*zone;
  FILE		*zonefile;
  char		buf[256];
  int			i;

  for (i=start;i<stop;i+=inc)  {
    sprintf(buf,"world/holodeck/%d.holo",i);
    if (!(zonefile=fopen(buf,"r")))  {
      break;
    }
    fclose(zonefile);
  }
  if ((i>=stop) || !(zonefile=fopen(buf,"w")))
    return NULL;

  zone = (void *)calloc(1,sizeof(HoloZone));
  strcpy (zone->creator,ch->player.name);
  fprintf (zonefile,"%s\n",ch->player.name);
  zone->holodeck = ch->in_room->number;
  zone->zonenum = i;
  zone->nextzone = FirstHoloZone;
  FirstHoloZone = zone;
  fclose(zonefile);

  return(zone);
}

HoloZone *holo_loadzone(struct char_data *ch, int num)
{
  HoloZone	*zone;
  FILE		*file;
  char		buf[256], *err;

  /*** First load num.holo file ***/
  sprintf(buf,"world/holodeck/%d.holo",num);
  if (!(file=fopen(buf,"r")))  {
    return NULL;
  }
  fgets(buf,64,file);
  buf[strlen(buf)-1] = 0;		/* remove trailing \n	*/
  fclose(file);
  if (strcmp(buf,ch->player.name))
    return NULL;			/* Not the creator		*/

  zone = (void *)calloc(1,sizeof(HoloZone));
  strcpy (zone->creator,ch->player.name);
  zone->holodeck = ch->in_room->number;
  zone->zonenum = num;
  zone->nextzone = FirstHoloZone;
  FirstHoloZone = zone;

  /*** Second load num.wld file ***/
  sprintf(buf,"world/holodeck/wld/%d.wld",num);
  if ((file=fopen(buf,"r")))  {
    if ((err = holo_loadwld(file, zone)))  {  /* file exists but... */
      send_to_char(err,ch);
      send_to_char("Error loading wld file.  Not loaded.\r\n",ch);
    }
  }
  return(zone);
}

int	holo_savezone (struct char_data *ch, HoloZone *zone)
{
  FILE		*file;
  char		buf[256];
  char		buf2[256];
  int			val = TRUE;

  /*** Save .holo file ***/
  sprintf(buf,"world/holodeck/%d.holo",zone->zonenum);
  if (!(file=fopen(buf,"w")))  {
    send_to_char ("Could not open holodeck zone file for saving.\r\n",ch);
    return FALSE;
  }

  sprintf(buf,"%s\n%u\n",ch->player.name,(unsigned int)time(NULL));
  fputs(buf,file);
  fclose(file);

  /*** Save .wld file ***/
  if (zone->wld)  {
    sprintf(buf2,"world/holodeck/wld/%d.wld.bak",zone->zonenum);
    sprintf(buf, "world/holodeck/wld/%d.wld",zone->zonenum);
    rename(buf,buf2);
    if (!(file=fopen(buf,"w")))  {
      send_to_char ("Could not open holodeck zone file for saving.\r\n",ch);
      val = FALSE;
    }
    else  {
      if (!holo_savewld (file, zone))  {
	send_to_char ("Error saving wld file.\r\n",ch);
	val = FALSE;
      }
      fclose(file);
    }
  }

  return val;
}

int holo_delzone (struct char_data *ch, int num)
{
  FILE		*zonefile;
  char		buf[256];
  char		buf2[256];

  sprintf(buf,"world/holodeck/%d.holo",num);
  if (!(zonefile=fopen(buf,"r")))  {
    return FALSE;
  }

  fgets(buf2,64,zonefile);
  fclose(zonefile);

  buf2[strlen(buf2)-1] = 0;		/* remove trailing \n	*/
  if (strcmp(buf2,ch->player.name))
    return FALSE;				/* Not the creator		*/

  sprintf(buf2,"world/holodeck/%d.holo.bak",num);
  if (rename(buf,buf2))
    return FALSE;				/* Error occured while trying to rename it.	*/

  holo_freezone(num);

  return TRUE;
}

void holo_freezone (int num)
{
  HoloZone	*zone, *ztmp;
  if (FirstHoloZone == NULL)
    return;

  if (FirstHoloZone->zonenum == num)  {
    zone = FirstHoloZone;
    FirstHoloZone = FirstHoloZone->nextzone;
  }
  else  {
    ztmp=FirstHoloZone;
    while ((ztmp->nextzone != NULL) && ((HoloZone *)ztmp->nextzone)->zonenum != num)
      ztmp = ztmp->nextzone;

    if (!(ztmp->nextzone))
      return;

    zone = ztmp->nextzone;
    ztmp->nextzone = ((HoloZone *)ztmp->nextzone)->nextzone;
  }

  free (zone);

  return;
}

char *holo_loadwld(FILE *file, HoloZone *zone)
{
  HoloRoom	*this;
  int			nr = -1, last = 0;
  char		line[256], *ch;
  char		mode[] = "world";

  for (;;) {
    if (!get_line(file, line)) {
      sprintf(buf, "Format error after %s #%d\n", mode, nr);
      return buf;
    }
    if (*line == '$')
      return 0;

    if (*line != '#')  {
      sprintf(buf, "Format error in %s file near %s #%d\nOffending line: '%s'\r\n",
	      mode, mode, nr, line);
      return buf;
    }
    last = nr;
    if (sscanf(line, "#%d", &nr) != 1) {
      sprintf(buf, "Format error after %s #%d\r\n", mode, last);
      holo_rmrooms(zone);
      return buf;
    }
    if (nr >= 99999)  {
      sprintf(buf, "Exceeded vnum limit: %s #%d\r\n", mode, last);
      holo_rmrooms(zone);
      return buf;
    }
    CREATE (this,HoloRoom,1);
    this->zone = zone->zonenum;
    if ((ch = holo_parse_room(file, nr, this)))  {
      holo_rmrooms(zone);
      free(this); /* maybe leaving desc's allocated!! */
      return ch;
    }
    holo_addroom(this, zone);
  }
}


void holo_rmrooms(HoloZone *zone)
{
  HoloRoom	*tmp;

  while (zone->wld)  {
    tmp = zone->wld->nextroom;
    free(zone->wld);
    zone->wld = tmp;
  }
}

int holo_addroom(HoloRoom *this, HoloZone *zone)
{
  HoloRoom *tmp;

  if (zone->wld == NULL)  {
    zone->wld = this;
    return 0;
  }

  if (zone->wld->number > this->number)  {
    this->nextroom = zone->wld;
    zone->wld = this;
    return 0;
  }

  tmp = zone->wld;
  while (tmp->nextroom != NULL)  {
    if (((HoloRoom *)tmp->nextroom)->number > this->number)  {
      break;
    }
    tmp = (HoloRoom *)tmp->nextroom;
  }

  if (tmp->number == this->number)  {
    return -1;
  }
  this->nextroom = tmp->nextroom;
  tmp->nextroom = (void *)this;
  return 0;
}

void num2str(char *str, int num)
{
  str[0] = 0;

  if (num == 0)  {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  if (num & (1 << 0))
    strncat (str,"a",1);

  if (num & (1 << 1))
    strncat (str,"b",1);

  if (num & (1 << 2))
    strncat (str,"c",1);

  if (num & (1 << 3))
    strncat (str,"d",1);

  if (num & (1 << 4))
    strncat (str,"e",1);

  if (num & (1 << 5))
    strncat (str,"f",1);

  if (num & (1 << 6))
    strncat (str,"g",1);

  if (num & (1 << 7))
    strncat (str,"h",1);

  if (num & (1 << 8))
    strncat (str,"i",1);

  if (num & (1 << 9))
    strncat (str,"j",1);

  if (num & (1 << 10))
    strncat (str,"k",1);

  if (num & (1 << 11))
    strncat (str,"l",1);

  if (num & (1 << 12))
    strncat (str,"m",1);

  if (num & (1 << 13))
    strncat (str,"n",1);

  if (num & (1 << 14))
    strncat (str,"o",1);

  if (num & (1 << 15))
    strncat (str,"p",1);

  if (num & (1 << 16))
    strncat (str,"q",1);

  if (num & (1 << 17))
    strncat (str,"r",1);

  if (num & (1 << 18))
    strncat (str,"s",1);

  if (num & (1 << 19))
    strncat (str,"t",1);

  if (num & (1 << 20))
    strncat (str,"u",1);

  if (num & (1 << 21))
    strncat (str,"v",1);

  if (num & (1 << 22))
    strncat (str,"w",1);

  if (num & (1 << 23))
    strncat (str,"x",1);

  if (num & (1 << 24))
    strncat (str,"y",1);

  if (num & (1 << 25))
    strncat (str,"z",1);

  if (num & (1 << 26))
    strncat (str,"A",1);

  if (num & (1 << 27))
    strncat (str,"B",1);

  if (num & (1 << 28))
    strncat (str,"C",1);

  if (num & (1 << 29))
    strncat (str,"D",1);

  if (num & (1 << 30))
    strncat (str,"E",1);

  if (num & (1 << 31))
    strncat (str,"F",1);
}

/* load a room */
char *holo_parse_room(FILE *file, int virtual_nr, HoloRoom *room)
{
  int t[10], dir;
  char line[256], flags[128];
  HoloEdesc *new_descr;


  room->number = virtual_nr;
  holo_fread_string (file, room->name, 256);
  holo_fread_string (file, room->description, 2048);

  if (!get_line(file, line) || sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    sprintf(buf2, "Format error in room #%d\n", virtual_nr);
    return buf2;
  }

  /* t[0] is the zone number; ignored with the zone-file system */
  room->room_flags = asciiflag_conv(flags);
  room->sector_type = t[2];

  sprintf(buf2, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(file, line)) {
      sprintf(buf, "%s\n", buf2);
      return buf;
    }
    switch (*line) {
    case 'D':
      dir = atoi(line + 1);
      sprintf(buf, "room #%d, direction D%d", room->number, dir);

      holo_fread_string(file, room->exits[dir].desc, 2048);
      holo_fread_string(file, room->exits[dir].keyword, 256);

      if (!get_line(file, line)) {
	sprintf(buf, "Format error, %s\n", buf2);
	return buf;
      }
      if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
	sprintf(buf, "Format error, %s\n", buf2);
	return buf;
      }
      room->exits[dir].exists = 1;
      room->exits[dir].door = t[0];
      if ((room->exits[dir].door <0)||(room->exits[dir].door >2))
	room->exits[dir].door = 0;

      room->exits[dir].key = t[1];
      room->exits[dir].to_room = t[2];
      break;
    case 'E':
      sprintf(buf, "room #%d, extended description.", room->number);
      CREATE(new_descr, HoloEdesc, 1);
      holo_fread_string(file, new_descr->keyword, 256);
      holo_fread_string(file, new_descr->desc, 2048);
      new_descr->next = room->extdesc;
      room->extdesc = new_descr;
      break;
    case 'S':			/* end of room */
      return 0;
      break;
    default:
      sprintf(buf, "%s\n", buf2);
      return buf;
      break;
    }
  }
}

int holo_fread_string(FILE *file, char *string, int maxlength)
{
  char	tmp[512];
  char	*point;
  int		done = 0,
    length = 0,
    tmplen = 0;

  string[0] = '\0';

  do  {
    if (!fgets(tmp, 512, file)) {
      return -1;
    }
    /* If there is a '~', end the string */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    }

    tmplen = strlen(tmp);

    if (length + tmplen >= maxlength) {
      return -2;
    }
    else {
      strcat(string + length, tmp);
      length += tmplen;
    }
  }
  while (!done);

  return 0;
}

int holo_savewld(FILE *file, HoloZone *zone)
{
  HoloRoom	*this = zone->wld;
  HoloEdesc	*desc;
  int			i;
  char		buf[64];

  while (this != NULL)  {
    fprintf(file,"#%d\n",this->number);
    fprintf(file,"%s~\n",this->name);
    fprintf(file,"%s~\n",this->description);
    num2str(buf, this->room_flags);
    fprintf(file,"%d %s %d\n",this->zone, buf, this->sector_type);

    for (i=0;i<8;i++)  {
      if (this->exits[i].exists)  {
	fprintf(file,"D%d\n",i);
	fprintf(file,"%s~\n",this->exits[i].desc);
	fprintf(file,"%s~\n",this->exits[i].keyword);
	fprintf(file,"%d %d %d\n", this->exits[i].door, this->exits[i].key, 
		this->exits[i].to_room);
      }
    }

    desc = this->extdesc;
    while (desc != NULL)  {
      fprintf(file,"E\n");
      fprintf(file,"%s~\n",desc->keyword);
      fprintf(file,"%s~\n",desc->desc);
      desc = (HoloEdesc *)desc->next;
    }
    fprintf(file,"S\n");

    this = (HoloRoom *)this->nextroom;
  }
  fprintf(file,"$~\n");

  return TRUE;
}
