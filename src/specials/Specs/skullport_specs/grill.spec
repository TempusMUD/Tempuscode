//
// File: grill.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(corpse_griller)
{
  struct obj_data *corpse = NULL, *steak = NULL;
  char arg[MAX_INPUT_LENGTH];
  
  if (!cmd || !CMD_IS("buy"))
    return 0;

  argument = one_argument(argument, arg);
  
  if (!*arg || strncmp(arg, "grill", 5)) {
    send_to_char_
  

