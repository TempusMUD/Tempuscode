#!/usr/bin/perl

$change_count = $skip_count = 0;
$inf_path = shift;

if ( !$inf_path ) {
	print STDERR "Usage: check_rooms.pl <path>\n";
	exit -1;
}

if ( -d $inf_path ) {
	# Correct entire directory
	opendir( DIR,$inf_path ) || die( "Directory does not exist\n" );
	while ( $file = readdir( DIR ) ) {
		if ( ! -d "$inf_path/$file" && $file =~ /\.wld$/ ) {
			process_file( "$inf_path/$file");
		}
	}
	closedir( DIR );
} else {
	process_file( $inf_path);
}     # Correct single file

exit 0;

sub process_file
	{
	my $inf_name = shift,$line_num,$sub_id;
	my $ouf_path = shift;
	my %room;

	open( INF,"<$inf_name" ) || die( "Can't open $inf_name" );

	$state = "roomnum";
	undef $line_num;
	while ( <INF> )
		{
		$line_num++;
		chomp;
		if ( $state eq "roomnum" )
			{
			if ( !/^[#\$]/ )
				{ print STDERR "Invalid format in $inf_name:$line_num\n"; exit -1; }
			if ( /^\#/ )
				{
				$room{"id"} = substr( $_,1 );
				$state = "title";
				}
			}
		elsif ( $state eq "title" )
			{
			if ( /~/ )
				{
				$state = "fulldesc";
				s/~//g;
				}
			if ( $room{"title"} )
				{ print STDERR "WARNING: Multi-line title found in room " . $room{"id"} . "\n"; }
			$room{"title"} = $_;
			}
		elsif ( $state eq "fulldesc" )
			{
			if ( /~/ )
				{
				$state = "flags";
				s/~//g;
				}
			if ( $room{"fulldesc"} )
				{ $room{"fulldesc"} .= "\n"; }
			$room{"fulldesc"} .= $_;
			}
		elsif ( $state eq "flags" )
			{
			/^([0-9]+) ([^ ]+) ([0-9]+)/;

			$room{"zone"} = $1;
			$room{"flags"} = $2;
			$room{"sector"} = $3;
			$state = "optional";
			}
		elsif ( $state eq "optional" )
			{
			undef $sub_id;
			if ( /^S/ ) # End of room
				{
				%room = process_room( \%room );
				undef %room;
				$state = "roomnum";
				}
			elsif ( /^O/ ) # Occupancy
				{ $room{"occupancy"} = substr( $_,1 ); }
			elsif ( /^D/ ) # Direction
				{
				$dir = substr( $_,1 );
				if ( $room{"dir"}{$dir} )
					{ print STDERR "Multiple direction definition in $inf_name:$line_num\n"; exit}
				$sub_id = $dir;
				$state = "dir-desc";
				}
			elsif ( /^E/ ) # Extra Description
				{ $state = "extradesc-keywords" }
			elsif ( /^L/ ) # Sound
				{ $state = "sound"; }
			elsif ( /^F/ ) # Flow Information
				{ $state = "flow"; }
			elsif ( /^Z/ ) # Search
				{
				$state = "search-cmd";
				$sub_id = $room{"searchcount"} || 0;
				$room{"searchcount"}++;
				}
			elsif ( /^R/ ) # Prog
				{ $state = "prog" }
			else
				{ print STDERR "Invalid directive in $inf_name:$line_num\n"; exit }
			}
		elsif ( $state eq "dir-desc" )
			{
			if ( /~/ )
				{
				$state = "dir-keywords";
				s/~//g;
				}
			if ( $room{"dir"}{$sub_id}{"desc"} )
				{ $room{"dir"}{$sub_id}{"desc"} .= "\n"; }
			$room{"dir"}{$sub_id}{"desc"} .= $_;
			}
		elsif ( $state eq "dir-keywords" )
			{
			if ( /~/ )
				{
				$state = "dir-flags";
				s/~//g;
				}
			if ( $room{"dir"}{$sub_id}{"keywords"} )
				{ $room{"dir"}{$sub_id}{"keywords"} .= "\n"; }
			$room{"dir"}{$sub_id}{"keywords"} .= $_;
			}
		elsif ( $state eq "dir-flags" )
			{
			if ( !/^([^ ]+) +([-0-9]+) +([-0-9]+)/ )
				{
				print STDERR "Dir flags did not match at $inf_name:$line_num\n";
				print STDERR "Line is: $_\n";
				exit
				}
			$room{"dir"}{$sub_id}{"exitinfo"} = $1;
			$room{"dir"}{$sub_id}{"key"} = $2;
			$room{"dir"}{$sub_id}{"to_room"} = $3;
			$state = "optional";
			}
		elsif ( $state eq "extradesc-keywords" )
			{
			if ( /~/ )
				{
				$state = "extradesc-desc";
				s/~//g;
				}
			if ( $sub_id )
				{ $sub_id .= "\n"; }
			$sub_id .= $_;
			}
		elsif ( $state eq "extradesc-desc" )
			{
			if ( /~/ )
				{
				$state = "optional";
				s/~//g;
				}
			if ( $room{"extradesc"}{$sub_id} )
				{ $room{"extradesc"}{$sub_id} .= "\n"; }
			$room{"extradesc"}{$sub_id} .= $_;
			}
		elsif ( $state eq "sound" )
			{
			if ( /~/ )
				{
				$state = "optional";
				s/~//g;
				}
			if ( $room{"sound"} )
				{ $room{"sound"} .= "\n"; }
			$room{"sound"} .= $_;
			}
		elsif ( $state eq "flow" )
			{
			/([-0-9]+) ([-0-9]+) ([-0-9]+)/;
			if ( $room{"flow"}{$1} )
				{ print STDERR "Duplicate flow definition at $inf_name:$line_num\n"; exit }
			$room{"flow"}{$1}{"speed"} = $2;
			$room{"flow"}{$1}{"type"} = $3;
			$state = "optional";
			}
		elsif ( $state eq "search-cmd" )
			{
			if ( /~/ )
				{
				$state = "search-key";
				s/~//g;
				}
			if ( $room{"searches"}[$sub_id]{"cmd"} )
				{ $room{"searches"}[$sub_id]{"cmd"} .= "\n"; }
			$room{"searches"}[$sub_id]{"cmd"} .= $_;
			}
		elsif ( $state eq "search-key" )
			{
			if ( /~/ )
				{
				$state = "search-vict";
				s/~//g;
				}
			if ( $room{"searches"}[$sub_id]{"keywords"} )
				{ $room{"searches"}[$sub_id]{"keywords"} .= "\n"; }
			$room{"searches"}[$sub_id]{"keywords"} .= $_;
			}
		elsif ( $state eq "search-vict" )
			{
			if ( /~/ )
				{
				$state = "search-room";
				s/~//g;
				}
			if ( $room{"searches"}[$sub_id]{"vict"} )
				{ $room{"searches"}[$sub_id]{"vict"} .= "\n"; }
			$room{"searches"}[$sub_id]{"vict"} .= $_;
			}
		elsif ( $state eq "search-room" )
			{
			if ( /~/ )
				{
				$state = "search-remote";
				s/~//g;
				}
			if ( $room{"searches"}[$sub_id]{"room"} )
				{ $room{"searches"}[$sub_id]{"room"} .= "\n"; }
			$room{"searches"}[$sub_id]{"room"} .= $_;
			}
		elsif ( $state eq "search-remote" )
			{
			if ( /~/ )
				{
				$state = "search-flags";
				s/~//g;
				}
			if ( $room{"searches"}[$sub_id]{"remote"} )
				{ $room{"searches"}[$sub_id]{"remote"} .= "\n"; }
			$room{"searches"}[$sub_id]{"remote"} .= $_;
			}
		elsif ( $state eq "search-flags" )
			{
			if ( !/([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+)/ )
				{ print STDERR "No match for search flags in $inf_name:$line_num\n"; exit }
			$room{"searches"}[$sub_id]{"command"} .= $1;
			$room{"searches"}[$sub_id]{"arg0"} .= $2;
			$room{"searches"}[$sub_id]{"arg1"} .= $3;
			$room{"searches"}[$sub_id]{"arg2"} .= $4;
			$room{"searches"}[$sub_id]{"flags"} .= $5;
			$room{"searches"}[$sub_id]{"failure"} .= $6;
			$state = "optional";
			}
		elsif ( $state eq "prog" )
			{
			if ( /~/ )
				{
				$state = "optional";
				s/~//g;
				}
			if ( $room{"prog"} )
				{ $room{"prog"} .= "\n"; }
			$room{"prog"} .= $_;
			}
		else
			{ print STDERR "Invalid state '$state' reached at $inf_name:$line_num!!!\n"; exit }
		}
	close( INF );
	}

sub process_room
	{
	my %room = %{ shift() };
	my $idnum = $room{"id"};
	my @dirs = ( "north", "east", "south", "west",
		"up", "down", "future", "past" );

	if ( $room{"flags"} !~ /[lmn]/ ) {
		if ($room{"title"} ne capitalize( $room{"title"} )) {
			print "Title capitalization must be fixed in room $idnum\n";
		}

		if ( $room{"flags"} !~ /[b]/ ) {
			check_desc($room{"fulldesc"}, "room $idnum" );
			foreach $dir (keys %{$room{"dir"}}) {
				check_desc($room{"dir"}{$dir}{"desc"},
					$dirs[$dir] . " exit of room $idnum");
			}
			foreach $extradesc ( keys %{$room{"extradesc"}} ) {
				check_desc($room{"extradesc"}{$extradesc},
					"extradesc $extradesc of room $idnum");
			}
			foreach $search ( @{$room{"searches"}} ) {
				$emit = ${$search}{"vict"};
				$emit =~ s/\$.//g;
				check_desc($emit, "vict emit of search [${$search}{'cmd'}/${$search}{'keywords'}] in room $idnum");
				$emit = ${$search}{"room"};
				$emit =~ s/\$.//g;
				check_desc($emit, "room emit of search [${$search}{'cmd'}/${$search}{'keywords'}] in room $idnum");
				$emit = ${$search}{"remote"};
				$emit =~ s/\$.//g;
				check_desc($emit, "remote emit of search [${$search}{'cmd'}/${$search}{'keywords'}] in room $idnum");
			}
			if ( $room{"sound"} ) {
				check_desc($room{"sound"}, "sound of room $idnum");
			}
		}
	}
	return %room;
}

sub check_desc
{
	my $str,@lines,$line,@words,$word,$result,$where,$screen_width = 72;
	my $linenum;

	$str = shift;
	$where = shift;

	# Not properly wrapped
	$linenum = 1;
	foreach $line ( split /[\n\r]+/m,$str ) {
		if (length $line > $screen_width) {
			print "Line $linenum is " . (length $line) . " characters in $where\n";
			last;
		}
		$linenum++;
	}
	
	# Misspelled words
	open(OUF, ">/tmp/desc.txt");
	print OUF $str;
	close(OUF);
	open(EXT, "/home/httpd/lib/dspell -d /usr/dict/ds.words -d ./tempus.words /tmp/desc.txt|");
	while (<EXT>) {
		if (/^M \d+ \d+ \[([^\]]+)\]$/) {
			print "Possible mispelling of $1 in $where\n";
		}
	}
	close(EXT);

	# Bad period spacing
	if ($str =~ /\.\+( \S|\S)/) {
		print "Two spaces missing after period in $where (got '$1')\n";
	}
}

sub capitalize
{
	my $orig,$str,$result,$cap_next,$word;
	my @nocap_words = ( "a","an","is","if","or","the","of","in","into","to",
						"and","your","on","off","by","out","no","it","at",
						"as","for" );


	$orig = $str = shift;
	$str =~ s/\.+$//;
	$result = "";

	$cap_next = 1;
    foreach $word ( split /([^A-Za-z']+)/,$str ) {
		if ( $cap_next ) {
			$word = ucfirst $word;
			$cap_next = 0;
		} elsif ($word !~ /[^A-Za-z']/ && grep { $_ eq lc $word } @nocap_words) {
			$word = lc $word;
		} else {
			$cap_next = ( $word =~ /[!.?]/ );
			$word = ucfirst $word;
		}

		$word =~ s/^Mc(.)/Mc\u\1/;

        $result .= $word;
	}

	return $result;
}
