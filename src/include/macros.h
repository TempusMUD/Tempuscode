#ifndef _MACROS_
#define _MACROS_

#define SPECIAL(name) \
int (name)(struct char_data *ch, void *me, int cmd, char *argument, special_mode spec_mode)

#define GET_SCRIPT_VNUM(mob)   (IS_MOB(mob) ? \
                              mob->mob_specials.shared->svnum : -1)

#endif
