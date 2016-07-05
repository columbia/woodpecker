#!/usr/bin/perl

if (scalar(@ARGV) ne 1) {
	print "Usage: ./gen-figures.pl coreutils\n";
	exit 0;
}

$appSuite=$ARGV[0];

`mkdir -p ./$appSuite`;
`rm -rf ./$appSuite/*.dat`;

sub drawOneChecker {
	my $checkerName = $_[0];
	my $table = "./results/$appSuite-$checkerName.table";
	if (-e $table) {
		print "\n\n\nGenerating figure for app suite $appSuite with checker $checkerName with data from $table...\n\n";
		print `perl ./table-to-dat.pl $table`;
		print `gnuplot ./draw-reduction.$checkerName.gp > ./$appSuite/$checkerName.eps`;
		print `mv ./draw-reduction.$checkerName.gp ./$appSuite`;
		print `mv ./results/*.dat ./$appSuite/`;
	} else {
		print "Not generating figure for app suite $appSuite with checker $checkerName with data from $table...\n";
	}
}

drawOneChecker("OpenClose");
drawOneChecker("File");
drawOneChecker("Assert");
drawOneChecker("Leak");
drawOneChecker("DataLoss");

