///////////////////////////////////////////////////////////////
//
// Written by Fireball for TempusMUD May 18, 1998
//
///////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define __MKIMMLIST_C__

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "screen.h"

int noAnsi = 0;

int
print_ansi(char *ansi)
{
    if ( !noAnsi && ansi ) 
	printf(ansi);

    return 0;
}
int
print_spaces(int num)
{
    int i;

    for ( i = 0; i < num; i++ ) {
	printf(" ");
    }

    return 0;
}

int
print_centered(char *Str, char *ansi)
{
    char *Ptr = NULL;
    int len = 0;
    char *tmpPtr, *newStr = strdup(Str);

    if ( ansi && !noAnsi )
	printf(ansi);
    
    tmpPtr = newStr;
    
    for ( ; ; ) {
    
	Ptr = strchr(newStr, '\n');
	
	if ( Ptr ) {
	    len = Ptr - newStr;
	    *Ptr = '\0';
	} 
	else {
	    len = strlen(newStr);
	}

	print_spaces(( 80 - len + 1 ) / 2);


	printf(newStr);
	printf("\n");

	if ( Ptr ) {
	    newStr = Ptr + 1;
	}
	else {
	    break;
	}
	    
    }

    free(tmpPtr);

    if ( ansi && !noAnsi )
	printf(KNRM);

    return 0;
}
	
char *levelTitles[] = {
    "AMBASSADORS",
    "IMMORTALS",
    "BUILDERS",
    "LUMINARIES",
    "ETHEREALS",
    "ETERNALS",
    "DEMIGODS",
    "ELEMENTALS",
    "SPIRITS",
    "BEINGS",
    "POWERS",
    "FORCES",
    "ENERGIES",
    "GODS",
    "DEITIES"
};

char *
make_bar(char c, int len)
{
    static char buf[1024];
    int i;

    for ( i = 0; i < len; i++ ) {

	buf[i] = c;

    }
    
    buf[i] = '\0';

    return (buf);
}
	
    
int
make_immlist(FILE *fl)
{
    struct char_file_u player;
    int curLev = 64;
    int minLev = 50;
    char buf[1024];

    print_centered("********************************************************************\n"
		   "*         I     M     M     O     R     T     A    L    S          *\n"
		   "********************************************************************\n", KCYN);
    
    for ( ; curLev >= minLev; curLev-- ) {

	print_ansi(KYEL);

	print_centered(levelTitles[curLev - minLev], 0);
	
	print_ansi(KBLD);

	print_centered(make_bar('~', 14), 0);

	fseek(fl, 0, SEEK_SET);

	*buf = '\0';

	print_ansi(KBLU);

	for ( ; ; ) {
	    
	    if ( !fread(&player, sizeof(struct char_file_u), 1, fl) ) {
		break;
	    }

	    if ( player.char_specials_saved.act & ( PLR_FROZEN | PLR_NOWIZLIST | PLR_DELETED ) )
		continue;

	    if ( player.level == curLev ) {
		
		if ( *buf ) {
		    if ( (strlen(buf) + strlen(player.name)) > 40 ) {
			print_centered(buf, 0);
			*buf = '\0';
		    }
		    else {
			strcat(buf, "    ");
		    }
		}
		strcat(buf, player.name);
	    }
	}
	
	if ( *buf ) 
	    print_centered(buf, 0);

	print_ansi(KNRM);
	printf("\n");
    }
    return 0;
}


int
main(int argc, char **argv)
{
    FILE *fl;
    if ( argc < 2 ) {
	printf("Usage: %s <player file>\n", argv[0]);
	return 1;
    }

    if ( argc > 2 && !strcasecmp(argv[2], "noansi") ) {
	noAnsi = 1;
    }

    if ( !( fl = fopen(argv[1], "r") ) ) {
	printf("%s: unable to open file '%s' for readn", argv[0], argv[1]);
	return 1;
    }

    make_immlist(fl);
    
    return 0;
}
    
#undef __MKIMMLIST_C__


