#!/usr/bin/perl

$block = "";
$plnr = $trnr = 0;
$c = 0;
while (<>) {
	$c++;
	if ($c <= 3) { print; next; }
	next if /^D/;

	tr/\015\012//d;
	if (/^([A-Z]*)$/) { $block = $1; next; }
	if ($block eq "PL") {
		if (/^(\d+)(.*)/) { $plnr = $1; $rest = $2; }
		else { $plnr++; $rest = $_; }
		if ($rest =~ /^"(.*?)"/) {
			$ident{$plnr} = $1;
			$rest =~ s/^"(.*?)"//g;
		} else {
			$ident{$plnr} = "P$plnr";
		}
		$initial{$plnr} = ($rest =~ /M1/)? 1 : 0;
		$places{$plnr} = "";
	} elsif ($block eq "TR") {
		$trnr = /^(\d+)/? $1 : $trnr+1;
		$trans{$trnr} = $_;
	} elsif ($block eq "TP") {
		($tr,$pl) = /^(\d*)<(\d*)/;
		$trpl{"$tr.$pl"} = "";
	} elsif ($block eq "PT") {
		($pl,$tr) = /^(\d*)>(\d*)/;
		$pltr{"$pl.$tr"} = "";
	} elsif ($block eq "RA") {
		($tr,$pl) = /^(\d*)<(\d*)/;
		$readers{$pl} = "" unless defined($readers{$pl});
		$readers{$pl} .= "%$tr";
	}
}

for $pl (keys %readers) {
	$readers{$pl} = substr($readers{$pl},1);
}

# The places and transitions remain unchanged.
print "PL\n";
for $pl (sort {$a <=> $b} keys %places) {
	printf "%d\"%s\"%s\n", $pl, $ident{$pl}, $initial{$pl}? "M1m1" : "";
}
print "TR\n";
for $tr (sort {$a <=> $b} keys %trans) {
	printf "%s\n",$trans{$tr};
}

print "TP\n";
for (keys %trpl) {
	($tr,$pl) = /(\d*).(\d*)/;
	printf "%d<%d\n",$tr,$pl;
}
for $pl (keys %readers) {
	for $tr (split(/%/,$readers{$pl})) {
		printf "%d<%d\n",$tr,$pl;
	}
}

print "PT\n";
for (keys %pltr) {
	($pl,$tr) = /(\d*).(\d*)/;
	printf "%d>%d\n",$pl,$tr;
}
for $pl (keys %readers) {
	for $tr (split(/%/,$readers{$pl})) {
		printf "%d>%d\n",$pl,$tr;
	}
}

exit 0;
