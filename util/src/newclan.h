
#define NEW_MAX_CLAN_NAME     MAX_CLAN_NAME
#define NEW_MAX_CLAN_BADGE    MAX_CLAN_BADGE
#define NEW_NUM_CLAN_RANKS    NUM_CLAN_RANKS
#define NEW_MAX_CLAN_RANKNAME MAX_CLAN_RANKNAME

struct NEWclan_file_elem_hdr {
  int  number;
  int  bank_account;
  char name[MAX_CLAN_NAME]; 
  char badge[MAX_CLAN_BADGE];
  char ranknames[NUM_CLAN_RANKS][MAX_CLAN_RANKNAME];
  ubyte top_rank;
  ubyte num_members;
  ubyte num_rooms;
  long owner;
  int flags;
  char  cSpares[20];
};
  
