//
// File: loudspeaker.spec                                         -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
ACMD(do_gecho);

SPECIAL(loud_speaker)
{
    struct obj_data *speaker = (struct obj_data *) me;
    
    if (!CMD_IS("yell"))
        return 0;

    skip_spaces(&argument);
    if (!*argument) 
        return 0;
        
    argument = one_argument(argument,buf);
    if (!isname(buf, speaker->name))
        return 0;

    if(!*argument) {
        send_to_char("Yell what?\r\n",ch);
        return 0;
    }
    act("$n yells into $p.", TRUE, ch, speaker, 0, TO_ROOM);
    act("You yell into $p.", TRUE, ch, speaker, 0, TO_CHAR);
    sprintf(buf,"%s BOOMS '%s'", speaker->short_description, argument);
    do_gecho(ch, buf, 0, 0);    
    return 1;
}

