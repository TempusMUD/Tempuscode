#!/bin/csh

# sleeptime is in units of seconds
set sleeptime = 6
set pid = 0

while ( 1 )

#    echo "top pid = $pid"
    set pid = `ps ux | grep bin/circle | grep -v grep | awk '{print $2}'`
#    echo "pid set = $pid"
    if ( $pid == "" ) then
	echo "circle is not alive... waiting."
	sleep $sleeptime
    else
	echo "found and signalling pid $pid to reset qps"
	kill -USR1 $pid
	break
    endif
	
end

exit
