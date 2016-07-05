$TABLE = @ARGV[0];
print "TABLE: ($TABLE)\n";
print $TABLE;
open(LOG, $TABLE);

$YES = " *Yes* ";
$NO = " *No* ";
$REAL = " Real ";
$MARK = " Mark ";

$OPENCLOSE = " OpenClose ";
$FILE = " File ";
$ASSERT = " Assert ";
$LEAK = " Leak ";
$DATALOSS = " DataLoss ";

%opencloseVerifiedProgsReal;
%opencloseVerifiedProgsMark;

%fileVerifiedProgsReal;
%fileVerifiedProgsMark;

%assertVerifiedProgsReal;
%assertVerifiedProgsMark;

%leakVerifiedProgsReal;
%leakVerifiedProgsMark;

%datalossVerifiedProgsReal;
%datalossVerifiedProgsMark;

%opencloseNVProgsReal;
%opencloseNVProgsMark;

%fileNVProgsReal;
%fileNVProgsMark;

%assertNVProgsReal;
%assertNVProgsMark;

%leakNVProgsReal;
%leakNVProgsMark;

%datalossNVProgsReal;
%datalossNVProgsMark;

%opencloseNVProgsRealPruned;
%opencloseNVProgsMarkPruned;

%fileNVProgsRealPruned;
%fileNVProgsMarkPruned;

%assertNVProgsRealPruned;
%assertNVProgsMarkPruned;

%leakNVProgsRealPruned;
%leakNVProgsMarkPruned;

%datalossNVProgsRealPruned;
%datalossNVProgsMarkPruned;


$numMarkPrunedPaths = 0;
$numMarkAllPaths = 0;

%opencloseRealPathCov;
%opencloseMarkPathCov;

%fileRealPathCov;
%fileMarkPathCov;

%assertRealPathCov;
%assertMarkPathCov;

%leakRealPathCov;
%leakMarkPathCov;

%datalossRealPathCov;
%datalossMarkPathCov;

# Get results first.
foreach $line (<LOG>) {
#| comm.bc | OpenClose | Real | *Yes* | 51.2674 | 1.69952 | 10.3719 | 517 | 687 | 21 | 2280974 | *2280974* | 67721 | 900 | 2273 | 1 | 4 | tbd |
    chomp($line);
    my @fields = split(/\|/, $line);
    my $progName = $fields[1];
    my $checkerName = $fields[2];
    my $mode = $fields[3];
    my $finish = $fields[4];
    my $finishTime = $fields[5];
    my $numStaticEvents = $fields[17];
    my $numPrunedPaths = $fields[8];
    my $numAllPaths = $fields[9];
    my $numValidPaths = $numAllPaths - $numPrunedPaths;

    if ($numStaticEvents > 0) { 
           if ($mode eq $MARK) {
		$numMarkPrunedPaths += $numPrunedPaths;
		$numMarkAllPaths += $numAllPaths;
           }
    
	    if ($finish eq $YES) {
		    if ($mode eq $REAL) {
			if ($checkerName eq $OPENCLOSE) {
				$opencloseVerifiedProgsReal{$progName} = $finishTime;
			} elsif ($checkerName eq $FILE) {
				$fileVerifiedProgsReal{$progName} = $finishTime;
			} elsif ($checkerName eq $ASSERT) {
				$assertVerifiedProgsReal{$progName} = $finishTime;
			} elsif ($checkerName eq $LEAK) {
				$leakVerifiedProgsReal{$progName} = $finishTime;
			} elsif ($checkerName eq $DATALOSS) {
				$datalossVerifiedProgsReal{$progName} = $finishTime;
			}
		    } else {
			if ($checkerName eq $OPENCLOSE) {
				$opencloseVerifiedProgsMark{$progName} = $finishTime;
			} elsif ($checkerName eq $FILE) {
				$fileVerifiedProgsMark{$progName} = $finishTime;
			} elsif ($checkerName eq $ASSERT) {
				$assertVerifiedProgsMark{$progName} = $finishTime;
			} elsif ($checkerName eq $LEAK) {
				$leakVerifiedProgsMark{$progName} = $finishTime;
			} elsif ($checkerName eq $DATALOSS) {
				$datalossVerifiedProgsMark{$progName} = $finishTime;
			}	
		    }
	    } else {
		    if ($mode eq $REAL) {
			if ($checkerName eq $OPENCLOSE) {
				$opencloseNVProgsReal{$progName} = $numValidPaths;
				$opencloseNVProgsRealPruned{$progName} = $numPrunedPaths;
				$opencloseRealPathCov{$progName} = $numValidPaths/$numAllPaths;
				#print "cov: ".$numValidPaths/$numAllPaths."\n";
			} elsif ($checkerName eq $FILE) {
				$fileNVProgsReal{$progName} = $numValidPaths;
				$fileNVProgsRealPruned{$progName} = $numPrunedPaths;
				$fileRealPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $ASSERT) {
				$assertNVProgsReal{$progName} = $numValidPaths;
				$assertNVProgsRealPruned{$progName} = $numPrunedPaths;
				$assertRealPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $LEAK) {
				$leakNVProgsReal{$progName} = $numValidPaths;
				$leakNVProgsRealPruned{$progName} = $numPrunedPaths;
				$leakRealPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $DATALOSS) {
				$datalossNVProgsReal{$progName} = $numValidPaths;
				$datalossNVProgsRealPruned{$progName} = $numPrunedPaths;
				$datalossRealPathCov{$progName} = $numValidPaths/$numAllPaths;
			}
		    } else {
			if ($checkerName eq $OPENCLOSE) {
				$opencloseNVProgsMark{$progName} = $numValidPaths;
				$opencloseNVProgsMarkPruned{$progName} = $numPrunedPaths;
				$opencloseMarkPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $FILE) {
				$fileNVProgsMark{$progName} = $numValidPaths;
				$fileNVProgsMarkPruned{$progName} = $numPrunedPaths;
				$fileMarkPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $ASSERT) {
				$assertNVProgsMark{$progName} = $numValidPaths;
				$assertNVProgsMarkPruned{$progName} = $numPrunedPaths;
				$assertMarkPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $LEAK) {
				$leakNVProgsMark{$progName} = $numValidPaths;
				$leakNVProgsMarkPruned{$progName} = $numPrunedPaths;
				$leakMarkPathCov{$progName} = $numValidPaths/$numAllPaths;
			} elsif ($checkerName eq $DATALOSS) {
				$datalossNVProgsMark{$progName} = $numValidPaths;
				$datalossNVProgsMarkPruned{$progName} = $numPrunedPaths;
				$datalossMarkPathCov{$progName} = $numValidPaths/$numAllPaths;
			}	
		    }
	    }
    }

}
close(LOG);

sub average { 
	@_ == 1 or die ('Sub usage: $average = average(\@array);'); 
	my ($array_ref) = @_; 
	my $sum; 
	my $count = scalar @$array_ref; 
	foreach (@$array_ref) { $sum += $_;} 
	return $sum / $count; 
} 

sub median { 
	@_ == 1 or die ('Sub usage: $median = median(\@array);'); 
	my ($array_ref) = @_; 
	my $count = scalar @$array_ref; 

	# Sort a COPY of the array, leaving the original untouched 
	my @array = sort { $a <=> $b } @$array_ref; 
	if ($count % 2) { 
		return $array[int($count/2)]; 
	} else { 
		return ($array[$count/2] + $array[$count/2 - 1]) / 2; 
	} 
} 

# Finish time for both finished.
$realFinishTimeTotal = 0;
$markFinishTimeTotal = 0;
 while (($key, $value) = each(%opencloseVerifiedProgsReal)) {
	$numRealVerified++;
	if ($opencloseVerifiedProgsMark{$key} ne "") {
		$realFinishTimeTotal += $opencloseVerifiedProgsReal{$key};
		$markFinishTimeTotal += $opencloseVerifiedProgsMark{$key};
	}
}
while (($key, $value) = each(%fileVerifiedProgsReal)) {
	if ($fileVerifiedProgsMark{$key} ne "") {
		$realFinishTimeTotal += $fileVerifiedProgsReal{$key};
		$markFinishTimeTotal += $fileVerifiedProgsMark{$key};
	}
}
while (($key, $value) = each(%assertVerifiedProgsReal)) {
	if ($assertVerifiedProgsMark{$key} ne "") {
		$realFinishTimeTotal += $assertVerifiedProgsReal{$key};
		$markFinishTimeTotal += $assertVerifiedProgsMark{$key};
	}
}
while (($key, $value) = each(%leakVerifiedProgsReal)) {
	if ($leakVerifiedProgsMark{$key} ne "") {
		$realFinishTimeTotal += $leakVerifiedProgsReal{$key};
		$markFinishTimeTotal += $leakVerifiedProgsMark{$key};
	}
}
while (($key, $value) = each(%datalossVerifiedProgsReal)) {
	if ($datalossVerifiedProgsMark{$key} ne "") {
		$realFinishTimeTotal += $datalossVerifiedProgsReal{$key};
		$markFinishTimeTotal += $datalossVerifiedProgsMark{$key};
	}
}


# Valid and pruned paths for neither finished.
$realValidPaths = 0;
$markValidPaths = 0;

$opencloseRealValidPaths = 0;
$opencloseMarkValidPaths = 0;

$fileRealValidPaths = 0;
$fileMarkValidPaths = 0;

$assertRealValidPaths = 0;
$assertMarkValidPaths = 0;

$leakRealValidPaths = 0;
$leakMarkValidPaths = 0;

$datalossRealValidPaths = 0;
$datalossMarkValidPaths = 0;

$realPrunedPaths = 0;
$markPrunedPaths = 0;

$opencloseRealPrunedPaths = 0;
$opencloseMarkPrunedPaths = 0;

$fileRealPrunedPaths = 0;
$fileMarkPrunedPaths = 0;

$assertRealPrunedPaths = 0;
$assertMarkPrunedPaths = 0;

$leakRealPrunedPaths = 0;
$leakMarkPrunedPaths = 0;

$datalossRealPrunedPaths = 0;
$datalossMarkPrunedPaths = 0;

@allRealpathCov;
@allMarkpathCov;

@opencloseRealpathCov;
@opencloseMarkpathCov;

@fileRealpathCov;
@fileMarkpathCov;

@assertRealpathCov;
@assertMarkpathCov;

@leakRealpathCov;
@leakMarkpathCov;

@datalossRealpathCov;
@datalossMarkpathCov;

while (($key, $value) = each(%opencloseNVProgsReal)) {
	if ($opencloseNVProgsMark{$key} ne "") {
		$realValidPaths += $opencloseNVProgsReal{$key};
		$markValidPaths += $opencloseNVProgsMark{$key};

		$opencloseRealValidPaths += $opencloseNVProgsReal{$key};
		$opencloseMarkValidPaths += $opencloseNVProgsMark{$key};

		$realPrunedPaths += $opencloseNVProgsRealPruned{$key};
		$markPrunedPaths += $opencloseNVProgsMarkPruned{$key};

		$opencloseRealPrunedPaths += $opencloseNVProgsRealPruned{$key};
		$opencloseMarkPrunedPaths += $opencloseNVProgsMarkPruned{$key};
		
		print "openclose real not-finished: $key: ".$opencloseRealPathCov{$key}."\n";
		print "openclose mark not-finished: $key: ".$opencloseMarkPathCov{$key}."\n";

		push(@allRealpathCov, $opencloseRealPathCov{$key});
		push(@allMarkpathCov, $opencloseMarkPathCov{$key});

		push(@opencloseRealpathCov, $opencloseRealPathCov{$key});
		push(@opencloseMarkpathCov, $opencloseMarkPathCov{$key});
	}
}
while (($key, $value) = each(%fileNVProgsReal)) {
	if ($fileNVProgsMark{$key} ne "") {
		$realValidPaths += $fileNVProgsReal{$key};
		$markValidPaths += $fileNVProgsMark{$key};

		$fileRealValidPaths += $fileNVProgsReal{$key};
		$fileMarkValidPaths += $fileNVProgsMark{$key};

		$realPrunedPaths += $fileNVProgsRealPruned{$key};
		$markPrunedPaths += $fileNVProgsMarkPruned{$key};

		$fileRealPrunedPaths += $fileNVProgsRealPruned{$key};
		$fileMarkPrunedPaths += $fileNVProgsMarkPruned{$key};
		
		push(@allRealpathCov, $fileRealPathCov{$key});
		push(@allMarkpathCov, $fileMarkPathCov{$key});

		push(@fileRealpathCov, $fileRealPathCov{$key});
		push(@fileMarkpathCov, $fileMarkPathCov{$key});
	}
}
while (($key, $value) = each(%assertNVProgsReal)) {
	if ($assertNVProgsMark{$key} ne "") {
		$realValidPaths += $assertNVProgsReal{$key};
		$markValidPaths += $assertNVProgsMark{$key};

		$assertRealValidPaths += $assertNVProgsReal{$key};
		$assertMarkValidPaths += $assertNVProgsMark{$key};

		$realPrunedPaths += $assertNVProgsRealPruned{$key};
		$markPrunedPaths += $assertNVProgsMarkPruned{$key};

		$assertRealPrunedPaths += $assertNVProgsRealPruned{$key};
		$assertMarkPrunedPaths += $assertNVProgsMarkPruned{$key};
		
		push(@allRealpathCov, $assertRealPathCov{$key});
		push(@allMarkpathCov, $assertMarkPathCov{$key});

		push(@assertRealpathCov, $assertRealPathCov{$key});
		push(@assertMarkpathCov, $assertMarkPathCov{$key});
	}
}
while (($key, $value) = each(%leakNVProgsReal)) {
	if ($leakNVProgsMark{$key} ne "") {
		$realValidPaths += $leakNVProgsReal{$key};
		$markValidPaths += $leakNVProgsMark{$key};

		$leakRealValidPaths += $leakNVProgsReal{$key};
		$leakMarkValidPaths += $leakNVProgsMark{$key};

		$realPrunedPaths += $leakNVProgsRealPruned{$key};
		$markPrunedPaths += $leakNVProgsMarkPruned{$key};

		$leakRealPrunedPaths += $leakNVProgsRealPruned{$key};
		$leakMarkPrunedPaths += $leakNVProgsMarkPruned{$key};
		
		push(@allRealpathCov, $leakRealPathCov{$key});
		push(@allMarkpathCov, $leakMarkPathCov{$key});

		push(@leakRealpathCov, $leakRealPathCov{$key});
		push(@leakMarkpathCov, $leakMarkPathCov{$key});
	}
}
while (($key, $value) = each(%datalossNVProgsReal)) {
	if ($datalossNVProgsMark{$key} ne "") {
		$realValidPaths += $datalossNVProgsReal{$key};
		$markValidPaths += $datalossNVProgsMark{$key};

		$datalossRealValidPaths += $datalossNVProgsReal{$key};
		$datalossMarkValidPaths += $datalossNVProgsMark{$key};

		$realPrunedPaths += $datalossNVProgsRealPruned{$key};
		$markPrunedPaths += $datalossNVProgsMarkPruned{$key};

		$datalossRealPrunedPaths += $datalossNVProgsRealPruned{$key};
		$datalossMarkPrunedPaths += $datalossNVProgsMarkPruned{$key};
		
		push(@allRealpathCov, $datalossRealPathCov{$key});
		push(@allMarkpathCov, $datalossMarkPathCov{$key});

		push(@datalossRealpathCov, $datalossRealPathCov{$key});
		push(@datalossMarkpathCov, $datalossMarkPathCov{$key});
	}
}


sub printStat() {
	print "\n";
	print "openCloseVerifiedReal: ".scalar(keys(%opencloseVerifiedProgsReal))."\n";
	print "fileVerifiedReal: ".scalar(keys(%fileVerifiedProgsReal))."\n";
	print "assertVerifiedReal: ".scalar(keys(%assertVerifiedProgsReal))."\n";
	print "leakVerifiedReal: ".scalar(keys(%leakVerifiedProgsReal))."\n";
	print "datalossVerifiedReal: ".scalar(keys(%datalossVerifiedProgsReal))."\n";

	print "\n";
	print "openCloseVerifiedMark: ".scalar(keys(%opencloseVerifiedProgsMark))."\n";
	print "fileVerifiedMark: ".scalar(keys(%fileVerifiedProgsMark))."\n";
	print "assertVerifiedMark: ".scalar(keys(%assertVerifiedProgsMark))."\n";
	print "leakVerifiedMark: ".scalar(keys(%leakVerifiedProgsMark))."\n";
	print "datalossVerifiedMark: ".scalar(keys(%datalossVerifiedProgsMark))."\n";

	print "\n";
	print "total real verification time: ".$realFinishTimeTotal."\n";
	print "total mark verification time: ".$markFinishTimeTotal."\n";
	print "both finish time gain: ".($markFinishTimeTotal-$realFinishTimeTotal)/$markFinishTimeTotal."\n";
	print "KLEE redundant path rate: ".$numMarkPrunedPaths."/".$numMarkAllPaths.
		"=".$numMarkPrunedPaths/$numMarkAllPaths."\n";

	print "\n";
	print "Neither finish path gain: ".$realValidPaths." / ".$markValidPaths." = ".$realValidPaths/$markValidPaths."\n";
	print "Neither finish openclose path gain: ".$opencloseRealValidPaths." / ".$opencloseMarkValidPaths." = ".
		$opencloseRealValidPaths/$opencloseMarkValidPaths."\n";
	print "Neither finish file path gain: ".$fileRealValidPaths." / ".$fileMarkValidPaths." = ".
		$fileRealValidPaths/$fileMarkValidPaths."\n";
	print "Neither finish assert path gain: ".$assertRealValidPaths." / ".$assertMarkValidPaths." = ".
		$assertRealValidPaths/$assertMarkValidPaths."\n";
	print "Neither finish leak path gain: ".$leakRealValidPaths." / ".$leakMarkValidPaths." = ".
		$leakRealValidPaths/$leakMarkValidPaths."\n";
	print "Neither finish dataloss path gain: ".$datalossRealValidPaths." / ".$datalossMarkValidPaths." = ".
		$datalossRealValidPaths/$datalossMarkValidPaths."\n";

	print "\n";
	print "Neither finish pruned path: ".$realPrunedPaths." / ".$markPrunedPaths." = ".$realPrunedPaths/$markPrunedPaths."\n";
	print "Neither finish openclose pruned path gain: ".$opencloseRealPrunedPaths." / ".$opencloseMarkPrunedPaths." = ".
		$opencloseRealPrunedPaths/$opencloseMarkPrunedPaths."\n";
	print "Neither finish file pruned path gain: ".$fileRealPrunedPaths." / ".$fileMarkPrunedPaths." = ".
		$fileRealPrunedPaths/$fileMarkPrunedPaths."\n";
	print "Neither finish assert pruned path gain: ".$assertRealPrunedPaths." / ".$assertMarkPrunedPaths." = ".
		$assertRealPrunedPaths/$assertMarkPrunedPaths."\n";
	print "Neither finish leak pruned path gain: ".$leakRealPrunedPaths." / ".$leakMarkPrunedPaths." = ".
		$leakRealPrunedPaths/$leakMarkPrunedPaths."\n";
	print "Neither finish dataloss pruned path gain: ".$datalossRealPrunedPaths." / ".$datalossMarkPrunedPaths." = ".
		$datalossRealPrunedPaths/$datalossMarkPrunedPaths."\n";



	print "\n";
	print "WoodPecker overall both not-finished path coverage: avg: ".average(\@allRealpathCov).", median: ".median(\@allRealpathCov)."\n";
	print "KLEE overall both not-finished path coverage: avg: ".average(\@allMarkpathCov).", median: ".median(\@allMarkpathCov)."\n";

	print "\n";
	print "WoodPecker openclose both not-finished path coverage: avg: ".average(\@opencloseRealpathCov).", median: ".median(\@opencloseRealpathCov)."\n";
	print "KLEE openclose both not-finished path coverage: avg: ".average(\@opencloseMarkpathCov).", median: ".median(\@opencloseMarkpathCov)."\n";

	print "\n";
	print "WoodPecker file both not-finished path coverage: avg: ".average(\@fileRealpathCov).", median: ".median(\@fileRealpathCov)."\n";
	print "KLEE file both not-finished path coverage: avg: ".average(\@fileMarkpathCov).", median: ".median(\@fileMarkpathCov)."\n";

	print "\n";
	print "WoodPecker assert both not-finished path coverage: avg: ".average(\@assertRealpathCov).", median: ".median(\@assertRealpathCov)."\n";
	print "KLEE assert both not-finished path coverage: avg: ".average(\@assertMarkpathCov).", median: ".median(\@assertMarkpathCov)."\n";

	print "\n";
	print "WoodPecker leak both not-finished path coverage: avg: ".average(\@leakRealpathCov).", median: ".median(\@leakRealpathCov)."\n";
	print "KLEE leak both not-finished path coverage: avg: ".average(\@leakMarkpathCov).", median: ".median(\@leakMarkpathCov)."\n";

	print "\n";
	print "WoodPecker dataloss both not-finished path coverage: avg: ".average(\@datalossRealpathCov).", median: ".median(\@datalossRealpathCov)."\n";
	print "KLEE overall both not-finished path coverage: avg: ".average(\@datalossMarkpathCov).", median: ".median(\@datalossMarkpathCov)."\n";
}

printStat();

