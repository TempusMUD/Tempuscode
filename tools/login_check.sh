#!/bin/sh

if pgrep 'tt\+\+' &>/dev/null; then
	echo mudders: `ps -f --no-header -o '%U' -p $(pgrep 'tt\+\+')`
else
	echo mudders: none
fi
if [ -e /home/azimuth/tempus/mail ]; then
    echo You have mudmail.
fi
