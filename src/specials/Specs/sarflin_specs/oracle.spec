//
// File: oracle.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(oracle)
{
   struct char_data *me2 = (struct char_data *)me;
   struct obj_data *od = NULL;
   if( spec_mode == SPECIAL_DEATH ) return 0;
/* 
   cmd line is Give #### coin XXXXXXX
   where #### is a number of gold
   xxxxx is the name of the orcal..
*/
  char p1[100], p2[100],p3[100];
  short num;

  if (!CMD_IS("give") && !CMD_IS("ask")) return 0;

  skip_spaces(&argument);
  if (!*argument) return 0;

  if (CMD_IS("ask"))
  {
     half_chop(argument,p1,p2);

     if (strstr(GET_NAME(me2),p1) == NULL) return 0;
     if (GET_EQ(me2,0) != NULL)
     {
        od = GET_EQ(me2,0);
     } else
     { 
       slog("oracle is missing data object. object must be in pos 0");
        return 0;
     }

     if (GET_OBJ_VAL(od,0) < GET_OBJ_VAL(od,1))
     {
        act("Sorry you have not payed my fee for this question", TRUE, ch, 0, 0, TO_CHAR);
        sprintf(buf,"My fee for this question is %d gold.",GET_OBJ_VAL(od,1));
        act(buf, TRUE, ch, 0, 0, TO_CHAR);
        act("Just give me the gold and ask your question again.", TRUE, ch, 0, 0, TO_CHAR);
        return 1;

     } else
     {
        GET_OBJ_VAL(od,0) -= GET_OBJ_VAL(od,1);
        return 0;
     }
  }
  if (CMD_IS("give"))
  {
     half_chop(argument,p1,p2);
     half_chop(p2,p2,p3);

     if (strncasecmp(p2, "coin", 4)) return 0;
     if (strstr(GET_NAME(me2),p3) == NULL) return 0;
     if (GET_EQ(me2,0) != NULL)
     {
        od = GET_EQ(me2,0);
     } else
     { 
       slog("oracle is missing data object. object must be in pos 0");
        return 0;
     }

     num = atoi (p1);
  
     GET_GOLD(me2)+= num;
     GET_OBJ_VAL(od,0)+= num;
  }

  return 0;
}

