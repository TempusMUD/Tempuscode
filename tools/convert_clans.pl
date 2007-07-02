#!/usr/bin/perl

use POSIX;
use DBI;

$base = "/usr/tempus/TempusMUD/lib/etc/clans.xml";

$dbh = DBI->connect("dbi:Pg:dbname=tempus", "realm");

print "Processing clans...\n";

open(INF, "<$base") || die("Can't open");

while (<INF>) {
	chomp;
	if (/<clan id="(\d+)" name="([^"]+)" badge="([^"]+)"/) {
		$idnum = $1;
		$name = $2;
		$badge = $3;
		$name =~ s/'/\\'/g;
		$badge =~ s/'/\\'/g;
	} elsif (/<data owner="(\d+)" top_rank="(\d+)" bank="(\d+)" flags="(\d+)"/) {
		$owner = $1;
		$bank = $3;
		$flags = $4;
		$rank = 0;
		if ($owner) {
			do_sql("insert into clans (idnum, name, badge, bank, owner) values ($idnum, '$name', '$badge', $bank, $owner)")
		} else {
			do_sql("insert into clans (idnum, name, badge, bank) values ($idnum, '$name', '$badge', $bank)")
		}
	} elsif (/<rank name="([^"]*)"/) {
		if ($1) {
			$rankname = $1;
			$rankname =~ s/'/\\'/g;
			do_sql("insert into clan_ranks (clan, rank, title) values ($idnum, $rank, '$rankname')");
			$rank++;
		}
	} elsif (/<member id="(\d+)" rank="(\d+)"/) {
		do_sql("insert into clan_members (clan, player, rank) values ($idnum, $1, $2)")
	} elsif (/<room number="(\d+)"/) {
		do_sql("insert into clan_rooms (clan, room) values ($idnum, $1)")
	} elsif (/<\/clan>/ || /<clanfile>/ || /<\/clanfile>/) {
		# Do nothing
	} else {
		print STDERR "Can't happen: $_\n";
		exit(-1);
	}
}
close INF;

sub do_sql {
my $str = shift;

#	print $str . "\n";
$sth = $dbh->prepare($str);
if (!$sth) {
	print $str . "\n";
	print $dbh->errstr;
}
if (!$sth->execute()) {
	print $str . "\n";
	print $dbh->errstr;
}
}
