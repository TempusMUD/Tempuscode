#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define _NEWDYNCONTROL_
#include "db.h"

int
main(int argc, char **argv)
{
    char buf[1024];
    FILE *fl = NULL;
    dynamic_text_file newdyn;
    int i;

    if (argc < 2) {
        printf("Usage: %s <filename>\n"
               "Working directory must be the mud working directory.\n", argv[0]);
        return 1;
    }

    sprintf(buf, "lib/text/%s", argv[1]);
    
    if (!(fl = fopen(buf, "w"))) {
        puts("Error opening text file.");
        return 1;
    }

    fputs("A NEW DYNAMIC TEXT FILE.\r\n", fl);
    
    fclose(fl);

    sprintf(buf, "lib/%s/%s.dyn", DYN_TEXT_CONTROL_DIR, argv[1]);

    if (!(fl = fopen(buf, "w"))) {
        puts("Error opening control file.");
        return 1;
    }

    newdyn.filename[0] = '\0';
    strcpy(newdyn.filename, argv[1]);

    // zero editing history
    for (i = 0; i < DYN_TEXT_HIST_SIZE; i++) {
        newdyn.last_edit[i].idnum = 0;
        newdyn.last_edit[i].tEdit = 0;
    }
    
    // zero permission list
    for (i = 0; i < DYN_TEXT_PERM_SIZE; i++)
        newdyn.perms[i]           = 0;

    newdyn.level = 60;
    newdyn.lock  = 0;
    newdyn.buffer = newdyn.tmp_buffer = NULL;
    newdyn.next = NULL;

    fwrite(&newdyn, sizeof(dynamic_text_file), 1, fl);
    
    fclose(fl);

    return 0;
}
