#!/usr/bin/perl

while (<>) {
	last if /TP/;
	print;
}

while (<>) {
	last if /PT/;
	($tr,$pl) = /^(\d*)<(\d*)/;
	$trpl{"$tr.$pl"} = "";
}

while (<>) {
	last if /^[A-Z]/;
	($pl,$tr) = /^(\d*)>(\d*)/;
	$pltr{"$pl.$tr"} = "";
}

print "TP\n";
for (keys %trpl) {
	($tr,$pl) = /(\d*).(\d*)/;
	next if defined $pltr{"$pl.$tr"};
	print "$tr<$pl\n";
}
print "PT\n";
for (keys %pltr) {
	($pl,$tr) = /(\d*).(\d*)/;
	next if defined $trpl{"$tr.$pl"};
	print "$pl>$tr\n";
}
print "RA\n";
for (keys %trpl) {
	($tr,$pl) = /(\d*).(\d*)/;
	next unless defined $pltr{"$pl.$tr"};
	print "$tr<$pl\n";
}

exit 0;
