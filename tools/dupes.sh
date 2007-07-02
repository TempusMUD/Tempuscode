#!/bin/bash

DIRS="/usr/tempus/TempusMUD/lib/players/equipment /usr/tempus/TempusMUD/lib/players/housing"
for num in `grep -r '<tracking ' $DIRS | grep -v 'tracking id="0"' | sed 's/[^:]*:[ 	]*<tracking id="//' | sed 's/".*//' | sort | uniq -d`; do
	echo -- $num -----
	for line in `grep -rl $num /usr/tempus/TempusMUD/lib/players/equipment`; do
		echo Equip: `echo $line | cut -c 47-`
	done
	for line in `grep -rl $num /usr/tempus/TempusMUD/lib/players/housing`; do
		echo House: `echo $line | cut -c 45-`
	done
done
