//
// File: credit_exchange.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

SPECIAL(credit_exchange)
{
  struct char_data *teller = (struct char_data *) me;
  int amount;

  if( spec_mode != SPECIAL_CMD && spec_mode != SPECIAL_TICK ) return FALSE;
  if (!cmd || !CMD_IS("exchange"))
    return 0;

  skip_spaces(&argument);
  if (!*argument) {
    perform_tell(teller, ch, "Exchange how much gold?");
    return 1;
  }
  
  amount = atoi(argument);

  if (amount <= 0) {
    perform_tell(teller, ch, "Haha.  Very funny.  Stop dicking around.");
    return 1;
  }

  if (amount > GET_GOLD(ch)) {
    perform_tell(teller, ch, "You some kinda wiseass?  You don't have that much gold.");
    return 1;
  }

  act("$N takes your gold and carefully inserts it in a security module.\r\n"
      "$E presses a button and the gold slowly disappears.",
      FALSE, ch, 0, teller, TO_CHAR);
  act("$N takes some gold from $n and inserts it in a security module.\r\n"
      "$E presses a button and the gold slowly disappears.",
      FALSE, ch, 0, teller, TO_NOTVICT);

  sprintf(buf2, "You recieve %d cash credits.", amount);
  perform_tell(teller, ch, buf2);

  GET_GOLD(ch) -= amount;
  GET_CASH(ch) += amount;

  return 1;
}

SPECIAL(gold_exchange)
{
  struct char_data *teller = (struct char_data *) me;

  int amount;

  if (!cmd || !CMD_IS("buy"))
    return 0;

  skip_spaces(&argument);
  if (!*argument) {
    perform_tell(teller, ch, "Buy how much gold?");
    return 1;
  }
  
  amount = atoi(argument);

  if (amount <= 0) {
    perform_tell(teller, ch, "Everybody's a comedian.");
    return 1;
  }

  if (amount > GET_CASH(ch)) {
    perform_tell(teller, ch, "You some kinda wiseass?  You can't afford that much gold.");
    return 1;
  }

  act("$N takes your money and gives you some gold coins.",
      FALSE, ch, 0, teller, TO_CHAR);
  act("$N takes some cash from $n and gives $m some gold coins.",
      FALSE, ch, 0, teller, TO_NOTVICT);
  
  sprintf(buf2, "You recieve %d gold coins.", amount);
  perform_tell(teller, ch, buf2);

  GET_CASH(ch) -= amount;
  GET_GOLD(ch) += amount;

  return 1;

}




