#include <stdlib.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
  FILE *fl1, *fl2;
  int pos = 0;
  int c1, c2;

  if (argc < 3)
    exit (1);

  if (!(fl1 = fopen(argv[1], "r"))) {
    printf("unable to open fl1\n");
    exit(1);
  }
  if (!(fl2 = fopen(argv[2], "r"))) {
    printf("unable to open fl2\n");
    exit(1);
  }

  fseek(fl1, 0, SEEK_END);
  printf("file1 is %ld bytes long.\n", ftell(fl1));
  fseek(fl1, 0, SEEK_SET);
  fseek(fl2, 0, SEEK_END);
  printf("file2 is %ld bytes long.\n", ftell(fl2));
  fseek(fl2, 0, SEEK_SET);

  while (1) {
    if ((c1 = fgetc(fl1)) == EOF) {
      printf("EOF encountered at position %d of file 1\n", pos);
      exit(1);
    }
    if ((c2 = fgetc(fl2)) == EOF) {
      printf("EOF encountered at position %d of file 2\n", pos);
      exit(1);
    }
    if (c1 != c2)
      printf("files differ at position %d\n", pos);
     
    pos++;
  }
  exit(1);
}
