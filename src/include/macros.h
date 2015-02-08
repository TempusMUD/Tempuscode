#ifndef _MACROS_H_
#define _MACROS_H_

#define SPECIAL(name) \
    int name(__attribute__ ((unused)) struct creature *ch, \
             __attribute__ ((unused)) void *me, \
             __attribute__ ((unused)) int cmd, \
             __attribute__ ((unused)) char *argument, \
             __attribute__ ((unused)) enum special_mode spec_mode)

#endif

