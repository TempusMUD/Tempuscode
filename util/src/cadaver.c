#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct player_data {
  char name[1024];
  char list[50][1024];
  int num;
  int unique[50];
  int num_unique;
  int dupes[50];
  int num_dupes;
  int score;
} player_data;

player_data players[20];
int num_players = 0;

int
main(int argc, char **argv)
{
  
  char buf[1024];
  char typebuf[1024];
  int ret;
  int i, j, k, l, m;
  int found;

  while (1) {

    gets(buf);
    
    // the end
    if (!strncmp(buf, "end", 3)) {
      break;
    }

    // new player entry
    else if (((ret = sscanf(buf, "%s %s", 
		       players[num_players].name, typebuf)) == 2) &&
	!strcmp(typebuf, "sack")) {
      //      printf("adding '%s'\n", players[num_players].name);
      players[num_players].num = 0;
      players[num_players].num_unique = 0;
      players[num_players].num_dupes  = 0;
      players[num_players].score      = 0;
      num_players++;
    }

    // a corpse entry
    else {
      if ((ret = sscanf(buf, "a steak made from the %s", typebuf)) < 1) {
	printf("Error on line: '%s'\n"
	       "Part of %s's sack, ret %d\n", 
	       buf, players[num_players-1].name, ret);
	exit(1);
      }
      //      printf("adding '%s' at %d\n", typebuf, players[num_players-1].num);
      strcpy(players[num_players-1].list[players[num_players-1].num], typebuf);
      players[num_players-1].num++;
    }
  }

  // parse the results

  // loop through the players
  for (i = 0; i < num_players; i++) {

    // loop this player's corpses
    for (j = 0; j < players[i].num; j++) {
      
      // loop through all players
      for (k = 0, found = 0; k < num_players; k++) {

	// loop through each set of corpses
	for (l = 0; l < players[k].num; l++) {

	  // found a match
	  if (!strcmp(players[i].list[j], players[k].list[l])) {
  
	    // whoops, a duplicate corpse!
	    if (i == k) {
	      // wait, it's it!
	      if (j == l)
		continue;
	      players[i].dupes[players[i].num_dupes] = j;
	      players[i].num_dupes +=1;
	      players[i].score--;
	    }
	    
	    found = 1;
	  }
	}
      }
      // well, it is unique!
      if (!found) {
	players[i].unique[players[i].num_unique] = j;
	players[i].num_unique++;
	players[i].score += 10;
      }
      // its only worth a point
      else {
	players[i].score++;
      }
    }

    printf("%s results:\n"
	   "  %3d total corpses\n"
	   "  %3d duplicates\n",
	   players[i].name, players[i].num, players[i].num_dupes);

    // list duplicates
    for (m = 0; m < players[i].num_dupes; m += 2)
      printf("      --> %s\n", players[i].list[players[i].dupes[m]]);
    
    printf("  %3d unique corpses\n", players[i].num_unique);

    // list uniques
    for (m = 0; m < players[i].num_unique; m++)
      printf("      *** %s\n", players[i].list[players[i].unique[m]]);
    
    printf("  TOTAL SCORE:             %d\n", players[i].score);

  }	  

  exit(1);
}

