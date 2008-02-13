#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "structs.h"
#include "newstructs.h"

#define IS_SET(flag,bit)  ((flag) & (bit))

void 
convert (struct char_file_u *oldst, struct NEWchar_file_u *newst)
{
  int i;

  strncpy (newst->name,         oldst->name,        MAX_NAME_LENGTH+1);
  strncpy (newst->description,  oldst->description, EXDSCR_LENGTH); //longer
  strncpy (newst->title,        oldst->title,       MAX_TITLE_LENGTH+1);
  strncpy (newst->poofin,       oldst->poofin,      MAX_POOF_LENGTH);
  strncpy (newst->poofout,      oldst->poofout,     MAX_POOF_LENGTH);

  newst->sex		= oldst->sex;
  newst->class		= (short)oldst->class;
  newst->remort_class   = (short)oldst->remort_class;
  newst->race		= oldst->race;
  newst->level		= oldst->level;
  newst->hometown	= oldst->hometown;
  newst->birth		= oldst->birth;  
  newst->death		= 0;                      // new
  newst->played		= oldst->played;
  newst->weight		= oldst->weight;
  newst->height		= oldst->height;

  strncpy (newst->pwd,oldst->pwd, MAX_PWD_LENGTH+1);

  newst->char_specials_saved.alignment		
    = oldst->char_specials_saved.alignment;
  newst->char_specials_saved.idnum		
    = oldst->char_specials_saved.idnum;
  newst->char_specials_saved.act			
    = oldst->char_specials_saved.act;
  newst->char_specials_saved.act2			
    = oldst->char_specials_saved.act2;
  newst->char_specials_saved.affected_by		
    = oldst->char_specials_saved.affected_by;
  newst->char_specials_saved.affected2_by		
    = oldst->char_specials_saved.affected2_by;
  newst->char_specials_saved.affected3_by		
    = oldst->char_specials_saved.affected3_by;

  newst->char_specials_saved.apply_saving_throw[0]= 
    oldst->char_specials_saved.apply_saving_throw[0];
  newst->char_specials_saved.apply_saving_throw[1]= 
    oldst->char_specials_saved.apply_saving_throw[1];
  newst->char_specials_saved.apply_saving_throw[2]= 
    oldst->char_specials_saved.apply_saving_throw[2];
  newst->char_specials_saved.apply_saving_throw[3]= 
    oldst->char_specials_saved.apply_saving_throw[3];
  newst->char_specials_saved.apply_saving_throw[4]= 
    oldst->char_specials_saved.apply_saving_throw[4];
  newst->char_specials_saved.apply_saving_throw[5]= 0;
  newst->char_specials_saved.apply_saving_throw[6]= 0;
  newst->char_specials_saved.apply_saving_throw[7]= 0;
  newst->char_specials_saved.apply_saving_throw[8]= 0;
  newst->char_specials_saved.apply_saving_throw[9]= 0;

  for (i=0;i<=MAX_SKILLS;i++)  {
    newst->player_specials_saved.skills[i]	
      = oldst->player_specials_saved.skills[i];
  }
  for (i = 0; i < MAX_WEAPON_SPEC; i++) {                 //  new 
    newst->player_specials_saved.weap_spec[i].vnum = -1;
    newst->player_specials_saved.weap_spec[i].level = 0;
  }
  //  newst->player_specials_saved.PADDING0		
  //    = oldst->player_specials_saved.PADDING0;
  //for (i=0;i<MAX_TONGUE;i++)  {
  //newst->player_specials_saved.talks[i]	
  //      = oldst->player_specials_saved.talks[i];
  //  }
  newst->player_specials_saved.wimp_level		
    = oldst->player_specials_saved.wimp_level;
  newst->player_specials_saved.freeze_level	
    = oldst->player_specials_saved.freeze_level;
  newst->player_specials_saved.invis_level	
    = oldst->player_specials_saved.invis_level;
  newst->player_specials_saved.load_room		
    = oldst->player_specials_saved.load_room;
  newst->player_specials_saved.pref		
    = oldst->player_specials_saved.pref;
  newst->player_specials_saved.pref2		
    = oldst->player_specials_saved.pref2;
  newst->player_specials_saved.bad_pws		
    = oldst->player_specials_saved.bad_pws;
  newst->player_specials_saved.conditions[0]	
    = oldst->player_specials_saved.conditions[0];
  newst->player_specials_saved.conditions[1]	
    = oldst->player_specials_saved.conditions[1];
  newst->player_specials_saved.conditions[2]	
    = oldst->player_specials_saved.conditions[2];

  newst->player_specials_saved.clan  		
    = oldst->player_specials_saved.clan;
  newst->player_specials_saved.hold_home  	
    = oldst->player_specials_saved.hold_home;
  newst->player_specials_saved.remort_invis_level 
    = oldst->player_specials_saved.remort_invis_level;
  newst->player_specials_saved.broken_component	
    = oldst->player_specials_saved.broken_component;
  newst->player_specials_saved.remort_generation  
    = oldst->player_specials_saved.remort_generation; 

  newst->player_specials_saved.quest_points = 0; // new
  newst->player_specials_saved.spare_c[0]		
    = oldst->player_specials_saved.spareu1;
  newst->player_specials_saved.spare_c[1] =   // new
    newst->player_specials_saved.spare_c[2] =  
    newst->player_specials_saved.spare_c[3] = 0;
  newst->player_specials_saved.deity		
    = oldst->player_specials_saved.deity;
  newst->player_specials_saved.spells_to_learn	
    = oldst->player_specials_saved.spells_to_learn;
  newst->player_specials_saved.life_points	
    = oldst->player_specials_saved.life_points;
  newst->player_specials_saved.pkills		
    = oldst->player_specials_saved.pkills;
  newst->player_specials_saved.mobkills   	
    = oldst->player_specials_saved.mobkills;
  newst->player_specials_saved.deaths 		
    = oldst->player_specials_saved.deaths; 
  newst->player_specials_saved.old_class		
    = oldst->player_specials_saved.old_class;
  newst->player_specials_saved.page_length        
    = oldst->player_specials_saved.page_length; 
  newst->player_specials_saved.total_dam		
    = oldst->player_specials_saved.total_dam;
  newst->player_specials_saved.columns		
    = oldst->player_specials_saved.columns;
  newst->player_specials_saved.hold_load_room		
    = oldst->player_specials_saved.hold_load_room;
  newst->player_specials_saved.spare_i[0] = // new
    newst->player_specials_saved.spare_i[1] =
    newst->player_specials_saved.spare_i[2] =
    newst->player_specials_saved.spare_i[3] = 0;

  newst->player_specials_saved.mana_shield_low		
    = oldst->player_specials_saved.mana_shield_low;
  newst->player_specials_saved.mana_shield_pct		
    = oldst->player_specials_saved.mana_shield_pct;
  newst->player_specials_saved.spare19		
    = oldst->player_specials_saved.spare19;
  newst->player_specials_saved.spare20		
    = oldst->player_specials_saved.spare20;
  newst->player_specials_saved.spare21		
    = oldst->player_specials_saved.spare21;

  newst->abilities.str		= oldst->abilities.str;
  newst->abilities.str_add	= oldst->abilities.str_add;
  newst->abilities.intel	= oldst->abilities.intel;
  newst->abilities.wis		= oldst->abilities.wis;
  newst->abilities.dex		= oldst->abilities.dex;
  newst->abilities.con		= oldst->abilities.con;
  newst->abilities.cha		= oldst->abilities.cha;

  newst->points.mana		= oldst->points.mana;
  newst->points.max_mana	= oldst->points.max_mana;
  newst->points.hit	      	= oldst->points.hit;
  newst->points.max_hit		= oldst->points.max_hit;
  newst->points.move	       	= oldst->points.move;
  newst->points.max_move	= oldst->points.max_move;
  newst->points.armor	       	= oldst->points.armor;
  newst->points.gold	       	= oldst->points.gold;
  newst->points.bank_gold	= oldst->points.bank_gold;
  newst->points.credits		= oldst->points.credits;
  newst->points.cash		= 0; // new
  newst->points.exp	       	= oldst->points.exp;
  newst->points.hitroll		= oldst->points.hitroll;
  newst->points.damroll		= oldst->points.damroll;

  for (i=0;i<MAX_AFFECT;i++)  {
    newst->affected[i].type          = oldst->affected[i].type;
    newst->affected[i].duration	     = oldst->affected[i].duration;
    newst->affected[i].modifier	     = oldst->affected[i].modifier;
    newst->affected[i].location	     = oldst->affected[i].location;
    newst->affected[i].bitvector     = oldst->affected[i].bitvector;
    newst->affected[i].aff_index     = oldst->affected[i].aff_index;
    newst->affected[i].next          = oldst->affected[i].next;
    newst->affected[i].level = 0; //new
    newst->affected[i].spare_c = 0; //new
  }

  newst->last_logon	       = oldst->last_logon;
  strncpy (newst->host, oldst->host, HOST_LENGTH+1); // longer
}

int
main (int argc, char **argv)
{
  FILE	*infile, *outfile;
  int		recs, size, i, ret = 0, del = 0;
  char	buf[1024];
  struct char_file_u		oldst;
  struct NEWchar_file_u	newst;
  time_t tm;

  if (((argc != 2) && (argc != 3)) || !(infile = fopen(argv[1],"r")))  {
    printf("SYNTAX:  %s pfile <newpfile>\n",argv[0]);
    exit(0);
  }

  if (argc == 3)
    strcpy(buf,argv[2]);
  else
    sprintf(buf,"%s.new",argv[1]);

  if (!(outfile = fopen(buf,"w")))  {
    printf("ERROR: could not open output file: %s\n",buf);
    exit(0);
  }

  fseek(infile, 0L, SEEK_END);
  size = ftell(infile);
  rewind(infile);
  if (size % sizeof(struct char_file_u))  {
    printf("ERROR:  The input file is the wrong size.\n");
    exit(0);
  }
  recs = size / sizeof(struct char_file_u);
  if (recs)  {
    sprintf(buf, "   %d players in database.", recs);
  } else {
    exit(0);
  }
 
  tm = time(NULL);
  for (i=0;i<recs;i++)  {
    fread (&oldst, sizeof(struct char_file_u), 1, infile);
    printf("   Converting %s.  Level %d.\n", 
	   oldst.name, oldst.level);
    convert(&oldst, &newst);
    fwrite (&newst, sizeof(struct NEWchar_file_u), 1, outfile);
    ret++;
  }

  printf("File Converted, %4d chars retained, %4d deleted.\n", ret, del);
  exit(1);
}
