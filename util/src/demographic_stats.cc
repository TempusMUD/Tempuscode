//
// gets class /class combo / alignment data
//

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "utils.h"

#define NUM_USED_CLASSES (CLASS_MERCENARY+1)

static const char *local_char_class_abbrevs[ NUM_USED_CLASSES ] = {
    "Mage",			/* 0 */
    "Cler",
    "Thie",
    "Warr",
    "Barb",
    "Psio",			/* 5 */
    "Phyz",
    "Borg",
    "Knig",
    "Rang",
    "Hood",			/* 10 */
    "Monk",
    "Vamp", 
    "Merc"
};

struct class_combo {
    int mort_class;
    int remort_class;
    int count;
    class_combo() {
	mort_class = -1;
	remort_class = -1;
	count = 0;
    }
    class_combo( class_combo& c ) {
	mort_class = c.mort_class;
	remort_class = c.remort_class;
	count = c.count;
    }
    class_combo& operator=( class_combo& );
};

class_combo& class_combo::operator=( class_combo& c ) {
    mort_class = c.mort_class;
    remort_class = c.remort_class;
    count = c.count;
    return *this;
}	

const int ALIGN_ALL = 0;
const int ALIGN_GOOD = 1;
const int ALIGN_NEUTRAL = 2;
const int ALIGN_EVIL = 3;

const char* align_str[] = {
    "ALL",
    "GOOD",
    "NEUTRAL",
    "EVIL"
};

int align_flag = ALIGN_ALL;

void	
get_stats( char *filename )
{
    FILE * fl;
    struct char_file_u player;
    
    int num_players = 0;
    int num_morts   = 0;
    int num_remorts = 0;

    int player_classes[ NUM_USED_CLASSES ][ NUM_USED_CLASSES+1 ];
    
    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	for ( int j = 0; j <= NUM_USED_CLASSES; ++j ) {
	    player_classes[i][j] = 0;
	}
    }

    if (!(fl = fopen(filename, "r"))) {
	perror("error opening playerfile");
	exit(1);
    }
  
    for ( ; ; ) {
	fread(&player, sizeof(struct char_file_u ), 1, fl);

	if (feof(fl)) {
	    fclose(fl);
	    break;
	}
    
	if ( player.level >= LVL_AMBASSADOR ) {
//	    cerr << "Skipping IMM player " << player.name << endl;
	    continue;
	}

	if ( player.char_specials_saved.act & PLR_TESTER ) {
//	    cerr << "Skipping TESTER player " << player.name << endl;
	    continue;
	}

	if ( align_flag != ALIGN_ALL ) {
	    if ( align_flag == ALIGN_EVIL && player.char_specials_saved.alignment > -300 )
		continue;
	    
	    if ( align_flag == ALIGN_NEUTRAL && abs( player.char_specials_saved.alignment ) >= 300 )
		continue;
	    
	    if ( align_flag == ALIGN_GOOD && player.char_specials_saved.alignment < 300 )
		continue;
	}
	

	if ( player.char_class < 0 || player.char_class >= NUM_CLASSES ) {
	    cerr << "Error, player " << player.name << " has a class=" << player.char_class << endl;
	    continue;
	}

	if ( player.remort_char_class >= NUM_CLASSES ) {
	    cerr << "Error, player " << player.name << " has a remort class=" << player.remort_char_class << endl;
	    continue;
	}

	if ( player.char_class == player.remort_char_class ) {
	    cerr << "Error, player " << player.name << " is a dual class " << local_char_class_abbrevs[ player.char_class ] << endl;
	    continue;
	}

	// a mortal
	if ( player.remort_char_class < 0 ) {
	    ++player_classes[ player.char_class ][ NUM_USED_CLASSES ];
	    ++num_morts;
	}

	// a remort
	else {
	    ++player_classes[ player.char_class ][ player.remort_char_class ];
	    ++num_remorts;
	}

	++num_players;
    }

    // get sorted list of mort classes
    
    class_combo mort_class_data[ NUM_USED_CLASSES ];
    
    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	mort_class_data[ i ].mort_class = i;
	mort_class_data[ i ].count = player_classes[ i ][ NUM_USED_CLASSES ];
    }

    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {

	for ( int j = i+1; j < NUM_USED_CLASSES; j++ ) {
	    
	    if ( mort_class_data[ j ].count > mort_class_data[ i ].count ) {
		class_combo tmp_c = mort_class_data[ j ];
		mort_class_data[ j ] = mort_class_data[ i ];
		mort_class_data[ i ] = tmp_c;
	    }
	}
    }

    
    // get sorted list of remort classes

    class_combo remort_class_data[ NUM_USED_CLASSES * NUM_USED_CLASSES - 1 ];
    class_combo *d = remort_class_data;

    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	for ( int j = 0; j < NUM_USED_CLASSES; ++j ) {
	    d->mort_class = i;
	    d->remort_class = j;
	    d->count = player_classes[ i ][ j ];
	    ++d;
	}
    }

    for ( int i = 0; i < NUM_USED_CLASSES * NUM_USED_CLASSES; ++i ) {

	for ( int j = i+1; j < NUM_USED_CLASSES * NUM_USED_CLASSES; j++ ) {
	    
	    if ( remort_class_data[j].count > remort_class_data[i].count ) {
		
		class_combo tmp_c = remort_class_data[j];
		remort_class_data[j] = remort_class_data[ i ];
		remort_class_data[i] = tmp_c;
	    }
	}
    }

    

    
    
    cout << "Results:  " << num_players << " players counted.  ( " << align_str[ align_flag ] << " aligns )" << endl << endl;
    
    cout << "MORT  |                   R E M O R T    C L A S S" << endl
	 << "CLASS |-----------------------------------------------------------------" << endl;
    cout << " ---- | ";

    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	if ( i == CLASS_VAMPIRE || i == CLASS_WARRIOR )
	    continue;
	cout << local_char_class_abbrevs[ i ] << " ";
    }

    cout << "MORT" << endl;

    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	if ( i == CLASS_VAMPIRE || i == CLASS_WARRIOR )
	    continue;

	cout << " " << local_char_class_abbrevs[ i ] << " | ";
	for ( int j = 0; j <= NUM_USED_CLASSES; ++j ) {

	    if ( j == CLASS_VAMPIRE || j == CLASS_WARRIOR )
		continue;


	    if ( j == i ) {
		cout << "   - ";
	    }
	    else {
		cout << setw( 4 ) << player_classes[ i ][ j ] << " ";
	    }
	}
	cout << endl;
    }
    
    cout << endl << "   Top Mortal Classes:" << endl;

    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {

	// skip vampires
	// skip warriors

	if ( mort_class_data[ i ].mort_class == CLASS_VAMPIRE || mort_class_data[ i ].mort_class == CLASS_WARRIOR )
	    continue;

	cout << setw( 3 ) << " - " << local_char_class_abbrevs[ mort_class_data[ i ].mort_class ] << "            "
	     << setw( 4 ) << mort_class_data[ i ].count << "   "
	     << setw( 2 ) << ( mort_class_data[ i ].count * 100 / num_morts ) << " %" << endl;
    }

    cout << endl << "   Top Re-Mortal Classes:" << endl;

    for ( int i = 0; i < NUM_USED_CLASSES * (NUM_USED_CLASSES - 1); ++i ) {

	// skip dual-classes
	// skip vampires
	// skip warriors
    
	if ( remort_class_data[ i ].remort_class == remort_class_data[ i ].mort_class ||
	     remort_class_data[ i ].remort_class == CLASS_VAMPIRE ||
	     remort_class_data[ i ].mort_class == CLASS_VAMPIRE ||
	     remort_class_data[ i ].remort_class == CLASS_WARRIOR ||
	     remort_class_data[ i ].mort_class == CLASS_WARRIOR )
	    continue;

	cout << setw( 3 ) << " - "
	     << local_char_class_abbrevs[ remort_class_data[i].mort_class ] << " / "
	     << local_char_class_abbrevs[ remort_class_data[i].remort_class ] << "     "
	     << setw( 4 ) << remort_class_data[ i ].count << "   "
	     << setw( 2 ) << ( remort_class_data[ i ].count * 100 / num_remorts ) << " %" <<  endl;
    }
    

}

int 
main(int argc, char **argv)
{
    if (argc < 2)
	printf("Usage: %s playerfile-name [align]\n", argv[0]);
    else {
	if ( argc > 2 ) {
	    if ( !strcasecmp( argv[2], "good" ) )
		align_flag = ALIGN_GOOD;
	    else if ( !strcasecmp( argv[2], "neutral" ) )
		align_flag = ALIGN_NEUTRAL;
	    else if ( !strcasecmp( argv[2], "evil" ) )
		align_flag = ALIGN_EVIL;
	}
	get_stats( argv[ 1 ] );
    }
    return 0;
}


