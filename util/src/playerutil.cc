/* ************************************************************************
*  file:  passwd.c                                    Part of CircleMud   *
*  Usage: changing passwords of chars in a Diku playerifle                *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include <stdio.h>
#include <ctype.h>
#include <crypt.h>
#include <unistd.h>

#include "../../src/include/structs.h"

#define FALSE 0
#define TRUE 1

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#endif

const int MODE_PASSWORD = 1;
const int MODE_LEVEL    = 2;

int mode = 0;

int        str_eq(char *s, char *t)
{
   for (; ; ) {
      if (*s == 0 && *t == 0) 
         return TRUE;
      if (tolower(*s) != tolower(*t)) 
         return FALSE;
      s++;
      t++;
   }
}

void process(char *filename, char *name, char *argument)
{
   FILE * fl;
   struct char_file_u buf;
   int        found = FALSE;
   int level = 0;

   if (!(fl = fopen(filename, "r+"))) {
      perror(filename);
      exit(1);
   }


   if( mode == MODE_LEVEL && !(isdigit(*argument))) {
       printf("Invalid level.\n");
       return;
   } else {
       level = atoi(argument);
   }

   for (; ; ) {
      fread(&buf, sizeof(buf), 1, fl);
      if (feof(fl))
         break;

      if (str_eq(name, buf.name)) {
         found = TRUE;
         switch( mode ) {
            case MODE_PASSWORD:
                strncpy(buf.pwd, crypt(argument, buf.name), MAX_PWD_LENGTH);
                break;
            case MODE_LEVEL:
                buf.level = level;
                break;
         }
         if (fseek(fl, -1L * sizeof(buf), SEEK_CUR) != 0) 
            perror("fseek");
         if (fwrite(&buf, sizeof(buf), 1, fl) != 1) 
            perror("fwrite");
         if (fseek(fl, 0L, SEEK_CUR) != 0) 
            perror("fseek");
      }
   }
   if (found) {
       switch( mode ) {
            case MODE_PASSWORD:
                printf("%s password is now %s\n", name, argument);
                break;
            case MODE_LEVEL:
                printf("%s is now level %d\n", name, level);
                break;
       }
   } else {
      printf("%s not found\n", name);
   }

   fclose(fl);
}


int main(int argc, char **argv)
{
    mode = 0;
    if (argc != 5) {
        fprintf(stderr, "Usage: %s playerfile <mode> character-name <argument>\n", argv[0]);
        return 1;
    }
    if(strcmp(argv[2],"level") == 0 ){
        mode = MODE_LEVEL;
    } else if( strcmp(argv[2],"password") == 0 ) {
        mode = MODE_PASSWORD;
    } else {
        fprintf(stderr, "Invalid Mode: %s\n", argv[2]);
        fprintf(stderr, "Usage: %s playerfile <mode> character-name <argument>\n", argv[0]);
        return 1;
    }
    process(argv[1], argv[3], argv[4]);
}


