#!/usr/bin/perl
use strict; use warnings;

if (@ARGV != 1 ) {
	exit -1;
}

my $temp_file = $ARGV[0];

print "waiting for $temp_file";

# Wait for file to be created
while(!open( FILE, "<$temp_file" )) {
	print ".";
	sleep 5;
}

print "\nfound $temp_file\n";

# Check if file has expected contents
my @lines = <FILE>;
foreach(@lines) {
	chomp $_;
	print "line: '$_'\n";
	if($_ eq "Hello There!") {
		exit 0;
	}
}

exit -1;
