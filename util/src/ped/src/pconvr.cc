#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "structs.h"

#define IS_SET(flag,bit)  ((flag) & (bit))

int
main (int argc, char **argv)
{
  FILE	*infile, *outfile;
  int		recs, size, i, ret = 0, del = 0;
  char	buf[1024], max_person[1024];
  struct char_file_u		oldst;
  time_t tm;
  int max_money = 0, cur_money;

  if (((argc != 2) && (argc != 3)) || !(infile = fopen(argv[1],"r")))  {
    printf("SYNTAX:  %s pfile <newpfile>\n",argv[0]);
    exit(0);
  }

  if (argc == 3)
    strcpy(buf,argv[2]);
  else
    sprintf(buf,"%s.new",argv[1]);

  if (!(outfile = fopen(buf,"w")))  {
    printf("ERROR: could not open output file: %s\n",buf);
    exit(0);
  }

  fseek(infile, 0L, SEEK_END);
  size = ftell(infile);
  rewind(infile);
  if (size % sizeof(struct char_file_u))  {
    printf("ERROR:  The input file is the wrong size.\n");
    exit(0);
  }
  recs = size / sizeof(struct char_file_u);
  if (recs)  {
    sprintf(buf, "   %d players in database.", recs);
  } else {
    exit(0);
  }
 
  tm = time(NULL);
  for (i=0;i<recs;i++)  {
    fread (&oldst, sizeof(struct char_file_u), 1, infile);
    if (oldst.level < 50) {
      cur_money = oldst.points.gold + oldst.points.credits + 
	oldst.points.bank_gold + oldst.points.cash;
      if (max_money < cur_money) {
	max_money = cur_money;
	strcpy(max_person, oldst.name);
      }

      printf("   Converting %s.  Level %d (money %9d -> %9d).\n", 
	     oldst.name, oldst.level,
	     cur_money, cur_money / 20);
      oldst.points.gold =  cur_money / 20;
      if (oldst.remort_char_class >= 0) {
	printf("     REMORT ZEROING %30s lev %2d (gen %2d)\n", oldst.name, oldst.level, oldst.player_specials_saved.remort_generation);
	oldst.level = 0;
      }
    }
    fwrite (&oldst, sizeof(struct char_file_u), 1, outfile);
    ret++;
  }

  printf("File Converted, %4d chars retained, %4d deleted.\n", ret, del);
  printf("Maximum money: %d on %s\n", max_money, max_person);
  exit(1);
}
