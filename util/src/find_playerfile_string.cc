//
// This program reads a player file and generates economic statistics.
// It calculates the average wealth of players, in mortal, remort, and combined categories.
// Immortals are ignored.
//

#include <stdio.h>
#include <stdlib.h>

#include "../structs.h"
#include "../utils.h"

void find_playerfile_stats( const char *filename, const char *str )
{
    FILE * fl;
    struct char_file_u player;
    unsigned int remort_sum = 0, remort_max = 0, remort_avg = 0;
    unsigned int mort_sum = 0, mort_max = 0, mort_avg = 0;
    unsigned int tot_sum = 0, tot_avg = 0;
    unsigned int tmp_sum;
    unsigned int remort_num = 0, mort_num = 0, tot_num = 0;
    char theTitle[ MAX_TITLE_LENGTH + 1 ];
    char theDescription[ MAX_CHAR_DESC + 1 ];
    char *ptr = 0;

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

	strncpy( theTitle, player.title, MAX_TITLE_LENGTH );
	strncpy( theDescription, player.description, MAX_CHAR_DESC );

	for ( ptr = theTitle; *ptr; ptr++ )
	    *ptr = toupper( *ptr );

	for ( ptr = theDescription; *ptr; ptr++ )
	    *ptr = toupper( *ptr );
	
    }
}

int 
main(int argc, char **argv)
{
    ios::sync_with_stdio();

    if (argc != 3)
	cout << "Usage: " << argv[0] << " <playerfile-name> <string-to-find>" << endl;
    else
	find_playerfile_string( argv[1], argv[2] );
    return 0;
}


