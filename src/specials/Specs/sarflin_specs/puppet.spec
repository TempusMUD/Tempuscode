//
// File: puppet.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

void make_corpse(struct char_data *ch, struct char_data *vict, int attacktype);
SPECIAL(puppet)
{ 
  if( spec_mode == SPECIAL_DEATH ) return 0;
  struct char_data *me2 = (struct char_data *) me;
  struct affected_type af;
  if (!cmd && !FIGHTING(me2)&& !number(0,3))
  { 
     switch (number (0,10))
     {
        case 0 : act("The puppet says: I am now the willing slave of $N.", FALSE, ch, 0, me2, TO_ROOM); break;
        case 1 : act("The puppet says: That @^#%* wizard always makeing me touch my toes!", FALSE, ch, 0, me2, TO_ROOM); break;
        case 2 : act("The puppet says: WOW, so this is what the world looks like!", FALSE, ch, 0, me2, TO_ROOM); break;
        case 3 : act("The puppet says: You know I have always thought that you don't need to worry about the guy that ", FALSE, ch, 0, me2, TO_ROOM);
                 act("pulls a sword from a stone. You need to worry about the guy that put it there! ", FALSE, ch, 0, me2, TO_ROOM);
                 break;
        case 4 : act("The puppet says: I am board, lets go get killed!", FALSE, ch, 0, me2, TO_ROOM); break;
        case 5 : act("The puppet says: I know that pesky imp is around here some where.", FALSE, ch, 0, me2, TO_ROOM); break;
        case 6 : act("The puppet says: HEY do you owe me gold?", FALSE, ch, 0, me2, TO_ROOM); break;
        case 7 : act("The puppet says: GOOOLLLDDD All I need is GOOLLDD!", FALSE, ch, 0, me2, TO_ROOM); break;
        case 8 : act("The puppet says: You arn't so tough. I could kill you with ... UMMM one giant!", FALSE, ch, 0, me2, TO_ROOM); break;
        case 9 : act("The puppet says: OHHH I don't feel so good. Must have been someone I ate! ", FALSE, ch, 0, me2, TO_ROOM); break;
        case 10: 
        {
           if (!number(0,10))
           { 
              act("The puppet turns grey and explodes", FALSE, ch, 0, me2, TO_ROOM);
              make_corpse(me2,me2,TYPE_CLAW );
           }
           break;
        }

     }
  }
  if (!CMD_IS("say")&&!CMD_IS("'")) return 0;

  skip_spaces(&argument);
  if (!*argument)
  return 0;
  half_chop(argument,buf,buf2);

  if (!strncasecmp(buf, "simonsez", 8)&& !strncasecmp(buf2, "obey me", 7))
  {
     if (me2->master)  stop_follower(me2);
     add_follower(me2, ch);

     af.type = SPELL_CHARM;

     af.duration = 24 * 20;

     af.modifier = 0;
     af.location = 0;
     af.bitvector = AFF_CHARM;
     affect_to_char(me2, &af);

  }else
  {
     char buf_it[256];
     sprintf(buf_it,"What do you mean by %s %s",buf,buf2);
     act(buf_it, FALSE, ch, 0, me2, TO_CHAR);
  }
   return 0;
}

