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

# Print the new places, assigning new numbers to the places.
# However, even if the numbers change, we keep their names so that
# finally we can compare the markings.
# Every place that has readers is split into as many copies as it has readers.
print "PL\n";
$newnr = 1;
for $pl (sort {$a <=> $b} keys %places) {
	$base{$pl} = $newnr;
	if (defined($readers{$pl})) {
		for $tr (split(/%/,$readers{$pl})) {
			printf "%d\"%s-%s\"9\@9%s\n", $newnr++, $ident{$pl}, $tr,
					$initial{$pl}? "M1m1" : "";
		}
	} else {
		printf "%d\"%s\"9\@9%s\n", $newnr++, $ident{$pl},
					$initial{$pl}? "M1m1" : "";
	}
}

# The transitions remain unchanged.
print "TR\n";
for $tr (sort {$a <=> $b} keys %trans) {
	printf "%s\n",$trans{$tr};
}

# Print arcs from transitions to places; if an arc outputs to a place
# with readers, output to all its copies instead.
print "TP\n";
for (keys %trpl) {
	($tr,$pl) = /(\d*).(\d*)/;
	$newnr = $base{$pl};
	if (defined($readers{$pl})) {
		@rdtr= split(/%/,$readers{$pl});
		for ($i = 0; $i <= $#rdtr; $i++) {
			printf "%d<%d\n",$tr,$newnr+$i;
		}
	} else {
		printf "%d<%d\n",$tr,$newnr;
	}
}
# Additional arcs to accomodate the readers.
for $pl (keys %readers) {
	$newnr = $base{$pl};
	for $tr (split(/%/,$readers{$pl})) {
		printf "%d<%d\n",$tr,$newnr++;
	}
}

# Print arcs from places to transitions with analogous transformations.
print "PT\n";
for (keys %pltr) {
	($pl,$tr) = /(\d*).(\d*)/;
	$newnr = $base{$pl};
	if (defined($readers{$pl})) {
		@rdtr= split(/%/,$readers{$pl});
		for ($i = 0; $i <= $#rdtr; $i++) {
			printf "%d>%d\n",$newnr+$i,$tr;
		}
	} else {
		printf "%d>%d\n",$newnr,$tr;
	}
}
# Additional arcs to accomodate the readers.
for $pl (keys %readers) {
	$newnr = $base{$pl};
	for $tr (split(/%/,$readers{$pl})) {
		printf "%d>%d\n",$newnr++,$tr;
	}
}

exit 0;
