//
// gets class /class combo / alignment data
//

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
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
    int count[4];
    
    class_combo() {
	mort_class = -1;
	remort_class = -1;
	count[0] = 0;
	count[1] = 0;
	count[2] = 0;
	count[3] = 0;
    }
    class_combo( class_combo& c ) {
	mort_class = c.mort_class;
	remort_class = c.remort_class;
	count[0] = c.count[0];
	count[1] = c.count[1];
	count[2] = c.count[2];
	count[3] = c.count[3];
    }
    class_combo& operator=( class_combo& );
};

class_combo& class_combo::operator=( class_combo& c ) {
    mort_class = c.mort_class;
    remort_class = c.remort_class;
    count[0] = c.count[0];
    count[1] = c.count[1];
    count[2] = c.count[2];
    count[3] = c.count[3];
    
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
int align_sort = ALIGN_ALL;

bool nomatrix = false;
bool nomort   = false;
bool noremort = false;

void get_stats( char *filename )
{
    FILE * fl;
    struct char_file_u player;
    
    int num_players[4] = { 0, 0, 0, 0 };
    int num_morts[4]   = { 0, 0, 0, 0 };
    int num_remorts[4] = { 0, 0, 0, 0 };

    int player_classes[4][ NUM_USED_CLASSES ][ NUM_USED_CLASSES+1 ];
    
    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	for ( int j = 0; j <= NUM_USED_CLASSES; ++j ) {
	    player_classes[0][i][j] = 0;
	    player_classes[1][i][j] = 0;
	    player_classes[2][i][j] = 0;
	    player_classes[3][i][j] = 0;
	}
    }

    if (!(fl = fopen(filename, "r"))) {
	cerr << "error opening playerfile '" << filename << "' : " << strerror( errno ) << endl;
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

	//
	// a mortal
	//

	if ( player.remort_char_class < 0 ) {
	    if ( player.char_specials_saved.alignment <= -350 ) { 
		++player_classes[ ALIGN_EVIL ][ player.char_class ][ NUM_USED_CLASSES ];
		++num_morts[ ALIGN_EVIL ];
	    }
	    else if ( player.char_specials_saved.alignment >= 350 ) {
		++player_classes[ ALIGN_GOOD ][ player.char_class ][ NUM_USED_CLASSES ];
		++num_morts[ ALIGN_GOOD ];
	    }
	    else {
		++player_classes[ ALIGN_NEUTRAL ][ player.char_class ][ NUM_USED_CLASSES ];
		++num_morts[ ALIGN_NEUTRAL ];
	    }

	}

	//
	// a remort
	//

	else {
	    if ( player.char_specials_saved.alignment <= -350 ) {
		++player_classes[ ALIGN_EVIL ][ player.char_class ][ player.remort_char_class ];
		++num_remorts[ ALIGN_EVIL ];
	    }
	    else if ( player.char_specials_saved.alignment >= 350 ) {
		++player_classes[ ALIGN_GOOD ][ player.char_class ][ player.remort_char_class ];
		++num_remorts[ ALIGN_GOOD ];
	    }
	    else {
		++player_classes[ ALIGN_NEUTRAL ][ player.char_class ][ player.remort_char_class ];
		++num_remorts[ ALIGN_NEUTRAL ];
	    }

	}

    }

    //
    // sum the player/remort/mort totals
    //

    num_morts[ ALIGN_ALL ] = 
	num_morts[ ALIGN_GOOD ] + 
	num_morts[ ALIGN_EVIL ] + 
	num_morts[ ALIGN_NEUTRAL ];

    num_remorts[ ALIGN_ALL ] = 
	num_remorts[ ALIGN_GOOD ] + 
	num_remorts[ ALIGN_EVIL ] + 
	num_remorts[ ALIGN_NEUTRAL ];

    for ( int i = 0; i < 4; ++i ) {
	num_players[ i ] = num_remorts[ i ] + num_morts[ i ];
    }


    //
    // sum the align-specific class counts
    //
    
    for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	for ( int j = i+1; j <= NUM_USED_CLASSES; j++ ) {
	    player_classes[ ALIGN_ALL ][ i ][ j ] = 
		player_classes[ ALIGN_GOOD ][ i ][ j ] +
		player_classes[ ALIGN_EVIL ][ i ][ j ] +
		player_classes[ ALIGN_NEUTRAL ][ i ][ j ];
	}
    }

    class_combo mort_class_data[ NUM_USED_CLASSES ];
    
    if ( nomort == false ) {
	//
	// create list of mort classes
	//

	for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	    mort_class_data[ i ].mort_class = i;

	    for ( int k = 0; k < 4; ++k ) {
		mort_class_data[ i ].count[ k ]    = player_classes[ k ][ i ][ NUM_USED_CLASSES ];
	    }
	}

	//
	// sort list of mortal classes
	//

	for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {

	    for ( int j = i+1; j < NUM_USED_CLASSES; j++ ) {
	    
		if ( mort_class_data[ j ].count[ align_sort ] > mort_class_data[ i ].count[ align_sort ] ) {
		    class_combo tmp_c = mort_class_data[ j ];
		    mort_class_data[ j ] = mort_class_data[ i ];
		    mort_class_data[ i ] = tmp_c;
		}
	    }
	}
    }

    class_combo remort_class_data[ NUM_USED_CLASSES * NUM_USED_CLASSES - 1 ];

    if ( noremort == false ) {
	//
	// create list of remort classes
	//

	class_combo *d = remort_class_data;

	for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {
	    for ( int j = 0; j < NUM_USED_CLASSES; ++j ) {
		d->mort_class = i;
		d->remort_class = j;

		for ( int k = 0; k < 4; ++k ) {
		    d->count[ k ] = player_classes[ k ][ i ][ j ];
		}
		++d;
	    }
	}

	//
	// sort list of remort classes
	//

	for ( int i = 0; i < NUM_USED_CLASSES * NUM_USED_CLASSES; ++i ) {

	    for ( int j = i+1; j < NUM_USED_CLASSES * NUM_USED_CLASSES; j++ ) {
	    
		if ( remort_class_data[j].count[ align_sort ] > remort_class_data[i].count[ align_sort ] ) {
		
		    class_combo tmp_c = remort_class_data[j];
		    remort_class_data[j] = remort_class_data[ i ];
		    remort_class_data[i] = tmp_c;
		}
	    }
	}

    }    

    //
    // output results
    //
    
    cout << "Results:  " 
	 << setw( 4 ) << num_players[ ALIGN_ALL ] << " players counted." << endl
	 << "Sorted on alignment: " << align_str[ align_sort ] << endl << endl
	 << "  ALIGN       |   MORTALS   REMORTS     TOTAL" << endl
	 << "---------------------------------------------" << endl;

    
    for ( int i = 3; i >= 0; --i ) {
	cout << "  " << setw( 8 ) << align_str[ i ] << "    |  "
	     << setw( 8 ) << num_morts[ i ] << "   " 
	     << setw( 7 ) << num_remorts[ i ] << "    "
	     << setw( 6 ) << num_players[ i ] << endl;
    }
    
    cout << "---------------------------------------------" << endl << endl;

    if ( nomatrix == false ) {

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
		    cout << setw( 4 ) << player_classes[ align_flag ][ i ][ j ] << " ";
		}
	    }
	    cout << endl;
	}
    }


    if ( nomort == false ) {
	cout << endl << "   Top Mort Classes:|       TOTAL |        GOOD |     NEUTRAL |        EVIL " << endl;

	for ( int i = 0; i < NUM_USED_CLASSES; ++i ) {

	    // skip vampires
	    // skip warriors

	    if ( mort_class_data[ i ].mort_class == CLASS_VAMPIRE || mort_class_data[ i ].mort_class == CLASS_WARRIOR )
		continue;

	    cout << setw( 3 ) << " - " << local_char_class_abbrevs[ mort_class_data[ i ].mort_class ] << "            ";

	    for ( int align = 0 ; align < 4; ++align ) { 
		cout << " | " 
		     << setw( 4 ) << mort_class_data[ i ].count[ align ] << "   "
		     << setw( 2 ) << ( mort_class_data[ i ].count[ align ] * 100 / num_morts[ ALIGN_ALL ] ) << " %";
	    }
	    cout << endl;

	}
    }

    if ( noremort == false ) {
//	cout << endl << "   Top Mort Classes:|       TOTAL |        GOOD |     NEUTRAL |        EVIL " << endl;
	cout << endl << "   Top ReMort Class:|       TOTAL |        GOOD |     NEUTRAL |        EVIL " << endl;
//	cout << endl << "   Top Re-Mortal Classes:" << endl;

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
		 << local_char_class_abbrevs[ remort_class_data[i].remort_class ] << "     ";
	    for ( int align = 0 ; align < 4; ++align ) { 
		cout << " | " 
		     << setw( 4 ) << remort_class_data[ i ].count[ align ] << "   "
		     << setw( 2 ) << ( remort_class_data[ i ].count[ align ] * 100 / num_remorts[ ALIGN_ALL ] ) << " %";
	    }
	    cout << endl;
	}
    
    }
}

int main( int argc, char **argv ) {

    if (argc < 2)
	printf("Usage: %s [options] <playerfile>\n", argv[0]);
    else {
	for ( int i = 1; i < argc; ++i ) {

	    //
	    // no more options, must be filename
	    //

	    if ( strncmp( argv[ i ], "-", 1 ) ) {
		get_stats( argv[ i ] );
		return 0;
	    }
	    
	    if ( !strcasecmp( argv[ i ], "-align" ) ) {
		if ( ++i >= argc ) {
		    cout << "-align reqires an argument, good, neutral, or evil" << endl;
		    return 1;
		}
	    
		for ( int j = 0; j < 4; ++j ) {
		    if ( !strcasecmp( argv[ i ], align_str[ j ] ) ) {
			align_flag = j;
			break;
		    }
		}
	    }

	    if ( !strcasecmp( argv[ i ], "-sort" ) ) {
		if ( ++i >= argc ) {
		    cout << "-sort reqires an argument, good, neutral, or evil" << endl;
		    return 1;
		}
	    
		for ( int j = 0; j < 4; ++j ) {
		    if ( !strcasecmp( argv[ i ], align_str[ j ] ) ) {
			align_sort = j;
			break;
		    }
		}
	    }

	    else if ( !strcasecmp( argv[ i ], "-nomatrix" ) ) {
		nomatrix = true;
	    }
	    
	    else if ( !strcasecmp( argv[ i ], "-nomort" ) ) {
		nomort = true;
	    }

	    else if ( !strcasecmp( argv[ i ], "-noremort" ) ) {
		noremort = true;
	    }
	    else {
		cout << "bad option: " << argv[ i ] << endl;
		return 1;
	    }
	}

	cout << "A playerfilename must be supplied." << endl;
    }
    return 1;
}

















