#ifndef __FLAGS_H__
#define __FLAGS_H__

#include <bvector.h>
#include <vector.h>
#include <map.h>
#include <algorithm>
#include <string>
using namespace std;
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>

#include "tokenizer.h"
#include "tmpstr.h"
#include "utils.h"
#include "xml_utils.h"

/** Struct to represent a flag object. **/
class Flag {
	public:
		Flag( const char *_name, const char *_bits, const char *_desc ) 
			: index(-1), name(_name),bits(_bits),desc(_desc) { }


		Flag( xmlChar *_name, xmlChar *_bits, xmlChar *_desc ) 
			: index(-1), name((char*)_name),bits((char*)_bits),desc((char*)_desc) { }
		
		int index;  // This Flag's index in the FlagTable
		string name;
		string bits;// The string used to represent this flag in sprintbits
		string desc;// 
	private:
		Flag() : index(-1), name(), bits(), desc() { }
};

/**
 * Table of name/bits/desc/index mappings for bitvector objects ( FlagSet );
**/
class FlagTable : protected vector<Flag> 
{
	private:
		string name;//the name of this FlagTable ( e.g. "Player Preferences" )
		map<string,Flag*> nameIndex;
		map<string,Flag*> bitsIndex;
		map<string,Flag*> descIndex;
	public:
		FlagTable( const char* name ) 
			: vector<Flag>(), nameIndex(), bitsIndex(), descIndex() 
		{
			this->name = name;
		}

		~FlagTable() {
        }

		// Initializes this FlagTable by add()'ing each FLAG entry
		bool initialize( xmlNodePtr flagTableDef ) {
            return false;
        }

        // Retrieves the name of this table
        const char* getName() { return name.c_str(); }
            
        int getIndexForName( const char *name ) const;
		int getIndexForBits( const char *bits ) const;
		int getIndexForDesc( const char *desc ) const;
        

		const char* getName( int index ) const;
		const char* getBits( int index ) const;
		const char* getDesc( int index ) const;

		void add( const char *name, const char *bits, const char *desc ) {
			Flag f( name, bits, desc );
			f.index = (int)(size());
			push_back(f);
			//TODO: Add to maps
		}

		// a FLAGTABLE tag containing FLAG's
		// <FLAG NAME="noecho" DESC="No Echo" BITS="NOECHO" />
		void add( xmlNodePtr flagDef );

		// vector<Flag> inherited members
		vector<Flag>::size;
};

/**
 * A bitvector class intended to be stored in the thing to be flagged.
 *
 * e.g. a Creature would have "FlagSet creatureFlags" within it.
 * 		an obj_data would have "FlagSet objectFlags( objectFlagTable )" 
 * 		within it.
 *
 * ***  Loading these flags from file ***
 * I see two ways:
 * 	1, pass in a string containing the bits of all flags to set true
 *	   "NOECHO NOSPEW PKILLER"
 * 	   void set( string &bits );
 *
**/
class FlagSet 
{
	private:
		bit_vector data;
		const FlagTable *table;
	public:
		FlagSet( const FlagTable &table ) : data() {
			this->table = &table;
			data.reserve(table.size());
			data.assign( table.size(), false );
		}

		bool isSet( const char* bit ) const { 
			return isSetBit( bit ); 
		}
		bool isSetBit(  const char *bit ) const;
		bool isSetDesc( const char *desc ) const;
		bool isSetName( const char *name ) const;

		/**
		 * pass in a string containing the bits of all flags to set true
		 * "NOECHO NOSPEW PKILLER" ( without the '"'s )
		**/
		void set( const char *bits, bool value ) {
			Tokenizer tokens(bits);
			char bit[ strlen(bits) + 1 ];
			while( tokens.next(bit) ) {
				setByBit( bit, value );
			}
		}
		/**
		 * Pass in the FLAGSET xmlNode to have loaded.
		 *<FLAGSET NAME="Player Preferences" BITS="NOECHO NOSPEW PKILLER" />
		**/
		void load( xmlNodePtr *flagSet );
		/** creates and stores a FLAGSET node in the given parent node **/
		void store( xmlNodePtr *parent );
		/** writes this flagset out as XML to the given ostream **/
		void save( std::ostream &out );
		/** writes this flagset out as XML to the given file **/
		void save( FILE *out );

		void setByBit( const char *bit, bool value );
		void setByName( const char* name, bool value );
		void setByDesc( const char *desc, bool value );

		/** Creates a string representation of this FlagSet **/
		const char* toString() const {
			char *s = "";
			for( bit_vector::size_type i = 0; i < data.size(); i++ ) {
				if( data[i] ) {
					s = tmp_strcat( s, table->getBits(i) );
				}
			}
			return s;
		}
};

#endif // __FLAGS_H__


/***  flags.cc ***/

/**
 *  Global FlagTable objects exist only in flags.cc by FlagSet
 *  instances.
 *
 *  Add FlagTables here to have them exist. 
 *  Add them to boot_flagsets to have them initialized.
**/
FlagTable playerPrefTable( "Player Preferences" );
FlagTable mobileFlagTable( "Mobile Flags" );
FlagTable creatureAffectTable( "Creature Affections" );
FlagTable objectFlagTable( "Object Flags" );
FlagTable roomFlagTable( "Room Flags" );


/**
 * Opens the flagtables file lib/etc/flagtables.xml and initializes the
 * flag tables with it.
**/
void 
boot_flagsets() {
	// A temp vector to simplify the xmlDoc processing code
	vector<FlagTable*> tables;
	
	// Add FlagTable's here to have them initialized at boot time.
	tables.push_back(&playerPrefTable);
	tables.push_back(&mobileFlagTable);
	tables.push_back(&creatureAffectTable);
	tables.push_back(&objectFlagTable);
	tables.push_back(&roomFlagTable);
	
	xmlDocPtr doc = xmlParseFile("etc/flagtables.xml");
	if (doc == NULL) {
		slog("SYSERR: Failed to load flag tables. Exiting."); 
		safe_exit(1);
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		xmlFreeDoc(doc);
		slog("SYSERR: Failed to load flag tables. Exiting."); 
		safe_exit(1);
	}

	cur = cur->xmlChildrenNode;
	// Load all the nodes in the file
	while (cur != NULL) {
		// But only question nodes
		if ((xmlMatches(cur->name, "FLAGTABLE"))) {
			char *name = xmlGetProp(cur,"NAME");
			for( unsigned int i = 0; i < tables.size(); i++ ) {
				if( strcmp( tables[i]->getName(), name ) == 0 ) {
					tables[i]->initialize( cur );
				}
			}
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
}

/*** end flags.cc ***/
