#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "oldstructs.h"
#include "newstructs.h"

#define IS_SET(flag,bit)  ((flag) & (bit))

void convert (struct obj_file_elem *oldob, struct NEWobj_file_elem *newob)
{
  int i;

  strncpy(newob->short_desc, oldob->short_desc, EXDSCR_LENGTH);
  strncpy(newob->name, oldob->name, EXDSCR_LENGTH);

  newob->item_number      = oldob->item_number;
  newob->value[0]         = oldob->value[0];
  newob->value[1]         = oldob->value[1];
  newob->value[2]         = oldob->value[2];
  newob->value[3]         = oldob->value[3];
  newob->extra_flags      = oldob->extra_flags;
  newob->extra2_flags     = oldob->extra2_flags;
  newob->bitvector[0]     = 0;
  newob->bitvector[1]     = 0;
  newob->bitvector[2]     = 0;
  newob->weight           = oldob->weight;
  newob->timer            = oldob->timer;
  newob->contains         = oldob->contains;
  newob->in_room_vnum     = oldob->in_room_vnum;
  newob->wear_flags       = oldob->wear_flags;
  newob->damage           = oldob->damage;
  newob->max_dam          = oldob->max_dam;
  newob->material         = 0;
  newob->worn_on_position = oldob->worn_on_position;
  newob->plrtext_len      = 0;
  newob->type             = oldob->type;
  newob->sparebyte1       = oldob->sparebyte1;
  newob->sparebyte2       = oldob->sparebyte2;
  newob->spareint1        = 0;
  newob->spareint2        = 0;
  newob->spareint3        = 0;
  newob->spareint4        = 0;


   for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    newob->affected[i].location = oldob->affected[i].location;
    newob->affected[i].modifier = oldob->affected[i].modifier;
  }

}

#define TMPFILE ".__tmpobj__"

void
main (int argc, char **argv)
{
    FILE *infile, *outfile;
    int  i, j, e=0;
    struct obj_file_elem    oldob;
    struct rent_info rent_header;
    struct NEWobj_file_elem newob;

    if (argc == 1)  {
      printf("SYNTAX:  %s <objfile> <objfile> ...\n",argv[0]);
      exit(0);
    }

    for (i=1;i<argc;i++) {
      if (!(infile = fopen(argv[i],"r"))) {
	printf("ERROR: Could not open %s for reading.\n",argv[i]);
        e++;
	break;
      }
      if (!(outfile = fopen(TMPFILE,"w"))) {
	printf("ERROR: Could not open %s for writing.\n",TMPFILE);
        fclose(infile);
	e++;
	break;
      }

      j = 0;
      printf("  Converting %s...  ", argv[i]);

      while (fread(&oldob, sizeof(struct obj_file_elem), 1, infile)) {
        j++;
        convert(&oldob, &newob);
        fwrite (&newob, sizeof(struct NEWobj_file_elem), 1, outfile);
      }
      fclose(infile);
      fclose(outfile);
      if (rename(TMPFILE,argv[i])) {
	printf("\nERROR: Unable to store new obj file as %s.\n",argv[i]);
	e++;
      }
      else {
	printf("%d objects.\n",j);
      }
    }
    printf("%d files converted, %d ignored.\n", argc-e-1, e);
}

