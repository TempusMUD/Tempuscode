struct NEWobj_file_elem {
  obj_num item_number;
  
  char short_desc[EXDSCR_LENGTH];
  char name[EXDSCR_LENGTH];
  int	value[4];
  int	extra_flags;
  int   extra2_flags;
  int   bitvector[3];
  int	weight;
  int	timer;
  int   contains;	/* # of items this item contains */
  int   in_room_vnum;
  int   wear_flags;     /* positions which this can be worn on */
  int   damage;
  int   max_dam;
  int   material;
  unsigned int plrtext_len;
  byte  worn_on_position;
  byte  type;
  byte  sparebyte1;
  byte  sparebyte2;
  int   spareint1;
  int   spareint2;
  int   spareint3;
  int   spareint4;
  struct obj_affected_type affected[MAX_OBJ_AFFECT];
};
