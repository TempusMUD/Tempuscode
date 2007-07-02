#!/usr/bin/perl

use POSIX;
use DBI;

$base = "/usr/tempus/TempusMUD/lib";

$dbh = DBI->connect("dbi:Pg:dbname=devtempus", "realm", "tarrasque");

process_index("$base/text/help_data/index");

#opendir(DIR, "$base/$num") || die("Can't open characters");
#while ($fname = readdir(DIR)) {
#	next if $fname !~ /^board.(.*)/;
#	process_board("$base/$num/$fname", $1);
#}
#closedir DIR;

exit 0;

sub process_index {
	my $inf_path = shift;
	my ($msg_count, $slot, $level, $heading_len, $message_len);

	open(INF, "<$inf_path") || die("Couldn't open $inf_path");
	$state = 'start';
	while (<INF>) {
		chomp;
		if ($state eq 'start') {
			$state = 'entry-nums';
		} elsif ($state eq 'entry-nums') {
			($idnum, $groups, $counter, $flags, $owner) = split;
			if ($groups & 1) { $group = 1; }
			elsif ($groups & (1 << 23)) { $group = 2; }
			else { $group = 0; }

			$state = 'entry-title';
		} elsif ($state eq 'entry-title') {
			$title = $_;
			$state = 'entry-aliases';
		} elsif ($state eq 'entry-aliases') {
			$title =~ s/'/''/g;
			@aliases = /(".*?"|\S+)/g;
			# read in the entry text
			$entry_path = sprintf("%s/text/help_data/%04d.topic", $base, $idnum);
			open(ENTRY, "<$entry_path") || die("Can't open entry $idnum!");
			<ENTRY>;
			$text = "";
			while (<ENTRY>) { $text .= $_; }
			close ENTRY;
			$text =~ s/'/''/g;

			# now insert into db
			$sql = "insert into help_entries (idnum, counter, flags, category, title, text) values ($idnum, $counter, $flags, $group, '$title', '$text')";
			die($sql) if !$dbh->do($sql);
			for $cur_alias (@aliases) {
				$cur_alias =~ s/^"//;
				$cur_alias =~ s/"$//;
				$cur_alias =~ s/'/''/;
				$sql = "insert into help_aliases (entry, alias) values ($idnum, '$cur_alias')";
				die($sql) if !$dbh->do($sql);
			}
			$state = 'entry-nums';
		}
	}
	close(INF);
}
