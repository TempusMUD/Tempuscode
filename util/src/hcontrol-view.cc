
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <errno.h>
#include <string>
#include "structs.h"
#include "house.h"


ostream &operator<<( ostream &os, struct house_control_rec &h ) {
    os << setw( 5 ) << h.owner1 << ", " << setw( 5 ) << h.owner2 << " : ";
    
    for ( int i = 0; i < h.num_of_rooms; ++i )
	os << h.house_rooms[ i ] << " ";
    
    os << endl;
	
    return os;
}


int main( int argc, char **argv ) {

    if ( argc < 2 ) {
	cout << "Usage: " << argv[0] << " <filename>" << endl;
	return 1;
    }

    string filename = argv[1];

    FILE *fl = 0;

    if ( ! ( fl = fopen( filename.c_str(), "r" ) ) ) {
	cout << "Cannot open file " << filename << " : " << strerror( errno ) << endl;
	return 1;
    }
    
    struct house_control_rec h;

    while ( fread( &h, sizeof( struct house_control_rec ), 1, fl ) ) {
	cout << h;
    }
    
    return 0;
}
