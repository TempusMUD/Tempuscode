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

    printf("oload %d; ", object.item_number);

    if (object.plrtext_len) {
      fread(buf, object.plrtext_len, 1, infile);
    }

  }
  fclose(infile);
}
