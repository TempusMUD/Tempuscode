#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "structs.h"

#define IS_SET(flag,bit)  ((flag) & (bit))

void
main (int argc, char **argv)
{
  FILE *infile;
  int  j;
  struct obj_file_elem    object;
  char buf[MAX_STRING_LENGTH];

  if (argc < 2)  {
    printf("SYNTAX:  %s <objfile> <objfile> ...\n",argv[0]);
    exit(0);
  }
  
  if (!(infile = fopen(argv[1],"r"))) {
    printf("ERROR: Could not open %s for reading.\n",argv[1]);
    exit(1);
  }

  j = 0;
  while (fread(&object, sizeof(struct obj_file_elem), 1, infile)) {
    
    printf("OBJ %3d: #%d::\n"
	   "     short_desc: '%s'\n"
	   "     name:       '%s'\n"
	   "     values: [%5d]  [%5d]  [%5d]  [%5d]\n"
	   "     extra: %6d, extra2: %6d\n"
	   "     num_contains: %3d, room_vnum: %5d\n",
	   
	   ++j, object.item_number, object.short_desc, object.name,
	   object.value[0], object.value[1], object.value[2], object.value[3],
	   object.extra_flags, object.extra2_flags, 
	   object.contains, object.in_room_vnum);

    if (object.plrtext_len) {
      fread(buf, object.plrtext_len, 1, infile);
      printf("   --- text (%6d)---\n%s\n"
	     "   ---              ---\n", object.plrtext_len, buf);
    }

  }
  fclose(infile);
  
}

