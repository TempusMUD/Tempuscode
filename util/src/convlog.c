#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

int
main(int argc, char *argv[])
{
  FILE *fl, *outfl;
  int i, j, logcount = 0, filesize, filepos, in_slog = 0, nearend, slog_flags = 0;
  char buf[1024], *inbuf, outbuf[4136], tmp_buf[1024];

  for (i = 1; i < argc; i++) {
    if (!(fl = fopen(argv[i], "r"))) {
      printf("Error opening file '%s'\n", argv[i]);
      continue;
    }
    strcpy(buf, argv[i]);
    strcat(buf, ".new");
    if (!(outfl = fopen(buf, "w"))) {
      printf("Error opening file '%s'\n", buf);
      fclose(fl);
      continue;
    }
    fseek(fl, 0, SEEK_END);
    filesize = ftell(fl);
    inbuf = malloc(filesize);
    fseek(fl, 0, SEEK_SET);
    nearend = filesize - 4;
    if (!(fread(inbuf, filesize, 1, fl))) {
      printf("Error reading file '%s'\n", argv[i]);
      free(inbuf);
      fclose(fl);
      fclose(outfl);
      continue;
    }

    for (filepos = 0; filepos < filesize; ) {
      for (j = 0; j < 4096 && filepos < filesize; j++) {
	if (in_slog == 4 && !slog_flags && inbuf[filepos] == ')') {
	  in_slog = 0;
	  outbuf[j] = '\0';
	  sprintf(tmp_buf, ", %d)", logcount++);
	  j += strlen(tmp_buf);
	  strcat(outbuf, tmp_buf);
	  filepos++;
	  break;
	}
	outbuf[j] = inbuf[filepos];
	
	if (in_slog == 0 && outbuf[j] == 's')
	  in_slog = 1;
	else if (in_slog == 1 && outbuf[j] == 'l')
	  in_slog = 2;
	else if (in_slog == 2 && outbuf[j] == 'o')
	  in_slog = 3;
	else if (in_slog == 3 && outbuf[j] == 'g') {
	  in_slog = 4;
	  slog_flags = 1;
	} else if (in_slog == 4) {
	  if (slog_flags == 1) {
	    if (outbuf[j] == '(')
	      slog_flags = 0;
	    else if (outbuf[j] != ' ')
	      in_slog = 0;
	  }
	  if (outbuf[j] == '"') {
	    if (slog_flags == 2)
	      slog_flags = 0;	    
	    else
	      slog_flags = 2;
	  }
	} else
	  in_slog = 0;

	filepos++;
      }
      if (!(fwrite(outbuf, j, 1, outfl))) {
	printf("Error writing to file '%s'\n", buf);
	break;
      }
    }
    free(inbuf);
    fclose(outfl);
    fclose(fl);
    strcpy(tmp_buf, argv[i]);
    strcat(tmp_buf, ".old");
    rename(argv[i], tmp_buf);
    rename(buf, argv[i]);
  }
  exit(1);
}
