//
// File: dwarven_hermit.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(dwarven_hermit)
{
   struct obj_data *od = NULL;
   struct char_data *me2 = (struct char_data *) me;
   if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return 0;
   if (CMD_IS("ask")) 
     act("$N ignores you!", TRUE, me2, 0, 0, TO_CHAR);

   if (cmd)
     return (0);

   if (!(od = GET_EQ(me2,WEAR_HOLD)))
      return (0);

   
   if (me2->in_room->people.size() == 1) {
      GET_OBJ_VAL(od,0) = 0;
      return (0);
   }
   switch (GET_OBJ_VAL(od,0)) {
   case -1: GET_OBJ_VAL(od,1)++;
     if (GET_OBJ_VAL(od,1)>12)
       {
         GET_OBJ_VAL(od,0)=0;
         GET_OBJ_VAL(od,1)=0;
       }                    
     break; 
   case  0: do_say(me2,"Back in the old days when clan Brightaxe was",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  1: do_say(me2,"great, our clan lived in the 'Smoking hills'",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  2: do_say(me2,"strong hold. We lived in spelender that could",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  3: do_say(me2,"not be believed and believing that no enemy could",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  4: do_say(me2,"pierce our mighty gates. But we fell to the",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  5: do_say(me2,"Duergar, The vile beasts!, and we fled our ",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  6: do_say(me2,"glorious halls. For you see our gates where not ",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  7: do_say(me2,"pierced, they came up from below.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case  8: 
     GET_OBJ_VAL(od,0)++; break;
   case  9: do_say(me2,"For many years I have waited and looked for some",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 10: do_say(me2,"way to retake our halls. And against that day I",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 11: do_say(me2,"hid a key to the gates. I hid it in a old stump.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 12: do_say(me2,"But I don't rember where it is anymore!",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 13: 
     GET_OBJ_VAL(od,0)++; break;
   case 14: do_say(me2,"Back in the old days when clan Brightaxe was",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 15: do_say(me2,"great our clan lived in the 'Smoking hills'",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 16: do_say(me2,"strong hold. We had treasure beyond all imagination.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 17: do_say(me2,"The magical guardian was build into the door of",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 18: do_say(me2,"the treasure room if you did not speak the right",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 19: do_say(me2,"words it would send you to certen death.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 20: 
     GET_OBJ_VAL(od,0)++; break;
   case 21: do_say(me2,"First speak the name of our Homeland....",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 22: do_say(me2,"Then speak the name of our greatest Battle....",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 23: do_say(me2,"Next speak the name of our greatest Hero....",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 24: do_say(me2,"Finaly speak the name of our greatest King....",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 25: do_say(me2,"And so the entrance to the chamber will be opened.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 26: do_say(me2,"And then state your name for the record.",0,0);
     GET_OBJ_VAL(od,0)++; break;
   case 27: GET_OBJ_VAL(od,0) = -1; break;
   }
   return (0);
}




