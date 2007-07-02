#!/bin/bash

function dupe_ids
{
	grep -r '<tracking ' /usr/tempus/TempusMUD/lib/players/equipment /usr/tempus/TempusMUD/lib/players/housing | grep -v 'tracking id="0"' | sed 's/[^:]*:[ 	]*<tracking id="//' | sed 's/".*//' | sort | uniq -d
}

for dupe in `dupe_ids`; do echo -- -- $dupe -------------; grep -lr $dupe /usr/tempus/TempusMUD/lib/players/equipment/ /usr/tempus/TempusMUD/lib/players/housing; done
