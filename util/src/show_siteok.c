/* ************************************************************************
*  file:  showplay.c                                  Part of CircleMud   *
*  Usage: list a diku playerfile                                          *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
*  Edited for TempusMUD 6-21-95                                           *
************************************************************************* */

#include <stdio.h>
#include <stdlib.h>
#include "../structs.h"
#include "../utils.h"

void	
show(char *filename)
{
  FILE * fl;
  struct char_file_u player;
  int	num = 0;
  
  if (!(fl = fopen(filename, "r"))) {
    perror("error opening playerfile");
    exit(1);
  }
  
  for (; ; ) {
    fread(&player, sizeof(struct char_file_u ), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      exit(1);
    }

    
    if (!IS_SET(player.char_specials_saved.act, PLR_SITEOK))
	continue;

    printf("%3d.] %-25s\n", ++num, player.name);
  }
}


int 
main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    show(argv[1]);

  return 0;
}

