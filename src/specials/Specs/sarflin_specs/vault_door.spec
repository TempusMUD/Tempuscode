//
// File: vault_door.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(vault_door)
{
   struct obj_data *me2 = (struct obj_data *)me;

   if (!CMD_IS("east") && !CMD_IS("say") && !CMD_IS("'")) return (0);
   if ((CMD_IS("east"))&&(GET_OBJ_VAL(me2,0)!= 5))
   {
      GET_OBJ_VAL(me2,0)= 6; 
      return (0);
   }

   if (CMD_IS("say") || CMD_IS("'"))
   {
      skip_spaces(&argument);
      if (!*argument) return 0;
      half_chop(argument,buf,buf2);
      if ((!strncasecmp(buf, "valamar", 7))&&(GET_OBJ_VAL(me2,0)==0))
      {
         GET_OBJ_VAL(me2,0)++; 
         send_to_room("Click\r\n",me2->in_room);
         return(1);
      }
      if ((!strncasecmp(buf, "dhurri", 6))&&(GET_OBJ_VAL(me2,0)==1))      
      {
         GET_OBJ_VAL(me2,0)++; 
         send_to_room("Click\r\n",me2->in_room);
         return(1);
      }
      if ((!strncasecmp(buf, "hatho", 5))&&(GET_OBJ_VAL(me2,0)==2))      
      {
         GET_OBJ_VAL(me2,0)++; 
         send_to_room("Click\r\n",me2->in_room);
         return(1);
      }
      if ((!strncasecmp(buf, "grysygonth", 10))&&(GET_OBJ_VAL(me2,0)==3))      
      {
         GET_OBJ_VAL(me2,0)++; 
         send_to_room("Click\r\n",me2->in_room);
         return(1);
      }
      if (GET_OBJ_VAL(me2,0) == 4) 
      {
         send_to_room("A haunting voice sayes 'Thank you.'\r\n",me2->in_room);
         GET_OBJ_VAL(me2,0)++; 
      }
      if (GET_OBJ_VAL(me2,0) == 5) 
      {
         send_to_room("The vault door pulse bright blue\r\n",me2->in_room);
         GET_OBJ_VAL(me2,0)++; 
         if (real_room(19429) >=0)      
         (real_room(19429))->dir_option[1]->to_room = real_room(19401);
         return(1);
      }
      return(0);
   }
   if (GET_OBJ_VAL(me2,0) == 6) 
   {
      send_to_room("The vault door pulse bright blue\r\n",me2->in_room);
      GET_OBJ_VAL(me2,0) = 0; 
      if (real_room(19429) >=0)      
      (real_room(19429))->dir_option[1]->to_room = real_room(19445);
   }
   return (1);   
}

