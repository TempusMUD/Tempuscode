#!/usr/bin/perl

$now = time;
$num = 0;
while ($num < 10) {
	opendir(DIR, "/usr/tempus/TempusMUD/lib/players/character/$num") || die("Can't open");
	while ($fname = readdir(DIR)) {
		next if $fname !~ /.dat$/;
		open(INF, "</usr/tempus/TempusMUD/lib/players/character/$num/$fname") || die("Can't open $fname");
		while (<INF>) {
			if (/<time birth=\"([^"]+)\" /) {
				$age = $now - $1;
				$weeks{int($age / 604800)} += 1;
			}
		}
		close(INF);
	}
	closedir(DIR);
	$num++;
}

for $week (keys %weeks) { print $week . " " . $weeks{$week} . "\n" }
