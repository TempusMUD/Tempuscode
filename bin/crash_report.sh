#!/bin/bash

MAIL_RECIPIENTS=crashreport@tempusmud.com

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
    echo " >>>> TAIL of last syslog (log/syslog.0)"
    echo " >>>>"
    echo

    tail $2

    echo
    echo " >>>>"
    echo " >>>> GDB OUTPUT"
    echo " >>>>"
    echo

    tail -${TAILCOUNT} $0 >/tmp/tempus.crash.cmds
	gdb bin/circle $1 </tmp/tempus.crash.cmds
	chmod g+r $1
}


print_report $1 $2

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
print last_cmd
quit
