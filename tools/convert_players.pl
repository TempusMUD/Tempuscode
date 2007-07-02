#!/usr/bin/perl

use POSIX;
use DBI;
use XML::LibXML;

%keytype = (
	"sex"				=> "char",
	"name"				=> "str",
	"title"				=> "str",
	"poofin"			=> "str",
	"poofout"			=> "str",
	"badge"				=> "str",
	"state"				=> "str",
	"descrip"			=> "str",
	"bits-flag1"		=> "hex",
	"bits-flag2"		=> "hex",
	"prefs-flag1"		=> "hex",
	"prefs-flag2"		=> "hex",
	"prefs-flag3"		=> "hex",
	"class"				=> "class",
	"remort"			=> "class",
	"race"				=> "race",
	"last"				=> "time",
	"birth"				=> "time",
	"death"				=> "time",
	"played"			=> "interval",
	"clan"				=> "drop",
	"stradd"			=> "drop",
	"pracs"				=> "drop",
);

%keymap = (
	'hit'		=> 'hitp',
	'maxhit'	=> 'maxhitp',
	'invis'		=> 'invis_lvl',
	'xp'		=> 'exp',
	'lp'		=> 'lifepoints',
	'severity'		=> 'flag_severity',
	'code'		=> 'rent_kind',
	'perdiem'	=> 'rent_per_day',
	'state'		=> 'cxn_mode',
	'town'		=> 'home_town',
	'birth'		=> 'birth_time',
	'death'		=> 'death_time',
	'played'	=> 'played_time',
	'last'		=> 'login_time',
	'homeroom'		=> 'home_room',
	'loadroom'		=> 'load_room',
	'points'		=> 'qpoints',
	'allowance'		=> 'qp_allow',
	'bits-flag1'	=> 'bits_1',
	'bits-flag2'	=> 'bits_2',
	'prefs-flag1'	=> 'prefs_1',
	'prefs-flag2'	=> 'prefs_2',
	'qlog'	=> 'qlog_level',
);

$base = "/usr/tempus/TempusMUD/lib/players/character";

$dbh = DBI->connect("dbi:Pg:dbname=devtempus", "realm", "tarrasque");

$num = 0;
while ($num < 10) {
	print "Processing players ending in $num...\n";
	opendir(DIR, "$base/$num") || die("Can't open characters");
	while ($fname = readdir(DIR)) {
		next if $fname !~ /.dat$/;
		process_player("$base/$num/$fname");
	}
	closedir DIR;
	$num++;
}

exit 0;

sub process_player {
	my $inf_path = shift;
	my $parser = XML::LibXML->new();
	my $tree = $parser->parse_file($inf_path);
	my $root = $tree->documentElement;
	my $node = $root->firstChild;
	my %char;
	my @keys;
	my @vals;
	my @skills;
	my @aliases;
	my @weaponspecs;

	$char{"idnum"} = $root->getAttribute("idnum");

	@res = $dbh->selectrow_array("select COUNT(*) from players where idnum=" . $char{"idnum"});
	return 0 if ($res[0] == 0);

	$char{"name"} = $root->getAttribute("name");
	while ($node) {
		if ($node->nodeName eq "points"
				|| $node->nodeName eq "money"
				|| $node->nodeName eq "stats"
				|| $node->nodeName eq "time"
				|| $node->nodeName eq "carnage"
				|| $node->nodeName eq "attr"
				|| $node->nodeName eq "condition"
				|| $node->nodeName eq "player"
				|| $node->nodeName eq "rent"
				|| $node->nodeName eq "home"
				|| $node->nodeName eq "quest"
				|| $node->nodeName eq "immort") {
			for $attr ($node->attributes) {
				$char{$attr->nodeName} = $attr->value;
			}
		} elsif ($node->nodeName eq "bits"
				|| $node->nodeName eq "prefs") {
			for $attr ($node->attributes) {
				$char{$node->nodeName . "-" . $attr->nodeName} = $attr->value;
			}
		} elsif ($node->nodeName eq "class") {
			$val = $node->getAttribute("class");
			$char{"class"} = $val if $val;
			$val = $node->getAttribute("remort");
			$char{"remort"} = $val if $val;
			$val = $node->getAttribute("gen");
			$char{"gen"} = $val if $val;
			$val = $node->getAttribute("manash_pct");
			$char{"manash_pct"} = $val if $val;
			$val = $node->getAttribute("manash_low");
			$char{"manash_low"} = $val if $val;
		} elsif ($node->nodeName eq "weaponspec") {
			$spec = $node->getAttribute("vnum") . "+" . $node->getAttribute("level");
			push @weaponspecs, $spec;
		} elsif ($node->nodeName eq "alias") {
			$alias = $node->getAttribute("alias") . " " . $node->getAttribute("replace");
			push @aliases, $alias;
		} elsif ($node->nodeName eq "skill") {
			$skill = $node->getAttribute("level") . " " . $node->getAttribute("name");
			push @skills, $skill;
		} elsif ($node->nodeName eq "title") {
			$char{"title"} = $node->textContent;
		} elsif ($node->nodeName eq "description") {
			$char{"descrip"} = $node->textContent;
		} elsif ($node->nodeName eq "poofin") {
			$char{"poofin"} = $node->textContent;
		} elsif ($node->nodeName eq "poofout") {
			$char{"poofout"} = $node->textContent;
		} elsif ($node->nodeName eq "text"
				|| $node->nodeName eq "affect"
				|| $node->nodeName eq "frozen"
				|| $node->nodeName eq "affects") {
			# Do nothing
		} else {
			print STDERR "Invalid node '" . $node->nodeName . "'\n";
			exit(-1);
		}
			
		$node = $node->nextSibling;
	}

	for $key (keys %char) {
		$skip = 0;
		$val = $char{$key};
		if ($keytype{$key} eq "str") {
			$val =~ s/\\/\\\\/g;
			$val =~ s/'/''/g;
			$val = "'" . $val . "'";
		} elsif ($keytype{$key} eq "hex") {
			$val = "x'" . ('0' x (8 - (length $val))) . $val . "'";
		} elsif ($keytype{$key} eq "char") {
			$val = "'" . (uc (substr($val, 0, 1))) . "'";
		} elsif ($keytype{$key} eq "time") {
			$val = "'" . (strftime("%c", localtime($val))) . "'";
		} elsif ($keytype{$key} eq "interval") {
			$val = "'$val seconds'";
		} elsif ($keytype{$key} eq "class") {
			@res = $dbh->selectrow_array("select idnum from classes where name ilike '$val'");
			if (scalar @res > 0) {
				$val = $res[0];
			} else {
				$skip = 1;
			}
		} elsif ($keytype{$key} eq "race") {
			@res = $dbh->selectrow_array("select idnum from races where name ilike '$val'");
			if (scalar @res > 0) {
				$val = $res[0];
			} else {
				$skip = 1;
			}
		}

		if ($skip || $keytype{$key} ne "drop") {
			$key = $keymap{$key} || $key;
			push @keys, $key;
			push @vals, $val;
		}
	}

	$sql = "update players set ";
	$key = pop @keys;
	$val = pop @vals;
	while ($key) {
		$sql .= $key . "=" . $val;
		$key = pop @keys;
		$val = pop @vals;
		$sql .= ", " if ($key);
	}
	$sql .= " where idnum=" . $char{"idnum"};
#	print "$sql\n";
	if (!$dbh->do($sql)) {
		print "$sql\n";
		exit(-1);
	}

	$dbh->do("delete from weapon_specs where player=" . $char{"idnum"});
	foreach $spec (@weaponspecs) {
		($vnum, $lvl) = split(/\+/, $spec, 2);
		$sql = "insert into weapon_specs (player, vnum, lvl) values (";
		$sql .= $char{"idnum"} . ", ";
		$sql .= $vnum . ", " . $lvl . ")";

		if (!$dbh->do($sql)) {
			print "$sql\n";
			print "$spec\n";
			exit(-1);
		}
	}
}
