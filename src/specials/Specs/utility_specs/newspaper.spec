//
// File: newspaper.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

ACMD(do_dyntext_show);
//void (name)(struct char_data *ch, char *argument, int cmd, int subcmd, int *return_flags = 0 )

SPECIAL(newspaper)
{
  struct obj_data *paper = (struct obj_data *) me;
  
  if (!CMD_IS("look") && !CMD_IS("examine") && !CMD_IS("read"))
    return 0;
  skip_spaces(&argument);
  if (!isname(argument, paper->name)) {
    return 0;
  }
  
  switch (GET_OBJ_VNUM(paper)) {
  case 3160:
    do_dyntext_show(ch, "", 0, SCMD_DYNTEXT_NEWS);
    break;
  default:
    send_to_char("$p is completely blank!  Looks like someone forgot to print it.\r\n", ch);
    break;
  }
  return 1;
}
