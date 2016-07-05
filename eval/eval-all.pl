#!/usr/bin/env perl

use File::Copy;
use File::Basename;
use Cwd;

@checkers = ("File", "Assert", "DataLoss", "Leak", "OpenClose");
$klee_eval_time = 3600;
$slicer_eval_time = 3600;

$evaler = "./eval-path-slicer-coverage.pl";

sub evalOnce {
    my ($program, $outpath, $eval_time, $slicing, $checker) = @_;

    $prog_path = $outpath . "/" . basename($program);
    print "Program path: $prog_path\n";
    copy($program, $prog_path);
    $cmd = $evaler . " " . $eval_time . " " . $outpath . " " . ($slicing ? "real-prune-only" : "mark-pruned-only") . " " . $checker;
    system($cmd);
    unlink($prog_path);
}

$progs_path = $ARGV[0];
$conf_path = $progs_path;

if (! -e $progs_path) {
    die "Programs' path does not exist!\n";
}

if (! -e $conf_path . "/bc2bdd.conf" || ! -e $conf_path . "/local.options") {
    die "Config files missing!\n";
}

$outpath = "";

for ($i=0; ; $i++) {
    $outpath = "result-$i";
    if (! -e $outpath) {
        last;
    }
}

mkdir($outpath);

foreach $program (glob $progs_path . "/*.bc") {
    print "==== Evaluating $program ====\n";
    $progname = basename($program, ".bc");

    foreach $checker(@checkers) {
        print "\tWith checker $checker\n";
        for ($with_slicer=0; $with_slicer<2; $with_slicer++) {
            $outpath_full = $outpath . "/" . $progname . "-" . $checker . "-" . ($with_slicer ? "sliced" : "not_sliced");
            mkdir($outpath_full);
            if ($with_slicer) {
                print "\t\tWith slicer\n";
                $eval_time = $slicer_eval_time;
            } else {
                print "\t\tWithout slicer\n";
                $eval_time = $klee_eval_time;
            }

            copy($conf_path . "/bc2bdd.conf", $outpath_full);
            copy($conf_path . "/local.options", $outpath_full);

            evalOnce($program, $outpath_full, $eval_time, $with_slicer, $checker);
        }
    }
}
