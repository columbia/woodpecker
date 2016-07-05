#!/usr/bin/perl

$YES = " *Yes* ";
$NO = " *No* ";
$REAL = " Real ";
$MARK = " Mark ";
$PATHS = "PATHS";
$INSTRS = "INSTRS";
$EPSILON = 0.000001;
$GET_REAL_PATHS_ORDER = "GET_REAL_PATHS_ORDER";
$SORT_WITH_REAL_PATHS = "SORT_WITH_REAL_PATHS";
$SORT_WITH_MARK_INSTRS = "SORT_WITH_MARK_INSTRS";
$SORT_WITH_RELATIVE_REAL_PATHS = "SORT_WITH_RELATIVE_REAL_PATHS";

$TABLE = @ARGV[0];
print "TABLE: ($TABLE)\n";
print $TABLE;
open(LOG, $TABLE);
@programs;
%events;

%realLines;
%markLines;

%realFinish;
%markFinish;

%realPrunedPaths;
%markPrunedPaths;

%realAllPaths;
%markAllPaths;

%realReductionPaths;
%markReductionPaths;
%relativeRealReductionPaths;
%orderOfRealReductionPaths;

%realValidInstrs;
%markValidInstrs;

%realAllInstrs;
%markAllInstrs;

%realReductionInstrs;
%markReductionInstrs;

$checkerName;

# Get results first.
foreach $line (<LOG>) {
#| comm.bc | OpenClose | Real | *Yes* | 51.2674 | 1.69952 | 10.3719 | 517 | 687 | 21 | 2280974 | *2280974* | 67721 | 900 | 2273 | 1 | 4 | tbd |
    chomp($line);
    my @fields = split(/\|/, $line);
    my $progName = $fields[1];
    $checkerName = $fields[2];
    my $mode = $fields[3];
    my $numStaticEvents = $fields[17];
    my $timeCost = $fields[5]-$fields[7]; # Discard bc2bdd init time.
    my $numPrunedPaths = $fields[8];
    chop($numPrunedPaths);
    $numPrunedPaths = substr($numPrunedPaths, 1);
    my $numAllPaths = $fields[9];
    chop($numAllPaths);
    $numAllPaths = substr($numAllPaths, 1);
    my $rate = ($numAllPaths - $numPrunedPaths)/$numAllPaths;
    my $numInstrs = $fields[11];
    chop($numInstrs);
    $numInstrs = substr($numInstrs, 1);
    my $numValidInstrs = $fields[12];
    my @numInstr = split(/\*/, $numValidInstrs);
    $numValidInstrs = $numInstr[1];
    #print "$fields[12] : $mode : ($numValidInstrs) ($numInstrs)\n";
    
    if ($mode eq $REAL) {
	push(@programs, $progName);
	$events{$progName} = $numStaticEvents;
	
    	$realLines{$progName} = $line;
	$realFinish{$progName} = $fields[4];
	$realPrunedPaths{$progName} = $numPrunedPaths;
	$realAllPaths{$progName} = $numAllPaths;
	$realReductionPaths{$progName} = $rate;
	$realValidInstrs{$progName} = $numValidInstrs;
	$realAllInstrs{$progName} = $numInstrs;
	#$realReductionInstrs{$progName} = $numValidInstrs/$numInstrs;
     } else {
    	$markLines{$progName} = $line;
	$markFinish{$progName} = $fields[4];
	$markPrunedPaths{$progName} = $numPrunedPaths;
	$markAllPaths{$progName} = $numAllPaths;
	$markReductionPaths{$progName} = $rate;
	$markValidInstrs{$progName} = $numValidInstrs;
	$markAllInstrs{$progName} = $numInstrs;
	$markReductionInstrs{$progName} = $numValidInstrs/$numInstrs;
    }

    #print "progName: $progName\n";
    #print "checkerName: $checkerName\n";
    #print "mode: ($mode)\n";
}

close(LOG);

# Check if data is missing.
sub checkDataMiss() {
	while (($key, $value) = each(%realLines)) {
	  if ($markLines{$key} eq "") {
	    print "Program $key is in Real but not in Mark.\n";
	    exit 1;
	  }
	}
	while (($key, $value) = each(%markLines)) {
	  if ($realLines{$key} eq "") {
	    print "Program $key is in Mark but not in Real.\n";
	    exit 1;
	  }
	}
}

checkDataMiss();

# Calculate real reduction rates, since we want to normalize it with mark prune approach.
while (($key, $value) = each(%realLines)) {
	$realReductionInstrs{$key} = $realValidInstrs{$key}/($markValidInstrs{$key});
	$relativeRealReductionPaths{$key} = 
		($realAllPaths{$key} - $realPrunedPaths{$key})/($markAllPaths{$key} - $markPrunedPaths{$key});
}

# Output xtics.
$BOTH_FINISHED_REAL_DATFILE = "./$TABLE."."finished.real.paths.dat";
$COMMON_REAL_DATFILE = "./$TABLE."."not-finished.real.paths.dat";
$checker = $checkerName;
chop($checker);
$checker = substr($checker, 1);
$GNUPLOT_FILE = "./draw-reduction.$checker.gp";
`cp ./draw-reduction $GNUPLOT_FILE`;

# Pring programs list.
sub appendGPFile {
	my $category = $_[0];
	my $originX = $_[1];
	my $originY = $_[2];
	my $dataFileLoc = $_[3];
	my $type = $_[4];
	my $offset = $_[5];
	my $metric = $_[6];
	my $sortType = $_[7];
	my $i = $offset;
	my %finishResults;
	my %reductionRates;
	my %sortedReductionRates;

	if ($type eq $REAL) {
		print "category real $category $metric----------------------\n";
		if ($metric eq $PATHS) { 
			%reductionRates = %realReductionPaths;
		}
		if ($metric eq $INSTRS) {
			%reductionRates = %realReductionInstrs;
		}
		%finishResults = %realFinish;
	} else {
		print "category mark $category $metric----------------------\n";
		if ($metric eq $PATHS) { 
			%reductionRates = %markReductionPaths;
		}
		if ($metric eq $INSTRS) {
			%reductionRates = %markReductionInstrs;
		}
		%finishResults = %markFinish;
	}
	%sortedReductionRates = %reductionRates;
	if ($sortType eq "SORT_WITH_MARK_INSTRS") {
		print "appendGPFile sorted_with_mark_instrs\n";
		%sortedReductionRates = %markReductionInstrs;
	}
	if ($sortType eq "SORT_WITH_RELATIVE_REAL_PATHS") {
		print "appendGPFile sorted_with_real_path_reduction\n";
		%sortedReductionRates = %relativeRealReductionPaths;
	}
	if ($sortType eq "SORT_WITH_REAL_PATHS") {
		print "appendGPFile sorted_with_real_path_reduction!!!!!!!!!!!!!!\n";
		%sortedReductionRates = %orderOfRealReductionPaths;
	}
	
	foreach $key (sort {$sortedReductionRates{$a} <=> $sortedReductionRates{$b} } keys %sortedReductionRates) {
		my @progs = split(/\./, $key);
		my $prog = substr($progs[0], 1);
		if ($category eq "invalid") {
			# NOP.
		} elsif ($events{$key} != 0) {
			if ($category eq "mix") {
				if ($i ne 0) {
					print GPFILE ", ";
				}
	 			$i++;
				print GPFILE "\"$prog\" $i";
			}
			if ($category eq "finished") {
				#print "check $key result $finishResults{$key}\n";
				if ($finishResults{$key} eq $YES) {
					if ($i ne 0) {
						print GPFILE ", ";
					}
					$i++;
					print GPFILE "\"$prog\" $i";
					if ($sortType eq "GET_REAL_PATHS_ORDER") {
						$orderOfRealReductionPaths{$key} = $i;
					}
				}
			}
			if ($category eq "not-finished") {
				if ($finishResults{$key} ne $YES) {
					#print "prog $key not-finished $type, category $category\n";
					if ($i ne 0) {
						print GPFILE ", ";
					}
					$i++;
					print GPFILE "\"$prog\" $i";
					if ($sortType eq "GET_REAL_PATHS_ORDER") {
						$orderOfRealReductionPaths{$key} = $i;
					}
				}
			}
		}
	}

	$i++;
	return ($i-1);
}

sub genDat {
	my $category = $_[0];
	my $type = $_[1];
	my $offset = $_[2];
	my $metric = $_[3];
	my $sortType = $_[4];
	my %finishResults;
	my %reductionRates;
	my %sortedReductionRates;
	
	if ($type eq $REAL) {
		#print "genDat category real $category----------------------\n";
		if ($metric eq $PATHS) { 
			%reductionRates = %realReductionPaths;
		}
		if ($metric eq $INSTRS) {
			%reductionRates = %realReductionInstrs;
		}
		%finishResults = %realFinish;
	} else {
		#print "genDat category mark----------------------\n";
		if ($metric eq $PATHS) { 
			%reductionRates = %markReductionPaths;
		}
		if ($metric eq $INSTRS) {
			%reductionRates = %markReductionInstrs;
		}
		%finishResults = %markFinish;
	}
	%sortedReductionRates = %reductionRates;
	if ($sortType eq "SORT_WITH_MARK_INSTRS") {
		print "genDat sorted_with_mark_instrs\n";
		%sortedReductionRates = %markReductionInstrs;
	}
	if ($sortType eq "SORT_WITH_RELATIVE_REAL_PATHS") {
		print "genDat sorted_with_real_path_reduction\n";
		%sortedReductionRates = %relativeRealReductionPaths;
		if ($type eq $REAL) {
			%reductionRates = %relativeRealReductionPaths;
		}
	}
	if ($sortType eq "SORT_WITH_REAL_PATHS") {
		%sortedReductionRates = %orderOfRealReductionPaths;
	}
	
	my $mixI = $offset;
	my $bfI = $offset;
	my $cmI = $offset;

	foreach $key (sort {$sortedReductionRates{$a} <=> $sortedReductionRates{$b} } keys %sortedReductionRates) {
		if ($events{$key} == 0) {
			# NOP.
		} else {			
			if ($category eq "mix") {
				$mixI++;
				print MIXDAT "# $markLines{$key}\n";
				print MIXDAT "# $realLines{$key}\n";
				my $val = 0;
				if ($finishResults{$key} eq $YES) {
					$val = 1;
				}
				print MIXDAT "$mixI     $reductionRates{$key}     0   $val\n";
			}
			if ($category eq "finished") {
				if ($finishResults{$key} eq $YES) {
					#print "program $type genDat $key ($finishResults{$key})\n";
					$bfI++;
					print BFDAT "# $markLines{$key}\n";
					print BFDAT "# $realLines{$key}\n";
					print BFDAT "$bfI     $reductionRates{$key}     0\n";
				}
			}
			if ($category eq "not-finished") {
				if ($finishResults{$key} ne $YES) {
					#print "program $type genDat $key ($finishResults{$key})\n";
					$cmI++;
					print CMDAT "# $markLines{$key}\n";
					print CMDAT "# $realLines{$key}\n";
					print CMDAT "$cmI     $reductionRates{$key}     0\n";
				}
			}
		}
	}

	if ($category eq "mix") {
		return $mixI;
	}
	if ($category eq "finished") {
		return $bfI;
	}
	if ($category eq "not-finished") {
		return $cmI;
	}
}




##################################################################





# Append gnuplot commands to the script.
open(GPFILE, ">>".$GNUPLOT_FILE);
#print GPFILE "set output \"$GNUPLOT_FILE.eps\"\n\n";
print GPFILE "set multiplot\n\n";
print GPFILE "set ylabel \"\% of not pruned paths\"\n";
print GPFILE "set xlabel \"$checkerName checker ($REAL prune)\""."\n";
print GPFILE "set origin 0.0, 1.2\n";
print GPFILE "set size 2.2, 1\n";
print GPFILE "set xtics (";
%orderOfRealReductionPaths = ();
$finishCnt = appendGPFile("finished", 0.0, 1.3, $BOTH_FINISHED_REAL_DATFILE, $REAL, 0, $PATHS, "GET_REAL_PATHS_ORDER");
print GPFILE " \\\n";
$cnt = appendGPFile("not-finished", 0.8, 1.3, $COMMON_REAL_DATFILE, $REAL, $finishCnt, $PATHS, "GET_REAL_PATHS_ORDER");
$cnt++;
print GPFILE ")\n";
print GPFILE "set yrange [0:1]\n";
print GPFILE "set xrange [0:$cnt]\n";
print GPFILE "plot ";
if ($finishCnt > 0) {
	print GPFILE "\"$BOTH_FINISHED_REAL_DATFILE\"  u (\$1):(\$2):3 t \"\" fs solid 0.5";
}
if ($cnt > $finishCnt + 1) {
	if ($finishCnt > 0) {
		print GPFILE ", \\\n";
	}
	print GPFILE "\"$COMMON_REAL_DATFILE\"  u (\$1):(\$2):3 t \"\" fs pattern 0.5\n";
} else {
	print GPFILE "\n";
}

close(GPFILE);

open(BFDAT, ">".$BOTH_FINISHED_REAL_DATFILE);
open(CMDAT, ">".$COMMON_REAL_DATFILE);
$cnt = genDat("finished", $REAL, 0, $PATHS);
genDat("not-finished", $REAL, $cnt, $PATHS);
close(BFDAT);
close(CMDAT);



# Print commands of reduction paths to gnuplot script.
$MIX_MARK_DATFILE = "./$TABLE."."mix.mark.paths.dat";

open(GPFILE, ">>".$GNUPLOT_FILE);
print GPFILE "set ylabel \"\% of not pruned paths\"\n";
print GPFILE "set xlabel \"$checkerName checker ($MARK prune)\""."\n";
print GPFILE "set origin 0.0, 0.2\n";
print GPFILE "set size 2.2, 1\n";
print GPFILE "set xtics (";
$cnt = appendGPFile("mix", 0.0, 0.0, $MIX_MARK_DATFILE, $MARK, 0, $PATHS, "SORT_WITH_REAL_PATHS");
$cnt++;
print GPFILE ")\n";
print GPFILE "set yrange [0:1]\n";
print GPFILE "set xrange [0:$cnt]\n";
print GPFILE "plot \"$MIX_MARK_DATFILE\"  u (\$1):(\$4>0 ? \$2:NaN):3 t \"\" fs solid 0.5, \\\n";
print GPFILE "\"$MIX_MARK_DATFILE\"  u (\$1):(\$4>0 ? NaN:\$2):3 t \"\" fs pattern 0.5\n";

print GPFILE "\n";
print GPFILE "set nomultiplot\n\n";
close(GPFILE);

# Print path reduction of mark pruned.
open(MIXDAT, ">".$MIX_MARK_DATFILE);
$cnt = genDat("mix", $MARK, 0, $PATHS, "SORT_WITH_REAL_PATHS");
close(MIXDAT);


##################################################################


# Draw the relative path reduction of real prune.
#$BOTH_FINISHED_RELATIVE_REAL_PATHS_DATFILE = "./$TABLE."."mix.relative.real.paths.dat";

#open(GPFILE, ">>".$GNUPLOT_FILE);
#print GPFILE "set ylabel \"\Relative not pruned paths\"\n";
#print GPFILE "set xlabel \"$checkerName checker ($REAL prune)\""."\n";
#print GPFILE "set origin 0.0, 9.8\n";
#print GPFILE "set size 2.2, 1\n";
#print GPFILE "set xtics (";
#$cnt = appendGPFile("mix", 0.0, 0.0, $BOTH_FINISHED_RELATIVE_REAL_PATHS_DATFILE, 
#	$REAL, 0, $PATHS, "SORT_WITH_RELATIVE_REAL_PATHS");
#$cnt++;
#print GPFILE ")\n";
#print GPFILE "set yrange [0:10]\n";
#print GPFILE "set xrange [0:$cnt]\n";
#print GPFILE "plot \"$BOTH_FINISHED_RELATIVE_REAL_PATHS_DATFILE\"  u (\$1):(\$4>0 ? \$2:NaN):3 t \"\" fs solid 0.5, \\\n";
#print GPFILE "\"$BOTH_FINISHED_RELATIVE_REAL_PATHS_DATFILE\"  u (\$1):(\$4>0 ? NaN:\$2):3 t \"\" fs pattern 0.5\n";

#print GPFILE "\n";
#print GPFILE "set nomultiplot\n\n";
#close(GPFILE);

# Print insrt reduction of real pruned.
#open(MIXDAT, ">".$BOTH_FINISHED_RELATIVE_REAL_PATHS_DATFILE);
#$cnt = genDat("mix", $REAL, 0, $PATHS, "SORT_WITH_RELATIVE_REAL_PATHS");
#close(MIXDAT);

# Draw the relative path reduction of mark prune. mix.
#$MIX_MARK_PATHS_DATFILE = "./$TABLE."."mix.mark.paths.dat";
#open(GPFILE, ">>".$GNUPLOT_FILE);
#print GPFILE "set ylabel \"\% of not pruned paths\"\n";
#print GPFILE "set xlabel \"$checkerName checker ($MARK prune, mix)\""."\n";
#print GPFILE "set origin 0.0, 9.0\n";
#print GPFILE "set size 2.2, 1\n";
#print GPFILE "set xtics (";
#$cnt = appendGPFile("mix", 0.0, 0.0, $MIX_MARK_PATHS_DATFILE, $MARK, 0, $PATHS, "SORT_WITH_RELATIVE_REAL_PATHS");
#$cnt++;
#print GPFILE ")\n";
#print GPFILE "set yrange [0:1]\n";
#print GPFILE "set xrange [0:$cnt]\n";
#print GPFILE "plot \"$MIX_MARK_PATHS_DATFILE\"  u (\$1):(\$4>0 ? \$2:NaN):3 t \"\" fs solid 0.5, \\\n";
#print GPFILE "\"$MIX_MARK_PATHS_DATFILE\"  u (\$1):(\$4>0 ? NaN:\$2):3 t \"\" fs pattern 0.5\n";
#print GPFILE "\n";
#print GPFILE "set nomultiplot\n\n";
#close(GPFILE);

# Print path reduction of real pruned.
#open(MIXDAT, ">".$MIX_MARK_PATHS_DATFILE);
#$cnt = genDat("mix", $MARK, 0, $PATHS, "SORT_WITH_RELATIVE_REAL_PATHS");
#close(MIXDAT);


