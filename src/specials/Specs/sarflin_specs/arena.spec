//
// File: arena.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(arena_object)
{
   struct obj_data *me2 = (struct obj_data *) me;
   struct char_data *new_mob = NULL;
   if (!CMD_IS("say") && !CMD_IS("'"))  return 0;
   skip_spaces(&argument);
   if (!*argument)   return 0; 
   half_chop(argument,buf,buf2);

   if (!strncasecmp(buf, "reset", 5))
   {
       send_to_room("Games room reset. Say release to each opponent!\n",IN_ROOM(me2));
       GET_OBJ_VAL(me2,0) = 0;
       return 1;
   } 
   if (!strncasecmp(buf, "release", 7))
   {
       new_mob = NULL;
       send_to_room("Releasing.... Prepare your self.\n",IN_ROOM(me2));

       switch (GET_OBJ_VAL(me2,0)++)
       {
           case  0: new_mob = read_mobile (30091); break; /* mutant rat*/
           case  1: new_mob = read_mobile (3360);  break; /* imp */
           case  2: new_mob = read_mobile (3364);  break; /* imp */
           case  3: new_mob = read_mobile (3365);  break; /* snake dog */
           case  4: new_mob = read_mobile (30094); break; /* sand dragon */
           case  5: new_mob = read_mobile (3412);  break; /* skeleton */
           case  6: new_mob = read_mobile (3214);  break; /* saber tooth tiger */
           case  7: new_mob = read_mobile (3370);  break; /* sword swalower */
           case  8: new_mob = read_mobile (3215);  break; /* hydra */
           case  9: new_mob = read_mobile (9007);  break; /* ghoul */
           case 10: new_mob = read_mobile (20022);  break; /* ogre */
           case 11: new_mob = read_mobile (20022);  break;  
           case 12: new_mob = read_mobile (20022);  break;
           case 13: new_mob = read_mobile (3068);  break; /* green blob */
           case 14: new_mob = read_mobile (13413); break; /* black mage */
           case 15: new_mob = read_mobile (13410); break; /* bone golem */
           case 16: new_mob = read_mobile (3059);  break; /* holy defender */
           case 17: new_mob = read_mobile (5004);  break; /* sand worm */
           case 18: new_mob = read_mobile (20010);  break; /* hill giant */
           case 19: new_mob = read_mobile (13804);  break; /* shambling */
           case 20: new_mob = read_mobile (9901);  break; /* wight */
           case 21: new_mob = read_mobile (13406); break; /* death knight */
           default:
           {

              send_to_room("I am sorry you have exceeded the maximum power of this device\n",IN_ROOM(me2));
              send_to_room("Say reset to start again.",IN_ROOM(me2));
              break;
            }
       }
       if (new_mob != NULL)
       { 
          SET_BIT(MOB_FLAGS(new_mob), MOB_AGGRESSIVE);
          REMOVE_BIT(MOB_FLAGS(new_mob), MOB_WIMPY);
          GET_GOLD(ch) = 0;
          GET_EXP( new_mob ) >>= 2;
          char_to_room(new_mob,IN_ROOM(me2));
       } else printf("NULL\n");
       return 1;
   }
   return 0;
}

