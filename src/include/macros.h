#ifndef _MACROS_H_
#define _MACROS_H_

#ifndef _MACROS_
#define _MACROS_


#define SPECIAL(name) \
int (name)(__attribute__ ((unused)) struct Creature *ch, \
	__attribute__ ((unused)) void *me, \
	__attribute__ ((unused)) int cmd, \
	__attribute__ ((unused)) char *argument, \
	__attribute__ ((unused)) special_mode spec_mode)

#define GET_SCRIPT_VNUM(mob)   (IS_MOB(mob) ? \
                              mob->mob_specials.shared->svnum : -1)

#endif
#endif
