//
// File: telescope.spec                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//

/* To make a telescope, simply make an object for the scope. To use
   it, one will type 'look scope <keyword>', where the keyword will
   be an exdesc on the scope.

   You must set value 3 of the scope to be -999.
 */

char *find_exdesc(char *word, struct extra_descr_data *list, bool find_exact);

SPECIAL(telescope)
{
    struct obj_data *scope = (struct obj_data *)me;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char *desc;

    if (!CMD_IS("look") || !can_see_object(ch, scope) || !AWAKE(ch)) {
        return 0;
    }

    half_chop(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
        return 0;
    }

    if (!isname(arg1, scope->aliases)) {
        return 0;
    }

    if ((desc = find_exdesc(arg2, scope->ex_description, false)) != NULL) {
        page_string(ch->desc, desc);
    } else {
        act("You cannot look at that with $p.", false, ch, scope, NULL, TO_CHAR);
    }

    return 1;
}
