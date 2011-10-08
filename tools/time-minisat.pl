#!/usr/bin/perl -w
$file = $ARGV[0];
open FD,"minisat $file|";
while (<FD>) {
#	print STDERR "$_\n";
	$time = $1 if /CPU.*?(\d.*) s/;
}
close FD;

printf "time\t%.3f\nnet\t%s\n",$time,$file;
exit 0;
