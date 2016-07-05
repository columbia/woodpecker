#!/usr/bin/perl

 open (LIST, 'commands.list');
 while (<LIST>) {
 	chomp;
 	my $line = $_;
 	print "$line\n";
 	my @array = split(/ /, $line);
 	my $cmd = $array[1];
 	my $func = $array[2];
 	chop($cmd);
 	if (scalar(@array) > 4) {
 		chop($func);
 	}
 	print $cmd."\n";
 	my $patchName = "git-$cmd";
 	
 	`cp patch.tmpl $patchName`;
	`sed -i 's/TO_BE_CHANGED1/$line/g' $patchName`;
	`sed -i 's/TO_BE_CHANGED2/$cmd/g' $patchName`;
	`sed -i 's/TO_BE_CHANGED3/$func/g' $patchName`;
}
close (LIST);

