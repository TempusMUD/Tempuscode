//
// scans rentfiles for objects which satisfy the parameters
//

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <string>

#include "structs.h"
#include "utils.h"

int vnum = -1;
char *name = 0;
char *alias = 0;

void scan_file( char *filename ) {

    FILE *fl = fopen( filename, "r" );

    if ( !fl ) {
	cerr << "Error opening file " << filename << " : " << strerror( errno ) << endl;
	return;
    }
    
    struct rent_info rent_header;

    fread( &rent_header, sizeof( struct rent_info ), 1, fl );

    struct obj_file_elem o;

    while ( fread( &o, sizeof( struct obj_file_elem ), 1, fl ) ) {
	if ( vnum >= 0 && o.item_number != vnum )
	    continue;
	else if ( name && strcmp( name, o.short_desc ) )
	    continue;
	else if ( alias )
	    continue;

	cout << " FILE: " << setw( 14 ) << filename << "  ITEM #: " << o.item_number << endl;
    }

    fclose( fl );
}

void show_usage( char *exe ) {
    cerr << "Usage: " << exe << " [options] <rentfile ... >" << endl
	 << "       Options: " << endl
	 << "             -vnum <vnum>" << endl
	 << "             -name <name>" << endl
	 << "             -alias <alias>" << endl;
}

int main( int argc, char **argv ) {

    bool in_filenames = false;

    if ( argc < 2 ) {
	show_usage( argv[0] );
	return 1;
    }

    for ( int i = 1; i < argc; ++i ) {

	if ( *argv[i] != '-' ) {
	    in_filenames = true;
	    scan_file( argv[i] );
	    continue;
	}

	if ( !strcmp( argv[i++], "-vnum" ) ) {
	    if ( i >= argc ) {
		cerr << "Error: " << argv[i-1] << " requires an argument." << endl;
		return 1;
	    }
	    vnum = atoi( argv[i] );
	    continue;
	}
	
	else if ( !strcmp( argv[i++], "-name" ) ) {
	    if ( i >= argc ) {
		cerr << "Error: " << argv[i-1] << " requires an argument." << endl;
		return 1;
	    }
	    name = argv[i];
	    continue;
	}
	    
	else if ( !strcmp( argv[i++], "-alias" ) ) {
	    if ( i >= argc ) {
		cerr << "Error: " << argv[i-1] << " requires an argument." << endl;
		return 1;
	    }
	    alias = argv[i];
	    continue;
	}
    }
    return 0;
}
	    
	    

    
    
