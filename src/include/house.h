//
// File: house.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/***************************************************************************
*  house.h     created by: cj@ph.msstate.edu on: 15 October 1995
*  This is a program is designed implement a house system for
*  Tempus mud (130.18.152.61 2020) which is circle 3.0 based.
*  I used the original house code from cicrle an a skeleton for my code.
*
***************************************************************************/
#define HOUSE_VERSION "0.02 ALPHA"

#define MAX_HOUSES          400
#define MAX_HOUSE_TITLE     79
#define MAX_ROOMS_PER_HOUSE	100
#define MAX_GUESTS          50

#define MAX_HOUSE_ITEMS     50
/* Ownership modes */
#define HOUSE_PUBLIC	0
#define HOUSE_PRIVATE	1
#define HOUSE_RENTAL    2
#define BASE_HSRM_COST  10000

#define HOUSE_PASS     1         /* period of passes in minutes */
#define HOUSE_PASSES_MIN 60      /* passes per minute */
#define HOUSE_PASSES_CHECK 20


struct house_control_rec {
   char title[MAX_HOUSE_TITLE+1]; /* title of the house         */
   int num_of_rooms;    /* how rooms are in house               */
   room_num house_rooms[MAX_ROOMS_PER_HOUSE];	/* vnum house rooms */
   time_t built_on;     /* date this house was built            */
   int mode;            /* mode of ownership                    */
   int  owner1;         /* idnum of house's owner               */
   int  owner2;         /* idnum of second owner (if biprivate) */
   int  num_of_guests;   /* how many guests for house           */
   int  guests[MAX_GUESTS]; /* idnums of house's guests         */
   int  base_cost;      /* base value of house			*/
   time_t last_payment; /* date of last house payment           */
   int  rent_sum;	/* running tally of rent owed		*/
   int  rent_time;      /* time in minutes */
   int  hourly_rent_sum;
   int  rent_rate;     /* daily rent cost of rental property */
   int  landlord;
   int  spare5;
   int  spare6;
   int  spare7;
};


typedef struct house_door_elem {
  int room_num;
  int dir;
  int flags;
} house_door_elem;
  
   
#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

void	House_listrent(struct char_data *ch, int vnum);
void	House_boot(void);
void	House_save_all(bool mode);
int 	House_can_enter(struct char_data *ch, room_num house);
void	House_crashsave(int vnum);
void    House_countobjs(void);
void    hcontrol_list_house_rooms(struct char_data *ch, room_num atrium_vnum);
void    print_room_contents_to_buf(struct char_data *ch, char *buf, 
				   struct room_data *real_house_room);
int recurs_obj_cost (struct obj_data * obj, bool mode, struct obj_data *top_o);
int recurs_obj_contents(struct obj_data *obj, struct obj_data *top_o);

int find_house(room_num vnum);

struct house_control_rec *real_house( room_num vnum );
void House_save_control(void);
