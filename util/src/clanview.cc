#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>

#include "structs.h"
#include "clan.h"
#include "utils.h"

#define MODE_BRIEF 0
#define MODE_NORMAL 1
#define MODE_FULL 2
#define MODE_FILE 99

int display_clan_info(FILE *fl, int type, int file);

int
main(int argc, char **argv)
{
  char filename[100];
  int mode, to_file;
  FILE *fp;

  mode = atoi(argv[1]);
 
 if(argv[2]) {
     to_file = atoi(argv[2]); // 0 means not to file 1 writes to file
  }
 else{
     to_file = 0;
 }

  if (mode != MODE_FILE){
    sprintf(filename, "../../lib/etc/clans" ); 
  }

  else{
    sprintf(filename, "%s", argv[1]);
  }
  
  if (!(fp = fopen(filename, "r"))) {  
    puts("Error opening clan file."); 
    return 1;
  }

display_clan_info(fp, mode, to_file);

}



int display_clan_info(FILE *fl, int type, int file) {

    ofstream fout("Clanview.out", ios::app);
    char buf[4096];
    char rank_buf[4096];
    struct clan_file_elem_hdr    clan_hdr;
    int i;
 

    if (type == MODE_BRIEF) {
	while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, fl)) {
	    sprintf(buf, "%d) Clan: %s    Badge: %s    Bank: %d    \n\n", clan_hdr.number, clan_hdr.name, clan_hdr.badge, clan_hdr.bank_account);
	    cout << buf;
	    
	    if(file){
		fout << buf;
	    }
	    
	}
	
    }
    
    else if (type == MODE_NORMAL) {
	while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, fl)) {
	    
	    sprintf(buf, "%d) Clan: %s    Badge: %s      Bank: %d \n", clan_hdr.number, clan_hdr.name, clan_hdr.badge, clan_hdr.bank_account );
	    sprintf(buf, "%sTop Rank:  %d               Flags: %d\n", buf, clan_hdr.top_rank, clan_hdr.flags);
	    sprintf(buf, "%sMembers:   %d               Rooms: %d\n", buf, clan_hdr.num_members, clan_hdr.num_rooms);
	    sprintf(buf, "%sOwner: %ldl\n", buf, clan_hdr.owner);
	    cout << buf << "\n\n";
	   
	    if(file) {
		fout << buf << "\n";
	    }
	    
	}
	
    }

    else if (type == MODE_FULL ){ 
	while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, fl)) {
	    
	    sprintf(buf, "%d) Clan: %s    Badge: %s      Bank: %d \n", clan_hdr.number, clan_hdr.name, clan_hdr.badge, clan_hdr.bank_account );
	    sprintf(buf, "%sTop Rank:  %d               Flags: %d\n", buf, clan_hdr.top_rank, clan_hdr.flags);
	    sprintf(buf, "%sMembers:   %d               Rooms: %d\n", buf, clan_hdr.num_members, clan_hdr.num_rooms);
	    sprintf(buf, "%sOwner: %ld\n", buf, clan_hdr.owner);
	    sprintf(buf, "%sClan Ranks:\n", buf);
	    for(i = clan_hdr.top_rank; i >= 0; i--) {
		sprintf(rank_buf, "(%1d) %s\n", i, clan_hdr.ranknames[i]);
	    }
	    strcat(buf, rank_buf); 
	    cout << buf << "\n\n";
	  
	    if(file) {
		fout << buf << "\n\n";
	    }
	    
	}
    
	
    
    }
    if(file){
	fout << "**************************************************************************\n";
    }
    
    return type;
}










