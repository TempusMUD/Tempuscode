#ifndef _MACROS_H_
#define _MACROS_H_

#ifndef _MACROS_
#define _MACROS_


#define SPECIAL(name) \
int (name)(struct Creature *ch, void *me, int cmd, char *argument, special_mode spec_mode)

#define GET_SCRIPT_VNUM(mob)   (IS_MOB(mob) ? \
                              mob->mob_specials.shared->svnum : -1)

#endif
#endif
