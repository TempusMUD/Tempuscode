//
// File: maze_cleaner.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(maze_cleaner)
{
   if( spec_mode == SPECIAL_DEATH ) return 0;
   struct obj_data *od = NULL,*od2= NULL;
   struct char_data *me2 = (struct char_data *) me;
   struct room_data *rm_number;

   if (GET_EQ(me2,0) != NULL)
     {
       od = GET_EQ(me2,0);
     } else
       {
	 slog("iron manis missing heat gem");
	 return 0;
       }
   
   if (!CMD_IS("say")&& !CMD_IS("'"))
     { 
       skip_spaces(&argument);
       if (!*argument)  return 0;
       half_chop(argument,buf,buf2);
       if (!strncasecmp(buf, "status", 6))
	 {
	   sprintf(buf,"my stats is : %d,%d,%d,%d\r\n",
		   GET_OBJ_VAL(od,0),GET_OBJ_VAL(od,1),
		   GET_OBJ_VAL(od,2),GET_OBJ_VAL(od,3));
	   send_to_char(buf, ch);
	 }
     }
   if (cmd) return 0;
   rm_number = real_room(GET_OBJ_VAL(od,0));
   do
     {
       if (rm_number->contents != NULL)
	 {
	   od2 = rm_number->contents;
	   obj_from_room(od2);
	   obj_to_room(od2,IN_ROOM(me2));
	 }
     } while (rm_number->contents != NULL);
   GET_OBJ_VAL(od,0)++;
   if (GET_OBJ_VAL(od,0) > GET_OBJ_VAL(od,2))
     {
       GET_OBJ_VAL(od,0) = GET_OBJ_VAL(od,1);
     }
   return 0;
}

