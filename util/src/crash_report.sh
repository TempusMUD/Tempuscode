#!/bin/bash

MAIL_RECIPIENTS=crashreport@sanctuary.org

function print_report() {

    LINECOUNT=`wc -l $0 | awk '{print $1}'`
    CUTLINE=`grep --line-number "^\# GDB COMMAND LIST CUT HERE" $0 | awk -F: '{print $1}'`
    TAILCOUNT=`echo $((LINECOUNT-CUTLINE))`


    echo " >>>>"
    echo " >>>> MUD CRASH REPORT <<<<"
    echo " >>>> `date`"
    echo " >>>>"
    echo
    echo " >>>>"
    echo " >>>> TAIL of last syslog (log/syslog.6)"
    echo " >>>>"
    echo

    tail log/syslog.6

    echo
    echo " >>>>"
    echo " >>>> GDB OUTPUT"
    echo " >>>>"
    echo

    tail -${TAILCOUNT} $0 | gdb bin/circle lib/core | cat

}


print_report $0 | mail -s "TempusMUD CRASH REPORT" ${MAIL_RECIPIENTS}


exit

# gbd coms
#
# GDB COMMAND LIST CUT HERE
set prompt \n
set print null-stop on
set print pretty on
printf " >>>>\n >>>> STACK TRACE\n >>>>\n"
bt
printf " >>>>\n >>>> LAST CMD ISSUED\n >>>>\n"
print last_cmd[0]
quit
