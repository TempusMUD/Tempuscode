//
// File: beer_tree.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


SPECIAL(beer_tree)
{
  struct obj_data *obj = NULL;
  int beer;
  const char *beers[] = { "heineken","grolsch","becks","\n"};

  skip_spaces(&argument);

  if (!(CMD_IS("get") || CMD_IS("pick")))
    return 0;

  if (!*argument)
    return 0;

  if ((beer = search_block(argument, beers, FALSE)) < 0)
    return 0;

  switch (beer)  {
  case 0:
    obj = read_object(2771);
    break;
  case 1:
    obj = read_object(2772);
    break;
  case 2:
    obj = read_object(2773);
    break;
  }

  if (!obj)
    return 0;

  obj_to_char(obj, ch);

  sprintf(buf,"You pick %s from the tree.\r\n",obj->short_description);

  return 1;
}

  
