#!/usr/bin/perl -w

# Random asynchronous boolean network
# There is N nodes, each of one can be up or down (represented by two places).
# A node is regulated by either one or two nodes.  The regulation logic is
# randomly decided for all the possible combinations of states of the regulator
# nodes

$n = $ARGV[0];

printf "PEP\nPetriBox\nFORMAT_N2\nPL\n";

$plc = 0;
for ($i = 0; $i < $n; $i++) {
	$marked = int(rand(2));
	$plc++;
	printf "%d\"P%d-0\"%s\n",$plc,$i,$marked? "" : "M1m1";
	$plc++;
	printf "%d\"P%d-1\"%s\n",$plc,$i,$marked? "M1m1" : "";
}

printf "TR\n";
for ($i = 0; $i < $n; $i++) {
	if (int(rand(2)) < 1) {
		onereg($i);
	} else {
		tworeg($i);
	}
}

printf "TP\n"; for $t (@produce) { print "$t\n"; }
for $t (@readarc) { $x = $t; $x =~ s/-/</; print "$x\n"; }

printf "PT\n"; for $t (@consume) { print "$t\n"; }
for $t (@readarc) { $t =~ s/(\d*)-(\d*)/$2>$1/; print "$t\n"; }

exit 0;

sub onereg {
	my ($i) = @_;
	my $j = int(rand($n));
	while ($j == $i) { $j = int(rand($n)); }

	newtrans($i,"$j-0"); ra($j,0); random($i);
	newtrans($i,"$j-1"); ra($j,1); random($i);
}

sub tworeg {
	my ($i) = @_;
	my $j = int(rand($n));
	my $k = int(rand($n));
	while ($j == $i) { $j = int(rand($n)); }
	while ($k == $i || $k == $j) { $k = int(rand($n)); }

	newtrans($i,"$j-$k-00"); ra($j,0); ra($k,0); random($i);
	newtrans($i,"$j-$k-01"); ra($j,0); ra($k,1); random($i);
	newtrans($i,"$j-$k-10"); ra($j,1); ra($k,0); random($i);
	newtrans($i,"$j-$k-11"); ra($j,1); ra($k,1); random($i);
}

sub random {
	my ($i) = @_;
	if (int(rand(2)) < 1) { zero($i); } else { one($i); }
}

sub ra {
	my ($x,$v) = @_;
	my $p = place($x,$v);
	push @readarc,"$trc-$p";
}

sub one {
	my ($b) = @_;
	my $p = place($b,0);
	my $q = place($b,1);
	push @consume,"$p>$trc";
	push @produce,"$trc<$q";
}

sub zero {
	my ($b) = @_;
	my $p = place($b,1);
	my $q = place($b,0);
	push @consume,"$p>$trc";
	push @produce,"$trc<$q";
}

sub newtrans {
	my ($b,$s) = @_;
	$trc++;
	printf "%d\"%s<-%s\"\n",$trc,$b,$s;
}

sub place {
	my ($b,$v) = @_;
	return $b*2 + $v + 1;
}

sub mp {
	my ($i,$j) = @_;
	return "$i,$j";
}
