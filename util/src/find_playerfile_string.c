//
// This program reads a player file and generates economic statistics.
// It calculates the average wealth of players, in mortal, remort, and combined categories.
// Immortals are ignored.
//

#include <stdio.h>
#include <stdlib.h>

#include "../structs.h"
#include "../utils.h"

void find_playerfile_string( const char *filename, const char *str )
{
    FILE * fl;
    struct char_file_u player;
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
	    *ptr = tolower( *ptr );

	for ( ptr = theDescription; *ptr; ptr++ )
	    *ptr = tolower( *ptr );
	
	if ( ( ptr = strstr( theTitle, str ) ) )
	    printf( " %20s  has a TITLE '%s'\n", player.name, player.title );
	if ( ( ptr = strstr( theDescription, str ) ) )
	    printf( " %20s  has a DESCR with %s\n", player.name, str );
	
    }
}

int 
main(int argc, char **argv)
{
    if (argc != 3)
	return 1;
    else
	find_playerfile_string( argv[1], argv[2] );
    return 0;
}


