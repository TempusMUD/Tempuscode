//
// This program reads a player file and generates economic statistics.
// It calculates the average wealth of players, in mortal, remort, and combined categories.
// Immortals are ignored.
//

#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "utils.h"

void	
get_stats( char *filename )
{
    FILE * fl;
    struct char_file_u player;
    unsigned int remort_sum = 0, remort_max = 0, remort_avg = 0;
    unsigned int mort_sum = 0, mort_max = 0, mort_avg = 0;
    unsigned int tot_sum = 0, tot_avg = 0;
    unsigned int tmp_sum;
    unsigned int remort_num = 0, mort_num = 0, tot_num = 0;

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
    
	tmp_sum = 0;

	tmp_sum += ( player.points.bank_gold / 1000 );
	tmp_sum += ( player.points.gold / 1000 );
	tmp_sum += ( player.points.credits / 1000 );
	tmp_sum += ( player.points.cash / 1000 );

	if ( player.level >= LVL_AMBASSADOR ) {
	    continue;
	}
	if ( player.player_specials_saved.remort_generation > 0 ) {
	    remort_sum += tmp_sum;
	    if ( tmp_sum > remort_max )
		remort_max = tmp_sum;
	    remort_num++;
	}
	else {
	    mort_sum += tmp_sum;
	    if ( tmp_sum > mort_max )
		mort_max = tmp_sum;
	    mort_num++;
	}
    
    }

    if ( remort_num )
	remort_avg = remort_sum / remort_num;
    else
	remort_avg = 0;
    
    if ( mort_num )
	mort_avg = mort_sum / mort_num;
    else
	mort_avg = 0;

    tot_num = mort_num + remort_num;
    tot_sum = mort_sum + remort_sum;

    if ( tot_num )
	tot_avg = tot_sum / tot_num;
    else
	tot_num = 0;

    printf( "Results:\n"
	    "                  total          avg             max\n"
	    "     Mortals:      %4d       %6d k        %6d k\n"
	    "     Remorts:      %4d       %6d k        %6d k\n"
	    "      Totals:      %4d       %6d k        %6d k\n"
	    "      ------\n"
	    "     Total currency in circulation: %d M\n",
	    mort_num, mort_avg, mort_max,
	    remort_num, remort_avg, remort_max,
	    tot_num, tot_avg,
	    MAX( mort_max, remort_max ), tot_sum / 1000 );
}

int 
main(int argc, char **argv)
{
    if (argc != 2)
	printf("Usage: %s playerfile-name\n", argv[0]);
    else
	get_stats( argv[ 1 ] );

    return 0;
}


