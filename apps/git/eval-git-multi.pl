#!/usr/bin/env perl

@modes = ("real-prune-only", "mark-pruned-only");
@checkers = ("Assert", "Leak", "File", "DataLoss", "OpenClose");
$time = 10800;
$program = $ARGV[0];

%children = ();

print "Launching everything...\n";

foreach $mode (@modes) {
    foreach $checker (@checkers) {
        my $pid = fork();
        if ($pid == 0) {
            print "Executing $checker - $mode\n";
            system("./eval-git.pl $time . $mode $checker 0 1 '0 4 8' $program");
            exit 0;
        } else {
            $children{$checker . '-' . $mode} = $pid;
        }
    }
}

print "Waiting for everything...\n";

for my $key ( keys %children ) {
    print "Wait for $key\n";
    my $pid = $children{$key};

    waitpid($pid, 0);
}

