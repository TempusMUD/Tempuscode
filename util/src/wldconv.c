/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "../structs.h"
#include "../utils.h"
#include "../comm.h"
#include "../handler.h"
#include "../flow_room.h"

char buf[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
int 
get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}


char *
fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n",
	      error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      printf("SYSERR: fread_string: string too large (db.c)\n");
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}

long 
asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return flags;
}

/* read direction data */
void 
setup_dirNEW(FILE * fl, struct room_data *room, int dir)
{
  int t[5];
  char line[256], flags[128];

  sprintf(buf2, "room #%d, direction D%d", room->number, dir);
  
  if (dir >= NUM_DIRS) {
    sprintf(buf, "ERROR!! 666!! (%d)", room->number);
    printf(buf);
    exit(1);
  }
  /*
  if (dir >= FORWARD && dir <= LEFT) {
    if (!room->dir_option[dir-FORWARD])
      dir -= FORWARD;
  } else if (dir == FUTURE || dir == PAST) {
    if (!room->dir_option[dir-4])
      dir -= 4;
  }*/

  CREATE(room->dir_option[dir], struct room_direction_data, 1);
  room->dir_option[dir]->general_description = fread_string(fl, buf2);
  room->dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }
  if (sscanf(line, " %s %d %d ", flags, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    exit(1);
  }

  room->dir_option[dir]->exit_info = asciiflag_conv(flags);

  room->dir_option[dir]->key = t[1];
  room->dir_option[dir]->to_room = (struct room_data*) t[2];
}

/* load the rooms */
void 
parse_roomNEW(FILE * fl, int virtual_nr)
{
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr = NULL, *t_exdesc = NULL;
  struct special_search_data *new_search = NULL, *t_srch = NULL;
  struct room_data *room = NULL, *tmp_room = NULL;
  struct room_data *roomlist = NULL;

  sprintf(buf2, "room #%d", virtual_nr);

  CREATE(room, struct room_data, 1);
  
  room->number = virtual_nr;
  room->name = fread_string(fl, buf2);
  room->description = fread_string(fl, buf2);
  room->sounds = NULL;

  if (!get_line(fl, line) || sscanf(line, " %d %s %d ", t, flags, t + 2)!=3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    exit(1);
  }

  room->room_flags = asciiflag_conv(flags);
  room->sector_type = t[2];
  room->func = NULL;
  room->ticl_ptr = NULL;
  room->contents = NULL;
  room->people = NULL;
  room->light = 0;	       /* Zero light sources     */
  room->max_occupancy = 256;  /* Default value set here */

  room->zone = (void *) t[0];

  for (i = 0; i < NUM_OF_DIRS; i++)
    room->dir_option[i] = NULL;

  room->ex_description = NULL;
  room->search = NULL;
  room->flow_dir = 0;
  room->flow_speed = 0;
  room->flow_type = 0;

  sprintf(buf, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
    switch (*line) {
    case 'O':
      room->max_occupancy = atoi(line + 1);
      break;
    case 'D':
      setup_dirNEW(fl, room, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);

      /* put the new exdesc at the end of the linked list, not head */

      new_descr->next = NULL;
      if (!room->ex_description)
	room->ex_description = new_descr;
      else {
	for (t_exdesc=room->ex_description;t_exdesc;t_exdesc=t_exdesc->next) {
	  if (!(t_exdesc->next)) {
	    t_exdesc->next = new_descr;
	    break;
	  }
	}

	if (!t_exdesc) {
	  fprintf(stderr, "Error building exdesc list in room %d.",
		  virtual_nr);
	  exit(1);
	}
      }
      break;
    case 'L':
      room->sounds = fread_string(fl, buf2);
      break;
    case 'F':
      if (!get_line(fl, line) || sscanf(line, "%d %d %d", t, t+1, t+2) != 3) {
        fprintf(stderr, "Flow field incorrect in room #%d.\n", virtual_nr);
        exit(1);
      }
      if (t[0] < 0 || t[0] >= NUM_DIRS) {
	fprintf(stderr, "Direction '%d' in room #%d flow field BUNK!\n",
                                                          t[0], virtual_nr);
	exit(1);
      }
      if (t[1] < 0) {
	fprintf(stderr, "Negative speed in room #%d flow field!\n",
                                                                virtual_nr);
	exit(1);
      }
      if (t[2] < 0 || t[2] >= NUM_FLOW_TYPES) {
        sprintf(buf, "SYSERR: Illegal flow type in room #%d.", virtual_nr);
	FLOW_TYPE(room) = F_TYPE_NONE;
      } else
        FLOW_TYPE(room) = t[2];
      FLOW_DIR(room) = t[0];
      FLOW_SPEED(room) = t[1];
      break;
    case 'Z':
      CREATE(new_search, struct special_search_data, 1);
      new_search->command_keys = fread_string(fl, buf2);
      new_search->keywords =     fread_string(fl, buf2);
      new_search->to_vict =      fread_string(fl, buf2);
      new_search->to_room =      fread_string(fl, buf2);
      new_search->to_remote =    fread_string(fl, buf2);
     
      if (!get_line(fl, line)) {
	fprintf(stderr, "Search error in room #%d.", virtual_nr);
	exit(1);
      }

      if ((sscanf(line, "%d %d %d %d %d", 
		  t+5, t+ 1, t + 2, t + 3, t + 4) != 5)) {
	fprintf(stderr,"Search Field format error in room #%d\n",virtual_nr);
	exit(1);
      }

      new_search->command = t[5];
      new_search->arg[0] = t[1];
      new_search->arg[1] = t[2];
      new_search->arg[2] = t[3];
      new_search->flags  = t[4];
      
      /*** place the search at the top of the list ***/
    
      new_search->next = NULL;
      
      if (!(room->search))
	room->search = new_search;
      else {
	for (t_srch = room->search; t_srch; t_srch = t_srch->next)
	  if (!(t_srch->next)) {
	    t_srch->next = new_search;
	    break;
	  }
	
	if (!t_srch) {
	  fprintf(stderr, "Error building main search list in room %d.",
		  virtual_nr);
	  exit(1);
	}
      }
      break;

    case 'S':			/* end of room */
      
      room->next = NULL;
      
      if (roomlist) {
	for (tmp_room = roomlist; tmp_room; tmp_room = tmp_room->next)
	  if (!tmp_room->next) {
	    tmp_room->next = room;
	    break;
	  }
      } else
        roomlist = room;
      
      return;
      break;
    default:
      fprintf(stderr, "%s\n", buf);
      exit(1);
      break;
    }
  }
  if (IS_SET(room->room_flags, ROOM_TUNNEL) && 
      room->max_occupancy > 5) {
    room->max_occupancy = 2;
    REMOVE_BIT(room->room_flags, ROOM_TUNNEL);
  }
}

void 
discrete_loadNEW(FILE * fl)
{
  int nr = -1, last = 0;
  char line[256];

  for (;;) {
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	fprintf(stderr, "Format error after room #%d\n", last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	parse_roomNEW(fl, nr);
    } else {
      fprintf(stderr, "Format error in wld file near room #%d\n", nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }
}

int
main(int argc, char *argv[])
{
  FILE *fl;
  int i;
  
  if (argc < 2) {
    printf("Usage: wldconv file1 file2 file3 ...\n");
    exit(1);
  }
  
  for (i = 1; i < argc; i++) {
    if (!(fl = fopen(argv[i], "r")))
      printf("could not open file '%s'\n", argv[i]);
    else {
      discrete_loadNEW(fl);
    }
  }
  exit(1);
}
