/* Program to count lines in a file */
#include <stdio.h>
#include <stdlib.h>

void
main(int argc, char **argv)
{
    int cnt = 0, total=0,i;
    FILE *in;
    char ch;

    if (argc < 2)  {
      printf("syntax: %s <filename>\n",argv[0]);
      exit(0);
    }

    for (i=1;i<argc;i++,cnt=0) {
      if ((in = fopen(argv[i],"r")) == NULL)  {
	fprintf (stderr,"error: could not open file: %s\n",argv[i]);
      }

      while(fread(&ch,1,1,in))
	if (ch == '\n')
	  cnt++;
    
      printf("%s: %d\n",argv[i],cnt);

      total += cnt;
      fclose (in);
    }
      printf("------------\n");
      printf("total: %d\n",total);}
