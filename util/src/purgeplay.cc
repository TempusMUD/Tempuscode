
/* ************************************************************************
*  file: purgeplay.c                                    Part of CircleMUD * 
*  Usage: purge useless chars from playerfile                             *
*  All Rights Reserved                                                    *
*  Copyright (C) 1992, 1993 The Trustees of The Johns Hopkins University  *
************************************************************************* */

//
// C++/Tempus conversion copywrite 1999 John Watson
//

#include <cstdio>
#include <ctime>
#include <string>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"

//
// purge the file
//

void purge( char *filename, int tmud ) {

    FILE * fl;
    FILE * outfile;
    struct char_file_u player;
    int	okay, num = 0;
    long	timeout;
    char	*ptr, reason[80];

    if (!(fl = fopen(filename, "r+"))) {
	printf("Can't open %s.", filename);
	exit(1);
    }

    if (tmud)
	printf("--Purging in ultra-harsh tmud mode!!\n");

    outfile = fopen("players.new", "w");
    printf("Deleting: \n");

    for (; ; ) {
	fread(&player, sizeof(struct char_file_u ), 1, fl);
	if (feof(fl)) {
	    fclose(fl);
	    fclose(outfile);
	    puts("Done.");
	    exit(1);
	}

	okay = 1;
	*reason = '\0';

	for (ptr = player.name; *ptr; ptr++)
	    if (!isalpha(*ptr) || *ptr == ' ') {
		okay = 0;
		strcpy(reason, "Invalid name");
	    }
      
	if (player.level == 0) {
	    okay = 0;
	    strcpy(reason, "Never entered game");
	}
      
	if (player.level < 0 || player.level > LVL_GRIMP) {
	    okay = 0;
	    sprintf(reason, "Invalid level (%d)", player.level);
	}

	// now, check for timeouts.  They only apply if the char is not
	// cryo-rented.   Lev 40+ and remorts do not time out

	if (okay && ((player.level <= 40 &&
		      !(player.char_specials_saved.act & PLR_CRYO) &&
		      player.remort_char_class < 0) || tmud)) {
	    timeout =  player.level * 5;
	    timeout = timeout < 14 ? 14 : timeout;

	    timeout *= SECS_PER_REAL_DAY;
	
	    if ((time(0) - player.last_logon) > timeout) {
		okay = 0;
		sprintf(reason, "Level %2d idle for %3ld days", player.level,
			((time(0) - player.last_logon) / SECS_PER_REAL_DAY));

	    }
      
	    if (player.char_specials_saved.act & PLR_DELETED) {
		okay = 0;
		sprintf(reason, "Deleted flag set");
	    }

	    if (tmud && player.level < 50) {
		okay = 0;
		sprintf(reason, "TMUD wipe.");
	    }

	    if (!okay && (player.char_specials_saved.act & PLR_NODELETE)) {
		okay = 2;
		strcat(reason, "; NOT deleted.");
	    }

	}

	if (okay) {
	    fwrite(&player, sizeof(struct char_file_u ), 1, outfile);

	    if (okay == 2)

		printf("      %-20s %s\n",  player.name, reason);
	} else
	    printf("%4d. %-20s %s\n", ++num, player.name, reason);
     
    }

}


int main( int argc, char **argv ) {

    int tmud = 0;

    if (argc < 2)
	printf("Usage: %s playerfile-name [testmud]\n", argv[0]);
    else { 
	if (argc > 2 && !strncmp(argv[2], "testmud", strlen(argv[2])))
	    tmud = 1;
	purge(argv[1], tmud);
    }
    return 1;
}

#define SECS_PER_REAL_MIN       60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
