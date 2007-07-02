#!/usr/bin/perl

%months = (
	"Jan" => 1,
	"Feb" => 2,
	"Mar" => 3,
	"Apr" => 4,
	"May" => 5,
	"Jun" => 6,
	"Jul" => 7,
	"Aug" => 8,
	"Sep" => 9,
	"Oct" => 10,
	"Nov" => 11,
	"Dec" => 12,
);

$inf_name = shift;
open(INF, "<$inf_name") || die("Can't open $inf_name");
$year = shift;
while (<INF>) {
	next if !/^\S+ (\S+)\s+(\d+) \d+:\d+:\d+ :: nusage: \d+\s+sockets (connected|input_mode), (\d+)/;
	if (!$new_year && $1 eq "Jan" && $2 eq "1") {
		$year++;
	}
	$new_year = ($1 eq "Jan" && $2 eq "1");
	$key = sprintf("%04d/%02d/%02d",$year,$months{$1}, $2);
	$usage_sum{$key} += $4;
	$usage_count{$key} += 1;
	$usage_peak{$key} = $4 if $4 > $usage_peak{$key};
}
close INF;

for $key (sort keys %usage_sum) {
	print $key . " " .
		int($usage_sum{$key} / $usage_count{$key}) . " " .
		$usage_peak{$key} . "\n";
}
