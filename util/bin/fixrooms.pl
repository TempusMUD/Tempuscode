#!/usr/bin/perl

$change_count = $skip_count = 0;
$path = shift;
if ( !$path )
	{
	print STDERR "Usage: fixtempus.pl <path>\n";
	exit -1;
	}

if ( -d $path )
	{
	# Correct entire directory
	opendir( DIR,$path ) || die( "Directory does not exist\n" );
	while ( $file = readdir( DIR ) )
		{
		if ( ! -d "$path/$file" && $file =~ /\.wld$/ )
			{ process_file( "$path/$file" ); }
		}
	closedir( DIR );
	}
else
	{ process_file( $path ); }     # Correct single file

print STDERR "$change_count total changes.\n";
print STDERR "$skip_count were ok.\n";

exit 0;

sub process_file
	{
	my $inf_name = shift,$line_num,$sub_id;
	my %room;

	open( INF,"<$inf_name" ) || die( "Can't open $inf_name" );
	open( STDOUT,">$inf_name.new" ) || die( "Can't open $inf_name.new" );

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
				output_room( \%room );
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
				{ $state = "search-cmd" }
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
			if ( $sub_id )
				{ $sub_id .= "\n"; }
			$sub_id .= $_;
			$search_cmd = $sub_id;
			}
		elsif ( $state eq "search-key" )
			{
			if ( /~/ )
				{
				$state = "search-vict";
				s/~//g;
				}
			if ( $search_key )
				{ $search_key .= "\n"; }
			$search_key .= $_;
			$sub_id = $search_cmd . "/" . $search_key;
			if ( $state eq "search-vict" )
				{
				$room{"searches"}{$sub_id}{"cmd"} = $search_cmd;
				$room{"searches"}{$sub_id}{"keywords"} = $search_key;
				}
			}
		elsif ( $state eq "search-vict" )
			{
			if ( /~/ )
				{
				$state = "search-room";
				s/~//g;
				}
			if ( $room{"searches"}{$sub_id}{"vict"} )
				{ $room{"searches"}{$sub_id}{"vict"} .= "\n"; }
			$room{"searches"}{$sub_id}{"vict"} .= $_;
			}
		elsif ( $state eq "search-room" )
			{
			if ( /~/ )
				{
				$state = "search-remote";
				s/~//g;
				}
			if ( $room{"searches"}{$sub_id}{"room"} )
				{ $room{"searches"}{$sub_id}{"room"} .= "\n"; }
			$room{"searches"}{$sub_id}{"room"} .= $_;
			}
		elsif ( $state eq "search-remote" )
			{
			if ( /~/ )
				{
				$state = "search-flags";
				s/~//g;
				}
			if ( $room{"searches"}{$sub_id}{"remote"} )
				{ $room{"searches"}{$sub_id}{"remote"} .= "\n"; }
			$room{"searches"}{$sub_id}{"remote"} .= $_;
			}
		elsif ( $state eq "search-flags" )
			{
			if ( !/([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+) ([-0-9]+)/ )
				{ print STDERR "No match for search flags in $inf_name:$line_num\n"; exit }
			$room{"searches"}{$sub_id}{"command"} .= $1;
			$room{"searches"}{$sub_id}{"arg0"} .= $2;
			$room{"searches"}{$sub_id}{"arg1"} .= $3;
			$room{"searches"}{$sub_id}{"arg2"} .= $4;
			$room{"searches"}{$sub_id}{"flags"} .= $5;
			$state = "optional";
			}
		else
			{ print STDERR "Invalid state '$state' reached at $inf_name:$line_num!!!\n"; exit }
		}

	print "\$~\n";

	close( INF );
	close( STDOUT );

	rename( $inf_name,"$inf_name.bak" );
	rename( "$inf_name.new",$inf_name );
	}

sub output_room
	{
	my %room = %{ shift() };

	print "#" . $room{"id"} . "\n";
	print $room{"title"} . "~\n";
	print $room{"fulldesc"} . "~\n";
	print $room{"zone"} . " " . $room{"flags"} . " " . $room{"sector"} . "\n";
	foreach $dir (keys %{$room{"dir"}})
		{
		print "D" . $dir . "\n";
		print $room{"dir"}{$dir}{"desc"} . "~\n";
		print $room{"dir"}{$dir}{"keywords"} . "~\n";
		print $room{"dir"}{$dir}{"exitinfo"} . " ";
		print $room{"dir"}{$dir}{"key"} . " ";
		print $room{"dir"}{$dir}{"to_room"} . "\n";
		}
	foreach $extradesc ( keys %{$room{"extradesc"}} )
		{
		print "E\n";
		print $extradesc . "~\n";
		print $room{"extradesc"}{$extradesc} . "~\n";
		}
	foreach $dir (keys %{$room{"flow"}})
		{
		print "F\n";
		print $dir . " ";
		print $room{"flow"}{$dir}{"speed"} . " ";
		print $room{"flow"}{$dir}{"type"} . "\n";
		}
	foreach $search ( keys %{$room{"searches"}} )
		{
		print "Z\n";
		print $room{"searches"}{$search}{"cmd"} . "~\n";
		print $room{"searches"}{$search}{"keywords"} . "~\n";
		print $room{"searches"}{$search}{"vict"} . "~\n";
		print $room{"searches"}{$search}{"room"} . "~\n";
		print $room{"searches"}{$search}{"remote"} . "~\n";
		print $room{"searches"}{$search}{"command"} . " ";
		print $room{"searches"}{$search}{"arg0"} . " ";
		print $room{"searches"}{$search}{"arg1"} . " ";
		print $room{"searches"}{$search}{"arg2"} . " ";
		print $room{"searches"}{$search}{"flags"} . "\n";
		}
	if ( $room{"sound"} )
		{ print "L\n" . $room{"sound"} . "~\n" }
	print "S\n";
	}

sub process_room
	{
	my %room = %{ shift() };

	if ( $room{"flags"} !~ /[lmn]/ ) # Skip houses
		{
		$room{"title"} = capitalize( $room{"title"} );

		if ( $room{"flags"} !~ /[b]/ ) # Skip DTs
			{
			$room{"fulldesc"} = format_desc( " " . $room{"fulldesc"} );
			}
		}

	return %room;
	}

sub format_desc
	{
	my $str,@lines,$line,@words,$word,$result,$screen_width = 70;

	$orig = $str = shift;
	$result = "";

	$line_width = 0;
	foreach $line ( split /[\n\r]+/m,$str )
		{
		if ( $line =~ /^ / )
			{
			$result =~ s/ +$//;
			if ( $line_width )
				{ $result .= "\n"; };
			$result .= "  ";
			$line_width = 2;
			}
		
		foreach $word ( split / +/,$line )
			{
			$line_width += length $word;
			if ( $line_width > $screen_width )
				{
				$result =~ s/ +$//;
				$result .= "\n";
				$line_width = length $word;
				}

			$result .= $word;
			if ( $word =~ /\.$/ )
				{ $result .= "  "; $line_width += 2;}
			else
				{ $result .= " "; $line_width += 1; }
			}
		}
	
	$result =~ s/ +$//;
	$result .= "\n";
	if ( $orig ne $result )
		{ $change_count++; }
	else
		{ $skip_count++; }

	return $result;
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
    foreach $word ( split /([^A-Za-z']+)/,$str )
        {
		if ( $cap_next )
			{
			$word = ucfirst $word;
			$cap_next = 0;
			}
		elsif ( $word !~ /[^A-Za-z']/ && grep { $_ eq lc $word } @nocap_words )
			{ $word = lc $word; }
		else
			{
			$cap_next = ( $word =~ /[!.?]/ );
			$word = ucfirst $word;
			}

		$word =~ s/^Mc(.)/Mc\u\1/;

        $result .= $word;
        }

	if ( $orig ne $result )
		{ $change_count++; }

	return $result;
	}
