#!/usr/bin/perl

use POSIX;
use DBI;

$base = "/usr/tempus/TempusMUD/lib/etc/security.dat";

$dbh = DBI->connect("dbi:Pg:dbname=tempus", "realm");

do_sql("delete from sgroup_members");
do_sql("delete from sgroup_commands");
do_sql("delete from sgroups");

print "Processing security groups...\n";

open(INF, "<$base") || return;

$idnum = 0;
while (<INF>) {
	chomp;
	if (/<Group Name="([^"]+)" Description="([^"]+)" Admin="([^"]*)"\/?>/) {
		$idnum++;
		$name = $1;
		$descrip = $2;
		$admins{$idnum} = $3 if $3;
		do_sql("insert into sgroups (idnum, name, descrip) values ($idnum, '$name', '$descrip')");
	} elsif (/<Command Name="([^"]+)"/) {
		$cmd = $1;
		do_sql("insert into sgroup_commands (sgroup, command) values ($idnum, '$cmd')");
	} elsif (/<Member Name="([^"]+)" ID="(\d+)"/) {
		$plr_id = $2;
		do_sql("insert into sgroup_members (sgroup, player) values ($idnum, $plr_id)");
	} elsif (/<\/Group>/ || /<Security>/ || /<\/Security>/ || /xml version/) {
		# Do nothing
	} else {
		print STDERR "Can't happen: $_\n";
		exit(-1);
	}
}
close INF;

for $idnum (keys %admins) {
	$sth = $dbh->prepare("select idnum from sgroups where name ilike '" . $admins{$idnum} . "'");
	$sth->execute();
	@row = $sth->fetchrow_array;
	$sth = $dbh->prepare("update sgroups set admin=" . $row[0] . " where idnum=" . $idnum);
	$sth->execute();
}

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
