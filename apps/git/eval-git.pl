#!/usr/bin/perl

#Usage (run the script with an absolute path of an app in direct-sym, we assume all bc are built in this directory):
# ./eval-path-slicer-coverage.pl 6000 /home/heming/rcs/direct-sym/apps/coreutils/ both File [0] [200] [0 3 2] [.bc] [no-sym-etc] [2 8]
# the ARGV[2], mode, can be "both" or "mark-pruned-only" or "real-prune-only".
# the ARGV[3], checker, can be any one of the valid checkers.
# the ARGV[4], optional (default 0), start index, starting from the given index of programs
#   (this is mainly for coreutils, since it has a lot programs).
# the ARGV[5], optional (default 200), end index, end at the given index of programs
#   (this is mainly for coreutils, since it has a lot programs).
# the ARGV[6], optional (default "0 3 2"), the symbolic args passed to the klee command.
# the ARGV[7], optional (default ".bc"), or you can specify "git-".
# the ARGV[8], optional (default "no-sym-etc"), or you can specify "sym-etc", which will turn on --make-etc-symbolic option.
# the ARGV[9], optional (default "2 8"), symbolic files setting.

# Global variables for configuration.
$defaultStartProgIdx = 0;
$defaultEndProgIdx = 200; # Just assign a very large number.

$maxTime = @ARGV[0];

$appPath = @ARGV[1];

$mode = @ARGV[2];

$checkerName = @ARGV[3];

$testCategory = "eval-".$mode."-".$checkerName."-".$maxTime."s";

$startProgIdx = $defaultStartProgIdx;
$endProgIdx = $defaultEndProgIdx;
if (scalar(@ARGV) > 4) {
	$startProgIdx = @ARGV[4];
}

if (scalar(@ARGV) > 5) {
	$endProgIdx = @ARGV[5];
}

# The same setting as KLEE OSDI '08.
#$useRandomPath = " --use-random-path ";
$useRandomPath = "";
$defaultSearcher = "--randomize-fork --use-interleaved-covnew-NURS ".
	"--use-batching-search --batch-instructions=10000";
$dfsSearcher = "";  # DFS is the default search of KLEE, so we do not need to specify anything.
$searcher = $defaultSearcher;

# One more symbolic argument than that in KLEE OSDI '08, since we may want to find bugs.
$defaultSymArgs = "0 3 2";
$symbolicArgs = $defaultSymArgs;
if (scalar(@ARGV) > 6) {
	$symbolicArgs = @ARGV[6];
}

# The same setting as KLEE OSDI '08.
$maxFail = "1";

$defaultBcKey = "*.bc";
$bcKey = $defaultBcKey;
if (scalar(@ARGV) > 7) {
	$bcKey = @ARGV[7];
}

$symbolicETC = " --disable-sym-filter "; # Default, no symbolic etc.
if (scalar(@ARGV) > 8) {
	if (@ARGV[8] eq "sym-etc") {
		$symbolicETC = "--make-etc-symbolic ";
		$useRandomPath = "";	# If it is shadow, do not use randome path, since it uses to much memory in sym fs.
	}
}

# The same setting as KLEE OSDI '08.
$symbolicFiles = "2 8";
if (scalar(@ARGV) > 9) {
	$symbolicFiles = @ARGV[9];
}

@excludeProgs = (
	# A program in shadow which uses too much memory.
	"sulogin", #"grpck", "pwck", "id",

	# This utility in shadow calls sigaction(), which calls klee to crash.
	"chfn", "gpasswd",

	# Run out of time much, no halt timer involved, weird.
	#"pr", "sort", # stdbuf,
	#"fmt", "fold",

	# floating pointer error in original klee.
	"printf",

	# Has an STP bug...
	"md5sum", "date"
	);



sub evalPathSlicerCoverage {
	my $appPath = $_[0];
	my @programs;
	my $curProgNum = 0;
	print "Application absolute path ($appPath), running all bc within this directory...\n";
	chdir($appPath);
	foreach (`ls $bcKey`) {
		push(@programs, $_);
	}
	chop(@programs);

	print "\nProgram region: [".$startProgIdx." ~ ".$endProgIdx."]\n";
	foreach $prog (@programs) {
		if ($curProgNum < $startProgIdx) {
			$curProgNum++;
			next;
		}
		if ($curProgNum > $endProgIdx) {
			last;
		}
		evalProgram($prog, $curProgNum, scalar(@programs));
		$curProgNum++;
	}
}

sub isExcludeProgram {
	my $prog = $_[0];
	foreach $exclude (@excludeProgs) {
		if ($prog eq $exclude) {
			return 1;
		}
	}
	return 0;
}

sub runKLEE {
	my $progName = $_[0];
	my $checkerName = $_[1];
	my $maxTime = $_[2];
	my $enablePathSlicer = $_[3];
	my $markPrunedOnly = $_[4];
	my $slicerStr = "";
	my $pruneStr = "";
	if ($enablePathSlicer eq 1) {
		$slicerStr = "with-slicer";
	} else {
		$slicerStr = "no-slicer";
	}
	if ($markPrunedOnly eq 1) {
		$pruneStr = "mark-pruned";
	} else {
		$pruneStr = "real-prune";
	}
	my $outFile = "./out-$slicerStr-$pruneStr.txt";
	#my $kleeOutDir = "./klee-out-$slicerStr-$pruneStr";
	
	my $command = "time klee --max-time $maxTime --libc=uclibc --posix-runtime --init-env ".
	"$searcher $useRandomPath --only-output-states-covering-new --disable-inlining ".
	"--use-one-checker=$checkerName ".
	"--use-path-slicer=$enablePathSlicer --mark-pruned-only=$markPrunedOnly ".
	"$progName --sym-args $symbolicArgs --sym-files $symbolicFiles $symbolicETC ".
	"--max-fail $maxFail 2>&1 | tee $outFile";
	print `date`;
	print "Process is running command ($command)...\n\n";
	$output = `$command`;
    print "Exit code: $?\n";
	open(OUT, ">./$outFile");
	print OUT $output;
	close(OUT);

	$output = `tail ./$outFile -n50`;
	print $output;
}

sub evalProgram {
	my $progName = $_[0];
	my $curProgNum = $_[1];
	my $totalProgNum = $_[2];
	my @progOrigName = split(/\./, $progName);
	print "\n\n\n========== [$curProgNum / $totalProgNum] $progName ==========\n";

	if (isExcludeProgram($progOrigName[0]) == 1) {
		print "Exclude program $progOrigName[0]...\n";
		return;
	}

	# Setup command and run.
	my $enablePathSlicer;
	my $output;

	if (($mode eq "both") || ($mode eq "mark-pruned-only")) {
		# Setup directory and necessary files for each program.
		my $progDir = $progOrigName[0]."-".$testCategory."-mark-pruned";
		`rm -rf $progDir`;
		mkdir($progDir, 0755);
		chdir($progDir);
		`cp ../$progName .`;
		`cp ../local.options .`;
		`cp ../bc2bdd.conf .`;
		`chmod -R 777 .`;
		$enablePathSlicer = 1;
		$markPrunedOnly = 1;
        system("git init");

		runKLEE($progName, $checkerName, $maxTime, $enablePathSlicer, $markPrunedOnly);
		chdir("../");
	}

	# Sleep for 1 second.
	    if ($mode eq "both") {
	        sleep(3);
	    }

	if (($mode eq "both") || ($mode eq "real-prune-only")) {
		# Setup directory and necessary files for each program.
		my $progDir = $progOrigName[0]."-".$testCategory."-real-prune";
		`rm -rf $progDir`;
		mkdir($progDir, 0755);
		chdir($progDir);
		`cp ../$progName .`;
		`cp ../local.options .`;
		`cp ../bc2bdd.conf .`;
		`chmod -R 777 .`;
		$enablePathSlicer = 1;
		$markPrunedOnly = 0;
        system("git init");

		runKLEE($progName, $checkerName, $maxTime, $enablePathSlicer, $markPrunedOnly);
		chdir("../");
	}
}

evalPathSlicerCoverage($appPath);

