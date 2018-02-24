#!/usr/bin/perl -w
$file = $ARGV[0];
open FD,"mcsmodels -v $file|";
$solver = 0;
while (<FD>) {
#	print STDERR "$_\n";
	$total = $1 if /Time needed.*?(\d+\.\d+)/;
	$solver = $1 if /Duration.*?(\d+\.\d+)/;
}
close FD;

$gen = $total - $solver;

printf "total\t%.3f\ngen\t%.3f\nsolve\t%.3f\nnet\t%s\n",$total,$gen,$solver,$file;
exit 0;
