//
// File: gunnery_device.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(gunnery_device)
{
   int hit = 0;
   struct room_data *dest;
   struct obj_data *ob = NULL;
   struct char_data *me2 = (struct char_data *)me;
   struct char_data *vict = NULL;

   ob = GET_EQ(me2,WEAR_HOLD);
   if (ob == NULL) return (0);
   if (CMD_IS("say") || CMD_IS("'"))
   {
      short num = 0;
      if (GET_LEVEL(ch) < LVL_IMMORT) return (0);
      skip_spaces(&argument);
      if (!*argument) return 0;
      half_chop(argument,buf,buf2);
      if (strncasecmp(buf, "fire", 4)) return (0);

      num = atoi(buf2);
      GET_OBJ_VAL(ob,0) = num;
      do_say(ch, argument, 0, 0);
      do_say(me2, "Yes, Sir.", 0, 0);
      return (1);
   }   
   if (cmd) return (0);
   if (!GET_OBJ_VAL(ob,0)) return (0);
   dest = real_room(GET_OBJ_VAL(ob,1));
   if (dest < 0) return (1);
/*28*/
   switch(GET_OBJ_VAL(ob,2))
   {
      case 1:
      {
         send_to_room("With a Thump the catapults fire.\n\r",ch->in_room);
         send_to_room("Then with a driven effort the crews work to reload.\n\r",ch->in_room);
         send_to_room("From above in the tower you hear the Thwack of a catapult fireing.\n\r",dest);
         vict = NULL;
         hit = 0;
         for (vict = dest->people;vict;vict = vict->next_in_room)
         {
            if (vict == NULL) break;
            if (!number (0,5))
            {
               damage(ch,vict,10,TYPE_CATAPULT, -1);
               hit = 1;
               break;
            }  
         }
         if (!hit) send_to_room("A boulder THUNKS into the ground missing you all.\n\r",dest);
         break;
      }
      case 2: 
      {
         send_to_room("With a mighty TWANG the balista fire.\n\r",ch->in_room);
         send_to_room("Then the Crew start to winch the string back into position.\n\r",ch->in_room);
         send_to_room("From above in the tower you hear the TWANG of a balista fireing.\n\r",dest);
         vict = NULL;
         hit = 0;
         for (vict = dest->people;vict;vict = vict->next_in_room)              
         {
            if (vict == NULL) break;
            if (!number (0,5))
            {
                damage(ch,vict,15,TYPE_BALISTA, -1);
                hit = 1;
                break;
            }  
         }
         if (!hit) send_to_room("A Balista bolt Thunks into the ground missing you all.\n\r",dest);
         break;
      }
      case 3:
      {
         if (number(0,1))
         {
            send_to_room("With a groaning CREAK the pots of oil are tipped.\n\r",ch->in_room);
            send_to_room("The the crews begin refilling the pots.\n\r",ch->in_room);
            send_to_room("From above in the tower you hear the creak of a pot tiping\n\r",dest);
            vict = NULL;
            for (vict = dest->people;vict;vict = vict->next_in_room)              
            {
               if (vict == NULL) break;
               damage(ch,vict,15,TYPE_BOILING_OIL, -1);
            }
         } else {
            send_to_room("With a groaning CREAK the pots of scalding sand are dumped.\n\r",ch->in_room);
            send_to_room("The the crews begin refilling the pots.\n\r",ch->in_room);
            send_to_room("From above in the tower you hear the creak of a pot tiping\n\r",dest);
            vict = NULL;
            for (vict = dest->people;vict;vict = vict->next_in_room)              
            {
               if (vict == NULL) break;
               damage(ch,vict,15,TYPE_BOILING_OIL, -1);
            }
         }
         break;
      }
   }
   GET_OBJ_VAL(ob,0) --;
   return (0);
}

