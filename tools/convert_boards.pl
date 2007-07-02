#!/usr/bin/perl

use POSIX;
use DBI;

%weekdays = (
	'Sun' => 0,
	'Mon' => 1,
	'Tue' => 2,
	'Wed' => 3,
	'Thu' => 4,
	'Fri' => 5,
	'Sat' => 6
);

%months = (
	'Jan' => 0,
	'Feb' => 1,
	'Mar' => 2,
	'Apr' => 3,
	'May' => 4,
	'Jun' => 5,
	'Jul' => 6,
	'Aug' => 7,
	'Sep' => 8,
	'Oct' => 9,
	'Nov' => 10,
	'Dec' => 11
);

$base = "/usr/tempus/TempusMUD/lib/etc";

$dbh = DBI->connect("dbi:Pg:dbname=tempus", "realm", "tarrasque");

opendir(DIR, "$base/$num") || die("Can't open characters");
while ($fname = readdir(DIR)) {
	next if $fname !~ /^board.(.*)/;
	process_board("$base/$num/$fname", $1);
}
closedir DIR;

exit 0;

sub process_board {
	my $inf_path = shift;
	my $board_name = shift;
	my ($msg_count, $slot, $level, $heading_len, $message_len);

	open(INF, "<$inf_path") || die("Couldn't open $inf_path");
	read(INF, $buf, 4);
	$msg_count = unpack("i", $buf);
	print "Importing $board_name: $msg_count messages\n";
	while ($msg_count--) {
		read(INF, $buf, 20);
		($slot, undef, $level, $heading_len, $message_len) = unpack("i5", $buf);
		read(INF, $header, $heading_len - 1);
		read(INF, $ignore, 1);
		read(INF, $message, $message_len - 1);
		read(INF, $ignore, 1);
		die("Bad pattern in $header!") if $header !~ /(\S+)\s+(\S+)\s+(\d+) \(([^)]+)\)\s+:: (.*)/;
		$wday = $weekdays{$1};
		$mon = $months{$2};
		$day = $3;
		$name = $4;
		$subject = $5;
		$name =~ s/\\/\\\\/g;
		$name =~ s/'/\\'/g;
		$subject =~ s/\\/\\\\/g;
		$subject =~ s/'/\\'/g;
		$message =~ s/\\/\\\\/g;
		$message =~ s/'/\\'/g;
		$year = 103;
		@tm = localtime(mktime(0, 0, 0, $day, $mon, $year, 0, 0, 0));
		while ($tm[6] != $wday) {
			$year -= 1;
			@tm = localtime(mktime(0, 0, 0, $day, $mon, $year, 0, 0, 0));
		}
		$year += 1900;
		$mon += 1;
		@res = $dbh->selectrow_array("select idnum from players where lower(name)='" . lc $name . "'");
		
		$author = $res[0] if $#res == 0;
		if ($author) {
			$sql = "insert into board_messages (board, post_time, author, name, subject, body) values ('$board_name', '$year/$mon/$day', $author, '$name', '$subject', '$message')";
		} else {
			$sql = "insert into board_messages (board, post_time, name, subject, body) values ('$board_name', '$year/$mon/$day', '$name', '$subject', '$message')";
		}
		die($sql) if !$dbh->do($sql);
	}
	close(INF);
}
