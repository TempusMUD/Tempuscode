//
//

#include <stdio.h>
#include <stdlib.h>
#include "../structs.h"
#include "../utils.h"

void	
get_stats( char *filename )
{
    FILE * fl;
    struct char_file_u player;
    time_t cur_time = time( 0 );
    unsigned int player_time = 0;

    if (!(fl = fopen(filename, "r"))) {
	perror("error opening playerfile");
	exit(1);
    }
  
    for (; ; ) {
	fread(&player, sizeof(struct char_file_u ), 1, fl);

	if (feof(fl)) {
	    fclose(fl);
	    break;
	}
    
	if ( player.level < LVL_AMBASSADOR ) {
	    continue;
	}
	
	player_time = player.played;
	
	// format : "name cur_time player_time"
	printf( "%-30s %11ld %11d\n", player.name, (unsigned long) cur_time, player_time );

    }
	
}

int 
main(int argc, char **argv)
{
    if (argc != 2) {
	printf("Usage: %s playerfile-name\n", argv[0]);
	return 1;
    }

    get_stats( argv[ 1 ] );
	
    return 0;
}


