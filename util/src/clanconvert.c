#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../structs.h"
#include "oldclan.h"
#include "newclan.h"

void
convert_header(struct clan_file_elem_hdr *clan_hdr, struct NEWclan_file_elem_hdr *NEWclan_hdr)
{
  int i;

  NEWclan_hdr->number = clan_hdr->number;
  NEWclan_hdr->bank_account = clan_hdr->bank_account;
  NEWclan_hdr->top_rank = clan_hdr->top_rank;
  NEWclan_hdr->num_members = clan_hdr->num_members;
  NEWclan_hdr->num_rooms = clan_hdr->num_rooms;
  NEWclan_hdr->owner = 1;
  NEWclan_hdr->flags = 0;

  strncpy(NEWclan_hdr->name,  clan_hdr->name,  NEW_MAX_CLAN_NAME);
  strncpy(NEWclan_hdr->badge, clan_hdr->badge, NEW_MAX_CLAN_BADGE);

  for (i = 0; i < NEW_NUM_CLAN_RANKS; i++) {
    if (i < NUM_CLAN_RANKS)
      strncpy(NEWclan_hdr->ranknames[i], clan_hdr->ranknames[i], NEW_MAX_CLAN_RANKNAME);
    else
      NEWclan_hdr->ranknames[i][0] = '\0';
  }

  for (i = 0; i < 20; i++)
    NEWclan_hdr->cSpares[i] = 0;
}   

int
main(int argc, char **argv)
{
  FILE *ifl, *ofl;
  char buf[4096];
  struct clan_file_elem_hdr    clan_hdr;
  struct NEWclan_file_elem_hdr NEWclan_hdr;
  int i;
  int member_id;             // tmp var
  byte member_rank;          // tmp var
  int virt;                  // tmp var

  if (argc < 2) {
    puts("Usage: clanconvert <filename>");
    return 1;
  }

  sprintf(buf, "%s.new", argv[1]);

  if (!(ifl = fopen(argv[1], "r"))) {
    puts("Error opening input file.");
    return 1;
  }
  
  if (!(ofl = fopen(buf, "w"))) {
    puts("Error opening output file.");
    return 1;
  }

  while (fread(&clan_hdr, sizeof(struct clan_file_elem_hdr), 1, ifl)) {

    // convert header and write out to output file
    convert_header(&clan_hdr, &NEWclan_hdr);

    fwrite(&NEWclan_hdr, sizeof(struct NEWclan_file_elem_hdr), 1, ofl);

    // copy members
    for (i = 0; i < clan_hdr.num_members; i++) {
      // read member data from input file
      fread(&member_id, sizeof(long), 1, ifl);
      fread(&member_rank, sizeof(byte), 1, ifl);

      // write back out to output file
      fwrite(&member_id, sizeof(long), 1, ofl);
      fwrite(&member_rank, sizeof(byte), 1, ofl);

    }

    // copy rooms
    for (i = 0; i < clan_hdr.num_rooms; i++) {
      fread(&virt, sizeof(int), 1, ifl);
      fwrite(&virt, sizeof(int), 1, ofl);
    }

  }
  return 0;
}

