#!/usr/bin/perl

$num = 0;
while ($num < 10) {
	opendir(DIR, "/usr/tempus/TempusMUD/lib/players/accounts/$num") || die("Can't open accounts");
	while ($fname = readdir(DIR)) {
		next if $fname !~ /.acct$/;
		open(INF, "</usr/tempus/TempusMUD/lib/players/accounts/$num/$fname") || die("Can't open $fname");
		while (<INF>) {
			$name = $1 if /<login name=\"([^"]+)\"/;
			$creation = $1 if /<creation time=\"([^"]+)\" /;
			$last = $1 if /<lastlogin time=\"([^"]+)\" /;
		}
		close(INF);
		if ($creation > time - (7 * 24 * 60 * 60)) {
			if ($last - $creation < 86400) {
				$lost_count++;
			} else {
				print "$name\n";
			}
			$total_count++;
		}
	}
	closedir(DIR);
	$num++;
}

print "Lost $lost_count out of $total_count\n";
